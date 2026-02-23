cmake_minimum_required(VERSION 3.16)
project(TetrisServer)

set(CMAKE_CXX_STANDARD 17)

add_executable(tetris_server
        server.cpp
)

target_include_directories(tetris_server PRIVATE .)

target_link_libraries(tetris_server
        pthread
)

# Message de confirmation
message(STATUS "TetrisServer configured — binary: tetris_server")
