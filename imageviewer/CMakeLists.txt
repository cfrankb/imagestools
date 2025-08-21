cmake_minimum_required(VERSION 3.16)
project(Qt6PNGGridViewer)

set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(Qt6PNGGridViewer main.cpp)
target_link_libraries(Qt6PNGGridViewer Qt6::Widgets)
