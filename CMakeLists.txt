cmake_minimum_required(VERSION 3.10)
project(SynapseVisionLab)

set(CMAKE_CXX_STANDARD 17)

# Find Qt5 Widgets
find_package(Qt5 COMPONENTS Widgets REQUIRED)

# Add your source file
add_executable(SynapseVisionLab main.cpp)

# Link Qt5 Widgets
target_link_libraries(SynapseVisionLab Qt5::Widgets)
