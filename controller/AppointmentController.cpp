// =====================================================================
// AppointmentController.cpp
// =====================================================================
#include "AppointmentController.h"
#include "../model/Appointable.h"
#include "../utils/Exceptions.h"

namespace scapms::controller {

using namespace scapms::model;
using namespace scapms::utils;

AppointmentController::AppointmentController(ClinicDatabase& database, IDGenerator& idGenerator)
    : database(database), idGenerator(idGenerator) {}

Appointment& AppointmentController::getAppointmentOrThrow(const std::string& appointmentId) {
    Appointment* appointment = database.findAppointment(appointmentId);
    if (appointment == nullptr) {
        throw NotFoundException("appointment " + appointmentId);
    }
    return *appointment;
}

std::string AppointmentController::schedule(const std::string& patientId, const std::string& dateTime,
                                            const std::string& doctor, const std::string& reason) {
    PatientRecord* patient = database.findPatient(patientId);
    if (patient == nullptr) {
        throw NotFoundException("patient " + patientId);
    }

    // Ask: "does this patient object implement the Appointable interface?"
    Appointable* bookable = dynamic_cast<Appointable*>(patient);
    if (bookable == nullptr) {
        throw InvalidAppointmentException(patient->getCategory() + " patients cannot book appointments.");
    }

    // Build the appointment with the NEXT id but don't use the id up yet:
    // if validation fails below, the next appointment can still have it.
    // (The Appointment constructor checks date format, doctor and reason.)
    Appointment appointment(idGenerator.peek(), patient->getId(), dateTime, doctor, reason);

    // POLYMORPHISM: runs GeneralPatient's / EmergencyPatient's /
    // FollowUpPatient's own booking rules. Throws if not allowed.
    bookable->scheduleAppointment(appointment);

    idGenerator.next(); // now the id is officially used
    database.addAppointment(appointment);
    return appointment.getId();
}

void AppointmentController::cancel(const std::string& appointmentId) {
    Appointment& appointment = getAppointmentOrThrow(appointmentId);
    PatientRecord* patient = database.findPatient(appointment.getPatientId());
    if (patient == nullptr) {
        throw NotFoundException("patient " + appointment.getPatientId());
    }
    Appointable* bookable = dynamic_cast<Appointable*>(patient);
    if (bookable == nullptr) {
        throw InvalidAppointmentException("this patient type cannot cancel appointments.");
    }
    bookable->cancelAppointment(appointment); // subclass rules (e.g. severity 5 can't cancel)
}

void AppointmentController::complete(const std::string& appointmentId) {
    Appointment& appointment = getAppointmentOrThrow(appointmentId);
    if (!appointment.isPending()) {
        throw InvalidAppointmentException("only pending appointments can be marked completed (this one is "
                                          + appointment.getStatusText() + ").");
    }
    appointment.setStatus(AppointmentStatus::Completed);
}

Appointment AppointmentController::remove(const std::string& appointmentId) {
    Appointment removed = database.removeAppointment(appointmentId); // throws if missing
    PatientRecord* patient = database.findPatient(removed.getPatientId());
    if (patient != nullptr) {
        patient->removeAppointmentId(appointmentId); // unlink from the patient
    }
    return removed;
}

void AppointmentController::restore(const Appointment& appointment) {
    database.addAppointment(appointment);
    PatientRecord* patient = database.findPatient(appointment.getPatientId());
    if (patient != nullptr) {
        patient->addAppointmentId(appointment.getId()); // re-link
    }
}

void AppointmentController::reopen(const std::string& appointmentId) {
    Appointment& appointment = getAppointmentOrThrow(appointmentId);
    appointment.setStatus(AppointmentStatus::Pending);
}

} // namespace scapms::controller
