cmake_minimum_required(VERSION 3.16)
project(ImageProcessingApp)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find GTKmm
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)
pkg_check_modules(OPENCV REQUIRED opencv4)

# Include directories  
include_directories(${GTKMM_INCLUDE_DIRS})
include_directories(${OPENCV_INCLUDE_DIRS})

# Add executable
add_executable(ImageProcessingApp
    main.cpp
    mainwindow.cpp
)

# Link libraries
target_link_libraries(ImageProcessingApp
    ${GTKMM_LIBRARIES}
    ${OPENCV_LIBRARIES}
)

# Add compiler flags
target_compile_options(ImageProcessingApp PRIVATE ${GTKMM_CFLAGS_OTHER})