// =====================================================================
// AppointmentPanel.cpp
// =====================================================================
#include "AppointmentPanel.h"
#include "GuiHelpers.h"
#include "../utils/Exceptions.h"

#include <QComboBox>
#include <QDateTimeEdit>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace scapms::gui {

using namespace scapms::model;

AppointmentPanel::AppointmentPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager(manager) {

    // ------------- "Schedule" form (QFormLayout) -------------
    patientCombo = new QComboBox;
    patientCombo->setToolTip("Choose the patient this appointment is for");

    dateTimeEdit = new QDateTimeEdit;
    dateTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    dateTimeEdit->setCalendarPopup(true);
    // Default: tomorrow at 10:00 (inside clinic hours).
    dateTimeEdit->setDateTime(QDateTime(QDate::currentDate().addDays(1), QTime(10, 0)));
    dateTimeEdit->setToolTip("Clinic hours are 08:00-17:59. Emergencies can be booked at any time.");

    doctorCombo = new QComboBox;
    doctorCombo->setEditable(true); // pick one or type a new name
    doctorCombo->addItems({"Dr. Bello", "Dr. Eze", "Dr. Okafor", "Dr. Adeyemi"});
    doctorCombo->setToolTip("Pick a doctor or type a name");

    reasonEdit = new QLineEdit;
    reasonEdit->setPlaceholderText("e.g. Routine checkup");

    bookingHint = new QLabel;
    bookingHint->setWordWrap(true);

    QPushButton* scheduleButton = new QPushButton("Schedule Appointment");
    scheduleButton->setToolTip("Book this appointment");

    QFormLayout* form = new QFormLayout;
    // Same look on every OS: labels on the left, fields stretch to fill the width.
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    form->addRow("Patient:", patientCombo);
    form->addRow("Date && time:", dateTimeEdit); // "&&" shows one "&"
    form->addRow("Doctor:", doctorCombo);
    form->addRow("Reason:", reasonEdit);
    form->addRow("", bookingHint);
    form->addRow("", scheduleButton);

    QGroupBox* scheduleBox = new QGroupBox("Schedule a new appointment");
    scheduleBox->setLayout(form);

    // ------------- Appointment list -------------
    statusFilter = new QComboBox;
    statusFilter->addItems({"All", "Pending", "Completed", "Cancelled"});
    statusFilter->setToolTip("Show only appointments with this status");

    table = new QTableWidget(0, 6);
    table->setHorizontalHeaderLabels({"ID", "Patient", "Date & Time", "Doctor", "Reason", "Status"});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);

    cancelButton = new QPushButton("Cancel Appointment");
    cancelButton->setToolTip("Mark the selected appointment as Cancelled (undoable)");
    completeButton = new QPushButton("Mark Completed");
    completeButton->setToolTip("The patient attended - mark as Completed");
    removeButton = new QPushButton("Remove Appointment");
    removeButton->setToolTip("Delete the selected appointment completely (undoable)");

    QHBoxLayout* filterRow = new QHBoxLayout;
    filterRow->addWidget(new QLabel("Show:"));
    filterRow->addWidget(statusFilter);
    filterRow->addStretch();

    QHBoxLayout* buttonRow = new QHBoxLayout;
    buttonRow->addWidget(completeButton);
    buttonRow->addWidget(cancelButton);
    buttonRow->addWidget(removeButton);
    buttonRow->addStretch();

    QGroupBox* listBox = new QGroupBox("Appointments");
    QVBoxLayout* listLayout = new QVBoxLayout(listBox);
    listLayout->addLayout(filterRow);
    listLayout->addWidget(table, 1);
    listLayout->addLayout(buttonRow);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(scheduleBox);
    layout->addWidget(listBox, 1);

    // ------------- Events -------------
    connect(scheduleButton, &QPushButton::clicked, this, &AppointmentPanel::scheduleClicked);
    connect(cancelButton, &QPushButton::clicked, this, &AppointmentPanel::cancelClicked);
    connect(completeButton, &QPushButton::clicked, this, &AppointmentPanel::completeClicked);
    connect(removeButton, &QPushButton::clicked, this, &AppointmentPanel::removeClicked);
    connect(statusFilter, &QComboBox::currentIndexChanged, this, &AppointmentPanel::refresh);
    // Live booking-rule check whenever the patient or time changes.
    connect(patientCombo, &QComboBox::currentIndexChanged, this, &AppointmentPanel::updateBookingHint);
    connect(dateTimeEdit, &QDateTimeEdit::dateTimeChanged, this, &AppointmentPanel::updateBookingHint);

    refresh();
}

