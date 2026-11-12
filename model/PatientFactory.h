// =====================================================================
// PatientFactory.h
// ---------------------------------------------------------------------
// One place that knows how to turn a type name ("General", "Emergency",
// "Follow-Up") into the right subclass object. This is the "Factory"
// design pattern.
//
// Why? Both the GUI (Add Patient dialog) and the FileHandler (loading a
// CSV) need to create patients from text. Without this, both would need
// the same if/else chain. The return type is a pointer to the BASE
// class, so callers don't need to know which subclass they got.
// =====================================================================
#ifndef SCAPMS_MODEL_PATIENTFACTORY_H
#define SCAPMS_MODEL_PATIENTFACTORY_H

#include "PatientRecord.h"

#include <memory>
#include <string>
#include <vector>

namespace scapms::model {

namespace PatientFactory {
    // The type names in the order the GUI shows them.
    std::vector<std::string> typeNames();

    // Builds a patient of the given type. `extraFields` are the 3
    // type-specific values (see each subclass's getExtraFields()).
    // Throws ValidationException for an unknown type or bad data.
    std::unique_ptr<PatientRecord> create(const std::string& type,
                                          const std::string& id,
                                          const std::string& name,
                                          int age,
                                          const ContactInfo& contact,
                                          const std::vector<std::string>& extraFields);
}

} // namespace scapms::model

#endif
