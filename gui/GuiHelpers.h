// =====================================================================
// GuiHelpers.h
// ---------------------------------------------------------------------
// Small helper functions shared by all the GUI panels.
//
// Our model/controller code uses std::string (standard C++), while Qt
// widgets use QString. These helpers convert between the two, so the
// conversion happens ONLY in the GUI layer and the core logic stays
// free of Qt.
//
// "inline" lets us define the functions in a header that is included
// by many .cpp files without "defined twice" linker errors.
// =====================================================================
#ifndef SCAPMS_GUI_GUIHELPERS_H
#define SCAPMS_GUI_GUIHELPERS_H

#include "../controller/ClinicManager.h"

#include <QComboBox>
#include <QMessageBox>
#include <QString>
#include <exception>
#include <string>

namespace scapms::gui {

// std::string -> QString
inline QString toQ(const std::string& text) {
    return QString::fromStdString(text);
}

// QString -> std::string
inline std::string toStd(const QString& text) {
    return text.trimmed().toStdString();
}

// Shows an error in a dialog box. Used inside every catch block so the
// user sees a clear message instead of the program crashing.
inline void showError(QWidget* parent, const std::exception& error) {
    QMessageBox::warning(parent, "Error", QString::fromStdString(error.what()));
}

// Fills a combo box with "P0001 - Ada Obi (General)" for every patient.
// The patient ID is stored as the item's hidden "data" so we can read
// it back with combo->currentData().toString().
inline void fillPatientCombo(QComboBox* combo, const controller::ClinicManager& manager) {
    QString previous = combo->currentData().toString(); // keep the selection if possible
    combo->blockSignals(true);                         // don't fire "changed" while refilling
    combo->clear();
    for (model::PatientRecord* patient : manager.getDatabase().getPatientList()) {
        QString label = toQ(patient->getId() + " - " + patient->getName() + " (" + patient->getCategory() + ")");
        combo->addItem(label, toQ(patient->getId()));
    }
    int index = combo->findData(previous);
    if (index >= 0) combo->setCurrentIndex(index);
    combo->blockSignals(false);
}

} // namespace scapms::gui

#endif
