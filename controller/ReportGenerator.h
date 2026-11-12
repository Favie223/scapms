// =====================================================================
// ReportGenerator.h
// ---------------------------------------------------------------------
// SIMPLE REPORTS (the brief asks for at least three; we have four):
//   1. Most Frequently Attended Patients  (most COMPLETED appointments)
//   2. Patients with Pending Appointments
//   3. Patient Category Distribution      (General / Emergency / Follow-Up)
//   4. Daily Appointment Summary          (total / completed / pending / cancelled)
//
// Report 3 is also our clearest RUNTIME POLYMORPHISM example
// (Requirement 1d): it loops over PatientRecord* pointers and calls the
// virtual getCategory(). It never checks the real type itself - each
// object answers for itself at runtime.
// =====================================================================
#ifndef SCAPMS_CONTROLLER_REPORTGENERATOR_H
#define SCAPMS_CONTROLLER_REPORTGENERATOR_H

#include "../model/ClinicDatabase.h"

#include <string>
#include <vector>

namespace scapms::controller {

// A patient together with a number (completed visits, pending count ...).
struct PatientCount {
    model::PatientRecord* patient = nullptr;
    int count = 0;
};

struct CategoryDistribution {
    int general = 0;
    int emergency = 0;
    int followUp = 0;
    int total() const { return general + emergency + followUp; }
};

struct DailySummary {
    std::string date;
    int total = 0;
    int completed = 0;
    int pending = 0;
    int cancelled = 0;
};

namespace ReportGenerator {

    // Report 1: patients with at least one completed appointment, the
    // most visits first. `limit` = how many to return (top 10 etc.).
    std::vector<PatientCount> mostFrequentlyAttended(const model::ClinicDatabase& database, int limit);

    // Report 2: patients with one or more pending appointments.
    std::vector<PatientCount> patientsWithPendingAppointments(const model::ClinicDatabase& database);

    // Report 3: how many patients of each type. Takes base-class
    // pointers on purpose, to show polymorphism.
    CategoryDistribution categoryDistribution(const std::vector<model::PatientRecord*>& patients);

    // Report 4: appointment counts for one day ("YYYY-MM-DD").
    DailySummary dailySummary(const model::ClinicDatabase& database, const std::string& date);

    // The reports as plain text (for the Reports tab and for exporting).
    std::vector<std::string> reportNames();
    std::string buildReportText(const model::ClinicDatabase& database, int reportIndex,
                                const std::string& date);

} // namespace ReportGenerator

} // namespace scapms::controller

#endif
