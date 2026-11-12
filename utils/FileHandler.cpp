// =====================================================================
// FileHandler.cpp
// =====================================================================
#include "FileHandler.h"
#include "Exceptions.h"
#include "Validation.h"
#include "../model/PatientFactory.h"

#include <fstream>  // ifstream (read files), ofstream (write files)

namespace scapms::utils {

using namespace scapms::model;

static const std::string kPatientHeader =
    "id,type,name,age,phone,email,address,registeredOn,extra1,extra2,extra3";
static const std::string kAppointmentHeader = "id,patientId,dateTime,doctor,reason,status";

// ---------------------------------------------------------------------
// CSV helpers
// ---------------------------------------------------------------------

std::string FileHandler::escapeCsv(const std::string& value) {
    bool needsQuotes = value.find(',') != std::string::npos
                       || value.find('"') != std::string::npos
                       || value.find('\n') != std::string::npos;
    if (!needsQuotes) return value;

    std::string result = "\"";
    for (char c : value) {
        if (c == '"') result += "\"\""; // a quote inside becomes two quotes
        else if (c == '\n') result += ' '; // keep one record per line
        else result += c;
    }
    result += "\"";
    return result;
}

// Splits one CSV line into its fields, respecting quoted values.
// Reads character by character, remembering whether we are inside quotes.
std::vector<std::string> FileHandler::splitCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool insideQuotes = false;

    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (insideQuotes) {
            if (c == '"') {
                bool doubledQuote = (i + 1 < line.size() && line[i + 1] == '"');
                if (doubledQuote) {
                    current += '"'; // "" means a real quote character
                    i++;
                } else {
                    insideQuotes = false; // closing quote
                }
            } else {
                current += c;
            }
        } else {
            if (c == '"') {
                insideQuotes = true;          // opening quote
            } else if (c == ',') {
                fields.push_back(current);    // end of this field
                current.clear();
            } else if (c != '\r') {           // ignore Windows line endings
                current += c;
            }
        }
    }
    fields.push_back(current); // the last field
    return fields;
}

bool FileHandler::fileExists(const std::string& filePath) {
    std::ifstream file(filePath);
    return file.good();
}

// ---------------------------------------------------------------------
// Saving
// ---------------------------------------------------------------------

void FileHandler::savePatients(const std::string& filePath, const std::vector<PatientRecord*>& patients) {
    std::ofstream file(filePath);
    if (!file) {
        throw FileLoadException("could not open '" + filePath + "' for writing.");
    }
    file << kPatientHeader << "\n";
    for (const PatientRecord* p : patients) {
        // getCategory() and getExtraFields() are virtual, so each
        // subclass writes its own type name and its own extra fields.
        std::vector<std::string> extra = p->getExtraFields();
        file << escapeCsv(p->getId()) << ','
             << escapeCsv(p->getCategory()) << ','
             << escapeCsv(p->getName()) << ','
             << p->getAge() << ','
             << escapeCsv(p->getContact().phone) << ','
             << escapeCsv(p->getContact().email) << ','
             << escapeCsv(p->getContact().address) << ','
             << escapeCsv(p->getRegisteredOn()) << ','
             << escapeCsv(extra[0]) << ','
             << escapeCsv(extra[1]) << ','
             << escapeCsv(extra[2]) << "\n";
    }
    if (!file) {
        throw FileLoadException("an error happened while writing '" + filePath + "'.");
    }
}

void FileHandler::saveAppointments(const std::string& filePath, const std::vector<Appointment>& appointments) {
    std::ofstream file(filePath);
    if (!file) {
        throw FileLoadException("could not open '" + filePath + "' for writing.");
    }
    file << kAppointmentHeader << "\n";
    for (const Appointment& a : appointments) {
        file << escapeCsv(a.getId()) << ','
             << escapeCsv(a.getPatientId()) << ','
             << escapeCsv(a.getDateTime()) << ','
             << escapeCsv(a.getDoctor()) << ','
             << escapeCsv(a.getReason()) << ','
             << a.getStatusText() << "\n";
    }
    if (!file) {
        throw FileLoadException("an error happened while writing '" + filePath + "'.");
    }
}

void FileHandler::saveText(const std::string& filePath, const std::string& text) {
    std::ofstream file(filePath);
    if (!file) {
        throw FileLoadException("could not open '" + filePath + "' for writing.");
    }
    file << text;
}

// ---------------------------------------------------------------------
// Loading
// ---------------------------------------------------------------------

std::vector<std::unique_ptr<PatientRecord>> FileHandler::loadPatients(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file) {
        throw FileLoadException("could not open '" + filePath + "'. Check that the file exists.");
    }

    std::vector<std::unique_ptr<PatientRecord>> patients;
    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        lineNumber++;
        if (lineNumber == 1) continue;                  // skip the header row
        if (Validation::trim(line).empty()) continue;   // skip blank lines

        std::vector<std::string> f = splitCsvLine(line);
        if (f.size() < 8) {
            throw FileLoadException(filePath + " line " + std::to_string(lineNumber)
                                    + ": expected at least 8 values, found " + std::to_string(f.size()) + ".");
        }
        // Make sure there are always 3 extra fields (may be empty).
        while (f.size() < 11) f.push_back("");

        // Any validation error (bad age, bad name ...) is turned into a
        // FileLoadException that says WHICH line is broken.
        try {
            ContactInfo contact;
            contact.phone = f[4];
            contact.email = f[5];
            contact.address = f[6];
            int age = Validation::parseAge(f[3]);
            std::unique_ptr<PatientRecord> patient =
                PatientFactory::create(f[1], f[0], f[2], age, contact, {f[8], f[9], f[10]});
            if (!Validation::trim(f[7]).empty()) {
                patient->setRegisteredOn(Validation::trim(f[7]));
            }
            patients.push_back(std::move(patient));
        } catch (const ClinicException& e) {
            throw FileLoadException(filePath + " line " + std::to_string(lineNumber) + ": " + e.what());
        }
    }
    return patients;
}

std::vector<Appointment> FileHandler::loadAppointments(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file) {
        throw FileLoadException("could not open '" + filePath + "'. Check that the file exists.");
    }

    std::vector<Appointment> appointments;
    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        lineNumber++;
        if (lineNumber == 1) continue;
        if (Validation::trim(line).empty()) continue;

        std::vector<std::string> f = splitCsvLine(line);
        if (f.size() < 6) {
            throw FileLoadException(filePath + " line " + std::to_string(lineNumber)
                                    + ": expected 6 values, found " + std::to_string(f.size()) + ".");
        }
        try {
            appointments.push_back(Appointment(f[0], f[1], f[2], f[3], f[4], statusFromString(f[5])));
        } catch (const ClinicException& e) {
            throw FileLoadException(filePath + " line " + std::to_string(lineNumber) + ": " + e.what());
        }
    }
    return appointments;
}

} // namespace scapms::utils
