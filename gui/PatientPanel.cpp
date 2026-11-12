// =====================================================================
// PatientPanel.cpp
// =====================================================================
#include "PatientPanel.h"
#include "GuiHelpers.h"
#include "PatientDialog.h"
#include "PatientTableModel.h"
#include "../utils/Validation.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

namespace scapms::gui {

using namespace scapms::model;
using utils::Validation::toLower;

PatientPanel::PatientPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager(manager) {

    // ---- Top row: filter box + count (QHBoxLayout = left to right) ----
    filterEdit = new QLineEdit;
    filterEdit->setPlaceholderText("Type to filter by name or ID...");
    filterEdit->setClearButtonEnabled(true);
    filterEdit->setToolTip("Shows only patients whose name or ID contains this text");
    countLabel = new QLabel;

    QHBoxLayout* topRow = new QHBoxLayout;
    topRow->addWidget(new QLabel("Filter:"));
    topRow->addWidget(filterEdit, 1); // stretch factor 1 = take the spare width
    topRow->addWidget(countLabel);

    // ---- The table ----
    tableModel = new PatientTableModel(this);
    table = new QTableView;
    table->setModel(tableModel);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    table->setToolTip("Red = Emergency (High Priority), Blue = Follow-Up (Scheduled). Double-click to edit.");

    // ---- Button row ----
    QPushButton* addButton = new QPushButton("Add Patient");
    addButton->setToolTip("Register a new patient (Ctrl+N)");
    editButton = new QPushButton("Edit Patient");
    editButton->setToolTip("Change the selected patient's details");
    deleteButton = new QPushButton("Delete Patient");
    deleteButton->setToolTip("Delete the selected patient (can be undone with Ctrl+Z)");
    queueButton = new QPushButton("Add to Waiting Queue");
    queueButton->setToolTip("Put the selected patient at the back of the waiting queue");

    QHBoxLayout* buttonRow = new QHBoxLayout;
    buttonRow->addWidget(addButton);
    buttonRow->addWidget(editButton);
    buttonRow->addWidget(deleteButton);
    buttonRow->addStretch(); // push the last button to the right
    buttonRow->addWidget(queueButton);

    // ---- Stack the three rows (QVBoxLayout = top to bottom) ----
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addLayout(topRow);
    layout->addWidget(table, 1);
    layout->addLayout(buttonRow);

    // ---- BUTTON / TEXT FIELD EVENTS -> slots ----
    connect(addButton, &QPushButton::clicked, this, &PatientPanel::addPatient);
    connect(editButton, &QPushButton::clicked, this, &PatientPanel::editSelected);
    connect(deleteButton, &QPushButton::clicked, this, &PatientPanel::deleteSelected);
    connect(queueButton, &QPushButton::clicked, this, &PatientPanel::addSelectedToQueue);
    connect(table, &QTableView::doubleClicked, this, &PatientPanel::editSelected);
    connect(filterEdit, &QLineEdit::textChanged, this, &PatientPanel::refresh);
    connect(table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &PatientPanel::onSelectionChanged);

    refresh();
}

void PatientPanel::refresh() {
    // Keep only patients whose name or ID contains the filter text.
    // (This is just a quick display filter. The ASSESSED search
    //  algorithms are used on the "Search & Sort" tab.)
    std::string filter = toLower(toStd(filterEdit->text()));
    std::vector<PatientRecord*> visible;
    for (PatientRecord* patient : manager.getPatientList()) {
        bool matches = filter.empty()
                       || toLower(patient->getName()).find(filter) != std::string::npos
                       || toLower(patient->getId()).find(filter) != std::string::npos;
        if (matches) visible.push_back(patient);
    }
    tableModel->setPatients(visible, &manager.getDatabase());
    table->resizeColumnsToContents();
    countLabel->setText(QString("%1 of %2 patients").arg(visible.size()).arg(manager.getDatabase().patientCount()));
    onSelectionChanged();
}

PatientRecord* PatientPanel::selectedPatient() const {
    QModelIndexList rows = table->selectionModel()->selectedRows();
    if (rows.isEmpty()) return nullptr;
    return tableModel->patientAt(rows.first().row());
}

// Buttons that need a selected patient are greyed out when none is selected.
void PatientPanel::onSelectionChanged() {
    PatientRecord* patient = selectedPatient();
    editButton->setEnabled(patient != nullptr);
    deleteButton->setEnabled(patient != nullptr);
    queueButton->setEnabled(patient != nullptr);
    if (patient != nullptr) {
        manager.viewPatient(patient->getId()); // record in the "recently viewed" cache
    }
}

void PatientPanel::addPatient() {
    PatientDialog dialog(manager, nullptr, this);
    if (dialog.exec() != QDialog::Accepted) return; // user pressed Cancel

    try {
        std::unique_ptr<PatientRecord> patient = dialog.takePatient();
        QString name = toQ(patient->getName());
        manager.addPatient(std::move(patient));
        emit dataChanged();
        emit statusMessage("Registered patient " + name);
    } catch (const std::exception& e) {
        showError(this, e);
    }
}

void PatientPanel::editSelected() {
    PatientRecord* patient = selectedPatient();
    if (patient == nullptr) return;

    PatientDialog dialog(manager, patient, this);
    if (dialog.exec() != QDialog::Accepted) return;

    try {
        std::string id = patient->getId();
        manager.updatePatient(id, dialog.takePatient());
        emit dataChanged();
        emit statusMessage("Updated patient " + toQ(id) + " (Ctrl+Z to undo)");
    } catch (const std::exception& e) {
        showError(this, e);
    }
}

void PatientPanel::deleteSelected() {
    PatientRecord* patient = selectedPatient();
    if (patient == nullptr) return;

    // Always confirm before deleting.
    QString question = QString("Delete %1 (%2)?\nTheir appointments will be removed too.\n"
                               "You can undo this from the Admin tab or with Ctrl+Z.")
                           .arg(toQ(patient->getName()), toQ(patient->getId()));
    if (QMessageBox::question(this, "Delete Patient", question) != QMessageBox::Yes) return;

    try {
        std::string id = patient->getId();
        manager.deletePatient(id);
        emit dataChanged();
        emit statusMessage("Deleted patient " + toQ(id) + " (Ctrl+Z to undo)");
    } catch (const std::exception& e) {
        showError(this, e);
    }
}

void PatientPanel::addSelectedToQueue() {
    PatientRecord* patient = selectedPatient();
    if (patient == nullptr) return;
    try {
        manager.addToQueue(patient->getId());
        emit dataChanged();
        emit statusMessage(toQ(patient->getName()) + " joined the waiting queue");
    } catch (const std::exception& e) {
        showError(this, e);
    }
}

} // namespace scapms::gui
