// =====================================================================
// PatientFactory.cpp
// =====================================================================
#include "PatientFactory.h"
#include "GeneralPatient.h"
#include "EmergencyPatient.h"
#include "FollowUpPatient.h"
#include "../utils/Exceptions.h"
#include "../utils/Validation.h"

namespace scapms::model {

using namespace scapms::utils;

std::vector<std::string> PatientFactory::typeNames() {
    return {"General", "Emergency", "Follow-Up"};
}

std::unique_ptr<PatientRecord> PatientFactory::create(const std::string& type,
                                                      const std::string& id,
                                                      const std::string& name,
                                                      int age,
                                                      const ContactInfo& contact,
                                                      const std::vector<std::string>& extraFields) {
    std::string lower = Validation::toLower(Validation::trim(type));
    std::unique_ptr<PatientRecord> patient;

    // Create the right subclass. The pointer type is the BASE class,
    // but the object inside is a GeneralPatient / EmergencyPatient / ...
    if (lower == "general") {
        patient.reset(new GeneralPatient(id, name, age, contact));
    } else if (lower == "emergency") {
        patient.reset(new EmergencyPatient(id, name, age, contact));
    } else if (lower == "follow-up" || lower == "followup") {
        patient.reset(new FollowUpPatient(id, name, age, contact));
    } else {
        throw ValidationException("unknown patient type '" + type + "'.");
    }

    // setExtraFields is virtual, so each subclass reads its own fields.
    patient->setExtraFields(extraFields);
    return patient;
}

} // namespace scapms::model
