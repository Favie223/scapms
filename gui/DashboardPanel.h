// =====================================================================
// DashboardPanel.h  -  the "Dashboard" tab (first thing the user sees)
// ---------------------------------------------------------------------
// - Six "stat cards" laid out with a QGridLayout (2 rows x 3 columns).
// - "Recently viewed patients" = the fixed-size array cache (Req 3d),
//   with its hit/miss counts.
// - "Upcoming appointments" in the next 2 hours.
//
// TIMER EVENT: MainWindow's dashboard timer calls refresh() every few
// seconds so the numbers stay current without the user clicking anything.
// =====================================================================
#ifndef SCAPMS_GUI_DASHBOARDPANEL_H
#define SCAPMS_GUI_DASHBOARDPANEL_H

#include "../controller/ClinicManager.h"

#include <QWidget>

class QLabel;
class QListWidget;

namespace scapms::gui {

class DashboardPanel : public QWidget {
    Q_OBJECT

public:
    explicit DashboardPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

public slots:
    void refresh();

private:
    // Builds one card: a small title above a big number.
    QWidget* makeCard(const QString& title, QLabel*& valueLabel, const QString& tooltip);

    controller::ClinicManager& manager;

    QLabel* totalPatientsValue;
    QLabel* emergencyValue;
    QLabel* queueValue;
    QLabel* todayValue;
    QLabel* pendingValue;
    QLabel* completedValue;

    QListWidget* recentList;
    QLabel* cacheStats;
    QListWidget* upcomingList;
    QLabel* lastRefreshed;
};

} // namespace scapms::gui

#endif
