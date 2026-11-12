// =====================================================================
// ClinicDatabase.h
// ---------------------------------------------------------------------
// REQUIREMENT 1e (Encapsulation & Composition) and 3a (std::vector).
//
// ClinicDatabase OWNS all patients and appointments ("composition":
// when the database is destroyed, everything inside it is destroyed too).
//
// Patients are stored as:
//     std::vector<std::unique_ptr<PatientRecord>>
//
//   - std::vector       : a growable array (Requirement 3a).
//   - PatientRecord*    : we must store POINTERS to the base class so
//                         one vector can hold all 3 subclasses
//                         (polymorphism). Storing PatientRecord by value
//                         is impossible because it is abstract.
//   - std::unique_ptr   : a "smart pointer" that deletes the patient
//                         automatically when it is removed from the
//                         vector, so we never leak memory or forget
//                         to call delete.
//
// Both vectors are private; other classes use the public functions,
// which keep the data consistent (e.g. no duplicate IDs).
// =====================================================================
#ifndef SCAPMS_MODEL_CLINICDATABASE_H
#define SCAPMS_MODEL_CLINICDATABASE_H

#include "PatientRecord.h"
#include "Appointment.h"

#include <memory>
#include <string>
#include <vector>

namespace scapms::model {

class ClinicDatabase {
public:
    // ---------------- Patients ----------------

    // Adds a patient. Throws DuplicateIdException if the ID is taken.
    void addPatient(std::unique_ptr<PatientRecord> patient);

    // Puts a patient back at a given position (used by Undo so the
    // patient re-appears where it was before being deleted).
    void insertPatientAt(int index, std::unique_ptr<PatientRecord> patient);

    // Removes a patient and hands it back to the caller (so it can be
    // kept for Undo). Throws NotFoundException if the ID does not exist.
    std::unique_ptr<PatientRecord> removePatient(const std::string& id);

    // Swaps the stored patient with a new version (used when editing).
    void replacePatient(const std::string& id, std::unique_ptr<PatientRecord> newVersion);

    // Returns the patient, or nullptr if there is none with that ID.
    PatientRecord* findPatient(const std::string& id) const;
    int indexOfPatient(const std::string& id) const; // -1 if not found
    bool hasPatient(const std::string& id) const { return indexOfPatient(id) != -1; }

    // A list of plain pointers to every patient, in storage order.
    // Sorting/searching work on this list so they never move the real
    // objects (the database keeps ownership).
    std::vector<PatientRecord*> getPatientList() const;
    int patientCount() const { return (int)patients.size(); }

    // ---------------- Appointments ----------------
    void addAppointment(const Appointment& appointment); // throws on duplicate ID
    Appointment removeAppointment(const std::string& id); // returns the removed one
    Appointment* findAppointment(const std::string& id);
    const std::vector<Appointment>& getAppointments() const { return appointments; }
    std::vector<Appointment> getAppointmentsForPatient(const std::string& patientId) const;

    // Earliest PENDING appointment date/time for a patient, or "" if none.
    // Used when sorting patients by appointment date.
    std::string getNextAppointmentDate(const std::string& patientId) const;

    // Counts a patient's appointments that have the given status.
    int countAppointments(const std::string& patientId, AppointmentStatus status) const;

    // Removes everything (File -> New).
    void clear();

private:
    std::vector<std::unique_ptr<PatientRecord>> patients;
    std::vector<Appointment> appointments;
};

} // namespace scapms::model

#endif
