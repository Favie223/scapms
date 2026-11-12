// =====================================================================
// Appointment.cpp
// =====================================================================
#include "Appointment.h"
#include "../utils/Exceptions.h"
#include "../utils/Validation.h"

namespace scapms::model {

using namespace scapms::utils;

std::string statusToString(AppointmentStatus status) {
    switch (status) {
        case AppointmentStatus::Pending:   return "Pending";
        case AppointmentStatus::Completed: return "Completed";
        case AppointmentStatus::Cancelled: return "Cancelled";
    }
    return "Pending"; // never reached, keeps the compiler happy
}

AppointmentStatus statusFromString(const std::string& text) {
    std::string lower = Validation::toLower(Validation::trim(text));
    if (lower == "pending")   return AppointmentStatus::Pending;
    if (lower == "completed") return AppointmentStatus::Completed;
    if (lower == "cancelled") return AppointmentStatus::Cancelled;
    throw InvalidAppointmentException("unknown status '" + text + "'.");
}

Appointment::Appointment(const std::string& id, const std::string& patientId,
                         const std::string& dateTime, const std::string& doctor,
                         const std::string& reason, AppointmentStatus status)
    : id(id), patientId(patientId), status(status) {
    if (Validation::trim(patientId).empty()) {
        throw InvalidAppointmentException("an appointment must belong to a patient.");
    }
    setDateTime(dateTime);
    setDoctor(doctor);
    setReason(reason);
}

void Appointment::setDateTime(const std::string& newDateTime) {
    std::string clean = Validation::trim(newDateTime);
    Validation::validateDateTime(clean);
    dateTime = clean;
}

void Appointment::setDoctor(const std::string& newDoctor) {
    std::string clean = Validation::trim(newDoctor);
    if (clean.empty()) {
        throw InvalidAppointmentException("doctor name is required.");
    }
    doctor = clean;
}

void Appointment::setReason(const std::string& newReason) {
    std::string clean = Validation::trim(newReason);
    if (clean.empty()) {
        throw InvalidAppointmentException("reason for the visit is required.");
    }
    reason = clean;
}

} // namespace scapms::model
