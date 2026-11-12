// =====================================================================
// PatientTableModel.cpp
// =====================================================================
#include "PatientTableModel.h"
#include "GuiHelpers.h"

#include <QBrush>
#include <QColor>
#include <QFont>

namespace scapms::gui {

using model::PatientRecord;

PatientTableModel::PatientTableModel(QObject* parent) : QAbstractTableModel(parent) {}

void PatientTableModel::setPatients(const std::vector<PatientRecord*>& newPatients,
                                    const model::ClinicDatabase* newDatabase) {
    // beginResetModel/endResetModel tell the view "everything changed,
    // redraw the whole table".
    beginResetModel();
    patients = newPatients;
    database = newDatabase;
    endResetModel();
}

PatientRecord* PatientTableModel::patientAt(int row) const {
    if (row < 0 || row >= (int)patients.size()) return nullptr;
    return patients[row];
}

int PatientTableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0; // a flat table has no child rows
    return (int)patients.size();
}

int PatientTableModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return ColumnCount;
}

QVariant PatientTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) return QVariant();
    switch (section) {
        case ColId:              return "ID";
        case ColName:            return "Name";
        case ColAge:             return "Age";
        case ColCategory:        return "Category";
        case ColPriority:        return "Priority";
        case ColPhone:           return "Phone";
        case ColNextAppointment: return "Next Appointment";
        case ColDetails:         return "Details";
    }
    return QVariant();
}

// Qt calls this for EVERY cell, once for each "role" (text, colour,
// tooltip ...). We return the right thing for each role.
QVariant PatientTableModel::data(const QModelIndex& index, int role) const {
    PatientRecord* patient = patientAt(index.row());
    if (patient == nullptr) return QVariant();

    std::string category = patient->getCategory(); // virtual call

    // ---- The text in the cell ----
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case ColId:       return toQ(patient->getId());
            case ColName:     return toQ(patient->getName());
            case ColAge:      return patient->getAge();
            case ColCategory: return toQ(category);
            case ColPriority: return toQ(patient->getPriorityLabel());
            case ColPhone:    return toQ(patient->getContact().phone);
            case ColNextAppointment: {
                std::string next = database ? database->getNextAppointmentDate(patient->getId()) : "";
                return next.empty() ? QString("-") : toQ(next);
            }
            case ColDetails:  return toQ(patient->describe()); // virtual call
        }
    }

    // ---- Row background colour by category ----
    if (role == Qt::BackgroundRole) {
        if (category == "Emergency") return QBrush(QColor(255, 205, 205)); // light red
        if (category == "Follow-Up") return QBrush(QColor(205, 225, 255)); // light blue
        return QVariant();                                                 // default
    }

    // ---- Text colour: dark text on the coloured rows so it is readable
    //      in dark mode too ----
    if (role == Qt::ForegroundRole) {
        if (category == "Emergency" || category == "Follow-Up") return QBrush(QColor(20, 20, 20));
        return QVariant();
    }

    // ---- Bold priority for emergencies ----
    if (role == Qt::FontRole && index.column() == ColPriority && category == "Emergency") {
        QFont font;
        font.setBold(true);
        return font;
    }

    // ---- Tooltip when the mouse hovers over a row ----
    if (role == Qt::ToolTipRole) {
        return toQ(patient->describe());
    }

    return QVariant();
}

} // namespace scapms::gui
