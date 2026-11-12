// =====================================================================
// Validation.cpp  -  implementation of the input checks in Validation.h
// =====================================================================
#include "Validation.h"
#include "Exceptions.h"

#include <cctype>  // isalpha, isdigit, isspace, tolower
#include <ctime>   // time, localtime_r, strftime

namespace scapms::utils::Validation {

// ---------------------------------------------------------------------
// String helpers
// ---------------------------------------------------------------------

// Removes spaces/tabs from the start and end: "  Ada  " -> "Ada"
std::string trim(const std::string& text) {
    size_t start = 0;
    size_t end = text.size();
    while (start < end && std::isspace((unsigned char)text[start])) start++;
    while (end > start && std::isspace((unsigned char)text[end - 1])) end--;
    return text.substr(start, end - start);
}

// Makes every letter lowercase so searches can ignore upper/lower case.
std::string toLower(const std::string& text) {
    std::string result = text;
    for (size_t i = 0; i < result.size(); i++) {
        result[i] = (char)std::tolower((unsigned char)result[i]);
    }
    return result;
}

// ---------------------------------------------------------------------
// Validators
// ---------------------------------------------------------------------

// A name must not be empty and may only contain letters, spaces,
// hyphens, dots and apostrophes (e.g. "Mary-Jane O'Neil Jr.").
void validateName(const std::string& name) {
    std::string clean = trim(name);
    if (clean.empty()) {
        throw ValidationException("name cannot be empty.");
    }
    if (clean.size() > 60) {
        throw ValidationException("name is too long (maximum 60 characters).");
    }
    for (char c : clean) {
        bool allowed = std::isalpha((unsigned char)c) || c == ' ' || c == '-'
                       || c == '\'' || c == '.';
        if (!allowed) {
            throw ValidationException("name may only contain letters, spaces, '-', '.' and apostrophes.");
        }
    }
}

// Age must be between kMinAge (0) and kMaxAge (120).
void validateAge(int age) {
    if (age < kMinAge || age > kMaxAge) {
        throw InvalidAgeException(age);
    }
}

// Phone is optional. If given: digits, spaces, '-', and an optional '+'
// at the very start. Must contain 7 to 15 digits in total.
void validatePhone(const std::string& phone) {
    std::string clean = trim(phone);
    if (clean.empty()) return; // optional field

    int digitCount = 0;
    for (size_t i = 0; i < clean.size(); i++) {
        char c = clean[i];
        if (std::isdigit((unsigned char)c)) {
            digitCount++;
        } else if (c == '+' && i == 0) {
            // a leading '+' (country code) is fine
        } else if (c == ' ' || c == '-') {
            // separators are fine
        } else {
            throw ValidationException("phone number may only contain digits, spaces, '-' and a leading '+'.");
        }
    }
    if (digitCount < 7 || digitCount > 15) {
        throw ValidationException("phone number must have between 7 and 15 digits.");
    }
}

// Email is optional. If given it needs something before '@', a '.'
// after the '@', and no spaces. (A simple check, not a perfect one.)
void validateEmail(const std::string& email) {
    std::string clean = trim(email);
    if (clean.empty()) return; // optional field

    size_t atPos = clean.find('@');
    bool ok = atPos != std::string::npos           // has an '@'
              && atPos > 0                          // something before '@'
              && clean.find('.', atPos) != std::string::npos // '.' after '@'
              && clean.back() != '.'                // does not end with '.'
              && clean.find(' ') == std::string::npos; // no spaces
    if (!ok) {
        throw ValidationException("email address is not valid.");
    }
}

// Returns true if characters text[from .. from+count-1] are all digits.
static bool allDigits(const std::string& text, size_t from, size_t count) {
    for (size_t i = from; i < from + count; i++) {
        if (!std::isdigit((unsigned char)text[i])) return false;
    }
    return true;
}

// Checks a date like "2026-11-12": right shape AND a real calendar day
// (so "2026-02-30" is rejected). Handles leap years.
bool isValidDate(const std::string& date) {
    // Shape check: 10 characters, dashes in positions 4 and 7.
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') return false;
    if (!allDigits(date, 0, 4) || !allDigits(date, 5, 2) || !allDigits(date, 8, 2)) return false;

    int year = std::stoi(date.substr(0, 4));
    int month = std::stoi(date.substr(5, 2));
    int day = std::stoi(date.substr(8, 2));
    if (year < 1900 || year > 2200) return false;
    if (month < 1 || month > 12) return false;

    // Days in each month (index 0 = January).
    int daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    bool isLeapYear = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    if (isLeapYear) daysInMonth[1] = 29;

    return day >= 1 && day <= daysInMonth[month - 1];
}

// Checks a date + time like "2026-11-12 14:30" (24-hour clock).
bool isValidDateTime(const std::string& dateTime) {
    if (dateTime.size() != 16 || dateTime[10] != ' ' || dateTime[13] != ':') return false;
    if (!isValidDate(dateTime.substr(0, 10))) return false;
    if (!allDigits(dateTime, 11, 2) || !allDigits(dateTime, 14, 2)) return false;

    int hour = std::stoi(dateTime.substr(11, 2));
    int minute = std::stoi(dateTime.substr(14, 2));
    return hour < 24 && minute < 60;
}

void validateDate(const std::string& date) {
    if (!isValidDate(date)) {
        throw ValidationException("date must be a real date in the form YYYY-MM-DD.");
    }
}

void validateDateTime(const std::string& dateTime) {
    if (!isValidDateTime(dateTime)) {
        throw InvalidAppointmentException("date/time must be in the form YYYY-MM-DD HH:MM.");
    }
}

// Converts typed text to an age. Example: "34" -> 34, "abc" -> exception.
int parseAge(const std::string& text) {
    std::string clean = trim(text);
    if (clean.empty()) {
        throw ValidationException("age is required.");
    }
    // Allow a leading '-' so that "-5" gives the nicer "age must be 0-120"
    // message instead of "not a number".
    for (size_t i = 0; i < clean.size(); i++) {
        bool isMinusSign = (i == 0 && clean[i] == '-');
        if (!std::isdigit((unsigned char)clean[i]) && !isMinusSign) {
            throw ValidationException("age must be a whole number.");
        }
    }
    if (clean.size() > 4) {
        throw InvalidAgeException(9999); // far too big, avoid stoi overflow
    }
    int age = std::stoi(clean);
    validateAge(age);
    return age;
}

// ---------------------------------------------------------------------
// Current date / time
// ---------------------------------------------------------------------

// Formats the current local time (+ an offset in seconds) using a
// strftime pattern such as "%Y-%m-%d".
static std::string formatCurrentTime(const char* pattern, long offsetSeconds = 0) {
    std::time_t seconds = std::time(nullptr) + offsetSeconds;
    std::tm local{};
    localtime_r(&seconds, &local);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), pattern, &local);
    return std::string(buffer);
}

std::string today() { return formatCurrentTime("%Y-%m-%d"); }
std::string now() { return formatCurrentTime("%Y-%m-%d %H:%M"); }
std::string minutesFromNow(int minutes) { return formatCurrentTime("%Y-%m-%d %H:%M", minutes * 60L); }

} // namespace scapms::utils::Validation
