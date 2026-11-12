// =====================================================================
// EmergencyPatient.h
// ---------------------------------------------------------------------
// REQUIREMENT 1b (Subclass #2): a patient who needs urgent care.
//
// Extra data:  severity (1 = mild ... 5 = critical), condition,
//              arrival time.
// Behaviour:   HIGH priority (0 = most urgent). Can be booked at ANY
//              hour, including right now (emergencies don't wait for
//              clinic hours).
// =====================================================================
#ifndef SCAPMS_MODEL_EMERGENCYPATIENT_H
#define SCAPMS_MODEL_EMERGENCYPATIENT_H

#include "PatientRecord.h"
#include "Appointable.h"

namespace scapms::model {

class EmergencyPatient : public PatientRecord, public Appointable {
public:
    EmergencyPatient(const std::string& id, const std::string& name, int age,
                     const ContactInfo& contact,
                     int severity = 3,
                     const std::string& condition = "",
                     const std::string& arrivalTime = "");

    // ----- PatientRecord -----
    std::string getCategory() const override { return "Emergency"; }
    int getPriority() const override { return 0; }
    std::string getPriorityLabel() const override { return "High Priority"; }
    std::string describe() const override;
    std::vector<std::string> getExtraFields() const override;
    void setExtraFields(const std::vector<std::string>& values) override;
    std::unique_ptr<PatientRecord> clone() const override;

    // ----- Appointable -----
    bool canSchedule(const std::string& dateTime, std::string& reason) const override;
    void scheduleAppointment(Appointment& appointment) override;
    void cancelAppointment(Appointment& appointment) override;

    // ----- Own getters / setters -----
    int getSeverity() const { return severity; }
    const std::string& getCondition() const { return condition; }
    const std::string& getArrivalTime() const { return arrivalTime; }
    void setSeverity(int newSeverity); // must be 1..5

private:
    int severity = 3;
    std::string condition;   // e.g. "Fracture"
    std::string arrivalTime; // "YYYY-MM-DD HH:MM"
};

} // namespace scapms::model

#endif
