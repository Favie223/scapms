// =====================================================================
// AdminPanel.cpp
// =====================================================================
#include "AdminPanel.h"
#include "GuiHelpers.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace scapms::gui {

AdminPanel::AdminPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager(manager) {

    QLabel* explanation = new QLabel(
        "Administrative operations (delete patient, edit patient, cancel or remove an appointment) "
        "are pushed onto an <b>undo stack</b>. A stack is <i>Last In, First Out</i>: "
        "only the most recent operation (top of the list) can be undone.");
    explanation->setWordWrap(true);

    topLabel = new QLabel;
    historyList = new QListWidget;
    historyList->setToolTip("Newest operation at the top");
    historyList->setSelectionMode(QAbstractItemView::NoSelection);

    undoButton = new QPushButton("Undo Last Operation");
    undoButton->setToolTip("Reverse the operation at the top of the stack (Ctrl+Z)");

    QHBoxLayout* buttonRow = new QHBoxLayout;
    buttonRow->addWidget(topLabel, 1);
    buttonRow->addWidget(undoButton);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(explanation);
    layout->addWidget(new QLabel("<b>Undo stack</b> (top = next to be undone):"));
    layout->addWidget(historyList, 1);
    layout->addLayout(buttonRow);

    connect(undoButton, &QPushButton::clicked, this, &AdminPanel::undoClicked);
    refresh();
}

void AdminPanel::refresh() {
    historyList->clear();
    std::vector<std::string> history = manager.getUndoHistory(); // newest first
    for (int i = 0; i < (int)history.size(); i++) {
        QString prefix = (i == 0) ? "TOP  >  " : "         ";
        historyList->addItem(prefix + toQ(history[i]));
    }
    if (history.empty()) {
        historyList->addItem("(empty - nothing to undo)");
    }
    undoButton->setEnabled(manager.canUndo());
    topLabel->setText(QString("%1 operation(s) on the stack").arg(history.size()));
}

void AdminPanel::undoClicked() {
    if (!manager.canUndo()) {
        QMessageBox::information(this, "Undo", "There is nothing to undo.");
        return;
    }
    try {
        std::string undone = manager.undoLastOperation();
        emit dataChanged();
        emit statusMessage("Undone: " + toQ(undone));
    } catch (const std::exception& e) {
        showError(this, e);
        emit dataChanged(); // the operation was popped, so refresh the list anyway
    }
}

} // namespace scapms::gui
