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
    EEGChartView.cpp
    EEGChartView.h
    EEGData.cpp
    EEGData.h
    EEGFileHandler.cpp
    EEGFileHandler.h
    SignalProcessor.h
)

target_link_libraries(SynapseVisionLab
    Qt5::Widgets
    Qt5::Charts
    Qt5::Core
)

# Add include directory for Qt Charts
target_include_directories(SynapseVisionLab PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Qt5Charts_INCLUDE_DIRS}
)