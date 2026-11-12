// =====================================================================
// MainWindow.h
// ---------------------------------------------------------------------
// The application window (a QMainWindow). It:
//   - OWNS the ClinicManager (the single copy of all clinic data),
//   - builds the MENU BAR      File / Edit / Help          (Req 6b)
//   - builds the TABS          one panel per tab           (Req 7)
//   - shows a STATUS BAR       counts + last action        (Req 7)
//   - runs three QTIMERS       reminders, dashboard, waits (Req 6d, 9d)
//   - sets KEYBOARD SHORTCUTS  Ctrl+S/F/Q/Z/N/O            (Req 9f)
//   - asks to save before closing if there are unsaved changes.
//
// SIGNALS & SLOTS: each panel emits dataChanged() after it changes
// something. MainWindow connects every panel's signal to refreshAll(),
// so all tabs always show the same, up-to-date data.
// =====================================================================
#ifndef SCAPMS_GUI_MAINWINDOW_H
#define SCAPMS_GUI_MAINWINDOW_H

#include "../controller/ClinicManager.h"

#include <QMainWindow>
#include <QSet>
#include <QString>

class QLabel;
class QTabWidget;
class QTimer;

namespace scapms::gui {

class AdminPanel;
class AppointmentPanel;
class DashboardPanel;
class PatientPanel;
class QueuePanel;
class ReportsPanel;
class SearchPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(const QString& dataFolder, QWidget* parent = nullptr);

protected:
    // Called by Qt when the window is about to close (X button / Ctrl+Q).
    void closeEvent(QCloseEvent* event) override;

private slots:
    void refreshAll();
    void showStatus(const QString& message);

    // File menu
    void newClinic();
    void saveData();
    void loadData();
    void importPatients();
    void exportPatients();

    // Edit / Help menu
    void undo();
    void focusSearch();
    void showAbout();

    // Timers
    void checkReminders();    // every 60 s
    void checkWaitingTimes(); // every 30 s

private:
    void buildMenus();
    void buildTimers();
    void updateStatusCounts();
    bool confirmDiscardChanges(); // true = OK to continue
    void loadFolder(const QString& folder, bool quietIfMissing);

    controller::ClinicManager manager; // ALL the clinic data lives here
    QString dataFolder;                // where patients.csv / appointments.csv are

    QTabWidget* tabs;
    DashboardPanel* dashboardPanel;
    PatientPanel* patientPanel;
    AppointmentPanel* appointmentPanel;
    QueuePanel* queuePanel;
    SearchPanel* searchPanel;
    AdminPanel* adminPanel;
    ReportsPanel* reportsPanel;

    // Permanent text on the right of the status bar. Starts as nullptr
    // because loadFolder() runs before the status bar is built.
    QLabel* countsLabel = nullptr;

    QTimer* reminderTimer;
    QTimer* dashboardTimer;
    QTimer* waitTimer;
    QSet<QString> remindedAppointments; // so each reminder pops up only once
};

} // namespace scapms::gui

#endif
