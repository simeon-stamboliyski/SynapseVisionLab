cmake_minimum_required(VERSION 3.16)
project(SynapseVisionLab)

set(CMAKE_CXX_STANDARD 17)

# Enable AUTOMOC so Qt's meta-object compiler runs automatically
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Widgets Charts REQUIRED)

add_executable(SynapseVisionLab
    main.cpp
    MainWindow.cpp
    MainWindow.h
)

target_link_libraries(SynapseVisionLab
    Qt5::Widgets
    Qt5::Charts
)
