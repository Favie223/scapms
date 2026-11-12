// =====================================================================
// PatientDialog.cpp
// =====================================================================
#include "PatientDialog.h"
#include "GuiHelpers.h"
#include "../model/PatientFactory.h"
#include "../utils/Exceptions.h"
#include "../utils/Validation.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedLayout>
#include <QVBoxLayout>

namespace scapms::gui {

using namespace scapms::model;
using namespace scapms::utils;

PatientDialog::PatientDialog(const controller::ClinicManager& manager,
                             const PatientRecord* existing, QWidget* parent)
    : QDialog(parent), manager(manager), editMode(existing != nullptr) {
    setWindowTitle(editMode ? "Edit Patient" : "Register New Patient");
    setMinimumWidth(460);

    // ------------------------------------------------------------
    // 1. Common fields, laid out with a QFormLayout
    // ------------------------------------------------------------
    typeCombo = new QComboBox;
    for (const std::string& type : PatientFactory::typeNames()) typeCombo->addItem(toQ(type));
    typeCombo->setToolTip("General = routine visit, Emergency = urgent, Follow-Up = returning after a visit");

    idEdit = new QLineEdit;
    idEdit->setToolTip("A unique patient ID. A free one is suggested for you.");
    idHint = new QLabel;

    nameEdit = new QLineEdit;
    nameEdit->setPlaceholderText("e.g. Ada Obi");
    nameEdit->setToolTip("Letters, spaces, '-', '.' and apostrophes only");

    ageEdit = new QLineEdit;
    ageEdit->setPlaceholderText("0 - 120");
    ageEdit->setToolTip("Age in whole years, between 0 and 120");
    ageHint = new QLabel;

    phoneEdit = new QLineEdit;
    phoneEdit->setPlaceholderText("e.g. 08031234567 (optional)");
    emailEdit = new QLineEdit;
    emailEdit->setPlaceholderText("e.g. ada@mail.com (optional)");
    addressEdit = new QLineEdit;
    addressEdit->setPlaceholderText("optional");

    QFormLayout* commonForm = new QFormLayout;
    commonForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow); // fields fill the width
    commonForm->addRow("Patient type:", typeCombo);
    commonForm->addRow("Patient ID:", idEdit);
    commonForm->addRow("", idHint);
    commonForm->addRow("Full name:", nameEdit);
    commonForm->addRow("Age:", ageEdit);
    commonForm->addRow("", ageHint);
    commonForm->addRow("Phone:", phoneEdit);
    commonForm->addRow("Email:", emailEdit);
    commonForm->addRow("Address:", addressEdit);

    // ------------------------------------------------------------
    // 2. Type-specific fields: one page per type in a QStackedLayout.
    //    Only ONE page is visible at a time.
    // ------------------------------------------------------------
    // Page 0 - General
    QWidget* generalPage = new QWidget;
    QFormLayout* generalForm = new QFormLayout(generalPage);
    generalForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    insuranceEdit = new QLineEdit;
    insuranceEdit->setPlaceholderText("optional");
    complaintEdit = new QLineEdit;
    complaintEdit->setPlaceholderText("e.g. Headache");
    generalForm->addRow("Insurance no.:", insuranceEdit);
    generalForm->addRow("Main complaint:", complaintEdit);

    // Page 1 - Emergency
    QWidget* emergencyPage = new QWidget;
    QFormLayout* emergencyForm = new QFormLayout(emergencyPage);
    emergencyForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    severitySpin = new QSpinBox;
    severitySpin->setRange(1, 5);
    severitySpin->setValue(3);
    severitySpin->setToolTip("1 = mild ... 5 = critical (critical appointments cannot be cancelled)");
    conditionEdit = new QLineEdit;
    conditionEdit->setPlaceholderText("e.g. Fracture");
    arrivalEdit = new QLineEdit;
    arrivalEdit->setPlaceholderText("YYYY-MM-DD HH:MM (blank = now)");
    emergencyForm->addRow("Severity (1-5):", severitySpin);
    emergencyForm->addRow("Condition:", conditionEdit);
    emergencyForm->addRow("Arrival time:", arrivalEdit);

    // Page 2 - Follow-Up
    QWidget* followUpPage = new QWidget;
    QFormLayout* followUpForm = new QFormLayout(followUpPage);
    followUpForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    previousVisitEdit = new QLineEdit;
    previousVisitEdit->setPlaceholderText("YYYY-MM-DD");
    previousVisitEdit->setToolTip("Follow-up appointments must be AFTER this date");
    referringEdit = new QLineEdit;
    referringEdit->setPlaceholderText("e.g. Dr. Bello");
    followReasonEdit = new QLineEdit;
    followReasonEdit->setPlaceholderText("e.g. Remove stitches");
    followUpForm->addRow("Previous visit:", previousVisitEdit);
    followUpForm->addRow("Referring doctor:", referringEdit);
    followUpForm->addRow("Follow-up reason:", followReasonEdit);

    QGroupBox* typeBox = new QGroupBox("Type details");
    typeStack = new QStackedLayout(typeBox);
    typeStack->addWidget(generalPage);   // index 0
    typeStack->addWidget(emergencyPage); // index 1
    typeStack->addWidget(followUpPage);  // index 2

