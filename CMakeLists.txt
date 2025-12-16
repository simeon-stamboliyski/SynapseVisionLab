cmake_minimum_required(VERSION 3.16)
project(SynapseVisionLab)

set(CMAKE_CXX_STANDARD 17)

# Enable AUTOMOC so Qt's meta-object compiler runs automatically
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Widgets Charts REQUIRED)

add_executable(SynapseVisionLab
    src/main.cpp
    src/MainWindow/MainWindow.cpp
    src/Visualization/EEGChartView.cpp
    src/DataModels/EEGData.cpp
    src/FileHandlers/EEGFileHandler.cpp
    src/MainWindow/MainWindow.h
    src/Visualization/EEGChartView.h
    src/DataModels/EEGData.h
    src/FileHandlers/EEGFileHandler.h
    src/Utils/SignalProcessor.h
)

target_link_libraries(SynapseVisionLab
    Qt5::Widgets
    Qt5::Charts
    Qt5::Core
)

# CRITICAL: Add ALL source directories to include path
target_include_directories(SynapseVisionLab PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Visualization
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DataModels
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileHandlers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils
    ${Qt5Charts_INCLUDE_DIRS}
)