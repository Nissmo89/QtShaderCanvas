cmake_minimum_required(VERSION 3.16)
project(QtShaderCanvasExample LANGUAGES CXX)

# Find required Qt6 packages
find_package(Qt6 COMPONENTS Widgets REQUIRED)

# Define target executable
add_executable(QtShaderCanvasExample
    main.cpp
    MainWindow.h
    MainWindow.cpp
    resources.qrc
)

# Link executable to our QtShaderCanvas library
target_link_libraries(QtShaderCanvasExample PRIVATE
    QtShaderCanvas
    Qt6::Widgets
)

# Define shader checker utility executable
add_executable(QtShaderCanvasShaderChecker
    shader_checker.cpp
)

target_link_libraries(QtShaderCanvasShaderChecker PRIVATE
    QtShaderCanvas
    Qt6::Widgets
)

