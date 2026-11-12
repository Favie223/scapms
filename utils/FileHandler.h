// =====================================================================
// FileHandler.h
// ---------------------------------------------------------------------
// REQUIREMENT 11d (Persistent): saves and loads patients and
// appointments as CSV ("comma separated values") text files.
//
// patients.csv
//   id,type,name,age,phone,email,address,registeredOn,extra1,extra2,extra3
//   P0001,General,Ada Obi,34,08031234567,ada@mail.com,Hall 3,2026-11-01,INS-22,Headache,
//
// appointments.csv
//   id,patientId,dateTime,doctor,reason,status
//   A0001,P0001,2026-11-14 10:30,Dr. Bello,Checkup,Pending
//
// CSV rule: if a value contains a comma or a quote, it is wrapped in
// double quotes and any quote inside is doubled:
//     Hall 3, Room 2     ->  "Hall 3, Room 2"
//
// Errors: if a file cannot be opened or a line is broken, a
// FileLoadException is thrown with the file name and line number, so the
// GUI can show a helpful message instead of crashing.
// =====================================================================
#ifndef SCAPMS_UTILS_FILEHANDLER_H
#define SCAPMS_UTILS_FILEHANDLER_H

#include "../model/Appointment.h"
#include "../model/ClinicDatabase.h"
#include "../model/PatientRecord.h"

#include <memory>
#include <string>
#include <vector>

namespace scapms::utils {

namespace FileHandler {

    // ---- Saving ----
    void savePatients(const std::string& filePath, const std::vector<model::PatientRecord*>& patients);
    void saveAppointments(const std::string& filePath, const std::vector<model::Appointment>& appointments);
    void saveText(const std::string& filePath, const std::string& text); // used for report export

    // ---- Loading ----
    std::vector<std::unique_ptr<model::PatientRecord>> loadPatients(const std::string& filePath);
    std::vector<model::Appointment> loadAppointments(const std::string& filePath);

    bool fileExists(const std::string& filePath);

    // ---- CSV helpers (public so they are easy to test) ----
    std::string escapeCsv(const std::string& value);
    std::vector<std::string> splitCsvLine(const std::string& line);

} // namespace FileHandler

} // namespace scapms::utils

#endif
