cmake_minimum_required(VERSION 3.16)

project(SmartClinicSystem VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)

# Turns on AUTOMOC (Qt's tool that generates code for Q_OBJECT classes).
qt_standard_project_setup()

# ---------------------------------------------------------------------
# 1. Core library: model + controller + utils.
#    Plain C++ with NO Qt, which proves the business logic is separate
#    from the GUI. Both the app and the tests use it.
# ---------------------------------------------------------------------
add_library(scapms_core STATIC
        utils/Exceptions.h
        utils/Validation.h          utils/Validation.cpp
        utils/IDGenerator.h         utils/IDGenerator.cpp
        utils/FileHandler.h         utils/FileHandler.cpp

        model/PatientRecord.h       model/PatientRecord.cpp
        model/Appointable.h
        model/GeneralPatient.h      model/GeneralPatient.cpp
        model/EmergencyPatient.h    model/EmergencyPatient.cpp
        model/FollowUpPatient.h     model/FollowUpPatient.cpp
        model/PatientFactory.h      model/PatientFactory.cpp
        model/Appointment.h         model/Appointment.cpp
        model/ClinicDatabase.h      model/ClinicDatabase.cpp

        controller/SortEngine.h            controller/SortEngine.cpp
        controller/SearchEngine.h          controller/SearchEngine.cpp
        controller/UndoManager.h           controller/UndoManager.cpp
        controller/RecentCache.h           controller/RecentCache.cpp
        controller/AppointmentController.h controller/AppointmentController.cpp
        controller/ReportGenerator.h       controller/ReportGenerator.cpp
        controller/ClinicManager.h         controller/ClinicManager.cpp
)
target_include_directories(scapms_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# ---------------------------------------------------------------------
# 2. The GUI application.
# ---------------------------------------------------------------------
qt_add_executable(SmartClinicSystem
        main.cpp
        gui/GuiHelpers.h
        gui/MainWindow.h          gui/MainWindow.cpp
        gui/DashboardPanel.h      gui/DashboardPanel.cpp
        gui/PatientPanel.h        gui/PatientPanel.cpp
        gui/PatientDialog.h       gui/PatientDialog.cpp
        gui/PatientTableModel.h   gui/PatientTableModel.cpp
        gui/AppointmentPanel.h    gui/AppointmentPanel.cpp
        gui/QueuePanel.h          gui/QueuePanel.cpp
        gui/SearchPanel.h         gui/SearchPanel.cpp
        gui/AdminPanel.h          gui/AdminPanel.cpp
        gui/ReportsPanel.h        gui/ReportsPanel.cpp
)
target_link_libraries(SmartClinicSystem PRIVATE scapms_core Qt6::Widgets)

# Tell the program where the sample data folder is.
target_compile_definitions(SmartClinicSystem PRIVATE SCAPMS_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

# ---------------------------------------------------------------------
# 3. Console tests for the core logic:  ./scapms_tests
# ---------------------------------------------------------------------
add_executable(scapms_tests tests/core_tests.cpp)
target_link_libraries(scapms_tests PRIVATE scapms_core)
