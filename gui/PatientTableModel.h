// =====================================================================
// PatientTableModel.h
// ---------------------------------------------------------------------
// REQUIREMENT 9a (Custom Table Presentation).
//
// Qt separates the DATA of a table (the "model") from how it is DRAWN
// (the QTableView). This class tells the table view:
//   - how many rows/columns there are,
//   - what text goes in each cell,
//   - what COLOUR each row should be:
//         Emergency  -> red    "High Priority"
//         Follow-Up  -> blue   "Scheduled"
//         General    -> normal "Normal"
//
// The colour comes from the patient's virtual getCategory() /
// getPriorityLabel() functions, so this is polymorphism in the GUI too.
//
// The model only holds POINTERS to the patients; it does not own them.
// =====================================================================
#ifndef SCAPMS_GUI_PATIENTTABLEMODEL_H
#define SCAPMS_GUI_PATIENTTABLEMODEL_H

#include "../model/ClinicDatabase.h"
#include "../model/PatientRecord.h"

#include <QAbstractTableModel>
#include <vector>

namespace scapms::gui {

class PatientTableModel : public QAbstractTableModel {
    Q_OBJECT // needed by every Qt class that uses signals/slots

public:
    enum Column { ColId, ColName, ColAge, ColCategory, ColPriority, ColPhone, ColNextAppointment, ColDetails, ColumnCount };

    explicit PatientTableModel(QObject* parent = nullptr);

    // Give the table a new list to show.
    void setPatients(const std::vector<model::PatientRecord*>& patients, const model::ClinicDatabase* database);

    // Which patient is in a given row (nullptr if the row is invalid).
    model::PatientRecord* patientAt(int row) const;

    // ---- Functions Qt calls to draw the table ----
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    std::vector<model::PatientRecord*> patients;
    const model::ClinicDatabase* database = nullptr; // for the "next appointment" column
};

} // namespace scapms::gui

#endif
