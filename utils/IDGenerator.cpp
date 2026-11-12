// =====================================================================
// IDGenerator.cpp
// =====================================================================
#include "IDGenerator.h"

#include <cctype>

namespace scapms::utils {

IDGenerator::IDGenerator(char prefix, int width) : prefix(prefix), width(width) {}

// Pads the number with zeros on the left: 7 -> "P0007".
std::string IDGenerator::format(int number) const {
    std::string digits = std::to_string(number);
    while ((int)digits.size() < width) {
        digits = "0" + digits;
    }
    return std::string(1, prefix) + digits;
}

std::string IDGenerator::next() {
    counter++;
    return format(counter);
}

std::string IDGenerator::peek() const {
    return format(counter + 1);
}

// Example: counter is 3 and we load "P0010" from a file.
// The counter becomes 10, so next() gives "P0011" (no clash).
void IDGenerator::observe(const std::string& existingId) {
    if (existingId.size() < 2 || existingId[0] != prefix) return;

    // Turn the digits after the prefix into a number by hand.
    int number = 0;
    for (size_t i = 1; i < existingId.size(); i++) {
        if (!std::isdigit((unsigned char)existingId[i])) return; // not our format
        number = number * 10 + (existingId[i] - '0');
    }
    if (number > counter) {
        counter = number;
    }
}

void IDGenerator::reset() {
    counter = 0;
}

} // namespace scapms::utils
