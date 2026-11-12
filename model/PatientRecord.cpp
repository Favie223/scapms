// =====================================================================
// PatientRecord.cpp  -  the shared (non-virtual) parts of every patient
// =====================================================================
#include "PatientRecord.h"
#include "../utils/Exceptions.h"
#include "../utils/Validation.h"

#include <cctype>

namespace scapms::model {

using namespace scapms::utils;

// The constructor checks every value. If anything is wrong an exception
// is thrown and the object is never created.
PatientRecord::PatientRecord(const std::string& id, const std::string& name, int age,
                             const ContactInfo& contact) {
    std::string cleanId = Validation::trim(id);
    if (cleanId.empty()) {
        throw ValidationException("patient ID cannot be empty.");
    }
    if (cleanId.size() > 12 || cleanId.find(',') != std::string::npos) {
        throw ValidationException("patient ID must be at most 12 characters and contain no commas.");
    }
    // Store IDs in UPPERCASE so "p0001" and "P0001" are the same patient,
    // and so sorting/binary search by ID always compare the same way.
    for (char& c : cleanId) c = (char)std::toupper((unsigned char)c);
    this->id = cleanId;

    // Reuse the setters so the rules are written in only one place.
    setName(name);
    setAge(age);
    setContact(contact);
    registeredOn = Validation::today();
}

void PatientRecord::setName(const std::string& newName) {
    Validation::validateName(newName);
    name = Validation::trim(newName);
}

void PatientRecord::setAge(int newAge) {
    Validation::validateAge(newAge);
    age = newAge;
}

void PatientRecord::setContact(const ContactInfo& newContact) {
    Validation::validatePhone(newContact.phone);
    Validation::validateEmail(newContact.email);
    contact.phone = Validation::trim(newContact.phone);
    contact.email = Validation::trim(newContact.email);
    contact.address = Validation::trim(newContact.address);
}

void PatientRecord::setRegisteredOn(const std::string& date) {
    Validation::validateDate(date);
    registeredOn = date;
}

void PatientRecord::addAppointmentId(const std::string& appointmentId) {
    if (!hasAppointment(appointmentId)) {
        appointmentIds.push_back(appointmentId);
    }
}

void PatientRecord::removeAppointmentId(const std::string& appointmentId) {
    for (size_t i = 0; i < appointmentIds.size(); i++) {
        if (appointmentIds[i] == appointmentId) {
            appointmentIds.erase(appointmentIds.begin() + i);
            return;
        }
    }
}

bool PatientRecord::hasAppointment(const std::string& appointmentId) const {
    for (const std::string& existing : appointmentIds) {
        if (existing == appointmentId) return true;
    }
    return false;
}

} // namespace scapms::model
