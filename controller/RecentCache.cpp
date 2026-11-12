// =====================================================================
// RecentCache.cpp
// =====================================================================
#include "RecentCache.h"

namespace scapms::controller {

using model::PatientRecord;

RecentCache::RecentCache() {
    // Start with every slot empty.
    for (int i = 0; i < CAPACITY; i++) {
        cacheSlots[i] = nullptr;
    }
}

int RecentCache::indexOf(const PatientRecord* patient) const {
    for (int i = 0; i < count; i++) {
        if (cacheSlots[i] == patient) return i;
    }
    return -1;
}

PatientRecord* RecentCache::find(const std::string& id) {
    for (int i = 0; i < count; i++) {
        if (cacheSlots[i]->getId() == id) {
            hits++;
            return cacheSlots[i];
        }
    }
    misses++;
    return nullptr;
}

// Closes the gap at `index` by shifting the later items one place left.
void RecentCache::removeAt(int index) {
    for (int i = index; i < count - 1; i++) {
        cacheSlots[i] = cacheSlots[i + 1];
    }
    count--;
    cacheSlots[count] = nullptr;
}

void RecentCache::recordAccess(PatientRecord* patient) {
    if (patient == nullptr) return;

    // If it is already in the cache, take it out first (it moves to the front).
    int existing = indexOf(patient);
    if (existing != -1) {
        removeAt(existing);
    }

    // If full, the LAST slot (oldest) will be pushed out.
    if (count == CAPACITY) {
        count--;
    }

    // Shift everything one place right to free slot 0 ...
    for (int i = count; i > 0; i--) {
        cacheSlots[i] = cacheSlots[i - 1];
    }
    // ... and put the patient at the front.
    cacheSlots[0] = patient;
    count++;
}

void RecentCache::remove(const PatientRecord* patient) {
    int index = indexOf(patient);
    if (index != -1) removeAt(index);
}

void RecentCache::replace(const PatientRecord* oldPatient, PatientRecord* newPatient) {
    int index = indexOf(oldPatient);
    if (index != -1) cacheSlots[index] = newPatient;
}

void RecentCache::clear() {
    for (int i = 0; i < CAPACITY; i++) cacheSlots[i] = nullptr;
    count = 0;
    hits = 0;
    misses = 0;
}

std::vector<PatientRecord*> RecentCache::getAll() const {
    std::vector<PatientRecord*> list;
    for (int i = 0; i < count; i++) list.push_back(cacheSlots[i]);
    return list;
}

} // namespace scapms::controller
