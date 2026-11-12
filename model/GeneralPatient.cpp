// =====================================================================
// GeneralPatient.cpp
// =====================================================================
#include "GeneralPatient.h"
#include "Appointment.h"
#include "../utils/Exceptions.h"
#include "../utils/Validation.h"

namespace scapms::model {

using namespace scapms::utils;

// The ": PatientRecord(...)" part calls the BASE class constructor first,
// which validates and stores the common fields.
GeneralPatient::GeneralPatient(const std::string& id, const std::string& name, int age,
                               const ContactInfo& contact,
                               const std::string& insuranceNumber,
                               const std::string& complaint)
    : PatientRecord(id, name, age, contact),
      insuranceNumber(Validation::trim(insuranceNumber)),
      complaint(Validation::trim(complaint)) {}

std::string GeneralPatient::describe() const {
    std::string text = "General patient " + getName() + " (age " + std::to_string(getAge()) + ")";
    if (!complaint.empty()) text += " - complaint: " + complaint;
    return text;
}

// Order of the 3 extra fields: insurance number, complaint, (unused).
std::vector<std::string> GeneralPatient::getExtraFields() const {
    return {insuranceNumber, complaint, ""};
}

void GeneralPatient::setExtraFields(const std::vector<std::string>& values) {
    insuranceNumber = values.size() > 0 ? Validation::trim(values[0]) : "";
    complaint = values.size() > 1 ? Validation::trim(values[1]) : "";
}

// "new GeneralPatient(*this)" uses the automatic copy constructor, which
// copies every field (including the ones in PatientRecord).
std::unique_ptr<PatientRecord> GeneralPatient::clone() const {
    return std::unique_ptr<PatientRecord>(new GeneralPatient(*this));
}

// RULE: general patients book during clinic hours (08:00-17:59) and
// never in the past.
bool GeneralPatient::canSchedule(const std::string& dateTime, std::string& reason) const {
    if (!Validation::isValidDateTime(dateTime)) {
        reason = "date/time must be in the form YYYY-MM-DD HH:MM.";
        return false;
    }
    if (dateTime < Validation::now()) {
        reason = "general appointments cannot be booked in the past.";
        return false;
    }
    int hour = std::stoi(dateTime.substr(11, 2));
    if (hour < 8 || hour > 17) {
        reason = "general appointments must be between 08:00 and 17:59 (clinic hours).";
        return false;
    }
    return true;
}

void GeneralPatient::scheduleAppointment(Appointment& appointment) {
    std::string reason;
    if (!canSchedule(appointment.getDateTime(), reason)) {
        throw InvalidAppointmentException(reason);
    }
    appointment.setStatus(AppointmentStatus::Pending);
    addAppointmentId(appointment.getId()); // link it to this patient
}

void GeneralPatient::cancelAppointment(Appointment& appointment) {
    if (!hasAppointment(appointment.getId())) {
        throw InvalidAppointmentException("this appointment does not belong to " + getName() + ".");
    }
    if (!appointment.isPending()) {
        throw InvalidAppointmentException("only pending appointments can be cancelled (this one is "
                                          + appointment.getStatusText() + ").");
    }
    appointment.setStatus(AppointmentStatus::Cancelled);
}

} // namespace scapms::model
