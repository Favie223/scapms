// =====================================================================
// core_tests.cpp
// ---------------------------------------------------------------------
// A small console program that checks the model and controller code
// WITHOUT the GUI. Run it after building:   ./scapms_tests
//
// Each CHECK prints PASS or FAIL. At the end it prints a total and
// returns 1 if anything failed (so it can be used in scripts).
// =====================================================================
#include "../controller/ClinicManager.h"
#include "../controller/ReportGenerator.h"
#include "../controller/SearchEngine.h"
#include "../controller/SortEngine.h"
#include "../model/EmergencyPatient.h"
#include "../model/FollowUpPatient.h"
#include "../model/GeneralPatient.h"
#include "../model/PatientFactory.h"
#include "../utils/Exceptions.h"
#include "../utils/FileHandler.h"
#include "../utils/Validation.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>

using namespace scapms;
using namespace scapms::model;
using namespace scapms::controller;
using namespace scapms::utils;

static int passed = 0;
static int failed = 0;

static void CHECK(bool condition, const std::string& name) {
    if (condition) {
        passed++;
        std::cout << "  PASS  " << name << "\n";
    } else {
        failed++;
        std::cout << "  FAIL  " << name << "\n";
    }
}

// Returns true if running `action` throws an exception of type E.
template <typename E, typename F>
static bool throwsType(F action) {
    try {
        action();
    } catch (const E&) {
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

static std::unique_ptr<PatientRecord> makeGeneral(const std::string& id, const std::string& name, int age) {
    return PatientFactory::create("General", id, name, age, ContactInfo{}, {});
}

// Checks that `list` is in order for the given key.
static bool isInOrder(const std::vector<PatientRecord*>& list, SortKey key) {
    for (size_t i = 1; i < list.size(); i++) {
        if (key == SortKey::Age && list[i - 1]->getAge() > list[i]->getAge()) return false;
        if (key == SortKey::Id && list[i - 1]->getId() > list[i]->getId()) return false;
        if (key == SortKey::Name
            && Validation::toLower(list[i - 1]->getName()) > Validation::toLower(list[i]->getName())) return false;
    }
    return true;
}

int main() {
    std::cout << "== Model & validation ==\n";
    CHECK(throwsType<InvalidAgeException>([] { makeGeneral("P1", "Ada", 121); }), "age 121 rejected");
    CHECK(throwsType<InvalidAgeException>([] { makeGeneral("P1", "Ada", -1); }), "age -1 rejected");
    CHECK(throwsType<ValidationException>([] { makeGeneral("P1", "", 30); }), "empty name rejected");
    CHECK(throwsType<ValidationException>([] { makeGeneral("P1", "Ad4", 30); }), "digits in name rejected");
    CHECK(throwsType<ValidationException>([] { Validation::parseAge("abc"); }), "non-numeric age rejected");
    CHECK(Validation::isValidDate("2028-02-29") && !Validation::isValidDate("2027-02-29"), "leap years handled");
    CHECK(makeGeneral("p0007", "Ada", 3)->getId() == "P0007", "IDs stored uppercase");

    // Polymorphism through base-class pointers
    std::vector<std::unique_ptr<PatientRecord>> mixed;
    mixed.push_back(PatientFactory::create("General", "P1", "Ada", 30, {}, {"INS1", "Cough", ""}));
    mixed.push_back(PatientFactory::create("Emergency", "P2", "Bola", 40, {}, {"4", "Fracture", ""}));
    mixed.push_back(PatientFactory::create("Follow-Up", "P3", "Chidi", 50, {}, {"2026-01-01", "Dr. Ade", "Stitches"}));
    CHECK(mixed[0]->getCategory() == "General" && mixed[1]->getCategory() == "Emergency"
          && mixed[2]->getCategory() == "Follow-Up", "virtual getCategory() via base pointer");
    CHECK(mixed[1]->getPriority() < mixed[2]->getPriority() && mixed[2]->getPriority() < mixed[0]->getPriority(),
          "priority: Emergency < Follow-Up < General");
    std::unique_ptr<PatientRecord> copy = mixed[1]->clone();
    CHECK(copy->getCategory() == "Emergency" && copy->getExtraFields()[0] == "4", "clone() keeps subclass + data");

    std::cout << "== Sorting (4 algorithms x 3 keys, 300 random patients) ==\n";
    ClinicDatabase sortDb;
    std::srand(42);
    const char* names[] = {"Zara", "ada", "Musa", "Bola", "Ngozi", "Emeka", "Tunde", "Kemi", "Ifeoma", "Yusuf"};
    for (int i = 0; i < 300; i++) {
        std::string id = "P" + std::to_string(1000 + std::rand() % 9000) + "_" + std::to_string(i);
        sortDb.addPatient(makeGeneral(id.substr(0, 12), names[std::rand() % 10], std::rand() % 121));
    }
    SortEngine engine(sortDb);
    SortAlgorithm algorithms[] = {SortAlgorithm::Selection, SortAlgorithm::Insertion,
                                  SortAlgorithm::Merge, SortAlgorithm::Quick};
    SortKey keys[] = {SortKey::Name, SortKey::Id, SortKey::Age};
    for (SortAlgorithm algorithm : algorithms) {
        for (SortKey key : keys) {
            std::vector<PatientRecord*> list = sortDb.getPatientList();
            engine.sort(list, algorithm, key);
            CHECK(isInOrder(list, key) && list.size() == 300,
                  SortEngine::algorithmName(algorithm) + " by " + SortEngine::keyName(key)
                      + " (" + std::to_string(engine.getComparisonCount()) + " comparisons)");
        }
    }
    // Insertion sort on already-sorted data should be ~n comparisons.
    std::vector<PatientRecord*> presorted = sortDb.getPatientList();
    engine.sort(presorted, SortAlgorithm::Merge, SortKey::Age);
    engine.sort(presorted, SortAlgorithm::Insertion, SortKey::Age);
    CHECK(engine.getComparisonCount() < 300, "insertion sort is O(n) on sorted data ("
                                                 + std::to_string(engine.getComparisonCount()) + " comparisons)");

    std::cout << "== Searching ==\n";
    ClinicManager manager;
    manager.addPatient(makeGeneral("P0003", "Chidi Okafor", 25));
    manager.addPatient(makeGeneral("P0001", "Ada Obi", 34));
    manager.addPatient(makeGeneral("P0002", "Bola Ade", 19));
    manager.addPatient(PatientFactory::create("Emergency", "P0004", "Ada Musa", 60, {}, {"5", "Chest pain", ""}));

    SearchResult r = manager.search(SearchField::Id, SearchMethod::Automatic, "p0002");
    CHECK(r.matches.size() == 1 && r.algorithmUsed.find("Linear") != std::string::npos, "unsorted ID -> linear search");
    CHECK(throwsType<InvalidSearchInputException>([&] { manager.search(SearchField::Id, SearchMethod::Binary, "P0002"); }),
          "binary search refused on unsorted data");
    manager.sortPatients(SortAlgorithm::Merge, SortKey::Id);
    r = manager.search(SearchField::Id, SearchMethod::Automatic, "P0002");
    CHECK(r.matches.size() == 1 && r.algorithmUsed.find("Binary") != std::string::npos, "sorted ID -> binary search");
    r = manager.search(SearchField::Id, SearchMethod::Automatic, "P0099");
    CHECK(r.matches.empty(), "binary search: missing ID not found");
    r = manager.search(SearchField::Name, SearchMethod::Recursive, "ada");
    CHECK(r.matches.size() == 2, "recursive partial name search ('ada' -> Ada Obi, Ada Musa)");
    manager.sortPatients(SortAlgorithm::Quick, SortKey::Name);
    r = manager.search(SearchField::Name, SearchMethod::Binary, "ada obi");
    CHECK(r.matches.size() == 1 && r.matches[0]->getId() == "P0001", "binary search by exact name");
    r = manager.search(SearchField::Category, SearchMethod::Automatic, "Emergency");
    CHECK(r.matches.size() == 1, "search by category");
    CHECK(throwsType<InvalidSearchInputException>([&] { manager.search(SearchField::Name, SearchMethod::Automatic, "  "); }),
          "empty search rejected");

    std::cout << "== Duplicate ID, queue, cache ==\n";
    CHECK(throwsType<DuplicateIdException>([&] { manager.addPatient(makeGeneral("P0001", "Copy", 1)); }), "duplicate ID rejected");
    CHECK(manager.suggestPatientId() == "P0005", "suggested next ID is P0005");
    manager.addToQueue("P0002");
    manager.addToQueue("P0001");
    CHECK(throwsType<ClinicException>([&] { manager.addToQueue("P0002"); }), "cannot queue twice");
    CHECK(manager.callNextPatient()->getId() == "P0002", "queue is FIFO (first in, first served)");
    CHECK(manager.getQueueSize() == 1, "queue size after call");
    for (int i = 0; i < 3; i++) manager.viewPatient("P0003");
    manager.viewPatient("P0001");
    CHECK(manager.getRecentPatients()[0]->getId() == "P0001" && manager.getRecentPatients().size() == 2,
          "recent cache: most recent first, no duplicates");

    std::cout << "== Appointments & undo ==\n";
    std::string tomorrow = Validation::minutesFromNow(24 * 60).substr(0, 10);
    std::string a1 = manager.scheduleAppointment("P0001", tomorrow + " 10:00", "Dr. Bello", "Checkup");
    CHECK(!a1.empty(), "general appointment in clinic hours booked");
    CHECK(throwsType<InvalidAppointmentException>([&] {
              manager.scheduleAppointment("P0001", tomorrow + " 22:00", "Dr. Bello", "Late");
          }), "general appointment at 22:00 rejected");
    std::string a2 = manager.scheduleAppointment("P0004", tomorrow + " 23:30", "Dr. Eze", "Chest pain");
    CHECK(!a2.empty(), "emergency appointment at 23:30 allowed");
    CHECK(throwsType<InvalidAppointmentException>([&] { manager.cancelAppointment(a2); }),
          "severity-5 emergency cannot be cancelled");
    CHECK(throwsType<InvalidAppointmentException>([&] {
              manager.scheduleAppointment("P0001", "2020-01-01 10:00", "Dr. Bello", "Past");
          }), "appointment in the past rejected");

    manager.cancelAppointment(a1);
    CHECK(manager.getDatabase().getAppointments()[0].getStatusText() == "Cancelled", "appointment cancelled");
    manager.undoLastOperation();
    CHECK(manager.getDatabase().getAppointments()[0].getStatusText() == "Pending", "undo cancel -> Pending again");

    manager.deletePatient("P0001");
    CHECK(manager.getPatient("P0001") == nullptr && manager.getDatabase().getAppointments().size() == 1,
          "delete patient also removes their appointments");
    CHECK(manager.getRecentPatients().size() == 1, "deleted patient removed from cache (no dangling pointer)");
    std::string undone = manager.undoLastOperation();
    CHECK(manager.getPatient("P0001") != nullptr && manager.getDatabase().getAppointments().size() == 2,
          "undo delete restores patient + appointments (" + undone + ")");

    std::unique_ptr<PatientRecord> edited = makeGeneral("P0003", "Chidi Renamed", 26);
    manager.updatePatient("P0003", std::move(edited));
    CHECK(manager.getPatient("P0003")->getName() == "Chidi Renamed", "edit patient");
    manager.undoLastOperation();
    CHECK(manager.getPatient("P0003")->getName() == "Chidi Okafor" && manager.getPatient("P0003")->getAge() == 25,
          "undo edit restores old values");
    CHECK(manager.getRecentPatients().back()->getName() == "Chidi Okafor", "cache follows edit/undo");

    manager.removeAppointment(a2);
    manager.undoLastOperation();
    CHECK(manager.getDatabase().getAppointments().size() == 2, "undo remove appointment");
    CHECK(throwsType<ClinicException>([&] { manager.undoLastOperation(); }), "undo on empty stack reports error");

    std::cout << "== Reports ==\n";
    manager.completeAppointment(a1);
    CategoryDistribution dist = ReportGenerator::categoryDistribution(manager.getDatabase().getPatientList());
    CHECK(dist.general == 3 && dist.emergency == 1 && dist.followUp == 0, "category distribution");
    std::vector<PatientCount> top = ReportGenerator::mostFrequentlyAttended(manager.getDatabase(), 10);
    CHECK(top.size() == 1 && top[0].patient->getId() == "P0001", "most frequently attended");
    CHECK(ReportGenerator::patientsWithPendingAppointments(manager.getDatabase()).size() == 1, "pending appointments report");
    DailySummary day = ReportGenerator::dailySummary(manager.getDatabase(), tomorrow);
    CHECK(day.total == 2 && day.completed == 1 && day.pending == 1, "daily summary");

    std::cout << "== Save / load ==\n";
    std::string folder = (std::filesystem::temp_directory_path() / "scapms_test_data").string();
    manager.saveToFolder(folder);
    CHECK(FileHandler::splitCsvLine("a,\"b, c\",\"say \"\"hi\"\"\"")[1] == "b, c", "CSV quoted comma");
    ClinicManager reloaded;
    reloaded.loadFromFolder(folder);
    CHECK(reloaded.getDatabase().patientCount() == 4 && reloaded.getAppointments().size() == 2, "reload counts match");
    CHECK(reloaded.getPatient("P0004")->getCategory() == "Emergency"
          && reloaded.getPatient("P0004")->getExtraFields()[1] == "Chest pain", "reload keeps subclass + extra fields");
    CHECK(reloaded.getPatient("P0001")->getAppointmentIds().size() == 1, "reload re-links appointments to patients");
    CHECK(reloaded.suggestPatientId() == "P0005", "ID generator continues after load");
    CHECK(throwsType<FileLoadException>([&] { reloaded.loadFromFolder("/no/such/folder"); }), "missing file -> FileLoadException");
    CHECK(reloaded.getDatabase().patientCount() == 4, "failed load keeps existing data");
    std::filesystem::remove_all(folder);

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