    // ------------------------------------------------------------
    // 3. Save / Cancel buttons
    // ------------------------------------------------------------
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Save)->setToolTip("Check the details and save the patient");

    // Put it all together, top to bottom (QVBoxLayout).
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(commonForm);
    mainLayout->addWidget(typeBox);
    mainLayout->addWidget(buttons);

    // ------------------------------------------------------------
    // 4. Signals & slots: connect events to the functions that handle them
    // ------------------------------------------------------------
    connect(typeCombo, &QComboBox::currentIndexChanged, this, &PatientDialog::onTypeChanged);
    connect(idEdit, &QLineEdit::textChanged, this, &PatientDialog::onIdEdited);
    connect(ageEdit, &QLineEdit::textChanged, this, &PatientDialog::onAgeEdited);
    connect(buttons, &QDialogButtonBox::accepted, this, &PatientDialog::onSaveClicked);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // ------------------------------------------------------------
    // 5. Starting values
    // ------------------------------------------------------------
    if (editMode) {
        fillFrom(existing);
        typeCombo->setEnabled(false); // the type can't change while editing
        idEdit->setReadOnly(true);    // nor can the ID
        idEdit->setToolTip("The patient ID cannot be changed");
    } else {
        idEdit->setText(toQ(manager.suggestPatientId()));
    }
    onTypeChanged(typeCombo->currentIndex());
}

// Copies an existing patient's values into the form (edit mode).
void PatientDialog::fillFrom(const PatientRecord* patient) {
    typeCombo->setCurrentText(toQ(patient->getCategory()));
    idEdit->setText(toQ(patient->getId()));
    nameEdit->setText(toQ(patient->getName()));
    ageEdit->setText(QString::number(patient->getAge()));
    phoneEdit->setText(toQ(patient->getContact().phone));
    emailEdit->setText(toQ(patient->getContact().email));
    addressEdit->setText(toQ(patient->getContact().address));

    // The 3 extra fields mean different things for each type.
    std::vector<std::string> extra = patient->getExtraFields();
    std::string category = patient->getCategory();
    if (category == "General") {
        insuranceEdit->setText(toQ(extra[0]));
        complaintEdit->setText(toQ(extra[1]));
    } else if (category == "Emergency") {
        severitySpin->setValue(QString::fromStdString(extra[0]).toInt());
        conditionEdit->setText(toQ(extra[1]));
        arrivalEdit->setText(toQ(extra[2]));
    } else {
        previousVisitEdit->setText(toQ(extra[0]));
        referringEdit->setText(toQ(extra[1]));
        followReasonEdit->setText(toQ(extra[2]));
    }
}

// DYNAMIC COMPONENTS: show only the fields for the selected type.
void PatientDialog::onTypeChanged(int index) {
    typeStack->setCurrentIndex(index);
}

// TEXT FIELD EVENT: runs on every key press in the ID box.
void PatientDialog::onIdEdited(const QString& text) {
    if (editMode) {
        idHint->clear();
        return;
    }
    if (text.trimmed().isEmpty()) {
        idHint->setText("<span style='color:#c0392b'>ID is required</span>");
    } else if (manager.getPatient(text.trimmed().toUpper().toStdString()) != nullptr) {
        idHint->setText("<span style='color:#c0392b'>This ID already exists</span>");
    } else {
        idHint->setText("<span style='color:#27ae60'>ID is available</span>");
    }
}

// TEXT FIELD EVENT: checks the age while the user types.
void PatientDialog::onAgeEdited(const QString& text) {
    if (text.trimmed().isEmpty()) {
        ageHint->clear();
        return;
    }
    try {
        Validation::parseAge(text.toStdString());
        ageHint->clear();
    } catch (const ClinicException& e) {
        ageHint->setText("<span style='color:#c0392b'>" + QString::fromStdString(e.what()).toHtmlEscaped() + "</span>");
    }
}

// Save button: try to build the patient. Any rule that is broken throws
// an exception, which we catch and show. The dialog stays open.
void PatientDialog::onSaveClicked() {
    try {
        std::string id = toStd(idEdit->text());
        if (!editMode && manager.getPatient(QString::fromStdString(id).toUpper().toStdString()) != nullptr) {
            throw DuplicateIdException(id);
        }

        int age = Validation::parseAge(toStd(ageEdit->text())); // throws InvalidAgeException etc.

        ContactInfo contact;
        contact.phone = toStd(phoneEdit->text());
        contact.email = toStd(emailEdit->text());
        contact.address = toStd(addressEdit->text());

        // Collect the 3 type-specific values from the visible page.
        std::vector<std::string> extra;
        int type = typeCombo->currentIndex();
        if (type == 0) {
            extra = {toStd(insuranceEdit->text()), toStd(complaintEdit->text()), ""};
        } else if (type == 1) {
            extra = {std::to_string(severitySpin->value()), toStd(conditionEdit->text()), toStd(arrivalEdit->text())};
        } else {
            extra = {toStd(previousVisitEdit->text()), toStd(referringEdit->text()), toStd(followReasonEdit->text())};
        }

        // The factory builds the right subclass. All validation happens
        // in the model classes, so the rules live in ONE place.
        result = PatientFactory::create(toStd(typeCombo->currentText()), id, toStd(nameEdit->text()),
                                        age, contact, extra);
        accept(); // close the dialog with "OK"
    } catch (const std::exception& e) {
        showError(this, e); // stay open so the user can correct it
    }
}

} // namespace scapms::gui
