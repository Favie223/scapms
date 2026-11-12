// =====================================================================
// MainWindow.cpp
// =====================================================================
#include "MainWindow.h"
#include "AdminPanel.h"
#include "AppointmentPanel.h"
#include "DashboardPanel.h"
#include "GuiHelpers.h"
#include "PatientPanel.h"
#include "QueuePanel.h"
#include "ReportsPanel.h"
#include "SearchPanel.h"
#include "../model/PatientRecord.h"
#include "../utils/FileHandler.h"

#include <QAction>
#include <QCloseEvent>
#include <QFileDialog>
#include <QKeySequence>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>

namespace scapms::gui {

MainWindow::MainWindow(const QString& dataFolder, QWidget* parent)
    : QMainWindow(parent), dataFolder(dataFolder) {
    setWindowTitle("SCAPMS - Smart Clinic Appointment & Patient Management System");
    resize(1100, 720);

    // Load saved data BEFORE building the panels, so they start filled in.
    loadFolder(dataFolder, true);

    // ---------------- Tabs: one panel per section ----------------
    // Every panel gets a reference to the SAME manager object.
    dashboardPanel = new DashboardPanel(manager);
    patientPanel = new PatientPanel(manager);
    appointmentPanel = new AppointmentPanel(manager);
    queuePanel = new QueuePanel(manager);
    searchPanel = new SearchPanel(manager);
    adminPanel = new AdminPanel(manager);
    reportsPanel = new ReportsPanel(manager);

    tabs = new QTabWidget;
    tabs->addTab(dashboardPanel, "Dashboard");
    tabs->addTab(patientPanel, "Patient Records");
    tabs->addTab(appointmentPanel, "Appointments");
    tabs->addTab(queuePanel, "Waiting Queue");
    tabs->addTab(searchPanel, "Search && Sort"); // "&&" shows a single "&"
    tabs->addTab(adminPanel, "Admin (Undo)");
    tabs->addTab(reportsPanel, "Reports");
    setCentralWidget(tabs);

    // ---------------- Signals & slots between panels ----------------
    // Whenever ANY panel changes data, refresh ALL panels.
    connect(patientPanel, &PatientPanel::dataChanged, this, &MainWindow::refreshAll);
    connect(appointmentPanel, &AppointmentPanel::dataChanged, this, &MainWindow::refreshAll);
    connect(queuePanel, &QueuePanel::dataChanged, this, &MainWindow::refreshAll);
    connect(searchPanel, &SearchPanel::dataChanged, this, &MainWindow::refreshAll);
    connect(adminPanel, &AdminPanel::dataChanged, this, &MainWindow::refreshAll);

    // Panels' status messages go to the status bar.
    connect(patientPanel, &PatientPanel::statusMessage, this, &MainWindow::showStatus);
    connect(appointmentPanel, &AppointmentPanel::statusMessage, this, &MainWindow::showStatus);
    connect(queuePanel, &QueuePanel::statusMessage, this, &MainWindow::showStatus);
    connect(searchPanel, &SearchPanel::statusMessage, this, &MainWindow::showStatus);
    connect(adminPanel, &AdminPanel::statusMessage, this, &MainWindow::showStatus);
    connect(reportsPanel, &ReportsPanel::statusMessage, this, &MainWindow::showStatus);

    // Refresh the dashboard as soon as the user switches back to it.
    connect(tabs, &QTabWidget::currentChanged, this, [this](int index) {
        if (tabs->widget(index) == dashboardPanel) dashboardPanel->refresh();
    });

    // ---------------- Status bar ----------------
    countsLabel = new QLabel;
    statusBar()->addPermanentWidget(countsLabel);

    buildMenus();
    buildTimers();
    updateStatusCounts();
    showStatus("Data folder: " + dataFolder);
}

// =====================================================================
// Menus and keyboard shortcuts
// =====================================================================
void MainWindow::buildMenus() {
    // ---- File ----
    QMenu* fileMenu = menuBar()->addMenu("&File");

    QAction* newAction = fileMenu->addAction("&New Clinic");
    newAction->setToolTip("Start with an empty clinic");
    connect(newAction, &QAction::triggered, this, &MainWindow::newClinic);

    QAction* newPatientAction = fileMenu->addAction("New &Patient...");
    newPatientAction->setShortcut(QKeySequence::New); // Ctrl+N
    connect(newPatientAction, &QAction::triggered, patientPanel, &PatientPanel::addPatient);

    fileMenu->addSeparator();

    QAction* saveAction = fileMenu->addAction("&Save");
    saveAction->setShortcut(QKeySequence::Save); // Ctrl+S
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveData);

