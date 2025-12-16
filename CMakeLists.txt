cmake_minimum_required(VERSION 3.16)
project(SynapseVisionLab)

set(CMAKE_CXX_STANDARD 17)

# Enable AUTOMOC so Qt's meta-object compiler runs automatically
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Widgets Charts REQUIRED)

# List all source files
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
    SignalProcessor.h  # Header-only, but still needs to be tracked
)

# Include current directory for headers
target_include_directories(SynapseVisionLab PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(SynapseVisionLab
    Qt5::Widgets
    Qt5::Charts
    Qt5::Core
)

# On Windows, set the runtime path for Qt DLLs
if(WIN32)
    set_target_properties(SynapseVisionLab PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    # Copy Qt DLLs to build directory (optional)
    get_target_property(Qt5Widgets_location Qt5::Widgets LOCATION)
    get_filename_component(Qt5Widgets_dir ${Qt5Widgets_location} DIRECTORY)
    add_custom_command(TARGET SynapseVisionLab POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${Qt5Widgets_dir}/Qt5Widgets.dll"
            "${Qt5Widgets_dir}/Qt5Core.dll"
            "${Qt5Widgets_dir}/Qt5Gui.dll"
            "${Qt5Widgets_dir}/Qt5Charts.dll"
            $<TARGET_FILE_DIR:SynapseVisionLab>
    )
endif()