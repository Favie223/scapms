// =====================================================================
// UndoManager.cpp
// =====================================================================
#include "UndoManager.h"
#include "../utils/Exceptions.h"

namespace scapms::controller {

void UndoManager::push(const AdminOperation& operation) {
    operations.push(operation);
}

AdminOperation UndoManager::pop() {
    if (operations.empty()) {
        throw utils::ClinicException("There is nothing to undo.");
    }
    AdminOperation top = operations.top(); // look at the top item
    operations.pop();                      // then remove it
    return top;
}

std::string UndoManager::peekDescription() const {
    if (operations.empty()) return "";
    return operations.top().description;
}

// std::stack does not let us loop through it, so we make a COPY and pop
// items off the copy. The real stack is not changed.
std::vector<std::string> UndoManager::getHistory() const {
    std::vector<std::string> history;
    std::stack<AdminOperation> copy = operations;
    while (!copy.empty()) {
        history.push_back(copy.top().description);
        copy.pop();
    }
    return history; // newest first
}

void UndoManager::clear() {
    while (!operations.empty()) {
        operations.pop();
    }
}

} // namespace scapms::controller
