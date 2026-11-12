// =====================================================================
// main.cpp  -  program entry point
// ---------------------------------------------------------------------
// Creates the Qt application, opens the main window and starts the
// "event loop" (app.exec()). From then on the program just waits for
// EVENTS - clicks, key presses, timer ticks - and runs the slot
// connected to each one. That is event-driven programming (Req 6).
// =====================================================================
#include "gui/MainWindow.h"

#include <QApplication>

// SCAPMS_DATA_DIR is set by CMakeLists.txt to the project's data/ folder,
// so the sample data is found no matter where the program is run from.
#ifndef SCAPMS_DATA_DIR
#define SCAPMS_DATA_DIR "data"
#endif

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("SCAPMS");

    scapms::gui::MainWindow window(SCAPMS_DATA_DIR);
    window.show();

    return app.exec(); // start the event loop
}
