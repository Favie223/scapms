// =====================================================================
// AdminPanel.h  -  the "Admin" tab
// ---------------------------------------------------------------------
// Shows the UNDO STACK (std::stack, Requirement 3c) as a list, newest
// operation at the top, and an "Undo Last Operation" button (Ctrl+Z).
// Only the TOP item can be undone - that is how a stack works (LIFO).
// =====================================================================
#ifndef SCAPMS_GUI_ADMINPANEL_H
#define SCAPMS_GUI_ADMINPANEL_H

#include "../controller/ClinicManager.h"

#include <QWidget>

class QLabel;
class QListWidget;
class QPushButton;

namespace scapms::gui {

class AdminPanel : public QWidget {
    Q_OBJECT

public:
    explicit AdminPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

public slots:
    void refresh();
    void undoClicked(); // also used by Edit -> Undo (Ctrl+Z)

signals:
    void dataChanged();
    void statusMessage(const QString& message);

private:
    controller::ClinicManager& manager;
    QListWidget* historyList;
    QPushButton* undoButton;
    QLabel* topLabel;
};

} // namespace scapms::gui

#endif
