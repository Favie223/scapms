// =====================================================================
// ReportsPanel.cpp
// =====================================================================
#include "ReportsPanel.h"
#include "GuiHelpers.h"
#include "../controller/ReportGenerator.h"
#include "../utils/FileHandler.h"

#include <QComboBox>
#include <QDateEdit>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace scapms::gui {

using namespace scapms::controller;

ReportsPanel::ReportsPanel(ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager(manager) {

    reportCombo = new QComboBox;
    for (const std::string& name : ReportGenerator::reportNames()) reportCombo->addItem(toQ(name));
    reportCombo->setToolTip("Choose which report to display");

    dateEdit = new QDateEdit(QDate::currentDate());
    dateEdit->setDisplayFormat("yyyy-MM-dd");
    dateEdit->setCalendarPopup(true);
    dateEdit->setToolTip("The day for the Daily Appointment Summary");

    QPushButton* generateButton = new QPushButton("Generate");
    generateButton->setToolTip("Build the selected report from the current data");
    QPushButton* exportButton = new QPushButton("Export...");
    exportButton->setToolTip("Save this report as a text file");

    QHBoxLayout* controls = new QHBoxLayout;
    controls->addWidget(new QLabel("Report:"));
    controls->addWidget(reportCombo, 1);
    controls->addWidget(new QLabel("Date:"));
    controls->addWidget(dateEdit);
    controls->addWidget(generateButton);
    controls->addWidget(exportButton);

    summaryLabel = new QLabel;
    summaryLabel->setWordWrap(true);

    table = new QTableWidget;
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addLayout(controls);
    layout->addWidget(summaryLabel);
    layout->addWidget(table, 1);

    connect(reportCombo, &QComboBox::currentIndexChanged, this, &ReportsPanel::onReportChanged);
    connect(dateEdit, &QDateEdit::dateChanged, this, &ReportsPanel::refresh);
    connect(generateButton, &QPushButton::clicked, this, &ReportsPanel::refresh);
    connect(exportButton, &QPushButton::clicked, this, &ReportsPanel::exportClicked);

    onReportChanged(0);
}

void ReportsPanel::onReportChanged(int index) {
    dateEdit->setEnabled(index == 3); // only the daily summary needs a date
    refresh();
}

void ReportsPanel::setRows(const QStringList& headers, const QList<QStringList>& rows) {
    table->clear();
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->setRowCount(rows.size());
    for (int r = 0; r < rows.size(); r++) {
        for (int c = 0; c < rows[r].size(); c++) {
            table->setItem(r, c, new QTableWidgetItem(rows[r][c]));
        }
    }
    table->resizeColumnsToContents();
}

// A simple text "bar chart": one block per 5% (e.g. 40% -> 8 blocks).
static QString bar(int part, int total) {
    if (total == 0) return QString();
    int percent = part * 100 / total;
    return QString(percent / 5, QChar(0x2588)) + QString("  %1%").arg(percent);
}

void ReportsPanel::refresh() {
    const model::ClinicDatabase& database = manager.getDatabase();
    QList<QStringList> rows;

    switch (reportCombo->currentIndex()) {
        case 0: { // Most frequently attended
            for (const PatientCount& row : ReportGenerator::mostFrequentlyAttended(database, 10)) {
                rows.append({toQ(row.patient->getId()), toQ(row.patient->getName()),
                             toQ(row.patient->getCategory()), QString::number(row.count)});
            }
            setRows({"ID", "Name", "Category", "Completed Visits"}, rows);
            summaryLabel->setText(rows.isEmpty() ? "No completed appointments yet."
                                                 : "Top patients by number of completed appointments.");
            break;
        }
        case 1: { // Pending appointments
            for (const PatientCount& row : ReportGenerator::patientsWithPendingAppointments(database)) {
                rows.append({toQ(row.patient->getId()), toQ(row.patient->getName()), QString::number(row.count),
                             toQ(database.getNextAppointmentDate(row.patient->getId()))});
            }
            setRows({"ID", "Name", "Pending", "Next Appointment"}, rows);
            summaryLabel->setText(QString("%1 patient(s) have appointments that are not yet completed.").arg(rows.size()));
            break;
        }
        case 2: { // Category distribution (polymorphism inside)
            CategoryDistribution counts = ReportGenerator::categoryDistribution(database.getPatientList());
            int total = counts.total();
            rows.append({"General Patients", QString::number(counts.general), bar(counts.general, total)});
            rows.append({"Emergency Patients", QString::number(counts.emergency), bar(counts.emergency, total)});
            rows.append({"Follow-Up Patients", QString::number(counts.followUp), bar(counts.followUp, total)});
            setRows({"Category", "Count", "Share"}, rows);
            summaryLabel->setText(QString("Total patients: %1").arg(total));
            break;
        }
        case 3: { // Daily summary
            std::string date = toStd(dateEdit->date().toString("yyyy-MM-dd"));
            DailySummary summary = ReportGenerator::dailySummary(database, date);
            rows.append({"Total Appointments", QString::number(summary.total)});
            rows.append({"Completed", QString::number(summary.completed)});
            rows.append({"Pending", QString::number(summary.pending)});
            rows.append({"Cancelled", QString::number(summary.cancelled)});
            setRows({"Measure", "Count"}, rows);
            summaryLabel->setText("Appointments on " + toQ(date));
            break;
        }
    }
}

// FILE DIALOG: let the user choose where to save the report.
void ReportsPanel::exportClicked() {
    QString path = QFileDialog::getSaveFileName(this, "Export Report", "report.txt", "Text files (*.txt)");
    if (path.isEmpty()) return; // user cancelled
    try {
        std::string text = ReportGenerator::buildReportText(
            manager.getDatabase(), reportCombo->currentIndex(), toStd(dateEdit->date().toString("yyyy-MM-dd")));
        utils::FileHandler::saveText(toStd(path), text);
        emit statusMessage("Report exported to " + path);
    } catch (const std::exception& e) {
        showError(this, e);
    }
}

} // namespace scapms::gui
