// =====================================================================
// SortEngine.h
// ---------------------------------------------------------------------
// REQUIREMENT 5 (Sorting Algorithms) - ALL written by hand, no std::sort.
//
// Four algorithms are provided so the user can pick one in the GUI:
//
//   Algorithm        Best        Average     Worst       Stable?  Extra memory
//   --------------   ---------   ---------   ---------   -------  ------------
//   Selection Sort   O(n^2)      O(n^2)      O(n^2)      No       O(1)
//   Insertion Sort   O(n)        O(n^2)      O(n^2)      Yes      O(1)
//   Merge Sort       O(n log n)  O(n log n)  O(n log n)  Yes      O(n)
//   Quick Sort       O(n log n)  O(n log n)  O(n^2)      No       O(log n)
//
// ("Stable" = two patients with the same key keep their original order.)
//
// Patients can be sorted by 4 keys: Name, Patient ID, Age, and
// Appointment date (the date of the patient's next pending appointment).
//
// What we sort: a std::vector<PatientRecord*> - a list of POINTERS.
// Swapping two pointers is very cheap (8 bytes) compared with copying
// whole patient objects, and the database still owns the real patients.
//
// We also COUNT comparisons so the GUI can show how much work each
// algorithm did - useful to prove the Big-O claims above.
// =====================================================================
#ifndef SCAPMS_CONTROLLER_SORTENGINE_H
#define SCAPMS_CONTROLLER_SORTENGINE_H

#include "../model/ClinicDatabase.h"
#include "../model/PatientRecord.h"

#include <string>
#include <vector>

namespace scapms::controller {

enum class SortAlgorithm { Selection, Insertion, Merge, Quick };
enum class SortKey { Name, Id, Age, AppointmentDate };

class SortEngine {
public:
    // The database is needed only for the "Appointment date" key.
    explicit SortEngine(const model::ClinicDatabase& database);

    // Sorts `list` in place (A -> Z / smallest first).
    void sort(std::vector<model::PatientRecord*>& list, SortAlgorithm algorithm, SortKey key);

    // How many times two patients were compared during the last sort.
    long long getComparisonCount() const { return comparisons; }

    // Names for the GUI combo boxes (same order as the enums).
    static std::vector<std::string> algorithmNames();
    static std::vector<std::string> keyNames();
    static std::string algorithmName(SortAlgorithm algorithm);
    static std::string keyName(SortKey key);

    // Explanation shown in the GUI of when each algorithm is a good choice.
    static std::string explainChoice(SortAlgorithm algorithm);

private:
    // Returns true if patient `a` should come BEFORE patient `b` for the
    // current key. Every algorithm uses only this function to compare,
    // so changing the key changes the sort order everywhere.
    bool comesBefore(const model::PatientRecord* a, const model::PatientRecord* b);

    // The four algorithms
    void selectionSort(std::vector<model::PatientRecord*>& list);
    void insertionSort(std::vector<model::PatientRecord*>& list);
    void mergeSort(std::vector<model::PatientRecord*>& list, int left, int right);
    void merge(std::vector<model::PatientRecord*>& list, int left, int middle, int right);
    void quickSort(std::vector<model::PatientRecord*>& list, int low, int high);
    int partition(std::vector<model::PatientRecord*>& list, int low, int high);

    // Swaps two items in the list (written by hand to keep it visible).
    static void swapItems(std::vector<model::PatientRecord*>& list, int i, int j);

    const model::ClinicDatabase& database;
    SortKey currentKey = SortKey::Name;
    long long comparisons = 0;
};

} // namespace scapms::controller

#endif
