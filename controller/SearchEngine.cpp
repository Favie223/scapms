// =====================================================================
// SearchEngine.cpp  -  searching algorithms, implemented by hand
// =====================================================================
#include "SearchEngine.h"
#include "../utils/Validation.h"

#include <cctype>

namespace scapms::controller {

using model::PatientRecord;
using utils::Validation::toLower;
using utils::Validation::trim;

// Does `text` contain `keyword`? Both should already be lowercase.
// Written by hand: slide the keyword along the text and compare.
static bool containsText(const std::string& text, const std::string& keyword) {
    if (keyword.size() > text.size()) return false;
    for (size_t start = 0; start + keyword.size() <= text.size(); start++) {
        bool allMatch = true;
        for (size_t k = 0; k < keyword.size(); k++) {
            if (text[start + k] != keyword[k]) {
                allMatch = false;
                break;
            }
        }
        if (allMatch) return true;
    }
    return false;
}

// =====================================================================
// LINEAR SEARCH by ID  -  O(n)
// Look at every patient from the first to the last until we find it.
// Works on ANY order, which is why it is used for unsorted data.
// =====================================================================
int SearchEngine::linearSearchById(const std::vector<PatientRecord*>& list,
                                   const std::string& id, int& steps) {
    std::string target = toLower(trim(id));
    for (int i = 0; i < (int)list.size(); i++) {
        steps++;
        if (toLower(list[i]->getId()) == target) {
            return i; // found it - stop early
        }
    }
    return -1; // checked everyone, not found
}

// LINEAR SEARCH by name (partial match) - O(n)
std::vector<PatientRecord*> SearchEngine::linearSearchByName(const std::vector<PatientRecord*>& list,
                                                             const std::string& keyword, int& steps) {
    std::vector<PatientRecord*> results;
    std::string target = toLower(trim(keyword));
    for (int i = 0; i < (int)list.size(); i++) {
        steps++;
        if (containsText(toLower(list[i]->getName()), target)) {
            results.push_back(list[i]);
        }
    }
    return results;
}

// =====================================================================
// BINARY SEARCH by ID  -  O(log n)   (list must be sorted by ID)
// ---------------------------------------------------------------------
// Look at the MIDDLE patient:
//   - same ID         -> found
//   - target smaller  -> it can only be in the LEFT half
//   - target bigger   -> it can only be in the RIGHT half
// Each step throws away half the list: 1024 -> 512 -> ... -> 1
// (at most about 10 steps for 1000 patients).
// =====================================================================
int SearchEngine::binarySearchById(const std::vector<PatientRecord*>& list,
                                   const std::string& id, int& steps) {
    // IDs are stored in uppercase (see PatientRecord), so convert what the
    // user typed: "p0003" -> "P0003". Then compare exactly the way
    // SortEngine sorts IDs (plain text order).
    std::string target = trim(id);
    for (char& c : target) c = (char)std::toupper((unsigned char)c);

    int low = 0;
    int high = (int)list.size() - 1;
    while (low <= high) {
        steps++;
        int middle = low + (high - low) / 2;
        const std::string& middleId = list[middle]->getId();

        if (middleId == target) {
            return middle;
        } else if (target < middleId) {
            high = middle - 1; // search the left half
        } else {
            low = middle + 1;  // search the right half
        }
    }
    return -1;
}

// BINARY SEARCH by exact name - O(log n)  (list must be sorted by name)
int SearchEngine::binarySearchByName(const std::vector<PatientRecord*>& list,
                                     const std::string& name, int& steps) {
    // Lowercase, to match SortEngine which sorts names case-insensitively.
    std::string target = toLower(trim(name));
    int low = 0;
    int high = (int)list.size() - 1;
    while (low <= high) {
        steps++;
        int middle = low + (high - low) / 2;
        std::string middleName = toLower(list[middle]->getName());

        if (middleName == target) {
            return middle;
        } else if (target < middleName) {
            high = middle - 1;
        } else {
            low = middle + 1;
        }
    }
    return -1;
}

// =====================================================================
// RECURSIVE SEARCH by name  -  O(n)
// ---------------------------------------------------------------------
// A function that CALLS ITSELF. Each call handles ONE patient:
//
//   BASE CASE      : index has reached the end of the list -> stop.
//   RECURSIVE STEP : check list[index]; then search the REST of the
//                    list by calling recursiveSearchByName(index + 1).
//
//   search(0) -> check Ada  -> search(1) -> check Bola -> search(2) -> end
//
// Note: every call uses a little stack memory, so the depth equals the
// number of patients. That is fine for a clinic (hundreds of records);
// for millions a normal loop would be safer.
// =====================================================================
void SearchEngine::recursiveSearchByName(const std::vector<PatientRecord*>& list,
                                         const std::string& keyword, int index,
                                         std::vector<PatientRecord*>& results, int& steps) {
    // BASE CASE
    if (index >= (int)list.size()) {
        return;
    }

    // Check the current patient
    steps++;
    if (containsText(toLower(list[index]->getName()), toLower(trim(keyword)))) {
        results.push_back(list[index]);
    }

    // RECURSIVE STEP: search the rest of the list
    recursiveSearchByName(list, keyword, index + 1, results, steps);
}

// CATEGORY FILTER - O(n). Uses the virtual getCategory(), so each
// patient answers with its own subclass's category (polymorphism).
std::vector<PatientRecord*> SearchEngine::searchByCategory(const std::vector<PatientRecord*>& list,
                                                           const std::string& category, int& steps) {
    std::vector<PatientRecord*> results;
    std::string target = toLower(trim(category));
    for (PatientRecord* patient : list) {
        steps++;
        std::string patientCategory = toLower(patient->getCategory());
        // Accept "followup" as well as "follow-up".
        if (patientCategory == target || (target == "followup" && patientCategory == "follow-up")) {
            results.push_back(patient);
        }
    }
    return results;
}

} // namespace scapms::controller
