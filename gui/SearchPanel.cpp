// =====================================================================
// SearchPanel.cpp
// =====================================================================
#include "SearchPanel.h"
#include "GuiHelpers.h"
#include "PatientTableModel.h"

#include <QComboBox>
#include <QElapsedTimer>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

namespace scapms::gui {

using namespace scapms::controller;

SearchPanel::SearchPanel(ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager(manager) {

    // ===================== SEARCH box =====================
    fieldCombo = new QComboBox;
    fieldCombo->addItems({"Patient ID", "Patient Name", "Category"}); // same order as SearchField
    fieldCombo->setCurrentIndex(1);
    fieldCombo->setToolTip("Which field to search");

    searchMethodCombo = new QComboBox;
    searchMethodCombo->addItems({"Automatic", "Linear Search", "Binary Search", "Recursive Search"}); // same order as SearchMethod
    searchMethodCombo->setToolTip("Automatic picks Binary search when the list is sorted by that field, "
                                  "otherwise Linear/Recursive search.");

    keywordEdit = new QLineEdit;
    keywordEdit->setClearButtonEnabled(true);
    keywordEdit->setToolTip("Type a name, an ID (e.g. P0003) or a category. Ctrl+F jumps here.");

    QPushButton* searchButton = new QPushButton("Search");
    searchButton->setToolTip("Run the search (Enter)");
    QPushButton* showAllButton = new QPushButton("Show All");
    showAllButton->setToolTip("Clear the search and show every patient");

    searchInfo = new QLabel("Choose a field and type to search.");
    searchInfo->setWordWrap(true);

    // QGridLayout: labels in column 0, controls in columns 1-3.
    QGridLayout* searchGrid = new QGridLayout;
    searchGrid->addWidget(new QLabel("Search by:"), 0, 0);
    searchGrid->addWidget(fieldCombo, 0, 1);
    searchGrid->addWidget(new QLabel("Algorithm:"), 0, 2);
    searchGrid->addWidget(searchMethodCombo, 0, 3);
    searchGrid->addWidget(new QLabel("Keyword:"), 1, 0);
    searchGrid->addWidget(keywordEdit, 1, 1, 1, 2); // spans 2 columns
    QHBoxLayout* searchButtons = new QHBoxLayout;
    searchButtons->addWidget(searchButton);
    searchButtons->addWidget(showAllButton);
    searchGrid->addLayout(searchButtons, 1, 3);
    searchGrid->addWidget(searchInfo, 2, 0, 1, 4);   // spans all 4 columns
    searchGrid->setColumnStretch(1, 1);
    searchGrid->setColumnStretch(3, 1);

    QGroupBox* searchBox = new QGroupBox("Search patient records");
    searchBox->setLayout(searchGrid);

    // ===================== SORT box =====================
    sortAlgorithmCombo = new QComboBox;
    for (const std::string& name : SortEngine::algorithmNames()) sortAlgorithmCombo->addItem(toQ(name));
    sortAlgorithmCombo->setCurrentIndex((int)SortAlgorithm::Merge);
    sortAlgorithmCombo->setToolTip("Which hand-written sorting algorithm to use");

    sortKeyCombo = new QComboBox;
    for (const std::string& name : SortEngine::keyNames()) sortKeyCombo->addItem(toQ(name));
    sortKeyCombo->setToolTip("Which field to sort the patients by");

    QPushButton* sortButton = new QPushButton("SORT");
    sortButton->setToolTip("Sort all patients with the chosen algorithm");

    sortExplanation = new QLabel;
    sortExplanation->setWordWrap(true);
    sortInfo = new QLabel;
    sortInfo->setWordWrap(true);

    QGridLayout* sortGrid = new QGridLayout;
    sortGrid->addWidget(new QLabel("Sort Algorithm:"), 0, 0);
    sortGrid->addWidget(sortAlgorithmCombo, 0, 1);
    sortGrid->addWidget(new QLabel("Sort By:"), 0, 2);
    sortGrid->addWidget(sortKeyCombo, 0, 3);
    sortGrid->addWidget(sortButton, 0, 4);
    sortGrid->addWidget(sortExplanation, 1, 0, 1, 5);
    sortGrid->addWidget(sortInfo, 2, 0, 1, 5);
    sortGrid->setColumnStretch(1, 1);
    sortGrid->setColumnStretch(3, 1);

    QGroupBox* sortBox = new QGroupBox("Sort patient records");
    sortBox->setLayout(sortGrid);

    // ===================== Results table =====================
    tableModel = new PatientTableModel(this);
    table = new QTableView;
    table->setModel(tableModel);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(searchBox);
    layout->addWidget(sortBox);
    layout->addWidget(table, 1);

    // ===================== Events =====================
    connect(searchButton, &QPushButton::clicked, this, &SearchPanel::searchClicked);
    connect(keywordEdit, &QLineEdit::returnPressed, this, &SearchPanel::searchClicked);
    connect(keywordEdit, &QLineEdit::textChanged, this, &SearchPanel::liveSearch);
    connect(showAllButton, &QPushButton::clicked, this, &SearchPanel::showAll);
    connect(sortButton, &QPushButton::clicked, this, &SearchPanel::sortClicked);
    connect(sortAlgorithmCombo, &QComboBox::currentIndexChanged, this, &SearchPanel::updateSortExplanation);
    connect(fieldCombo, &QComboBox::currentIndexChanged, this, &SearchPanel::updateSearchHint);

    updateSortExplanation();
    updateSearchHint();
    refresh();
}

void SearchPanel::focusSearchBox() {
    keywordEdit->setFocus();
    keywordEdit->selectAll();
}

// Shows every patient in the manager's current order (sorted if a sort was done).
void SearchPanel::refresh() {
    if (keywordEdit->text().trimmed().isEmpty()) {
        tableModel->setPatients(manager.getPatientList(), &manager.getDatabase());
        table->resizeColumnsToContents();
    } else {
        runSearch(false);
    }
    sortInfo->setText("Current order: " + toQ(manager.getSortDescription()));
}

void SearchPanel::showAll() {
    keywordEdit->clear(); // triggers liveSearch -> refresh
    searchInfo->setText("Showing all patients.");
    refresh();
}

void SearchPanel::updateSearchHint() {
    switch (fieldCombo->currentIndex()) {
        case 0: keywordEdit->setPlaceholderText("e.g. P0003"); break;
        case 1: keywordEdit->setPlaceholderText("e.g. Ada (partial names work)"); break;
        case 2: keywordEdit->setPlaceholderText("General, Emergency or Follow-Up"); break;
    }
}

void SearchPanel::updateSortExplanation() {
    SortAlgorithm algorithm = (SortAlgorithm)sortAlgorithmCombo->currentIndex();
    sortExplanation->setText("<i>" + toQ(SortEngine::explainChoice(algorithm)).toHtmlEscaped() + "</i>");
}

void SearchPanel::searchClicked() { runSearch(true); }

void SearchPanel::liveSearch() {
    if (keywordEdit->text().trimmed().isEmpty()) {
        tableModel->setPatients(manager.getPatientList(), &manager.getDatabase());
        searchInfo->setText("Choose a field and type to search.");
        return;
    }
    runSearch(false);
}

void SearchPanel::runSearch(bool showErrorsInDialog) {
    SearchField field = (SearchField)fieldCombo->currentIndex();
    SearchMethod method = (SearchMethod)searchMethodCombo->currentIndex();
    try {
        SearchResult result = manager.search(field, method, toStd(keywordEdit->text()));
        tableModel->setPatients(result.matches, &manager.getDatabase());
        table->resizeColumnsToContents();
        searchInfo->setText(QString("<b>%1</b> found %2 match(es) in %3 step(s).")
                                .arg(toQ(result.algorithmUsed).toHtmlEscaped())
                                .arg(result.matches.size())
                                .arg(result.steps));
    } catch (const std::exception& e) {
        tableModel->setPatients({}, &manager.getDatabase());
        searchInfo->setText("<span style='color:#c0392b'>" + QString(e.what()).toHtmlEscaped() + "</span>");
        if (showErrorsInDialog) showError(this, e);
    }
}

void SearchPanel::sortClicked() {
    SortAlgorithm algorithm = (SortAlgorithm)sortAlgorithmCombo->currentIndex();
    SortKey key = (SortKey)sortKeyCombo->currentIndex();
    try {
        QElapsedTimer timer; // measure how long the sort takes
        timer.start();
        long long comparisons = manager.sortPatients(algorithm, key);
        qint64 microseconds = timer.nsecsElapsed() / 1000;

        keywordEdit->clear();
        // Tell the other tabs first (this also refreshes THIS panel),
        // then write the result line so the refresh doesn't overwrite it.
        emit dataChanged();
        emit statusMessage("Patients " + toQ(manager.getSortDescription()));
        sortInfo->setText(QString("Sorted %1 patients by <b>%2</b> using <b>%3</b>: "
                                  "%4 comparisons, %5 microseconds.")
                              .arg(manager.getDatabase().patientCount())
                              .arg(toQ(SortEngine::keyName(key)), toQ(SortEngine::algorithmName(algorithm)))
                              .arg(comparisons)
                              .arg(microseconds));
    } catch (const std::exception& e) {
        showError(this, e);
    }
}

} // namespace scapms::gui