    QAction* loadAction = fileMenu->addAction("&Load...");
    loadAction->setShortcut(QKeySequence::Open); // Ctrl+O
    connect(loadAction, &QAction::triggered, this, &MainWindow::loadData);

    fileMenu->addSeparator();

    QAction* importAction = fileMenu->addAction("&Import Patients (CSV)...");
    connect(importAction, &QAction::triggered, this, &MainWindow::importPatients);

    QAction* exportAction = fileMenu->addAction("&Export Patients (CSV)...");
    connect(exportAction, &QAction::triggered, this, &MainWindow::exportPatients);

    fileMenu->addSeparator();

    QAction* exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q)); // Ctrl+Q
    connect(exitAction, &QAction::triggered, this, &QWidget::close); // goes through closeEvent

    // ---- Edit ----
    QMenu* editMenu = menuBar()->addMenu("&Edit");

    QAction* undoAction = editMenu->addAction("&Undo Last Operation");
    undoAction->setShortcut(QKeySequence::Undo); // Ctrl+Z
    connect(undoAction, &QAction::triggered, this, &MainWindow::undo);

    QAction* searchAction = editMenu->addAction("&Search Patients");
    searchAction->setShortcut(QKeySequence::Find); // Ctrl+F
    connect(searchAction, &QAction::triggered, this, &MainWindow::focusSearch);

    // ---- Help ----
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    QAction* aboutAction = helpMenu->addAction("&About SCAPMS");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
}

// =====================================================================
// Timers (TIMER EVENTS)
// A QTimer emits timeout() every N milliseconds; we connect that
// signal to a slot, just like a button's clicked() signal.
// =====================================================================
void MainWindow::buildTimers() {
    // 1. Appointment reminders - check once a minute.
    reminderTimer = new QTimer(this);
    connect(reminderTimer, &QTimer::timeout, this, &MainWindow::checkReminders);
    reminderTimer->start(60 * 1000);

    // 2. Dashboard refresh - every 5 seconds.
    dashboardTimer = new QTimer(this);
    connect(dashboardTimer, &QTimer::timeout, dashboardPanel, &DashboardPanel::refresh);
    dashboardTimer->start(5 * 1000);

    // 3. Waiting-time alerts - every 30 seconds.
    waitTimer = new QTimer(this);
    connect(waitTimer, &QTimer::timeout, this, &MainWindow::checkWaitingTimes);
    waitTimer->start(30 * 1000);

    // Run the reminder check once at start-up (after the window appears).
    QTimer::singleShot(1500, this, &MainWindow::checkReminders);
}

void MainWindow::checkReminders() {
    // Appointments that start within the next 30 minutes.
    for (const model::Appointment& appointment : manager.getUpcomingAppointments(30)) {
        QString id = toQ(appointment.getId());
        if (remindedAppointments.contains(id)) continue; // already reminded
        remindedAppointments.insert(id);

        model::PatientRecord* patient = manager.getPatient(appointment.getPatientId());
        QString text = QString("Reminder: %1 has an appointment with %2 at %3.")
                           .arg(patient ? toQ(patient->getName()) : toQ(appointment.getPatientId()),
                                toQ(appointment.getDoctor()),
                                toQ(appointment.getDateTime().substr(11)));
        showStatus(text);

        // A NON-blocking message box: the user can keep working.
        QMessageBox* box = new QMessageBox(QMessageBox::Information, "Appointment Reminder", text,
                                           QMessageBox::Ok, this);
        box->setAttribute(Qt::WA_DeleteOnClose); // free the memory when closed
        box->setModal(false);
        box->show();
    }
}

void MainWindow::checkWaitingTimes() {
    queuePanel->refresh(); // update the "minutes waiting" column
    int longWaits = queuePanel->countLongWaits();
    if (longWaits > 0) {
        showStatus(QString("Alert: %1 patient(s) have waited more than %2 minutes in the queue.")
                       .arg(longWaits).arg(QueuePanel::kLongWaitMinutes));
    }
}

// =====================================================================
// Keeping everything in sync
// =====================================================================
void MainWindow::refreshAll() {
    dashboardPanel->refresh();
    patientPanel->refresh();
    appointmentPanel->refresh();
    queuePanel->refresh();
    searchPanel->refresh();
    adminPanel->refresh();
    reportsPanel->refresh();
    updateStatusCounts();
}

void MainWindow::showStatus(const QString& message) {
    statusBar()->showMessage(message, 8000); // disappears after 8 seconds
    updateStatusCounts();
}

