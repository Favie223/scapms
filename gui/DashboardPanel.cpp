// =====================================================================
// DashboardPanel.cpp
// =====================================================================
#include "DashboardPanel.h"
#include "GuiHelpers.h"
#include "../controller/RecentCache.h"
#include "../controller/ReportGenerator.h"
#include "../utils/Validation.h"

#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QTime>
#include <QVBoxLayout>

namespace scapms::gui {

using namespace scapms::controller;

QWidget* DashboardPanel::makeCard(const QString& title, QLabel*& valueLabel, const QString& tooltip) {
    QFrame* card = new QFrame;
    card->setFrameShape(QFrame::StyledPanel);
    card->setToolTip(tooltip);

    QLabel* titleLabel = new QLabel(title);
    valueLabel = new QLabel("0");
    QFont bigFont = valueLabel->font();
    bigFont.setPointSize(bigFont.pointSize() + 14);
    bigFont.setBold(true);
    valueLabel->setFont(bigFont);

    QVBoxLayout* layout = new QVBoxLayout(card);
    layout->addWidget(titleLabel);
    layout->addWidget(valueLabel);
    return card;
}

DashboardPanel::DashboardPanel(ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager(manager) {

    QLabel* title = new QLabel("Smart Clinic Dashboard");
    QFont titleFont = title->font();
    titleFont.setPointSize(titleFont.pointSize() + 8);
    titleFont.setBold(true);
    title->setFont(titleFont);

    // ---- Stat cards in a QGridLayout: addWidget(widget, row, column) ----
    QGridLayout* cards = new QGridLayout;
    cards->addWidget(makeCard("Registered patients", totalPatientsValue, "All patients in the system"), 0, 0);
    cards->addWidget(makeCard("Emergency patients", emergencyValue, "Patients with High Priority"), 0, 1);
    cards->addWidget(makeCard("Waiting in queue", queueValue, "Patients waiting for consultation"), 0, 2);
    cards->addWidget(makeCard("Appointments today", todayValue, "All appointments scheduled for today"), 1, 0);
    cards->addWidget(makeCard("Pending today", pendingValue, "Today's appointments not yet completed"), 1, 1);
    cards->addWidget(makeCard("Completed today", completedValue, "Today's completed appointments"), 1, 2);

    // ---- Recently viewed (the array cache) ----
    recentList = new QListWidget;
    recentList->setToolTip(QString("The last %1 patients you selected (fixed-size array cache)")
                               .arg(RecentCache::CAPACITY));
    cacheStats = new QLabel;
    QGroupBox* recentBox = new QGroupBox("Recently viewed patients (cache)");
    QVBoxLayout* recentLayout = new QVBoxLayout(recentBox);
    recentLayout->addWidget(recentList);
    recentLayout->addWidget(cacheStats);

    // ---- Upcoming appointments ----
    upcomingList = new QListWidget;
    upcomingList->setToolTip("Pending appointments starting in the next 2 hours");
    QGroupBox* upcomingBox = new QGroupBox("Upcoming appointments (next 2 hours)");
    QVBoxLayout* upcomingLayout = new QVBoxLayout(upcomingBox);
    upcomingLayout->addWidget(upcomingList);

    QHBoxLayout* lists = new QHBoxLayout;
    lists->addWidget(recentBox);
    lists->addWidget(upcomingBox);

    lastRefreshed = new QLabel;

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addLayout(cards);
    layout->addLayout(lists, 1);
    layout->addWidget(lastRefreshed);

    refresh();
}

void DashboardPanel::refresh() {
    const model::ClinicDatabase& database = manager.getDatabase();

    CategoryDistribution categories = ReportGenerator::categoryDistribution(database.getPatientList());
    DailySummary today = ReportGenerator::dailySummary(database, utils::Validation::today());

    totalPatientsValue->setText(QString::number(database.patientCount()));
    emergencyValue->setText(QString::number(categories.emergency));
    queueValue->setText(QString::number(manager.getQueueSize()));
    todayValue->setText(QString::number(today.total));
    pendingValue->setText(QString::number(today.pending));
    completedValue->setText(QString::number(today.completed));

    recentList->clear();
    for (model::PatientRecord* patient : manager.getRecentPatients()) {
        recentList->addItem(toQ(patient->getId() + "  " + patient->getName() + "  (" + patient->getCategory() + ")"));
    }
    if (recentList->count() == 0) recentList->addItem("(select a patient on the Patients tab)");
    const RecentCache& cache = manager.getCache();
    cacheStats->setText(QString("%1 / %2 slots used   |   cache hits: %3   misses: %4")
                            .arg(cache.size()).arg(RecentCache::CAPACITY)
                            .arg(cache.getHits()).arg(cache.getMisses()));

    upcomingList->clear();
    for (const model::Appointment& appointment : manager.getUpcomingAppointments(120)) {
        model::PatientRecord* patient = manager.getPatient(appointment.getPatientId());
        upcomingList->addItem(toQ(appointment.getDateTime() + "  " + (patient ? patient->getName() : appointment.getPatientId())
                                  + "  with " + appointment.getDoctor()));
    }
    if (upcomingList->count() == 0) upcomingList->addItem("(nothing in the next 2 hours)");

    lastRefreshed->setText("Last refreshed: " + QTime::currentTime().toString("HH:mm:ss"));
}

} // namespace scapms::gui