void AppointmentPanel::refresh() {
    fillPatientCombo(patientCombo, manager);

    QString filter = statusFilter->currentText();
    table->setRowCount(0);
    for (const Appointment& appointment : manager.getAppointments()) {
        QString status = toQ(appointment.getStatusText());
        if (filter != "All" && filter != status) continue;

        PatientRecord* patient = manager.getPatient(appointment.getPatientId());
        QString patientText = toQ(appointment.getPatientId())
                              + (patient ? " - " + toQ(patient->getName()) : QString());

        int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(toQ(appointment.getId())));
        table->setItem(row, 1, new QTableWidgetItem(patientText));
        table->setItem(row, 2, new QTableWidgetItem(toQ(appointment.getDateTime())));
        table->setItem(row, 3, new QTableWidgetItem(toQ(appointment.getDoctor())));
        table->setItem(row, 4, new QTableWidgetItem(toQ(appointment.getReason())));
        table->setItem(row, 5, new QTableWidgetItem(status));

        // CUSTOM PRESENTATION: colour the whole row by status.
        QColor colour;
        if (appointment.getStatus() == AppointmentStatus::Pending) colour = QColor(255, 236, 179);        // amber
        else if (appointment.getStatus() == AppointmentStatus::Completed) colour = QColor(200, 240, 200); // green
        else colour = QColor(225, 225, 225);                                                              // grey
        for (int column = 0; column < table->columnCount(); column++) {
            table->item(row, column)->setBackground(colour);
            table->item(row, column)->setForeground(QColor(20, 20, 20));
        }
    }
    table->resizeColumnsToContents();
    updateBookingHint();
}

// Calls the patient's own canSchedule() rule and shows the answer.
void AppointmentPanel::updateBookingHint() {
    std::string patientId = toStd(patientCombo->currentData().toString());
    std::string dateTime = toStd(dateTimeEdit->dateTime().toString("yyyy-MM-dd HH:mm"));
    std::string reason;
    if (manager.checkBooking(patientId, dateTime, reason)) {
        bookingHint->setText("<span style='color:#27ae60'>&#10003; This time is allowed for this patient.</span>");
    } else {
        bookingHint->setText("<span style='color:#c0392b'>&#10007; " + toQ(reason).toHtmlEscaped() + "</span>");
    }
}

QString AppointmentPanel::selectedAppointmentId() const {
    int row = table->currentRow();
    if (row < 0 || table->item(row, 0) == nullptr) return QString();
    return table->item(row, 0)->text();
}

void AppointmentPanel::scheduleClicked() {
    try {
        if (patientCombo->currentIndex() < 0) {
            throw utils::InvalidAppointmentException("please register a patient first.");
        }
        std::string id = manager.scheduleAppointment(
            toStd(patientCombo->currentData().toString()),
            toStd(dateTimeEdit->dateTime().toString("yyyy-MM-dd HH:mm")),
            toStd(doctorCombo->currentText()),
            toStd(reasonEdit->text()));
        reasonEdit->clear();
        emit dataChanged();
        emit statusMessage("Scheduled appointment " + toQ(id));
    } catch (const std::exception& e) {
        showError(this, e);
    }
}

void AppointmentPanel::cancelClicked() {
    QString id = selectedAppointmentId();
    if (id.isEmpty()) {
        QMessageBox::information(this, "Cancel Appointment", "Please select an appointment first.");
        return;
    }
    try {
        manager.cancelAppointment(toStd(id));
        emit dataChanged();
        emit statusMessage("Cancelled appointment " + id + " (Ctrl+Z to undo)");
    } catch (const std::exception& e) {
        showError(this, e);
    }
}

void AppointmentPanel::completeClicked() {
    QString id = selectedAppointmentId();
    if (id.isEmpty()) {
        QMessageBox::information(this, "Mark Completed", "Please select an appointment first.");
        return;
    }
    try {
        manager.completeAppointment(toStd(id));
        emit dataChanged();
        emit statusMessage("Appointment " + id + " marked completed");
    } catch (const std::exception& e) {
        showError(this, e);
    }
}

void AppointmentPanel::removeClicked() {
    QString id = selectedAppointmentId();
    if (id.isEmpty()) {
        QMessageBox::information(this, "Remove Appointment", "Please select an appointment first.");
        return;
    }
    if (QMessageBox::question(this, "Remove Appointment",
                              "Remove appointment " + id + " completely?\n(You can undo this with Ctrl+Z.)")
        != QMessageBox::Yes) {
        return;
    }
    try {
        manager.removeAppointment(toStd(id));
        emit dataChanged();
        emit statusMessage("Removed appointment " + id + " (Ctrl+Z to undo)");
    } catch (const std::exception& e) {
        showError(this, e);
    }
}

} // namespace scapms::gui
