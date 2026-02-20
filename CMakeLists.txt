cmake_minimum_required(VERSION 3.16)
project(SynapseVisionLab)

# Force Qt 5 paths
set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt@5" ${CMAKE_PREFIX_PATH})
include_directories(BEFORE "/opt/homebrew/opt/qt@5/include")
link_directories(BEFORE "/opt/homebrew/opt/qt@5/lib")

# Hide Qt 6 (if any remains)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -idirafter /opt/homebrew/include")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets Charts PrintSupport)

add_executable(SynapseVisionLab
    src/main.cpp
    src/MainWindow/MainWindow.cpp
    src/Visualization/EEGChartView.cpp
    src/Visualization/qcustomplot.cpp
    src/DataModels/EEGData.cpp
    src/FileHandlers/EEGFileHandler.cpp
    src/NotchPreviewDialog/NotchPreviewDialog.cpp 
)

target_include_directories(SynapseVisionLab PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Visualization
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DataModels
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileHandlers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/NotchPreviewDialog
    "/opt/homebrew/include"
)

# Link everything
target_link_libraries(SynapseVisionLab
    Qt5::Widgets
    Qt5::Charts
    Qt5::PrintSupport
    "/opt/homebrew/lib/libfftw3.dylib"
)

if(APPLE)
    set_target_properties(SynapseVisionLab PROPERTIES
        MACOSX_BUNDLE YES
    )
endif()