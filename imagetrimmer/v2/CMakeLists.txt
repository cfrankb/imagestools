cmake_minimum_required(VERSION 3.16)
project(ImageProcessorZip VERSION 1.0 LANGUAGES CXX)

# Enable automatic processing of Qt UI, MOC, and resource files


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
#find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# QuaZip (on Arch: `pacman -S quazip-qt6`)
#find_package(QuaZipQt6 REQUIRED)

# ---- QuaZip ----
find_package(QuaZip-Qt5 REQUIRED)

# Source files
set(SOURCES
    main.cpp
    mainwindow.cpp
    imageprocessor.cpp
)

set(HEADERS
    mainwindow.h
    imageprocessor.h
)

set(UI
    mainwindow.ui
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
qt5_wrap_ui(UI_HEADERS ${UI})

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt5::Widgets
        QuaZip::QuaZip
)


install(TARGETS ${PROJECT_NAME} DESTINATION bin)
