cmake_minimum_required(VERSION 3.10)
project(NoveoDesktop LANGUAGES CXX) 

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets Network WebSockets Gui)

set(SOURCES
    main.cpp
    MainWindow.cpp
    WebSocketClient.cpp
    MessageBubble.cpp
    version.rc
)

set(HEADERS
    MainWindow.h
    WebSocketClient.h
    DataStructures.h
    MessageBubble.h
)

# Added WIN32 here to hide the console window
add_executable(NoveoDesktop WIN32 ${SOURCES} ${HEADERS})

target_link_libraries(NoveoDesktop PRIVATE Qt5::Widgets Qt5::Network Qt5::WebSockets Qt5::Gui)
