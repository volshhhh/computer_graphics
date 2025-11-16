cmake_minimum_required(VERSION 3.16)
project(Lab3)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CPP_STANDARD 17)

# Find packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)
pkg_check_modules(OPENCV REQUIRED opencv4)

# Include directories  
include_directories(${GTKMM_INCLUDE_DIRS})
include_directories(${OPENCV_INCLUDE_DIRS})

# Add executable
add_executable(Lab3
    main.cpp
)

# Link libraries
target_link_libraries(Lab3
    ${GTKMM_LIBRARIES}
    ${OPENCV_LIBRARIES}
)

# Add compiler flags
target_compile_options(Lab3 PRIVATE ${GTKMM_CFLAGS_OTHER})