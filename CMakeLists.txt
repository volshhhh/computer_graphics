cmake_minimum_required(VERSION 3.15)
project(ColorModelsApp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(include)

add_executable(ColorModelsApp
    src/main.cpp
    src/ColorConverter.cpp
)

target_link_libraries(ColorModelsApp ${OpenCV_LIBS})