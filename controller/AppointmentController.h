// =====================================================================
// AppointmentController.h
// ---------------------------------------------------------------------
// All the business rules for appointments, kept OUT of the GUI:
// schedule, cancel, mark as completed, remove, restore (for undo).
//
// It uses the Appointable INTERFACE. Every patient type inherits from
// Appointable, so we can ask any patient:
//
//     Appointable* bookable = dynamic_cast<Appointable*>(patient);
//     bookable->scheduleAppointment(appointment);
//
// dynamic_cast checks at RUNTIME that the object really implements
// Appointable. The call then runs the correct subclass's own rules
// (polymorphism) - e.g. EmergencyPatient allows any hour.
// =====================================================================
#ifndef SCAPMS_CONTROLLER_APPOINTMENTCONTROLLER_H
#define SCAPMS_CONTROLLER_APPOINTMENTCONTROLLER_H

#include "../model/Appointment.h"
#include "../model/ClinicDatabase.h"
#include "../utils/IDGenerator.h"

#include <string>

namespace scapms::controller {

class AppointmentController {
public:
    // Works on the database and ID generator owned by ClinicManager
    // (references: this class does not own them).
    AppointmentController(model::ClinicDatabase& database, utils::IDGenerator& idGenerator);

    // Creates a new appointment and returns its ID.
    // Throws NotFoundException / InvalidAppointmentException.
    std::string schedule(const std::string& patientId, const std::string& dateTime,
                         const std::string& doctor, const std::string& reason);

    void cancel(const std::string& appointmentId);   // Pending -> Cancelled
    void complete(const std::string& appointmentId); // Pending -> Completed

    // Deletes the appointment completely and returns a copy (for undo).
    model::Appointment remove(const std::string& appointmentId);

    // Puts back an appointment removed earlier (used by undo).
    void restore(const model::Appointment& appointment);

    // Undo for cancel: Cancelled -> Pending.
    void reopen(const std::string& appointmentId);

private:
    model::Appointment& getAppointmentOrThrow(const std::string& appointmentId);

    model::ClinicDatabase& database;
    utils::IDGenerator& idGenerator;
};

} // namespace scapms::controller

#endif
