cmake_minimum_required(VERSION 3.16)
project(ZipImageViewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
# adjust this if your minizip target or name differs:
find_library(MINIZIP_LIB minizip) # fallback; you may set manually
find_package(ZLIB REQUIRED)

add_executable(${PROJECT_NAME}
    main.cpp
    mainwindow.h mainwindow.cpp
    ziphandler.h ziphandler.cpp
    imageviewer.h imageviewer.cpp
    thumbnailgrid.h thumbnailgrid.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${ZLIB_INCLUDE_DIRS})
if(MINIZIP_LIB)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets ${MINIZIP_LIB} ZLIB::ZLIB)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets ZLIB::ZLIB)
    message(WARNING "minizip library not found by find_library; link manually if build fails.")
endif()
