// =====================================================================
// ClinicManager.h
// ---------------------------------------------------------------------
// The "brain" of the application (Controller layer).
//
// The GUI ONLY talks to this class. It never touches the database,
// the queue or the undo stack directly. This is the "separation of
// responsibilities" the brief asks for (Requirement 2):
//
//     GUI (gui/)  --->  ClinicManager (controller/)  --->  ClinicDatabase (model/)
//     buttons,          rules, undo, queue,                 patients and
//     tables            sorting, searching                  appointments
//
// Because ClinicManager uses NO Qt code, all the logic could be reused
// with a different interface (e.g. a console menu) unchanged.
//
// Data structures used here:
//   std::vector  - patients (inside ClinicDatabase)          Req 3a
//   std::queue   - patients waiting for consultation (FIFO)  Req 3b
//   std::stack   - undo history (inside UndoManager)         Req 3c
//   array[10]    - recently viewed patients (RecentCache)    Req 3d
// =====================================================================
#ifndef SCAPMS_CONTROLLER_CLINICMANAGER_H
#define SCAPMS_CONTROLLER_CLINICMANAGER_H

#include "AppointmentController.h"
#include "RecentCache.h"
#include "SearchEngine.h"
#include "SortEngine.h"
#include "UndoManager.h"
#include "../model/ClinicDatabase.h"
#include "../utils/IDGenerator.h"

#include <ctime>
#include <memory>
#include <queue>
#include <string>
#include <vector>

namespace scapms::controller {

// Which field the user is searching on, and which algorithm they picked.
enum class SearchField { Id, Name, Category };
enum class SearchMethod { Automatic, Linear, Binary, Recursive };

// One person in the waiting room.
struct QueueEntry {
    std::string patientId;
    std::time_t joinedAt = 0;  // when they joined (seconds since 1970)
    std::string joinedAtText;  // the same time as "YYYY-MM-DD HH:MM"
};

class ClinicManager {
public:
    ClinicManager();

    // ------------------------- Patients -------------------------
    std::string suggestPatientId() const; // next free automatic ID, e.g. "P0012"
    void addPatient(std::unique_ptr<model::PatientRecord> patient);
    void updatePatient(const std::string& id, std::unique_ptr<model::PatientRecord> newVersion); // undoable
    void deletePatient(const std::string& id);                                                 // undoable
    model::PatientRecord* getPatient(const std::string& id) const; // nullptr if not found
    model::PatientRecord* viewPatient(const std::string& id);      // same, but uses + updates the cache

    // Patients in the order the GUI should show them (sorted order after
    // a sort, otherwise the order they were added).
    std::vector<model::PatientRecord*> getPatientList() const;
    const model::ClinicDatabase& getDatabase() const { return database; }

    // ----------------------- Appointments -----------------------
    std::string scheduleAppointment(const std::string& patientId, const std::string& dateTime,
                                    const std::string& doctor, const std::string& reason);
    void cancelAppointment(const std::string& appointmentId);   // undoable
    void completeAppointment(const std::string& appointmentId);
    void removeAppointment(const std::string& appointmentId);   // undoable
    const std::vector<model::Appointment>& getAppointments() const { return database.getAppointments(); }

    // Asks the patient's own booking rule (Appointable::canSchedule)
    // without booking anything. Used by the GUI to give live feedback.
    bool checkBooking(const std::string& patientId, const std::string& dateTime, std::string& reason) const;

    // Pending appointments starting within the next `minutesAhead` minutes.
    std::vector<model::Appointment> getUpcomingAppointments(int minutesAhead) const;

    // ----------------------- Waiting queue ----------------------
    void addToQueue(const std::string& patientId);
    model::PatientRecord* callNextPatient(); // removes the FRONT of the queue
    std::vector<QueueEntry> getQueue() const;
    int getQueueSize() const { return (int)waitingQueue.size(); }
    bool isInQueue(const std::string& patientId) const;
    static int minutesWaiting(const QueueEntry& entry);

    // ---------------------- Sorting & searching -----------------
    // Returns the number of comparisons the algorithm made.
    long long sortPatients(SortAlgorithm algorithm, SortKey key);
    bool isSorted() const { return sorted; }
    std::string getSortDescription() const; // e.g. "sorted by Patient Name (Merge Sort)"
    SearchResult search(SearchField field, SearchMethod method, const std::string& text) const;

    // --------------------------- Undo ---------------------------
    std::string undoLastOperation(); // returns what was undone
    bool canUndo() const { return undoManager.canUndo(); }
    std::vector<std::string> getUndoHistory() const { return undoManager.getHistory(); }

    // --------------------------- Cache --------------------------
    std::vector<model::PatientRecord*> getRecentPatients() const { return recentCache.getAll(); }
    const RecentCache& getCache() const { return recentCache; }

    // --------------------------- Files --------------------------
    void newClinic();                                 // File -> New
    void saveToFolder(const std::string& folder);     // writes patients.csv + appointments.csv
    void loadFromFolder(const std::string& folder);   // reads them back
    void exportPatients(const std::string& filePath) const;
    // Adds patients from a CSV file, skipping IDs that already exist.
    // Returns how many were added; `skippedIds` lists the others.
    int importPatients(const std::string& filePath, std::vector<std::string>& skippedIds);
    bool hasUnsavedChanges() const { return unsavedChanges; }

private:
    void markChanged();                              // remember to save; old sort order is no longer valid
    void removeFromQueue(const std::string& patientId);
    void undoDeletePatient(const AdminOperation& operation);
    void undoModifyPatient(const AdminOperation& operation);

    // NOTE: the order matters - `appointments` uses `database` and
    // `appointmentIds`, so those must be created first (C++ creates
    // members in the order they are declared).
    model::ClinicDatabase database;
    utils::IDGenerator patientIds{'P'};
    utils::IDGenerator appointmentIds{'A'};
    AppointmentController appointments;

    std::queue<QueueEntry> waitingQueue; // <-- std::queue (first in, first out)
    UndoManager undoManager;             // <-- holds the std::stack
    RecentCache recentCache;             // <-- holds the fixed-size array

    std::vector<model::PatientRecord*> sortedList; // valid only while `sorted` is true
    bool sorted = false;
    SortKey sortKey = SortKey::Name;
    SortAlgorithm lastAlgorithm = SortAlgorithm::Merge;

    bool unsavedChanges = false;
};

} // namespace scapms::controller

#endif
