cmake_minimum_required(VERSION 3.10)
# Change Project Name
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
)

set(HEADERS
    MainWindow.h
    WebSocketClient.h
    DataStructures.h
    MessageBubble.h
)

# Change Executable Name here too
add_executable(NoveoDesktop ${SOURCES} ${HEADERS})
target_link_libraries(NoveoDesktop PRIVATE Qt5::Widgets Qt5::Network Qt5::WebSockets Qt5::Gui)
