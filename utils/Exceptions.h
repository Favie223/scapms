// =====================================================================
// Exceptions.h
// ---------------------------------------------------------------------
// REQUIREMENT 10 (Exception Handling): our own custom exception classes.
//
// Why custom exceptions?
//   - Each kind of error (bad age, duplicate ID, missing file ...) gets
//     its own class, so code can tell them apart if it needs to.
//   - They ALL inherit from ClinicException, so the GUI can catch every
//     clinic error with ONE catch block:
//
//         try { manager.addPatient(...); }
//         catch (const ClinicException& e) { show e.what() in a dialog }
//
//   - ClinicException inherits from std::runtime_error, so it also works
//     with any code that catches the standard std::exception.
//
// Inheritance tree:
//   std::runtime_error
//     └── ClinicException
//           ├── ValidationException          (bad name/phone/email ...)
//           ├── InvalidAgeException          (age not 0..120)
//           ├── DuplicateIdException         (ID already used)
//           ├── NotFoundException            (patient/appointment missing)
//           ├── InvalidAppointmentException  (bad date, wrong status ...)
//           ├── FileLoadException            (file missing / corrupt)
//           └── InvalidSearchInputException  (empty search box ...)
//
// Header-only: these classes are tiny, so no .cpp file is needed.
// =====================================================================
#ifndef SCAPMS_UTILS_EXCEPTIONS_H
#define SCAPMS_UTILS_EXCEPTIONS_H

#include <stdexcept>
#include <string>

namespace scapms::utils {

// Parent of all our exceptions. "explicit" stops accidental conversions
// from a plain string into an exception.
class ClinicException : public std::runtime_error {
public:
    explicit ClinicException(const std::string& message)
        : std::runtime_error(message) {}
};

// Thrown when a patient's details (name, phone, email, ...) are not valid.
class ValidationException : public ClinicException {
public:
    explicit ValidationException(const std::string& message)
        : ClinicException("Invalid patient information: " + message) {}
};

// Thrown when age is outside 0..120. Message matches the brief's example.
class InvalidAgeException : public ClinicException {
public:
    explicit InvalidAgeException(int age)
        : ClinicException("Patient age must be between 0 and 120 (you entered "
                          + std::to_string(age) + ").") {}
};

// Thrown when someone tries to register a patient ID that already exists.
class DuplicateIdException : public ClinicException {
public:
    explicit DuplicateIdException(const std::string& id)
        : ClinicException("Patient ID already exists: " + id) {}
};

// Thrown when we look for a patient or appointment that is not there.
class NotFoundException : public ClinicException {
public:
    explicit NotFoundException(const std::string& what)
        : ClinicException("Not found: " + what) {}
};

// Thrown for bad appointment data (wrong date format, date in the past,
// cancelling an appointment that is already cancelled, ...).
class InvalidAppointmentException : public ClinicException {
public:
    explicit InvalidAppointmentException(const std::string& message)
        : ClinicException("Invalid appointment: " + message) {}
};

// Thrown when a data file cannot be opened, read or written.
class FileLoadException : public ClinicException {
public:
    explicit FileLoadException(const std::string& message)
        : ClinicException("File error: " + message) {}
};

// Thrown when the search box input does not make sense (e.g. it is empty).
class InvalidSearchInputException : public ClinicException {
public:
    explicit InvalidSearchInputException(const std::string& message)
        : ClinicException("Invalid search input: " + message) {}
};

} // namespace scapms::utils

#endif
