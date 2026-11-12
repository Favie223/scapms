// =====================================================================
// QueuePanel.h  -  the "Waiting Queue" tab
// ---------------------------------------------------------------------
// Shows the std::queue of patients waiting for consultation.
//   - "Add to Queue"      -> patient joins at the BACK
//   - "Call Next Patient" -> patient at the FRONT is served first (FIFO)
//
// A timer in MainWindow calls refresh() regularly so the "Waiting"
// minutes stay up to date. Anyone waiting longer than
// kLongWaitMinutes is highlighted in red.
// =====================================================================
#ifndef SCAPMS_GUI_QUEUEPANEL_H
#define SCAPMS_GUI_QUEUEPANEL_H

#include "../controller/ClinicManager.h"

#include <QWidget>

class QComboBox;
class QLabel;
class QTableWidget;

namespace scapms::gui {

class QueuePanel : public QWidget {
    Q_OBJECT

public:
    static const int kLongWaitMinutes = 15;

    explicit QueuePanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

    // How many people have been waiting longer than kLongWaitMinutes.
    int countLongWaits() const;

public slots:
    void refresh();

signals:
    void dataChanged();
    void statusMessage(const QString& message);

private slots:
    void addClicked();
    void callNextClicked();

private:
    controller::ClinicManager& manager;
    QComboBox* patientCombo;
    QTableWidget* table;
    QLabel* nowServingLabel;
    QLabel* sizeLabel;
};

} // namespace scapms::gui

#endif
