cmake_minimum_required(VERSION 3.16)
project(TetrisClient)

set(CMAKE_CXX_STANDARD 17)

find_package(raylib REQUIRED)

add_executable(tetris
        main.cpp
        network.cpp
)

target_include_directories(tetris PRIVATE .)

target_link_libraries(tetris
        raylib
        pthread
)

# Copier le dossier sounds après la compilation
add_custom_command(TARGET tetris POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/sounds
        ${CMAKE_CURRENT_BINARY_DIR}/sounds
        COMMENT "Copying sounds directory to build folder"
)