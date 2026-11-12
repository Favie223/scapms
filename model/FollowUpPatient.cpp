// =====================================================================
// FollowUpPatient.cpp
// =====================================================================
#include "FollowUpPatient.h"
#include "Appointment.h"
#include "../utils/Exceptions.h"
#include "../utils/Validation.h"

namespace scapms::model {

using namespace scapms::utils;

FollowUpPatient::FollowUpPatient(const std::string& id, const std::string& name, int age,
                                 const ContactInfo& contact,
                                 const std::string& previousVisitDate,
                                 const std::string& referringDoctor,
                                 const std::string& followUpReason)
    : PatientRecord(id, name, age, contact),
      referringDoctor(Validation::trim(referringDoctor)),
      followUpReason(Validation::trim(followUpReason)) {
    setPreviousVisitDate(previousVisitDate);
}

void FollowUpPatient::setPreviousVisitDate(const std::string& date) {
    std::string clean = Validation::trim(date);
    if (!clean.empty()) {
        Validation::validateDate(clean);
        if (clean > Validation::today()) {
            throw ValidationException("previous visit date cannot be in the future.");
        }
    }
    previousVisitDate = clean;
}

std::string FollowUpPatient::describe() const {
    std::string text = "Follow-up for " + getName() + " (age " + std::to_string(getAge()) + ")";
    if (!previousVisitDate.empty()) text += ", last seen " + previousVisitDate;
    if (!followUpReason.empty()) text += " - " + followUpReason;
    return text;
}

// Order of the 3 extra fields: previous visit date, referring doctor, reason.
std::vector<std::string> FollowUpPatient::getExtraFields() const {
    return {previousVisitDate, referringDoctor, followUpReason};
}

void FollowUpPatient::setExtraFields(const std::vector<std::string>& values) {
    setPreviousVisitDate(values.size() > 0 ? values[0] : "");
    referringDoctor = values.size() > 1 ? Validation::trim(values[1]) : "";
    followUpReason = values.size() > 2 ? Validation::trim(values[2]) : "";
}

std::unique_ptr<PatientRecord> FollowUpPatient::clone() const {
    return std::unique_ptr<PatientRecord>(new FollowUpPatient(*this));
}

// RULE: clinic hours, not in the past, and strictly AFTER the previous visit.
bool FollowUpPatient::canSchedule(const std::string& dateTime, std::string& reason) const {
    if (!Validation::isValidDateTime(dateTime)) {
        reason = "date/time must be in the form YYYY-MM-DD HH:MM.";
        return false;
    }
    if (dateTime < Validation::now()) {
        reason = "follow-up appointments cannot be booked in the past.";
        return false;
    }
    if (!previousVisitDate.empty() && dateTime.substr(0, 10) <= previousVisitDate) {
        reason = "a follow-up must be after the previous visit (" + previousVisitDate + ").";
        return false;
    }
    int hour = std::stoi(dateTime.substr(11, 2));
    if (hour < 8 || hour > 17) {
        reason = "follow-up appointments must be between 08:00 and 17:59 (clinic hours).";
        return false;
    }
    return true;
}

void FollowUpPatient::scheduleAppointment(Appointment& appointment) {
    std::string reason;
    if (!canSchedule(appointment.getDateTime(), reason)) {
        throw InvalidAppointmentException(reason);
    }
    appointment.setStatus(AppointmentStatus::Pending);
    addAppointmentId(appointment.getId());
}

void FollowUpPatient::cancelAppointment(Appointment& appointment) {
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
