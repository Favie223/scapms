// =====================================================================
// AppointmentPanel.h  -  the "Appointments" tab
// ---------------------------------------------------------------------
// Schedule, cancel, complete and remove appointments, and view them in a
// colour-coded table (Pending = amber, Completed = green,
// Cancelled = grey).
//
// While the user picks a patient and a date, a hint line checks the
// patient type's own booking rule LIVE (text-field/combo events), so
// mistakes are shown before Schedule is pressed.
// =====================================================================
#ifndef SCAPMS_GUI_APPOINTMENTPANEL_H
#define SCAPMS_GUI_APPOINTMENTPANEL_H

#include "../controller/ClinicManager.h"

#include <QWidget>

class QComboBox;
class QDateTimeEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace scapms::gui {

class AppointmentPanel : public QWidget {
    Q_OBJECT

public:
    explicit AppointmentPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

public slots:
    void refresh();

signals:
    void dataChanged();
    void statusMessage(const QString& message);

private slots:
    void scheduleClicked();
    void cancelClicked();
    void completeClicked();
    void removeClicked();
    void updateBookingHint(); // live check of the booking rule

private:
    QString selectedAppointmentId() const;

    controller::ClinicManager& manager;

    QComboBox* patientCombo;
    QDateTimeEdit* dateTimeEdit;
    QComboBox* doctorCombo;
    QLineEdit* reasonEdit;
    QLabel* bookingHint;

    QComboBox* statusFilter;
    QTableWidget* table;
    QPushButton* cancelButton;
    QPushButton* completeButton;
    QPushButton* removeButton;
};

} // namespace scapms::gui

#endif
