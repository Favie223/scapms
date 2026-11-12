// =====================================================================
// Appointable.h
// ---------------------------------------------------------------------
// REQUIREMENT 1c (Interface / Abstract Interface).
//
// C++ has no "interface" keyword like Java. Instead we write a class
// that has ONLY pure virtual functions and no data. Any class that
// inherits from Appointable PROMISES to provide these operations.
//
// All three patient types inherit from BOTH PatientRecord and
// Appointable (multiple inheritance):
//
//     class GeneralPatient : public PatientRecord, public Appointable
//
// Each type has its own booking rule (see canSchedule in each subclass):
//   - GeneralPatient   : clinic hours only (08:00-17:59), not in the past
//   - EmergencyPatient : any time of day, even right now
//   - FollowUpPatient  : clinic hours, and only AFTER the previous visit
//
// Header-only: an interface has no code of its own, so no .cpp file.
// =====================================================================
#ifndef SCAPMS_MODEL_APPOINTABLE_H
#define SCAPMS_MODEL_APPOINTABLE_H

#include <string>

namespace scapms::model {

class Appointment; // "forward declaration": we only use references here

class Appointable {
public:
    virtual ~Appointable() = default;

    // Returns true if this patient may book at that date/time.
    // If not, `reason` is filled with an explanation for the user.
    virtual bool canSchedule(const std::string& dateTime, std::string& reason) const = 0;

    // Books the appointment for this patient (throws
    // InvalidAppointmentException if canSchedule says no).
    virtual void scheduleAppointment(Appointment& appointment) = 0;

    // Cancels one of this patient's pending appointments.
    virtual void cancelAppointment(Appointment& appointment) = 0;
};

} // namespace scapms::model

#endif
