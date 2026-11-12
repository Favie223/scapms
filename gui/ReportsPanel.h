// =====================================================================
// ReportsPanel.h  -  the "Reports" tab
// ---------------------------------------------------------------------
// Pick a report from the combo box and press Generate. The result is
// shown in a table and can be exported to a .txt file with a file
// dialog (Requirement 9c).
//
// Dynamic component: the date picker is only enabled for the
// "Daily Appointment Summary" report, which is the only one that needs it.
// =====================================================================
#ifndef SCAPMS_GUI_REPORTSPANEL_H
#define SCAPMS_GUI_REPORTSPANEL_H

#include "../controller/ClinicManager.h"

#include <QWidget>

class QComboBox;
class QDateEdit;
class QLabel;
class QTableWidget;

namespace scapms::gui {

class ReportsPanel : public QWidget {
    Q_OBJECT

public:
    explicit ReportsPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

public slots:
    void refresh(); // regenerate the current report

signals:
    void statusMessage(const QString& message);

private slots:
    void onReportChanged(int index);
    void exportClicked();

private:
    void setRows(const QStringList& headers, const QList<QStringList>& rows);

    controller::ClinicManager& manager;
    QComboBox* reportCombo;
    QDateEdit* dateEdit;
    QLabel* summaryLabel;
    QTableWidget* table;
};

} // namespace scapms::gui

#endif
