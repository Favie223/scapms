// =====================================================================
// PatientDialog.h
// ---------------------------------------------------------------------
// The pop-up form for ADDING or EDITING a patient.
//
// Shows off several GUI requirements:
//   - QFormLayout (Req 8): "Label: [field]" rows that line up neatly.
//   - QStackedLayout + DYNAMIC COMPONENTS (Req 9b): the "Type details"
//     box shows different fields depending on the chosen patient type.
//     Changing the Type combo box swaps the visible page.
//   - TEXT FIELD EVENTS (Req 6c): typing in the ID or Age box is checked
//     straight away and a hint appears ("ID already exists" etc.).
//   - INPUT VALIDATION (Req 9e): pressing Save builds the patient; if
//     any rule is broken an error dialog is shown and the form stays
//     open so the user can fix it.
// =====================================================================
#ifndef SCAPMS_GUI_PATIENTDIALOG_H
#define SCAPMS_GUI_PATIENTDIALOG_H

#include "../controller/ClinicManager.h"
#include "../model/PatientRecord.h"

#include <QDialog>
#include <memory>

class QComboBox;
class QLabel;
class QLineEdit;
class QSpinBox;
class QStackedLayout;

namespace scapms::gui {

class PatientDialog : public QDialog {
    Q_OBJECT

public:
    // existing == nullptr  -> "Add patient" mode
    // existing != nullptr  -> "Edit patient" mode (fields are pre-filled)
    PatientDialog(const controller::ClinicManager& manager,
                  const model::PatientRecord* existing,
                  QWidget* parent = nullptr);

    // After exec() returns Accepted, this gives the new patient object.
    std::unique_ptr<model::PatientRecord> takePatient() { return std::move(result); }

private slots:
    void onTypeChanged(int index); // swaps the type-specific fields
    void onIdEdited(const QString& text);
    void onAgeEdited(const QString& text);
    void onSaveClicked();

private:
    void fillFrom(const model::PatientRecord* patient);

    const controller::ClinicManager& manager;
    bool editMode = false;
    std::unique_ptr<model::PatientRecord> result;

    // Common fields
    QComboBox* typeCombo;
    QLineEdit* idEdit;
    QLabel* idHint;
    QLineEdit* nameEdit;
    QLineEdit* ageEdit;
    QLabel* ageHint;
    QLineEdit* phoneEdit;
    QLineEdit* emailEdit;
    QLineEdit* addressEdit;

    // Type-specific fields (one page each in the stacked layout)
    QStackedLayout* typeStack;
    QLineEdit* insuranceEdit;    // General
    QLineEdit* complaintEdit;    // General
    QSpinBox* severitySpin;      // Emergency
    QLineEdit* conditionEdit;    // Emergency
    QLineEdit* arrivalEdit;      // Emergency
    QLineEdit* previousVisitEdit;// Follow-Up
    QLineEdit* referringEdit;    // Follow-Up
    QLineEdit* followReasonEdit; // Follow-Up
};

} // namespace scapms::gui

#endif