void MainWindow::updateStatusCounts() {
    if (countsLabel == nullptr) return;
    QString text = QString("%1 patients  |  %2 appointments  |  %3 waiting  |  %4")
                       .arg(manager.getDatabase().patientCount())
                       .arg(manager.getAppointments().size())
                       .arg(manager.getQueueSize())
                       .arg(toQ(manager.getSortDescription()));
    if (manager.hasUnsavedChanges()) text += "  |  * unsaved changes";
    countsLabel->setText(text);
}

// =====================================================================
// File menu
// =====================================================================

// Asks "Save changes?" if needed. Returns false if the user pressed Cancel.
bool MainWindow::confirmDiscardChanges() {
    if (!manager.hasUnsavedChanges()) return true;
    QMessageBox::StandardButton answer = QMessageBox::question(
        this, "Unsaved Changes", "You have unsaved changes. Save them first?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Cancel) return false;
    if (answer == QMessageBox::Save) {
        saveData();
        return !manager.hasUnsavedChanges(); // false if the save failed
    }
    return true; // Discard
}

void MainWindow::newClinic() {
    if (!confirmDiscardChanges()) return;
    manager.newClinic();
    remindedAppointments.clear();
    refreshAll();
    showStatus("Started a new, empty clinic.");
}

void MainWindow::saveData() {
    try {
        manager.saveToFolder(toStd(dataFolder));
        showStatus("Saved to " + dataFolder);
    } catch (const std::exception& e) {
        showError(this, e);
    }
}

// FILE DIALOG: choose a folder that contains patients.csv.
void MainWindow::loadData() {
    if (!confirmDiscardChanges()) return;
    QString folder = QFileDialog::getExistingDirectory(this, "Choose the folder that contains patients.csv",
                                                       dataFolder);
    if (folder.isEmpty()) return; // cancelled
    loadFolder(folder, false);
    refreshAll();
}

void MainWindow::loadFolder(const QString& folder, bool quietIfMissing) {
    std::string patientsFile = toStd(folder) + "/patients.csv";
    if (quietIfMissing && !utils::FileHandler::fileExists(patientsFile)) {
        return; // first run: no data yet, start empty
    }
    try {
        manager.loadFromFolder(toStd(folder)); // keeps old data if this throws
        dataFolder = folder;
        remindedAppointments.clear();
        if (statusBar()) showStatus("Loaded data from " + folder);
    } catch (const std::exception& e) {
        // FILE LOADING FAILURE: tell the user, keep running.
        QMessageBox::warning(this, "Could not load data",
                             QString("%1\n\nThe previous data has been kept.").arg(e.what()));
    }
}

void MainWindow::importPatients() {
    QString path = QFileDialog::getOpenFileName(this, "Import Patients", dataFolder, "CSV files (*.csv)");
    if (path.isEmpty()) return;
    try {
        std::vector<std::string> skipped;
        int added = manager.importPatients(toStd(path), skipped);
        refreshAll();
        QString message = QString("Imported %1 patient(s).").arg(added);
        if (!skipped.empty()) {
            message += QString("\nSkipped %1 with IDs that already exist.").arg(skipped.size());
        }
        QMessageBox::information(this, "Import Patients", message);
    } catch (const std::exception& e) {
        showError(this, e);
    }
}

void MainWindow::exportPatients() {
    QString path = QFileDialog::getSaveFileName(this, "Export Patients", dataFolder + "/patients_export.csv",
                                                "CSV files (*.csv)");
    if (path.isEmpty()) return;
    try {
        manager.exportPatients(toStd(path));
        showStatus("Exported patients to " + path);
    } catch (const std::exception& e) {
        showError(this, e);
    }
}

// =====================================================================
// Edit / Help
// =====================================================================
void MainWindow::undo() {
    adminPanel->undoClicked();
}

void MainWindow::focusSearch() {
    tabs->setCurrentWidget(searchPanel);
    searchPanel->focusSearchBox();
}

void MainWindow::showAbout() {
    QMessageBox::about(this, "About SCAPMS",
                       "<b>Smart Clinic Appointment &amp; Patient Management System</b><br>"
                       "COS202 Programming II lab project.<br><br>"
                       "C++17 + Qt 6 Widgets.<br>"
                       "Shortcuts: Ctrl+N new patient, Ctrl+S save, Ctrl+O load, "
                       "Ctrl+F search, Ctrl+Z undo, Ctrl+Q exit.");
}

// =====================================================================
// Closing the window
// =====================================================================
void MainWindow::closeEvent(QCloseEvent* event) {
    if (confirmDiscardChanges()) {
        event->accept(); // close
    } else {
        event->ignore(); // stay open
    }
}

} // namespace scapms::gui
