cmake_minimum_required(VERSION 3.20)

project(SillyChat
    VERSION 0.1.0
    LANGUAGES C CXX
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

    third_party/md4c/md4c.c
    third_party/md4c/md4c.h
    third_party/md4c/md4c-html.c
    third_party/md4c/md4c-html.h
    third_party/md4c/entity.c
    third_party/md4c/entity.h
)

set_source_files_properties(
    third_party/md4c/md4c.c
    third_party/md4c/md4c-html.c
    third_party/md4c/entity.c
    PROPERTIES
    LANGUAGE C
)

target_link_libraries(SillyChat PRIVATE
    Qt6::Widgets
    Qt6::Network
)

