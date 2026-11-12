// =====================================================================
// FollowUpPatient.h
// ---------------------------------------------------------------------
// REQUIREMENT 1b (Subclass #3): a patient coming back after an earlier
// visit (e.g. to check results or remove stitches).
//
// Extra data:  previous visit date, referring doctor, follow-up reason.
// Behaviour:   "Scheduled" priority (1). Must book during clinic hours
//              AND on a date AFTER the previous visit.
// =====================================================================
#ifndef SCAPMS_MODEL_FOLLOWUPPATIENT_H
#define SCAPMS_MODEL_FOLLOWUPPATIENT_H

#include "PatientRecord.h"
#include "Appointable.h"

namespace scapms::model {

class FollowUpPatient : public PatientRecord, public Appointable {
public:
    FollowUpPatient(const std::string& id, const std::string& name, int age,
                    const ContactInfo& contact,
                    const std::string& previousVisitDate = "",
                    const std::string& referringDoctor = "",
                    const std::string& followUpReason = "");

    // ----- PatientRecord -----
    std::string getCategory() const override { return "Follow-Up"; }
    int getPriority() const override { return 1; }
    std::string getPriorityLabel() const override { return "Scheduled"; }
    std::string describe() const override;
    std::vector<std::string> getExtraFields() const override;
    void setExtraFields(const std::vector<std::string>& values) override;
    std::unique_ptr<PatientRecord> clone() const override;

    // ----- Appointable -----
    bool canSchedule(const std::string& dateTime, std::string& reason) const override;
    void scheduleAppointment(Appointment& appointment) override;
    void cancelAppointment(Appointment& appointment) override;

    // ----- Own getters -----
    const std::string& getPreviousVisitDate() const { return previousVisitDate; }
    const std::string& getReferringDoctor() const { return referringDoctor; }
    const std::string& getFollowUpReason() const { return followUpReason; }

private:
    void setPreviousVisitDate(const std::string& date); // validates the date

    std::string previousVisitDate; // "YYYY-MM-DD" (may be empty)
    std::string referringDoctor;
    std::string followUpReason;
};

} // namespace scapms::model

#endif
