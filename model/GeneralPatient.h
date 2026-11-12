// =====================================================================
// GeneralPatient.h
// ---------------------------------------------------------------------
// REQUIREMENT 1b (Subclass #1): a normal walk-in / routine patient.
//
// Inherits:
//   - PatientRecord  -> gets ID, name, age, contact (IS-A patient)
//   - Appointable    -> promises canSchedule/schedule/cancel
//
// Extra data:  insurance number, main complaint.
// Behaviour:   Normal priority (2). Can only book during clinic hours
//              (08:00 - 17:59) and not in the past.
// =====================================================================
#ifndef SCAPMS_MODEL_GENERALPATIENT_H
#define SCAPMS_MODEL_GENERALPATIENT_H

#include "PatientRecord.h"
#include "Appointable.h"

namespace scapms::model {

class GeneralPatient : public PatientRecord, public Appointable {
public:
    GeneralPatient(const std::string& id, const std::string& name, int age,
                   const ContactInfo& contact,
                   const std::string& insuranceNumber = "",
                   const std::string& complaint = "");

    // ----- PatientRecord's pure virtual functions -----
    // "override" asks the compiler to check we really are replacing a
    // virtual function from the base class (catches typos).
    std::string getCategory() const override { return "General"; }
    int getPriority() const override { return 2; }
    std::string getPriorityLabel() const override { return "Normal"; }
    std::string describe() const override;
    std::vector<std::string> getExtraFields() const override;
    void setExtraFields(const std::vector<std::string>& values) override;
    std::unique_ptr<PatientRecord> clone() const override;

    // ----- Appointable's pure virtual functions -----
    bool canSchedule(const std::string& dateTime, std::string& reason) const override;
    void scheduleAppointment(Appointment& appointment) override;
    void cancelAppointment(Appointment& appointment) override;

    // ----- Own getters -----
    const std::string& getInsuranceNumber() const { return insuranceNumber; }
    const std::string& getComplaint() const { return complaint; }

private:
    std::string insuranceNumber;
    std::string complaint;   // e.g. "Headache"
};

} // namespace scapms::model

#endif
