// =====================================================================
// SearchEngine.h
// ---------------------------------------------------------------------
// REQUIREMENT 4 (Searching Algorithms) - ALL written by hand.
//
//   Algorithm                    Time       Needs sorted data?
//   --------------------------   --------   ------------------
//   Linear search (by ID)        O(n)       No
//   Linear search (by name)      O(n)       No   (partial match)
//   Binary search (by ID)        O(log n)   YES - sorted by ID
//   Binary search (by name)      O(log n)   YES - sorted by name
//   Recursive search (by name)   O(n)       No   (partial match)
//   Category filter              O(n)       No
//
// Choosing the right one (done in ClinicManager::search):
//   - If the list is currently sorted by the field we are searching,
//     binary search is used (much faster: 1000 patients -> about 10 steps
//     instead of up to 1000).
//   - Otherwise linear/recursive search is used, because binary search
//     gives WRONG answers on unsorted data.
//
// Every function counts its "steps" (comparisons) so the GUI can show
// how much work the search did.
// =====================================================================
#ifndef SCAPMS_CONTROLLER_SEARCHENGINE_H
#define SCAPMS_CONTROLLER_SEARCHENGINE_H

#include "../model/PatientRecord.h"

#include <string>
#include <vector>

namespace scapms::controller {

// What a search gives back to the GUI.
struct SearchResult {
    std::vector<model::PatientRecord*> matches;
    std::string algorithmUsed; // e.g. "Binary search (list sorted by ID)"
    int steps = 0;             // number of comparisons made
};

namespace SearchEngine {

    // Checks each patient one by one from the start.
    // Returns the index of the patient with that ID, or -1.
    int linearSearchById(const std::vector<model::PatientRecord*>& list,
                         const std::string& id, int& steps);

    // Returns every patient whose name CONTAINS the keyword (any case).
    std::vector<model::PatientRecord*> linearSearchByName(
        const std::vector<model::PatientRecord*>& list,
        const std::string& keyword, int& steps);

    // Halves the search area each step. The list MUST be sorted by ID.
    // Returns the index, or -1.
    int binarySearchById(const std::vector<model::PatientRecord*>& list,
                         const std::string& id, int& steps);

    // Same idea for an exact (case-insensitive) name. The list MUST be
    // sorted by name. Returns the index of one match, or -1.
    int binarySearchByName(const std::vector<model::PatientRecord*>& list,
                           const std::string& name, int& steps);

    // RECURSIVE search: checks list[index], then calls itself for
    // index + 1. Adds every partial name match to `results`.
    void recursiveSearchByName(const std::vector<model::PatientRecord*>& list,
                               const std::string& keyword, int index,
                               std::vector<model::PatientRecord*>& results, int& steps);

    // Returns every patient of a category ("General", "Emergency", "Follow-Up").
    std::vector<model::PatientRecord*> searchByCategory(
        const std::vector<model::PatientRecord*>& list,
        const std::string& category, int& steps);

} // namespace SearchEngine

} // namespace scapms::controller

#endif
