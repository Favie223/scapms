// =====================================================================
// PatientRecord.h
// ---------------------------------------------------------------------
// REQUIREMENT 1a (Abstract Class).
//
// PatientRecord holds the information EVERY patient has:
//     Patient ID, Name, Age, Contact information.
//
// It is ABSTRACT because it has "pure virtual" functions (the ones that
// end in "= 0"). That means:
//   - you can NOT write  `PatientRecord p;`  (the compiler refuses),
//   - every subclass (GeneralPatient, EmergencyPatient, FollowUpPatient)
//     MUST provide its own version of those functions.
//
// ENCAPSULATION (Requirement 1e): the data members are private. Other
// code must use the getters/setters, and the setters VALIDATE the new
// value first (e.g. setAge(150) throws InvalidAgeException). This means
// a PatientRecord can never hold bad data.
//
// POLYMORPHISM (Requirement 1d): because the functions are "virtual",
// calling  basePointer->getCategory()  runs the SUBCLASS's version at
// runtime. The rest of the program can treat all patients the same way
// through a  PatientRecord*  pointer.
// =====================================================================
#ifndef SCAPMS_MODEL_PATIENTRECORD_H
#define SCAPMS_MODEL_PATIENTRECORD_H

#include <memory>
#include <string>
#include <vector>

namespace scapms::model {

// A small group of contact details. A struct is used because it is just
// plain data with no rules of its own.
struct ContactInfo {
    std::string phone;
    std::string email;
    std::string address;
};

class PatientRecord {
public:
    // Virtual destructor: REQUIRED in a base class. When we delete a
    // patient through a PatientRecord pointer, this makes sure the
    // subclass's destructor also runs.
    virtual ~PatientRecord() = default;

    // ---------- Pure virtual functions (make this class abstract) ----------

    // "General", "Emergency" or "Follow-Up".
    virtual std::string getCategory() const = 0;

    // Lower number = more urgent. Emergency 0, Follow-Up 1, General 2.
    virtual int getPriority() const = 0;

    // Text label for the priority, e.g. "High Priority" (used in tables).
    virtual std::string getPriorityLabel() const = 0;

    // A one-line summary that is different for each patient type.
    virtual std::string describe() const = 0;

    // Each type has 3 extra fields of its own (e.g. severity for
    // emergencies). These two functions let the file handler and the GUI
    // read/write them without knowing the exact type.
    virtual std::vector<std::string> getExtraFields() const = 0;
    virtual void setExtraFields(const std::vector<std::string>& values) = 0;

    // Makes an independent copy of this patient (the correct subclass).
    // Used by the Undo feature to keep a backup before deleting/editing.
    virtual std::unique_ptr<PatientRecord> clone() const = 0;

    // ---------- Getters (read-only access) ----------
    const std::string& getId() const { return id; }
    const std::string& getName() const { return name; }
    int getAge() const { return age; }
    const ContactInfo& getContact() const { return contact; }
    const std::string& getRegisteredOn() const { return registeredOn; }

    // ---------- Setters (each one validates first) ----------
    void setName(const std::string& newName);
    void setAge(int newAge);
    void setContact(const ContactInfo& newContact);
    void setRegisteredOn(const std::string& date);

    // ---------- Link to appointments (a patient can have MANY) ----------
    // We store appointment IDs only. The Appointment objects themselves
    // live in ClinicDatabase (this is an "association").
    const std::vector<std::string>& getAppointmentIds() const { return appointmentIds; }
    void addAppointmentId(const std::string& appointmentId);
    void removeAppointmentId(const std::string& appointmentId);
    bool hasAppointment(const std::string& appointmentId) const;
    void setAppointmentIds(const std::vector<std::string>& ids) { appointmentIds = ids; }

protected:
    // "protected" constructor: only subclasses can call it. Together with
    // the pure virtual functions this makes sure nobody creates a plain
    // PatientRecord.
    PatientRecord(const std::string& id, const std::string& name, int age,
                  const ContactInfo& contact);

private:
    std::string id;
    std::string name;
    int age = 0;
    ContactInfo contact;
    std::string registeredOn;               // date the patient was registered
    std::vector<std::string> appointmentIds; // IDs of this patient's appointments
};

} // namespace scapms::model

#endif
