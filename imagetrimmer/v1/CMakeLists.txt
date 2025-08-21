cmake_minimum_required(VERSION 3.16)
project(ImageTrimApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Qt ----
find_package(Qt5 COMPONENTS Widgets REQUIRED)

# ---- QuaZip ----
find_package(QuaZip-Qt5 REQUIRED)

# ---- Sources ----
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
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

qt5_wrap_ui(UI_HEADERS ${UI})

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${UI_HEADERS}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt5::Widgets
        QuaZip::QuaZip
)

# ---- Install rules ----
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# Optional: Install icons and desktop entry
install(FILES icons/app-icon.png DESTINATION share/icons/hicolor/64x64/apps)
install(FILES ${CMAKE_SOURCE_DIR}/ImageTrimApp.desktop DESTINATION share/applications)

# ---- CPack ArchLinux Package ----
set(CPACK_GENERATOR "TXZ") # .tar.xz by default, Arch expects .pkg.tar.zst
set(CPACK_PACKAGE_NAME "imagetrimapp")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_CONTACT "Your Name <you@example.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Qt image trimming tool with before/after preview")
set(CPACK_PACKAGE_VENDOR "Your Company or Name")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-1-x86_64.pkg.tar.zst")

# Arch-specific metadata
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)

# Ensure compression is zstd for Arch
set(CPACK_ARCHIVE_THREADS 0) # use all CPU cores
set(CPACK_ARCHIVE_FORMAT "zstd")

include(CPack)
