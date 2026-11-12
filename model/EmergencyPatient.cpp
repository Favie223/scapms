// =====================================================================
// EmergencyPatient.cpp
// =====================================================================
#include "EmergencyPatient.h"
#include "Appointment.h"
#include "../utils/Exceptions.h"
#include "../utils/Validation.h"

namespace scapms::model {

using namespace scapms::utils;

EmergencyPatient::EmergencyPatient(const std::string& id, const std::string& name, int age,
                                   const ContactInfo& contact, int severity,
                                   const std::string& condition,
                                   const std::string& arrivalTime)
    : PatientRecord(id, name, age, contact),
      condition(Validation::trim(condition)) {
    setSeverity(severity);
    // If no arrival time was given, the patient arrived "now".
    this->arrivalTime = Validation::trim(arrivalTime).empty() ? Validation::now()
                                                              : Validation::trim(arrivalTime);
}

void EmergencyPatient::setSeverity(int newSeverity) {
    if (newSeverity < 1 || newSeverity > 5) {
        throw ValidationException("emergency severity must be between 1 and 5.");
    }
    severity = newSeverity;
}

std::string EmergencyPatient::describe() const {
    std::string text = "EMERGENCY: " + getName() + " (age " + std::to_string(getAge())
                       + "), severity " + std::to_string(severity) + "/5";
    if (!condition.empty()) text += " - " + condition;
    return text;
}

// Order of the 3 extra fields: severity, condition, arrival time.
std::vector<std::string> EmergencyPatient::getExtraFields() const {
    return {std::to_string(severity), condition, arrivalTime};
}

void EmergencyPatient::setExtraFields(const std::vector<std::string>& values) {
    // Severity arrives as text (from a file or a text box), so convert it.
    if (values.size() > 0 && !Validation::trim(values[0]).empty()) {
        int value = 0;
        try {
            value = std::stoi(values[0]);
        } catch (...) {
            throw ValidationException("emergency severity must be a number from 1 to 5.");
        }
        setSeverity(value);
    }
    condition = values.size() > 1 ? Validation::trim(values[1]) : "";
    if (values.size() > 2 && !Validation::trim(values[2]).empty()) {
        arrivalTime = Validation::trim(values[2]);
    }
}

std::unique_ptr<PatientRecord> EmergencyPatient::clone() const {
    return std::unique_ptr<PatientRecord>(new EmergencyPatient(*this));
}

// RULE: emergencies can be booked at any hour, from today onwards.
// (Unlike general patients, we allow a time a few minutes ago so staff
//  can log an emergency that has just been seen.)
bool EmergencyPatient::canSchedule(const std::string& dateTime, std::string& reason) const {
    if (!Validation::isValidDateTime(dateTime)) {
        reason = "date/time must be in the form YYYY-MM-DD HH:MM.";
        return false;
    }
    if (dateTime.substr(0, 10) < Validation::today()) {
        reason = "emergency appointments cannot be on a past day.";
        return false;
    }
    return true;
}

void EmergencyPatient::scheduleAppointment(Appointment& appointment) {
    std::string reason;
    if (!canSchedule(appointment.getDateTime(), reason)) {
        throw InvalidAppointmentException(reason);
    }
    appointment.setStatus(AppointmentStatus::Pending);
    addAppointmentId(appointment.getId());
}

void EmergencyPatient::cancelAppointment(Appointment& appointment) {
    if (!hasAppointment(appointment.getId())) {
        throw InvalidAppointmentException("this appointment does not belong to " + getName() + ".");
    }
    if (!appointment.isPending()) {
        throw InvalidAppointmentException("only pending appointments can be cancelled (this one is "
                                          + appointment.getStatusText() + ").");
    }
    // Extra safety rule for emergencies: critical cases (severity 5)
    // must be seen, so their appointments cannot be cancelled.
    if (severity == 5) {
        throw InvalidAppointmentException("critical emergency (severity 5) appointments cannot be cancelled.");
    }
    appointment.setStatus(AppointmentStatus::Cancelled);
}

} // namespace scapms::model
