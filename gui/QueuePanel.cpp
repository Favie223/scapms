// =====================================================================
// QueuePanel.cpp
// =====================================================================
#include "QueuePanel.h"
#include "GuiHelpers.h"

#include <QComboBox>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace scapms::gui {

using namespace scapms::model;

QueuePanel::QueuePanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager(manager) {

    // ---- "Now serving" banner ----
    nowServingLabel = new QLabel("Now serving: nobody yet");
    QFont bigFont = nowServingLabel->font();
    bigFont.setPointSize(bigFont.pointSize() + 4);
    bigFont.setBold(true);
    nowServingLabel->setFont(bigFont);

    // ---- Add row ----
    patientCombo = new QComboBox;
    patientCombo->setToolTip("Choose a patient who has arrived");
    QPushButton* addButton = new QPushButton("Add to Queue");
    addButton->setToolTip("Put this patient at the BACK of the queue");
    QPushButton* callButton = new QPushButton("Call Next Patient");
    callButton->setToolTip("Serve the patient at the FRONT of the queue (first come, first served)");

    QHBoxLayout* addRow = new QHBoxLayout;
    addRow->addWidget(new QLabel("Patient:"));
    addRow->addWidget(patientCombo, 1);
    addRow->addWidget(addButton);

    // ---- Queue table ----
    table = new QTableWidget(0, 6);
    table->setHorizontalHeaderLabels({"Position", "ID", "Name", "Category", "Joined At", "Waiting (min)"});
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    table->setToolTip(QString("Front of the queue is at the top. Red rows have waited over %1 minutes.")
                          .arg(kLongWaitMinutes));

    sizeLabel = new QLabel;

    QHBoxLayout* bottomRow = new QHBoxLayout;
    bottomRow->addWidget(sizeLabel);
    bottomRow->addStretch();
    bottomRow->addWidget(callButton);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(nowServingLabel);
    layout->addLayout(addRow);
    layout->addWidget(table, 1);
    layout->addLayout(bottomRow);

    connect(addButton, &QPushButton::clicked, this, &QueuePanel::addClicked);
    connect(callButton, &QPushButton::clicked, this, &QueuePanel::callNextClicked);

    refresh();
}

void QueuePanel::refresh() {
    fillPatientCombo(patientCombo, manager);

    std::vector<controller::QueueEntry> queue = manager.getQueue(); // front first
    table->setRowCount(0);
    for (int i = 0; i < (int)queue.size(); i++) {
        PatientRecord* patient = manager.getPatient(queue[i].patientId);
        int waited = controller::ClinicManager::minutesWaiting(queue[i]);

        table->insertRow(i);
        table->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        table->setItem(i, 1, new QTableWidgetItem(toQ(queue[i].patientId)));
        table->setItem(i, 2, new QTableWidgetItem(patient ? toQ(patient->getName()) : "?"));
        table->setItem(i, 3, new QTableWidgetItem(patient ? toQ(patient->getCategory()) : "?"));
        table->setItem(i, 4, new QTableWidgetItem(toQ(queue[i].joinedAtText)));
        table->setItem(i, 5, new QTableWidgetItem(QString::number(waited)));

        if (waited >= kLongWaitMinutes) { // highlight long waits
            for (int column = 0; column < table->columnCount(); column++) {
                table->item(i, column)->setBackground(QColor(255, 190, 190));
                table->item(i, column)->setForeground(QColor(20, 20, 20));
            }
        }
    }
    table->resizeColumnsToContents();
    sizeLabel->setText(QString("%1 patient(s) waiting").arg(queue.size()));
}

int QueuePanel::countLongWaits() const {
    int count = 0;
    for (const controller::QueueEntry& entry : manager.getQueue()) {
        if (controller::ClinicManager::minutesWaiting(entry) >= kLongWaitMinutes) count++;
    }
    return count;
}

void QueuePanel::addClicked() {
    if (patientCombo->currentIndex() < 0) return;
    try {
        manager.addToQueue(toStd(patientCombo->currentData().toString()));
        emit dataChanged();
        emit statusMessage(patientCombo->currentText() + " joined the queue");
    } catch (const std::exception& e) {
        showError(this, e);
    }
}

void QueuePanel::callNextClicked() {
    try {
        PatientRecord* patient = manager.callNextPatient(); // throws if empty
        QString text = patient ? toQ(patient->getId() + " - " + patient->getName()) : "unknown patient";
        nowServingLabel->setText("Now serving: " + text);
        emit dataChanged();
        emit statusMessage("Called " + text);
    } catch (const std::exception& e) {
        showError(this, e);
    }
}

} // namespace scapms::gui
