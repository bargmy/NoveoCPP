cmake_minimum_required(VERSION 3.16)
project(NoveoDesktop LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets Gui WebEngineWidgets)
set(QT_PACKAGE Qt5)

set(SOURCES
    main.cpp
    MainWindow.cpp
)

if(WIN32)
    list(APPEND SOURCES version.rc)
endif()

set(HEADERS
    MainWindow.h
)

if(WIN32)
    add_executable(NoveoDesktop WIN32 ${SOURCES} ${HEADERS})
else()
    add_executable(NoveoDesktop ${SOURCES} ${HEADERS})
endif()

target_link_libraries(NoveoDesktop PRIVATE
    ${QT_PACKAGE}::Widgets
    ${QT_PACKAGE}::Gui
    ${QT_PACKAGE}::WebEngineWidgets
)
