// =====================================================================
// IDGenerator.h
// ---------------------------------------------------------------------
// Creates unique, readable IDs:  P0001, P0002, ...  (patients)
//                                A0001, A0002, ...  (appointments)
//
// It just keeps a counter and adds 1 each time next() is called.
// After loading data from a file we call observe() with every existing
// ID, so the counter jumps past them and we never create a duplicate.
// =====================================================================
#ifndef SCAPMS_UTILS_IDGENERATOR_H
#define SCAPMS_UTILS_IDGENERATOR_H

#include <string>

namespace scapms::utils {

class IDGenerator {
public:
    // prefix: the letter in front ('P' or 'A')
    // width:  how many digits (4 -> "0001")
    IDGenerator(char prefix, int width = 4);

    std::string next();                          // returns a new ID and moves the counter on
    std::string peek() const;                    // shows the next ID without using it
    void observe(const std::string& existingId); // "this ID is already taken"
    void reset();                                // start again from 1

private:
    std::string format(int number) const;        // 7 -> "P0007"

    char prefix;
    int width;
    int counter = 0; // the last number handed out
};

} // namespace scapms::utils

#endif
