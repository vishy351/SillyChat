cmake_minimum_required(VERSION 3.20)

project(SillyChat
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Widgets
    Network
)

add_executable(SillyChat
    src/main.cpp

    src/MainWindow.cpp
    src/MainWindow.h

    src/ChatPage.cpp
    src/ChatPage.h

    src/CharactersPage.cpp
    src/CharactersPage.h

    src/SettingsPage.cpp
    src/SettingsPage.h

    src/KoboldClient.cpp
    src/KoboldClient.h

    src/GenerationSettings.cpp
    src/GenerationSettings.h

    src/Character.cpp
    src/Character.h

    src/CharacterLoader.cpp
    src/CharacterLoader.h
)

target_link_libraries(SillyChat PRIVATE
    Qt6::Widgets
    Qt6::Network
)

