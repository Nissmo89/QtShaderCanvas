cmake_minimum_required(VERSION 3.16)
project(QtShaderCanvas VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Automatic Qt MOC/UIC/RCC processing
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find required Qt6 packages
find_package(Qt6 COMPONENTS Widgets OpenGL OpenGLWidgets REQUIRED)

# Specify public include directory
include_directories(include)

# Define the shared library target
add_library(QtShaderCanvas SHARED
    include/QtShaderCanvas.h
    src/QtShaderCanvas.cpp
)

# Add include interfaces
target_include_directories(QtShaderCanvas PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link library to Qt6 dependencies
target_link_libraries(QtShaderCanvas PUBLIC
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
)

# Setup library versioning
set_target_properties(QtShaderCanvas PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)

# QCodeEngine configuration
set(QCodeEngine_C_BUILD_TESTS OFF CACHE INTERNAL "")
add_subdirectory(QCodeEngine-C/QCodeEngine_C)

# Example subproject configuration
option(BUILD_EXAMPLES "Build example applications" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
