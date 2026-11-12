// =====================================================================
// ClinicDatabase.cpp
// ---------------------------------------------------------------------
// Note: the loops in here are ordinary "walk through the vector" loops
// used for basic bookkeeping. The ASSESSED searching and sorting
// algorithms are in controller/SearchEngine and controller/SortEngine.
// =====================================================================
#include "ClinicDatabase.h"
#include "../utils/Exceptions.h"

namespace scapms::model {

using namespace scapms::utils;

// ---------------------------------------------------------------------
// Patients
// ---------------------------------------------------------------------

void ClinicDatabase::addPatient(std::unique_ptr<PatientRecord> patient) {
    if (!patient) return;
    if (hasPatient(patient->getId())) {
        throw DuplicateIdException(patient->getId());
    }
    // std::move hands ownership of the patient over to the vector.
    patients.push_back(std::move(patient));
}

void ClinicDatabase::insertPatientAt(int index, std::unique_ptr<PatientRecord> patient) {
    if (!patient) return;
    if (hasPatient(patient->getId())) {
        throw DuplicateIdException(patient->getId());
    }
    // Keep the index inside the valid range 0..size.
    if (index < 0) index = 0;
    if (index > (int)patients.size()) index = (int)patients.size();
    patients.insert(patients.begin() + index, std::move(patient));
}

std::unique_ptr<PatientRecord> ClinicDatabase::removePatient(const std::string& id) {
    int index = indexOfPatient(id);
    if (index == -1) {
        throw NotFoundException("patient " + id);
    }
    // Take the patient out of the vector before erasing the empty slot.
    std::unique_ptr<PatientRecord> removed = std::move(patients[index]);
    patients.erase(patients.begin() + index);
    return removed;
}

void ClinicDatabase::replacePatient(const std::string& id, std::unique_ptr<PatientRecord> newVersion) {
    int index = indexOfPatient(id);
    if (index == -1) {
        throw NotFoundException("patient " + id);
    }
    // The old object is deleted automatically when it is overwritten.
    patients[index] = std::move(newVersion);
}

int ClinicDatabase::indexOfPatient(const std::string& id) const {
    for (int i = 0; i < (int)patients.size(); i++) {
        if (patients[i]->getId() == id) return i;
    }
    return -1;
}

PatientRecord* ClinicDatabase::findPatient(const std::string& id) const {
    int index = indexOfPatient(id);
    if (index == -1) return nullptr;
    return patients[index].get(); // .get() gives the plain pointer
}

std::vector<PatientRecord*> ClinicDatabase::getPatientList() const {
    std::vector<PatientRecord*> list;
    for (const auto& patient : patients) {
        list.push_back(patient.get());
    }
    return list;
}

// ---------------------------------------------------------------------
// Appointments
// ---------------------------------------------------------------------

void ClinicDatabase::addAppointment(const Appointment& appointment) {
    for (const Appointment& existing : appointments) {
        if (existing.getId() == appointment.getId()) {
            throw InvalidAppointmentException("appointment ID " + appointment.getId() + " already exists.");
        }
    }
    appointments.push_back(appointment);
}

Appointment ClinicDatabase::removeAppointment(const std::string& id) {
    for (size_t i = 0; i < appointments.size(); i++) {
        if (appointments[i].getId() == id) {
            Appointment removed = appointments[i];
            appointments.erase(appointments.begin() + i);
            return removed;
        }
    }
    throw NotFoundException("appointment " + id);
}

Appointment* ClinicDatabase::findAppointment(const std::string& id) {
    for (Appointment& appointment : appointments) {
        if (appointment.getId() == id) return &appointment;
    }
    return nullptr;
}

std::vector<Appointment> ClinicDatabase::getAppointmentsForPatient(const std::string& patientId) const {
    std::vector<Appointment> result;
    for (const Appointment& appointment : appointments) {
        if (appointment.getPatientId() == patientId) result.push_back(appointment);
    }
    return result;
}

std::string ClinicDatabase::getNextAppointmentDate(const std::string& patientId) const {
    std::string earliest = "";
    for (const Appointment& appointment : appointments) {
        if (appointment.getPatientId() == patientId && appointment.isPending()) {
            if (earliest.empty() || appointment.getDateTime() < earliest) {
                earliest = appointment.getDateTime();
            }
        }
    }
    return earliest;
}

int ClinicDatabase::countAppointments(const std::string& patientId, AppointmentStatus status) const {
    int count = 0;
    for (const Appointment& appointment : appointments) {
        if (appointment.getPatientId() == patientId && appointment.getStatus() == status) count++;
    }
    return count;
}

void ClinicDatabase::clear() {
    patients.clear();     // unique_ptr deletes every patient object
    appointments.clear();
}

} // namespace scapms::model
