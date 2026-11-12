// =====================================================================
// Appointment.h
// ---------------------------------------------------------------------
// REQUIREMENT 1e (Composition): one appointment at the clinic.
//
// Each appointment stores the ID of the patient it belongs to. One
// patient can have many appointments (a "one-to-many" association).
//
// Dates are stored as text "YYYY-MM-DD HH:MM". Because the biggest unit
// (year) comes first and every part has a fixed width, comparing the
// strings alphabetically gives the same order as comparing the dates:
//     "2026-01-05 09:00" < "2026-11-12 08:30"   (true, January < November)
// That keeps the code simple - no date library is needed.
// =====================================================================
#ifndef SCAPMS_MODEL_APPOINTMENT_H
#define SCAPMS_MODEL_APPOINTMENT_H

#include <string>

namespace scapms::model {

// The 3 possible states of an appointment. "enum class" is a safer enum:
// you must write AppointmentStatus::Pending (it cannot be mixed up with ints).
enum class AppointmentStatus {
    Pending,    // booked, not happened yet
    Completed,  // the patient attended
    Cancelled   // called off
};

// Convert the status to/from text (for tables and CSV files).
std::string statusToString(AppointmentStatus status);
AppointmentStatus statusFromString(const std::string& text); // throws if unknown

class Appointment {
public:
    Appointment() = default; // needed so we can make empty Appointment variables
    Appointment(const std::string& id, const std::string& patientId,
                const std::string& dateTime, const std::string& doctor,
                const std::string& reason,
                AppointmentStatus status = AppointmentStatus::Pending);

    // Getters
    const std::string& getId() const { return id; }
    const std::string& getPatientId() const { return patientId; }
    const std::string& getDateTime() const { return dateTime; }
    std::string getDate() const { return dateTime.substr(0, 10); } // "YYYY-MM-DD"
    const std::string& getDoctor() const { return doctor; }
    const std::string& getReason() const { return reason; }
    AppointmentStatus getStatus() const { return status; }
    std::string getStatusText() const { return statusToString(status); }
    bool isPending() const { return status == AppointmentStatus::Pending; }

    // Setters (validate their input)
    void setDateTime(const std::string& newDateTime);
    void setDoctor(const std::string& newDoctor);
    void setReason(const std::string& newReason);
    void setStatus(AppointmentStatus newStatus) { status = newStatus; }

private:
    std::string id;
    std::string patientId;   // which patient this appointment is for
    std::string dateTime;    // "YYYY-MM-DD HH:MM"
    std::string doctor;
    std::string reason;
    AppointmentStatus status = AppointmentStatus::Pending;
};

} // namespace scapms::model

#endif
