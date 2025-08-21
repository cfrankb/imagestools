cmake_minimum_required(VERSION 3.16)
project(ImageResizerApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

qt6_standard_project_setup()

set(SOURCES
    main.cpp
    ImageResizerApp.cpp
)

set(HEADERS
    ImageResizerApp.h
)

qt6_add_executable(ImageResizerApp ${SOURCES} ${HEADERS})

target_link_libraries(ImageResizerApp Qt6::Core Qt6::Widgets)

# Enable automatic MOC processing
set_target_properties(ImageResizerApp PROPERTIES
    AUTOMOC ON
)