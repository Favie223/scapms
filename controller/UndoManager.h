// =====================================================================
// UndoManager.h
// ---------------------------------------------------------------------
// REQUIREMENT 3c (std::stack): "Undo Last Administrative Operation".
//
// A STACK is "Last In, First Out" (LIFO) - like a pile of plates, you
// can only take the TOP one. That is exactly how undo should behave:
// the MOST RECENT operation is undone first.
//
//     push(delete P0003)   push(edit P0001)   pop() -> undoes "edit P0001"
//        [del P0003]         [edit P0001]          [del P0003]
//                            [del P0003]
//
// Every admin operation saves enough information to reverse itself:
//   - DeletePatient     : a full copy of the patient + their appointments
//                         + where they were in the list
//   - ModifyPatient     : a copy of the patient BEFORE the edit
//   - RemoveAppointment : a copy of the removed appointment
//   - CancelAppointment : the appointment ID (its old status was Pending)
//
// This class only STORES the operations. ClinicManager::undo() does the
// actual reversing because it has access to the database.
// =====================================================================
#ifndef SCAPMS_CONTROLLER_UNDOMANAGER_H
#define SCAPMS_CONTROLLER_UNDOMANAGER_H

#include "../model/Appointment.h"
#include "../model/PatientRecord.h"

#include <memory>
#include <stack>
#include <string>
#include <vector>

namespace scapms::controller {

enum class OperationType { DeletePatient, ModifyPatient, RemoveAppointment, CancelAppointment };

// One entry on the undo stack.
struct AdminOperation {
    OperationType type = OperationType::DeletePatient;
    std::string description; // shown in the Admin panel, e.g. "Deleted patient P0003 (Ada Obi)"

    // Copy of the patient (for DeletePatient / ModifyPatient).
    // shared_ptr (not unique_ptr) so an AdminOperation can be COPIED,
    // which we need to list the history without emptying the stack.
    std::shared_ptr<model::PatientRecord> patientBackup;
    int patientIndex = -1; // position in the list before deletion

    // Copies of appointments (the deleted patient's, or the one removed).
    std::vector<model::Appointment> appointmentBackups;

    // For CancelAppointment: which appointment to set back to Pending.
    std::string appointmentId;
};

class UndoManager {
public:
    void push(const AdminOperation& operation); // add to the top
    AdminOperation pop();                       // remove and return the top (throws if empty)
    bool canUndo() const { return !operations.empty(); }
    int size() const { return (int)operations.size(); }
    std::string peekDescription() const;        // "" if empty

    // Descriptions from newest to oldest (for the Admin panel list).
    std::vector<std::string> getHistory() const;

    void clear();

private:
    std::stack<AdminOperation> operations; // <-- the std::stack
};

} // namespace scapms::controller

#endif
