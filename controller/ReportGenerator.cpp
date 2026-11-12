// =====================================================================
// ReportGenerator.cpp
// =====================================================================
#include "ReportGenerator.h"

namespace scapms::controller {

using namespace scapms::model;

// Sorts a PatientCount list from the BIGGEST count to the smallest.
// A small hand-written insertion sort (no std::sort, as the brief
// requires); ties keep their original order because it is stable.
static void sortByCountDescending(std::vector<PatientCount>& list) {
    for (int i = 1; i < (int)list.size(); i++) {
        PatientCount current = list[i];
        int j = i - 1;
        while (j >= 0 && list[j].count < current.count) {
            list[j + 1] = list[j];
            j--;
        }
        list[j + 1] = current;
    }
}

std::vector<PatientCount> ReportGenerator::mostFrequentlyAttended(const ClinicDatabase& database, int limit) {
    std::vector<PatientCount> result;
    for (PatientRecord* patient : database.getPatientList()) {
        int completed = database.countAppointments(patient->getId(), AppointmentStatus::Completed);
        if (completed > 0) {
            PatientCount entry;
            entry.patient = patient;
            entry.count = completed;
            result.push_back(entry);
        }
    }
    sortByCountDescending(result);
    if (limit > 0 && (int)result.size() > limit) {
        result.resize(limit); // keep only the top `limit`
    }
    return result;
}

std::vector<PatientCount> ReportGenerator::patientsWithPendingAppointments(const ClinicDatabase& database) {
    std::vector<PatientCount> result;
    for (PatientRecord* patient : database.getPatientList()) {
        int pending = database.countAppointments(patient->getId(), AppointmentStatus::Pending);
        if (pending > 0) {
            PatientCount entry;
            entry.patient = patient;
            entry.count = pending;
            result.push_back(entry);
        }
    }
    sortByCountDescending(result);
    return result;
}

// RUNTIME POLYMORPHISM: `patient` is a PatientRecord* (base pointer).
// getCategory() is virtual, so C++ looks up the real object's type at
// runtime and calls GeneralPatient::getCategory(),
// EmergencyPatient::getCategory() or FollowUpPatient::getCategory().
CategoryDistribution ReportGenerator::categoryDistribution(const std::vector<PatientRecord*>& patients) {
    CategoryDistribution counts;
    for (const PatientRecord* patient : patients) {
        std::string category = patient->getCategory(); // <-- virtual call
        if (category == "General") counts.general++;
        else if (category == "Emergency") counts.emergency++;
        else if (category == "Follow-Up") counts.followUp++;
    }
    return counts;
}

DailySummary ReportGenerator::dailySummary(const ClinicDatabase& database, const std::string& date) {
    DailySummary summary;
    summary.date = date;
    for (const Appointment& appointment : database.getAppointments()) {
        if (appointment.getDate() != date) continue; // other day, skip
        summary.total++;
        switch (appointment.getStatus()) {
            case AppointmentStatus::Completed: summary.completed++; break;
            case AppointmentStatus::Pending:   summary.pending++;   break;
            case AppointmentStatus::Cancelled: summary.cancelled++; break;
        }
    }
    return summary;
}

std::vector<std::string> ReportGenerator::reportNames() {
    return {"Most Frequently Attended Patients", "Patients with Pending Appointments",
            "Patient Category Distribution", "Daily Appointment Summary"};
}

// Builds a printable version of a report.
std::string ReportGenerator::buildReportText(const ClinicDatabase& database, int reportIndex,
                                             const std::string& date) {
    std::string text = "SCAPMS REPORT: " + reportNames()[reportIndex] + "\n";
    text += "------------------------------------------------\n";

    if (reportIndex == 0) {
        std::vector<PatientCount> rows = mostFrequentlyAttended(database, 10);
        if (rows.empty()) text += "No completed appointments yet.\n";
        for (const PatientCount& row : rows) {
            text += row.patient->getId() + "  " + row.patient->getName() + "  ("
                    + row.patient->getCategory() + ")  completed visits: " + std::to_string(row.count) + "\n";
        }
    } else if (reportIndex == 1) {
        std::vector<PatientCount> rows = patientsWithPendingAppointments(database);
        if (rows.empty()) text += "No pending appointments.\n";
        for (const PatientCount& row : rows) {
            text += row.patient->getId() + "  " + row.patient->getName() + "  pending: "
                    + std::to_string(row.count) + "  next: "
                    + database.getNextAppointmentDate(row.patient->getId()) + "\n";
        }
    } else if (reportIndex == 2) {
        CategoryDistribution counts = categoryDistribution(database.getPatientList());
        text += "General Patients     " + std::to_string(counts.general) + "\n";
        text += "Emergency Patients   " + std::to_string(counts.emergency) + "\n";
        text += "Follow-Up Patients   " + std::to_string(counts.followUp) + "\n";
        text += "Total                " + std::to_string(counts.total()) + "\n";
    } else {
        DailySummary summary = dailySummary(database, date);
        text += "Date: " + summary.date + "\n";
        text += "Total Appointments: " + std::to_string(summary.total) + "\n";
        text += "Completed: " + std::to_string(summary.completed) + "\n";
        text += "Pending: " + std::to_string(summary.pending) + "\n";
        text += "Cancelled: " + std::to_string(summary.cancelled) + "\n";
    }
    return text;
}

} // namespace scapms::controller
