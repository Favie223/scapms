// =====================================================================
// SearchPanel.h  -  the "Search & Sort" tab
// ---------------------------------------------------------------------
// Requirement 4, 5 and 7d in the GUI:
//
//   SEARCH:  [Field: Name v] [Algorithm: Automatic v] [keyword...] [Search]
//   SORT:    [Algorithm: Merge Sort v] [Sort By: Patient Name v]  [SORT]
//
// After each search/sort, an info line shows WHICH algorithm ran and how
// many comparisons/steps it needed, plus why that algorithm suits the
// situation. This makes the Big-O differences visible during a demo.
//
// Text-field event: typing in the keyword box searches live (errors are
// shown quietly in the info line instead of popping up a dialog on
// every key press). Pressing Enter or the Search button shows errors
// in a dialog.
// =====================================================================
#ifndef SCAPMS_GUI_SEARCHPANEL_H
#define SCAPMS_GUI_SEARCHPANEL_H

#include "../controller/ClinicManager.h"

#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QTableView;

namespace scapms::gui {

class PatientTableModel;

class SearchPanel : public QWidget {
    Q_OBJECT

public:
    explicit SearchPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

    void focusSearchBox(); // used by Ctrl+F

public slots:
    void refresh();

signals:
    void dataChanged();
    void statusMessage(const QString& message);

private slots:
    void searchClicked();         // button / Enter: errors shown in a dialog
    void liveSearch();            // while typing: errors shown quietly
    void sortClicked();
    void showAll();
    void updateSortExplanation();
    void updateSearchHint();

private:
    void runSearch(bool showErrorsInDialog);

    controller::ClinicManager& manager;

    QComboBox* fieldCombo;
    QComboBox* searchMethodCombo;
    QLineEdit* keywordEdit;
    QLabel* searchInfo;

    QComboBox* sortAlgorithmCombo;
    QComboBox* sortKeyCombo;
    QLabel* sortExplanation;
    QLabel* sortInfo;

    PatientTableModel* tableModel;
    QTableView* table;
};

} // namespace scapms::gui

#endif
