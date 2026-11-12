// =====================================================================
// RecentCache.h
// ---------------------------------------------------------------------
// REQUIREMENT 3d (fixed-size array): a small cache of the
// "Frequently / Recently Accessed Patient Records".
//
//     PatientRecord* cacheSlots[10];      <-- a plain C++ array, size fixed at 10
//
// How it works ("Most Recently Used" order):
//   - cacheSlots[0] is the patient viewed most recently, cacheSlots[count-1] the oldest.
//   - When a patient is viewed:
//       * already in the cache -> move it to the front  (a cache HIT)
//       * not in the cache     -> put it at the front; if all 10 slots
//                                 are full the oldest one drops off the end
//   - The Dashboard shows this list so staff can quickly reopen patients.
//
// Why a fixed array? It uses a fixed, small amount of memory and never
// grows, which is exactly what a "small cache" should do.
//
// SAFETY: the cache stores plain pointers but does NOT own the patients.
// When a patient is deleted or replaced, ClinicManager calls remove() /
// replace() so the cache never holds a "dangling" pointer to a deleted
// object.
// =====================================================================
#ifndef SCAPMS_CONTROLLER_RECENTCACHE_H
#define SCAPMS_CONTROLLER_RECENTCACHE_H

#include "../model/PatientRecord.h"

#include <string>
#include <vector>

namespace scapms::controller {

class RecentCache {
public:
    static const int CAPACITY = 10;

    RecentCache();

    // Look up a patient by ID in the cache only. Returns nullptr on a miss.
    // Counts hits and misses for the statistics shown on the dashboard.
    model::PatientRecord* find(const std::string& id);

    // Record that a patient was just viewed (moves it to the front).
    void recordAccess(model::PatientRecord* patient);

    void remove(const model::PatientRecord* patient);   // patient was deleted
    void replace(const model::PatientRecord* oldPatient, // patient was edited
                 model::PatientRecord* newPatient);
    void clear();

    int size() const { return count; }
    std::vector<model::PatientRecord*> getAll() const; // most recent first
    int getHits() const { return hits; }
    int getMisses() const { return misses; }

private:
    int indexOf(const model::PatientRecord* patient) const; // -1 if absent
    void removeAt(int index);                                // shift left to close the gap

    model::PatientRecord* cacheSlots[CAPACITY]; // <-- the fixed-size array (not called "slots": that word is reserved by Qt)
    int count = 0;                         // how many slots are in use
    int hits = 0;
    int misses = 0;
};

} // namespace scapms::controller

#endif
