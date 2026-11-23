cmake_minimum_required(VERSION 3.16)
project(HitboxEditor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

qt6_standard_project_setup()

set(SOURCES
    main.cpp
    editor.cpp
    hitbox.cpp
)

set(HEADERS
    editor.h
    hitbox.h
)

qt6_add_executable(HitboxEditor ${SOURCES} ${HEADERS})

target_link_libraries(HitboxEditor PRIVATE Qt6::Core Qt6::Widgets)

# Enable automatic MOC processing
set_target_properties(HitboxEditor PROPERTIES
    AUTOMOC ON
)
