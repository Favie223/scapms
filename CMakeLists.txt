
cmake_minimum_required(VERSION 3.16)

project(SmartClinicSystem VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(SmartClinicSystem
        main.cpp
)

target_link_libraries(SmartClinicSystem
        PRIVATE Qt6::Widgets
)
