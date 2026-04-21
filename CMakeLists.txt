cmake_minimum_required(VERSION 3.10)
project(NoveoDesktop LANGUAGES CXX) 

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets Network WebSockets Gui Multimedia MultimediaWidgets)

set(SOURCES
    main.cpp
    MainWindow.cpp
    WebSocketClient.cpp
    RestClient.cpp
    SessionStore.cpp
    UpdaterService.cpp
    VoiceAudioBridge.cpp
    MessageItemWidget.cpp
    MediaViewerDialog.cpp
    SettingsDialog.cpp
    ChatSettingsDialog.cpp
)

if(WIN32)
    list(APPEND SOURCES version.rc)
endif()

set(HEADERS
    AppConfig.h
    MainWindow.h
    WebSocketClient.h
    DataStructures.h
    RestClient.h
    SessionStore.h
    UpdaterService.h
    VoiceAudioBridge.h
    MessageItemWidget.h
    MediaViewerDialog.h
    SettingsDialog.h
    ChatSettingsDialog.h
)

# Added WIN32 here to hide the console window
if(WIN32)
    add_executable(NoveoDesktop WIN32 ${SOURCES} ${HEADERS})
else()
    add_executable(NoveoDesktop ${SOURCES} ${HEADERS})
endif()

target_link_libraries(NoveoDesktop PRIVATE Qt5::Widgets Qt5::Network Qt5::WebSockets Qt5::Gui Qt5::Multimedia Qt5::MultimediaWidgets)


