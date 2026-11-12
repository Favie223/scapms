// =====================================================================
// Validation.h
// ---------------------------------------------------------------------
// Helper functions that check user input BEFORE it goes into the system.
// (Requirement 9e "Input Validation" and Requirement 10 "Exceptions".)
//
// How they work: each validateXxx() function does nothing when the input
// is fine, and THROWS an exception (see Exceptions.h) when it is wrong.
// The GUI catches the exception and shows the message in a dialog box,
// so the program never crashes because of bad input.
//
// These are free functions inside a namespace (not a class) because they
// do not need to remember any data between calls.
// =====================================================================
#ifndef SCAPMS_UTILS_VALIDATION_H
#define SCAPMS_UTILS_VALIDATION_H

#include <string>

namespace scapms::utils::Validation {

// Allowed age range, written once so it is easy to change.
const int kMinAge = 0;
const int kMaxAge = 120;

// ---- small string helpers ----
std::string trim(const std::string& text);     // remove spaces at both ends
std::string toLower(const std::string& text);  // "Ada" -> "ada"

// ---- validators (throw on bad input) ----
void validateName(const std::string& name);
void validateAge(int age);
void validatePhone(const std::string& phone);  // empty is allowed
void validateEmail(const std::string& email);  // empty is allowed
void validateDate(const std::string& date);        // "YYYY-MM-DD"
void validateDateTime(const std::string& dateTime); // "YYYY-MM-DD HH:MM"

// ---- checks that return true/false instead of throwing ----
bool isValidDate(const std::string& date);
bool isValidDateTime(const std::string& dateTime);

// Turns the text typed in the "Age" box into an int.
// Throws if it is not a number or is outside 0..120.
int parseAge(const std::string& text);

// ---- current date/time as text ----
std::string today(); // e.g. "2026-11-12"
std::string now();   // e.g. "2026-11-12 14:05"
std::string minutesFromNow(int minutes); // now + N minutes, same format as now()

} // namespace scapms::utils::Validation

#endif
