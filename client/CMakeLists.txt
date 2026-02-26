cmake_minimum_required(VERSION 3.16)
project(tetris VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ═══════════════════════════════════════════════════════════════
#  Sources
# ═══════════════════════════════════════════════════════════════
add_executable(tetris main.cpp)
target_include_directories(tetris PRIVATE ${CMAKE_SOURCE_DIR})

# ═══════════════════════════════════════════════════════════════
#  Plateforme cible
# ═══════════════════════════════════════════════════════════════
if(WIN32 OR MINGW OR TOOLCHAIN_WINDOWS)
    message(STATUS "Target: Windows")

    if(NOT DEFINED RAYLIB_WIN_DIR)
        set(RAYLIB_WIN_DIR "${CMAKE_SOURCE_DIR}/../deps/raylib-win")
    endif()

    target_include_directories(tetris PRIVATE "${RAYLIB_WIN_DIR}/include")
    target_link_directories(tetris PRIVATE "${RAYLIB_WIN_DIR}/lib")
    target_link_libraries(tetris raylib winmm gdi32 opengl32 ws2_32)
    target_link_options(tetris PRIVATE -mwindows)
    set(CPACK_GENERATOR "ZIP")

elseif(APPLE)
    message(STATUS "Target: macOS")

    find_package(raylib 5 REQUIRED)
    target_link_libraries(tetris raylib
        "-framework OpenGL" "-framework Cocoa"
        "-framework IOKit" "-framework CoreVideo"
        "-framework CoreAudio" "-framework AudioToolbox")

    set_target_properties(tetris PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Tetris Online"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
    set(CPACK_GENERATOR "DragNDrop")

else()
    message(STATUS "Target: Linux")

    find_package(raylib 5 QUIET)
    if(raylib_FOUND)
        target_link_libraries(tetris raylib)
    else()
        find_library(RAYLIB_LIB NAMES raylib libraylib
            PATHS "${CMAKE_SOURCE_DIR}/../deps/raylib-linux/lib"
                  /usr/local/lib /usr/lib)
        if(NOT RAYLIB_LIB)
            message(FATAL_ERROR "raylib introuvable. Installez-le ou placez-le dans deps/raylib-linux/")
        endif()
        target_include_directories(tetris PRIVATE
            "${CMAKE_SOURCE_DIR}/../deps/raylib-linux/include")
        target_link_libraries(tetris ${RAYLIB_LIB})
    endif()

    target_link_libraries(tetris dl pthread m)

    if(STATIC_BUILD)
        target_link_options(tetris PRIVATE -static-libgcc -static-libstdc++)
        message(STATUS "Static build enabled")
    endif()

    set(CPACK_GENERATOR "TGZ")
endif()

# ═══════════════════════════════════════════════════════════════
#  Optimisations Release
# ═══════════════════════════════════════════════════════════════
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(tetris PRIVATE -O2 -DNDEBUG)
    if(NOT APPLE)
        target_link_options(tetris PRIVATE -s)
    endif()
endif()

# ═══════════════════════════════════════════════════════════════
#  Installation & packaging (cpack)
# ═══════════════════════════════════════════════════════════════
install(TARGETS tetris DESTINATION .)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/sounds" DESTINATION .)

set(CPACK_PACKAGE_NAME "TetrisOnline")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Tetris Online - multijoueur")
include(CPack)
