// =====================================================================
// SortEngine.cpp  -  four sorting algorithms, implemented by hand
// =====================================================================
#include "SortEngine.h"
#include "../utils/Validation.h"

namespace scapms::controller {

using model::PatientRecord;
using utils::Validation::toLower;

SortEngine::SortEngine(const model::ClinicDatabase& database) : database(database) {}

// ---------------------------------------------------------------------
// Entry point: pick the algorithm the user chose in the combo box.
// ---------------------------------------------------------------------
void SortEngine::sort(std::vector<PatientRecord*>& list, SortAlgorithm algorithm, SortKey key) {
    currentKey = key;
    comparisons = 0;
    if (list.size() < 2) return; // 0 or 1 item is already sorted

    int last = (int)list.size() - 1;
    switch (algorithm) {
        case SortAlgorithm::Selection: selectionSort(list); break;
        case SortAlgorithm::Insertion: insertionSort(list); break;
        case SortAlgorithm::Merge:     mergeSort(list, 0, last); break;
        case SortAlgorithm::Quick:     quickSort(list, 0, last); break;
    }
}

// ---------------------------------------------------------------------
// The comparison used by every algorithm.
// ---------------------------------------------------------------------
bool SortEngine::comesBefore(const PatientRecord* a, const PatientRecord* b) {
    comparisons++;
    switch (currentKey) {
        case SortKey::Name:
            // Compare lowercase so "ada" and "Ada" sort together.
            return toLower(a->getName()) < toLower(b->getName());

        case SortKey::Id:
            // IDs are zero-padded (P0002, P0010) so text order = number order.
            return a->getId() < b->getId();

        case SortKey::Age:
            return a->getAge() < b->getAge();

        case SortKey::AppointmentDate: {
            // "YYYY-MM-DD HH:MM" strings sort correctly as text.
            // Patients with NO pending appointment ("") go to the end.
            std::string dateA = database.getNextAppointmentDate(a->getId());
            std::string dateB = database.getNextAppointmentDate(b->getId());
            if (dateA.empty()) return false;   // a has none -> a is not before b
            if (dateB.empty()) return true;    // b has none, a has one -> a first
            return dateA < dateB;
        }
    }
    return false;
}

void SortEngine::swapItems(std::vector<PatientRecord*>& list, int i, int j) {
    PatientRecord* temp = list[i];
    list[i] = list[j];
    list[j] = temp;
}

// =====================================================================
// 1. SELECTION SORT  -  O(n^2) always
// ---------------------------------------------------------------------
// Idea: find the SMALLEST item in the unsorted part and swap it to the
// front. Repeat for the next position.
//
//   [D B A C]  smallest of all is A -> swap with pos 0 -> [A B D C]
//   [A|B D C]  smallest of rest is B -> already there   -> [A B D C]
//   [A B|D C]  smallest of rest is C -> swap with pos 2 -> [A B C D]
//
// Good point: does at most n-1 swaps (few writes).
// Bad point: always scans the whole remaining list, even if sorted.
// =====================================================================
void SortEngine::selectionSort(std::vector<PatientRecord*>& list) {
    int n = (int)list.size();
    for (int i = 0; i < n - 1; i++) {
        int smallest = i; // assume the first unsorted item is the smallest
        for (int j = i + 1; j < n; j++) {
            if (comesBefore(list[j], list[smallest])) {
                smallest = j;
            }
        }
        if (smallest != i) {
            swapItems(list, i, smallest);
        }
    }
}

// =====================================================================
// 2. INSERTION SORT  -  O(n) best (already sorted), O(n^2) worst
// ---------------------------------------------------------------------
// Idea: like sorting playing cards in your hand. Take the next card and
// slide it left until it is in the right place among the sorted cards.
//
//   [B|D A C]  take D: B<D, stays           -> [B D|A C]
//   [B D|A C]  take A: shift D, shift B     -> [A B D|C]
//   [A B D|C]  take C: shift D, stop at B   -> [A B C D]
//
// Why it suits our clinic: patients are added ONE AT A TIME to a list
// that is usually already sorted, so each new patient only moves a few
// places -> close to O(n). It is also STABLE.
// =====================================================================
void SortEngine::insertionSort(std::vector<PatientRecord*>& list) {
    int n = (int)list.size();
    for (int i = 1; i < n; i++) {
        PatientRecord* current = list[i]; // the "card" we are placing
        int j = i - 1;
        // Shift bigger items one step right to make room.
        while (j >= 0 && comesBefore(current, list[j])) {
            list[j + 1] = list[j];
            j--;
        }
        list[j + 1] = current; // drop it into the gap
    }
}

// =====================================================================
// 3. MERGE SORT  -  O(n log n) always   (RECURSIVE)
// ---------------------------------------------------------------------
// Idea: "divide and conquer".
//   1. Split the list into two halves.
//   2. Sort each half (by calling mergeSort on it - RECURSION).
//   3. Merge the two sorted halves into one sorted list.
// Base case: a list of 1 item is already sorted, so we stop.
//
//   [D B A C] -> [D B] [A C] -> [D][B] [A][C] -> [B D] [A C] -> [A B C D]
//
// Why: guaranteed O(n log n) even in the worst case, and STABLE, so it
// is the best choice for re-sorting the WHOLE list by a new key or after
// importing many records. Cost: needs a temporary list (O(n) memory).
// =====================================================================
void SortEngine::mergeSort(std::vector<PatientRecord*>& list, int left, int right) {
    if (left >= right) return; // BASE CASE: 0 or 1 item

    int middle = left + (right - left) / 2; // (avoids overflow of left+right)
    mergeSort(list, left, middle);          // RECURSIVE CALL: sort left half
    mergeSort(list, middle + 1, right);     // RECURSIVE CALL: sort right half
    merge(list, left, middle, right);       // combine the two halves
}

// Merges list[left..middle] and list[middle+1..right], both already sorted.
void SortEngine::merge(std::vector<PatientRecord*>& list, int left, int middle, int right) {
    std::vector<PatientRecord*> temp;
    temp.reserve(right - left + 1);

    int i = left;       // next item in the left half
    int j = middle + 1; // next item in the right half

    // Repeatedly take the smaller front item of the two halves.
    while (i <= middle && j <= right) {
        // Take from the RIGHT only if it is strictly smaller. On a tie we
        // take from the LEFT, which is what keeps merge sort stable.
        if (comesBefore(list[j], list[i])) {
            temp.push_back(list[j]);
            j++;
        } else {
            temp.push_back(list[i]);
            i++;
        }
    }
    // One half is used up; copy whatever is left of the other.
    while (i <= middle) { temp.push_back(list[i]); i++; }
    while (j <= right)  { temp.push_back(list[j]); j++; }

    // Copy the merged result back into the original list.
    for (int k = 0; k < (int)temp.size(); k++) {
        list[left + k] = temp[k];
    }
}

// =====================================================================
// 4. QUICK SORT  -  O(n log n) average, O(n^2) worst   (RECURSIVE)
// ---------------------------------------------------------------------
// Idea:
//   1. Pick a "pivot" item.
//   2. PARTITION: move everything smaller than the pivot to its left
//      and everything else to its right. The pivot is now in its final
//      place.
//   3. Quick-sort the left part and the right part (RECURSION).
//
// We choose the MIDDLE item as the pivot. If we always took the last
// item, an already-sorted list would give the O(n^2) worst case; the
// middle item avoids that common case.
//
// Why: usually the fastest in practice and sorts in place (no big temp
// list). Not stable, and has a (rare) O(n^2) worst case.
// =====================================================================
void SortEngine::quickSort(std::vector<PatientRecord*>& list, int low, int high) {
    if (low >= high) return; // BASE CASE

    int pivotIndex = partition(list, low, high);
    quickSort(list, low, pivotIndex - 1);  // RECURSIVE CALL: left part
    quickSort(list, pivotIndex + 1, high); // RECURSIVE CALL: right part
}

// Lomuto partition scheme. Returns the pivot's final position.
int SortEngine::partition(std::vector<PatientRecord*>& list, int low, int high) {
    // Move the middle item to the end and use it as the pivot.
    int middle = low + (high - low) / 2;
    swapItems(list, middle, high);
    PatientRecord* pivot = list[high];

    int boundary = low; // everything before `boundary` is smaller than pivot
    for (int j = low; j < high; j++) {
        if (comesBefore(list[j], pivot)) {
            swapItems(list, boundary, j);
            boundary++;
        }
    }
    swapItems(list, boundary, high); // put the pivot right after the smaller items
    return boundary;
}

// ---------------------------------------------------------------------
// Names and explanations for the GUI
// ---------------------------------------------------------------------
std::vector<std::string> SortEngine::algorithmNames() {
    return {"Selection Sort", "Insertion Sort", "Merge Sort", "Quick Sort"};
}

std::vector<std::string> SortEngine::keyNames() {
    return {"Patient Name", "Patient ID", "Age", "Appointment Date"};
}

std::string SortEngine::algorithmName(SortAlgorithm algorithm) {
    return algorithmNames()[(int)algorithm];
}

std::string SortEngine::keyName(SortKey key) {
    return keyNames()[(int)key];
}

std::string SortEngine::explainChoice(SortAlgorithm algorithm) {
    switch (algorithm) {
        case SortAlgorithm::Selection:
            return "Selection Sort: O(n^2) every time, but makes very few swaps. "
                   "Fine for a small clinic list; mainly included for comparison.";
        case SortAlgorithm::Insertion:
            return "Insertion Sort: O(n) when the list is already nearly sorted, which is "
                   "common here because patients are added one at a time. Stable.";
        case SortAlgorithm::Merge:
            return "Merge Sort: always O(n log n) and stable - the safest choice for "
                   "re-sorting the whole list by a new key or after a big import.";
        case SortAlgorithm::Quick:
            return "Quick Sort: O(n log n) on average and usually fastest in practice; "
                   "sorts in place. Middle pivot avoids the worst case on sorted data.";
    }
    return "";
}

} // namespace scapms::controller
