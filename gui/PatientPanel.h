// =====================================================================
// PatientPanel.h  -  the "Patient Records" tab
// ---------------------------------------------------------------------
// View, add, edit and delete patients; send a patient to the waiting
// queue. A filter box narrows the table while you type.
//
// Signals: when this panel changes data it emits dataChanged(). The
// MainWindow listens for that signal and refreshes every other tab, so
// the panels never need to know about each other.
// =====================================================================
#ifndef SCAPMS_GUI_PATIENTPANEL_H
#define SCAPMS_GUI_PATIENTPANEL_H

#include "../controller/ClinicManager.h"

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QTableView;

namespace scapms::gui {

class PatientTableModel;

class PatientPanel : public QWidget {
    Q_OBJECT

public:
    explicit PatientPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

public slots:
    void refresh();         // reload the table from the manager
    void addPatient();      // also used by the File menu / Ctrl+N
    void editSelected();
    void deleteSelected();

signals:
    void dataChanged();                         // "I changed something"
    void statusMessage(const QString& message); // text for the status bar

private slots:
    void addSelectedToQueue();
    void onSelectionChanged();

private:
    model::PatientRecord* selectedPatient() const;

    controller::ClinicManager& manager;
    PatientTableModel* tableModel;
    QTableView* table;
    QLineEdit* filterEdit;
    QLabel* countLabel;
    QPushButton* editButton;
    QPushButton* deleteButton;
    QPushButton* queueButton;
};

} // namespace scapms::gui

#endif
