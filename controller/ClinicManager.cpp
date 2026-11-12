// =====================================================================
// ClinicManager.cpp
// =====================================================================
#include "ClinicManager.h"
#include "../model/Appointable.h"
#include "../utils/Exceptions.h"
#include "../utils/FileHandler.h"
#include "../utils/Validation.h"

#include <cctype>     // isdigit
#include <filesystem> // create_directories (make the data folder if missing)

namespace scapms::controller {

using namespace scapms::model;
using namespace scapms::utils;

ClinicManager::ClinicManager() : appointments(database, appointmentIds) {}

// Called after EVERY change to the data.
//  - unsavedChanges: so we can warn the user before they quit.
//  - sorted = false: the sorted list may now be out of date (a patient
//    was added/removed/renamed), so we fall back to normal order. This
//    also stops binary search from running on a list that is no longer
//    sorted (which would give wrong answers).
void ClinicManager::markChanged() {
    unsavedChanges = true;
    sorted = false;
    sortedList.clear();
}

// =====================================================================
// Patients
// =====================================================================

std::string ClinicManager::suggestPatientId() const {
    // peek() might suggest an ID someone typed in by hand already, so
    // keep looking until we find a free one.
    IDGenerator copy = patientIds;
    std::string id = copy.peek();
    while (database.hasPatient(id)) {
        copy.next();
        id = copy.peek();
    }
    return id;
}

void ClinicManager::addPatient(std::unique_ptr<PatientRecord> patient) {
    std::string id = patient->getId();
    database.addPatient(std::move(patient)); // throws DuplicateIdException
    patientIds.observe(id);                  // keep auto IDs ahead of this one
    markChanged();
}

void ClinicManager::updatePatient(const std::string& id, std::unique_ptr<PatientRecord> newVersion) {
    PatientRecord* current = database.findPatient(id);
    if (current == nullptr) {
        throw NotFoundException("patient " + id);
    }
    if (newVersion->getId() != current->getId()) {
        throw ValidationException("the patient ID cannot be changed when editing.");
    }

    // Keep things the edit form doesn't change.
    newVersion->setAppointmentIds(current->getAppointmentIds());
    newVersion->setRegisteredOn(current->getRegisteredOn());

    // Save a copy of the OLD version on the undo stack.
    AdminOperation operation;
    operation.type = OperationType::ModifyPatient;
    operation.description = "Edited patient " + id + " (" + current->getName() + ")";
    operation.patientBackup = std::shared_ptr<PatientRecord>(current->clone());

    PatientRecord* newPointer = newVersion.get();
    database.replacePatient(id, std::move(newVersion)); // old object is deleted here
    recentCache.replace(current, newPointer);           // so fix the cache pointer
    undoManager.push(operation);
    markChanged();
}

void ClinicManager::deletePatient(const std::string& id) {
    PatientRecord* patient = database.findPatient(id);
    if (patient == nullptr) {
        throw NotFoundException("patient " + id);
    }

    // 1. Back up everything needed to bring the patient back.
    AdminOperation operation;
    operation.type = OperationType::DeletePatient;
    operation.description = "Deleted patient " + id + " (" + patient->getName() + ")";
    operation.patientBackup = std::shared_ptr<PatientRecord>(patient->clone());
    operation.patientIndex = database.indexOfPatient(id);
    operation.appointmentBackups = database.getAppointmentsForPatient(id);

    // 2. Remove everything that points at this patient FIRST, so nothing
    //    is left pointing at a deleted object.
    recentCache.remove(patient);
    removeFromQueue(id);
    for (const Appointment& appointment : operation.appointmentBackups) {
        database.removeAppointment(appointment.getId());
    }

    // 3. Remove the patient itself. The returned unique_ptr is not
    //    kept, so the object is deleted at the end of this line.
    database.removePatient(id);

    undoManager.push(operation);
    markChanged();
}

PatientRecord* ClinicManager::getPatient(const std::string& id) const {
    return database.findPatient(id);
}

// Checks the small cache first (fast, at most 10 slots). Only on a cache
// MISS do we look in the full database. Either way the patient becomes
// the "most recent" one.
PatientRecord* ClinicManager::viewPatient(const std::string& id) {
    PatientRecord* patient = recentCache.find(id);
    if (patient == nullptr) {
        patient = database.findPatient(id);
    }
    if (patient != nullptr) {
        recentCache.recordAccess(patient);
    }
    return patient;
}

std::vector<PatientRecord*> ClinicManager::getPatientList() const {
    if (sorted) return sortedList;
    return database.getPatientList();
}

// =====================================================================
// Appointments
// =====================================================================

std::string ClinicManager::scheduleAppointment(const std::string& patientId, const std::string& dateTime,
                                               const std::string& doctor, const std::string& reason) {
    std::string id = appointments.schedule(patientId, dateTime, doctor, reason);
    markChanged();
    return id;
}

void ClinicManager::cancelAppointment(const std::string& appointmentId) {
    appointments.cancel(appointmentId); // throws if not allowed

    AdminOperation operation;
    operation.type = OperationType::CancelAppointment;
    operation.description = "Cancelled appointment " + appointmentId;
    operation.appointmentId = appointmentId;
    undoManager.push(operation);
    markChanged();
}

void ClinicManager::completeAppointment(const std::string& appointmentId) {
    appointments.complete(appointmentId);
    markChanged();
}

void ClinicManager::removeAppointment(const std::string& appointmentId) {
    Appointment removed = appointments.remove(appointmentId);

    AdminOperation operation;
    operation.type = OperationType::RemoveAppointment;
    operation.description = "Removed appointment " + appointmentId + " (" + removed.getDateTime() + ")";
    operation.appointmentBackups.push_back(removed);
    undoManager.push(operation);
    markChanged();
}

bool ClinicManager::checkBooking(const std::string& patientId, const std::string& dateTime,
                                 std::string& reason) const {
    PatientRecord* patient = database.findPatient(patientId);
    if (patient == nullptr) {
        reason = "please choose a patient.";
        return false;
    }
    // dynamic_cast: "treat this patient as an Appointable" (the interface).
    const Appointable* bookable = dynamic_cast<const Appointable*>(patient);
    if (bookable == nullptr) {
        reason = "this patient type cannot book appointments.";
        return false;
    }
    return bookable->canSchedule(dateTime, reason); // subclass's own rule
}

std::vector<Appointment> ClinicManager::getUpcomingAppointments(int minutesAhead) const {
    std::string from = Validation::now();
    std::string to = Validation::minutesFromNow(minutesAhead);
    std::vector<Appointment> result;
    for (const Appointment& appointment : database.getAppointments()) {
        // Text comparison works because of the "YYYY-MM-DD HH:MM" format.
        if (appointment.isPending() && appointment.getDateTime() >= from && appointment.getDateTime() <= to) {
            result.push_back(appointment);
        }
    }
    return result;
}

// =====================================================================
// Waiting queue  (std::queue = First In, First Out)
// ---------------------------------------------------------------------
//   addToQueue(Ada), addToQueue(Bola), addToQueue(Chidi)
//   front -> [Ada] [Bola] [Chidi] <- back
//   callNextPatient() returns Ada (she arrived first)
// =====================================================================

void ClinicManager::addToQueue(const std::string& patientId) {
    PatientRecord* patient = database.findPatient(patientId);
    if (patient == nullptr) {
        throw NotFoundException("patient " + patientId);
    }
    if (isInQueue(patient->getId())) {
        throw ClinicException(patient->getName() + " is already in the waiting queue.");
    }
    QueueEntry entry;
    entry.patientId = patient->getId();
    entry.joinedAt = std::time(nullptr);
    entry.joinedAtText = Validation::now();
    waitingQueue.push(entry); // joins at the BACK
}

PatientRecord* ClinicManager::callNextPatient() {
    if (waitingQueue.empty()) {
        throw ClinicException("The waiting queue is empty.");
    }
    QueueEntry next = waitingQueue.front(); // the person at the FRONT
    waitingQueue.pop();                     // leaves the queue
    return database.findPatient(next.patientId);
}

// std::queue can't be looped over, so copy it and pop from the copy.
std::vector<QueueEntry> ClinicManager::getQueue() const {
    std::vector<QueueEntry> list;
    std::queue<QueueEntry> copy = waitingQueue;
    while (!copy.empty()) {
        list.push_back(copy.front());
        copy.pop();
    }
    return list;
}

bool ClinicManager::isInQueue(const std::string& patientId) const {
    for (const QueueEntry& entry : getQueue()) {
        if (entry.patientId == patientId) return true;
    }
    return false;
}

// Rebuilds the queue without the given patient (keeps everyone else's order).
void ClinicManager::removeFromQueue(const std::string& patientId) {
    std::queue<QueueEntry> rebuilt;
    while (!waitingQueue.empty()) {
        if (waitingQueue.front().patientId != patientId) {
            rebuilt.push(waitingQueue.front());
        }
        waitingQueue.pop();
    }
    waitingQueue = rebuilt;
}

int ClinicManager::minutesWaiting(const QueueEntry& entry) {
    return (int)((std::time(nullptr) - entry.joinedAt) / 60);
}

// =====================================================================
// Sorting & searching
// =====================================================================

long long ClinicManager::sortPatients(SortAlgorithm algorithm, SortKey key) {
    std::vector<PatientRecord*> list = database.getPatientList();
    SortEngine engine(database);
    engine.sort(list, algorithm, key);

    // Remember the result and WHAT it is sorted by, so search() knows
    // when binary search is allowed.
    sortedList = list;
    sorted = true;
    sortKey = key;
    lastAlgorithm = algorithm;
    return engine.getComparisonCount();
}

std::string ClinicManager::getSortDescription() const {
    if (!sorted) return "not sorted";
    return "sorted by " + SortEngine::keyName(sortKey) + " (" + SortEngine::algorithmName(lastAlgorithm) + ")";
}

SearchResult ClinicManager::search(SearchField field, SearchMethod method, const std::string& text) const {
    std::string keyword = Validation::trim(text);
    if (keyword.empty()) {
        throw InvalidSearchInputException("please type something to search for.");
    }

    std::vector<PatientRecord*> list = getPatientList();
    SearchResult result;

    // ---------------------------- by ID ----------------------------
    if (field == SearchField::Id) {
        if (method == SearchMethod::Recursive) {
            throw InvalidSearchInputException("recursive search is used for patient NAMES. "
                                              "Choose 'Name' or another algorithm.");
        }
        bool sortedById = sorted && sortKey == SortKey::Id;
        bool useBinary = (method == SearchMethod::Binary)
                         || (method == SearchMethod::Automatic && sortedById);
        if (useBinary && !sortedById) {
            throw InvalidSearchInputException("binary search only works on SORTED data. "
                                              "Sort the list by Patient ID first, or choose Automatic.");
        }

        int index;
        if (useBinary) {
            index = SearchEngine::binarySearchById(list, keyword, result.steps);
            result.algorithmUsed = "Binary search (list is sorted by ID)";
        } else {
            index = SearchEngine::linearSearchById(list, keyword, result.steps);
            result.algorithmUsed = "Linear search (list is not sorted by ID)";
        }
        if (index != -1) result.matches.push_back(list[index]);
        return result;
    }

    // --------------------------- by Name ---------------------------
    if (field == SearchField::Name) {
        for (char c : keyword) {
            if (std::isdigit((unsigned char)c)) {
                throw InvalidSearchInputException("patient names do not contain numbers. "
                                                  "Did you mean to search by ID?");
            }
        }
        bool sortedByName = sorted && sortKey == SortKey::Name;

        if (method == SearchMethod::Binary && !sortedByName) {
            throw InvalidSearchInputException("binary search only works on SORTED data. "
                                              "Sort the list by Patient Name first, or choose Automatic.");
        }

        // Binary search (exact full name) when the list is sorted by name.
        if (method == SearchMethod::Binary || (method == SearchMethod::Automatic && sortedByName)) {
            int index = SearchEngine::binarySearchByName(list, keyword, result.steps);
            if (index != -1) {
                // Several patients may share a name. In a sorted list they
                // sit next to each other, so look left and right too.
                std::string target = Validation::toLower(keyword);
                int start = index;
                while (start > 0 && Validation::toLower(list[start - 1]->getName()) == target) start--;
                int end = index;
                while (end + 1 < (int)list.size() && Validation::toLower(list[end + 1]->getName()) == target) end++;
                for (int i = start; i <= end; i++) result.matches.push_back(list[i]);
                result.algorithmUsed = "Binary search (list is sorted by name, exact match)";
                return result;
            }
            if (method == SearchMethod::Binary) {
                result.algorithmUsed = "Binary search (exact full name; no match)";
                return result;
            }
            // Automatic: no exact match, so fall through to a partial search.
            result.algorithmUsed = "Binary search found no exact match, then ";
        }

        if (method == SearchMethod::Linear) {
            result.matches = SearchEngine::linearSearchByName(list, keyword, result.steps);
            result.algorithmUsed += "Linear search (partial name match)";
        } else {
            SearchEngine::recursiveSearchByName(list, keyword, 0, result.matches, result.steps);
            result.algorithmUsed += "Recursive search (partial name match)";
        }
        return result;
    }

    // ------------------------- by Category -------------------------
    std::string lower = Validation::toLower(keyword);
    if (lower != "general" && lower != "emergency" && lower != "follow-up" && lower != "followup") {
        throw InvalidSearchInputException("category must be General, Emergency or Follow-Up.");
    }
    result.matches = SearchEngine::searchByCategory(list, keyword, result.steps);
    result.algorithmUsed = "Linear scan by category";
    return result;
}

// =====================================================================
// Undo
// =====================================================================

std::string ClinicManager::undoLastOperation() {
    AdminOperation operation = undoManager.pop(); // throws if the stack is empty

    switch (operation.type) {
        case OperationType::DeletePatient:
            undoDeletePatient(operation);
            break;
        case OperationType::ModifyPatient:
            undoModifyPatient(operation);
            break;
        case OperationType::RemoveAppointment:
            appointments.restore(operation.appointmentBackups[0]);
            break;
        case OperationType::CancelAppointment:
            appointments.reopen(operation.appointmentId);
            break;
    }
    markChanged();
    return operation.description;
}

void ClinicManager::undoDeletePatient(const AdminOperation& operation) {
    // Put a fresh copy of the backup back where it used to be ...
    database.insertPatientAt(operation.patientIndex, operation.patientBackup->clone());
    // ... and give back their appointments.
    for (const Appointment& appointment : operation.appointmentBackups) {
        appointments.restore(appointment);
    }
    // (They are NOT put back in the waiting queue, because their place
    //  in the queue would no longer be fair to people who arrived later.)
}

void ClinicManager::undoModifyPatient(const AdminOperation& operation) {
    const std::string& id = operation.patientBackup->getId();
    PatientRecord* current = database.findPatient(id);
    if (current == nullptr) {
        throw NotFoundException("patient " + id + " (it may have been deleted after the edit)");
    }
    std::unique_ptr<PatientRecord> oldVersion = operation.patientBackup->clone();
    oldVersion->setAppointmentIds(current->getAppointmentIds()); // keep newer bookings
    PatientRecord* oldPointer = oldVersion.get();
    database.replacePatient(id, std::move(oldVersion));
    recentCache.replace(current, oldPointer);
}

// =====================================================================
// Files
// =====================================================================

void ClinicManager::newClinic() {
    database.clear();
    patientIds.reset();
    appointmentIds.reset();
    waitingQueue = std::queue<QueueEntry>(); // empty queue
    undoManager.clear();
    recentCache.clear();
    markChanged();
    unsavedChanges = false; // a brand-new empty clinic has nothing to save
}

void ClinicManager::saveToFolder(const std::string& folder) {
    std::error_code error;
    std::filesystem::create_directories(folder, error); // OK if it already exists
    FileHandler::savePatients(folder + "/patients.csv", database.getPatientList());
    FileHandler::saveAppointments(folder + "/appointments.csv", database.getAppointments());
    unsavedChanges = false;
}

void ClinicManager::loadFromFolder(const std::string& folder) {
    // Load everything into TEMPORARY variables first. If anything fails,
    // an exception is thrown here and the current data is left untouched.
    std::vector<std::unique_ptr<PatientRecord>> loadedPatients =
        FileHandler::loadPatients(folder + "/patients.csv");

    std::vector<Appointment> loadedAppointments;
    std::string appointmentFile = folder + "/appointments.csv";
    if (FileHandler::fileExists(appointmentFile)) {
        loadedAppointments = FileHandler::loadAppointments(appointmentFile);
    }

    ClinicDatabase temp;
    for (auto& patient : loadedPatients) {
        try {
            temp.addPatient(std::move(patient));
        } catch (const DuplicateIdException& e) {
            throw FileLoadException(std::string("patients.csv contains the same ID twice. ") + e.what());
        }
    }
    for (const Appointment& appointment : loadedAppointments) {
        PatientRecord* owner = temp.findPatient(appointment.getPatientId());
        if (owner == nullptr) {
            throw FileLoadException("appointment " + appointment.getId() + " refers to unknown patient "
                                    + appointment.getPatientId() + ".");
        }
        temp.addAppointment(appointment);
        owner->addAppointmentId(appointment.getId()); // link patient -> appointment
    }

    // Everything loaded fine: now replace the old data.
    newClinic();
    database = std::move(temp);

    // Make sure new IDs continue after the highest loaded ones.
    for (PatientRecord* patient : database.getPatientList()) patientIds.observe(patient->getId());
    for (const Appointment& appointment : database.getAppointments()) appointmentIds.observe(appointment.getId());
    unsavedChanges = false;
}

void ClinicManager::exportPatients(const std::string& filePath) const {
    FileHandler::savePatients(filePath, getPatientList());
}

int ClinicManager::importPatients(const std::string& filePath, std::vector<std::string>& skippedIds) {
    std::vector<std::unique_ptr<PatientRecord>> imported = FileHandler::loadPatients(filePath);
    int added = 0;
    for (auto& patient : imported) {
        if (database.hasPatient(patient->getId())) {
            skippedIds.push_back(patient->getId());
            continue;
        }
        patientIds.observe(patient->getId());
        database.addPatient(std::move(patient));
        added++;
    }
    if (added > 0) markChanged();
    return added;
}

} // namespace scapms::controller
