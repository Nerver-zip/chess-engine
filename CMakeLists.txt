cmake_minimum_required(VERSION 3.25)

set(CAPY_VERSION "0.1.0" CACHE STRING "capy chess engine version")
if(NOT CAPY_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "CAPY_VERSION must use MAJOR.MINOR.PATCH, got '${CAPY_VERSION}'")
endif()

project(
    capy_chess_engine
    VERSION ${CAPY_VERSION}
    DESCRIPTION "Capy Chess Engine and Raylib GUI"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)
include(Sanitizers)

option(CAPY_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(CAPY_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(BUILD_TESTING "Build test suite" ON)
option(BUILD_GUI "Build Raylib graphical interface executable" ON)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/capy_version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/capy_version.h"
    @ONLY
)

# -----------------------------------------------------------------------------
# Core Engine Library (Headless, no GUI/Raylib dependencies)
# -----------------------------------------------------------------------------
add_library(capy_chess_core STATIC
    src/board/board.cpp
    src/move/move.cpp
    src/move/movegen.cpp
    src/eval/eval.cpp
    src/search/search.cpp
    src/tt/tt.cpp
    src/zobrist/zobrist.cpp
    src/debuglib/debug.cpp
)

target_include_directories(
    capy_chess_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
)

target_compile_features(capy_chess_core PUBLIC cxx_std_23)
capy_chess_set_project_warnings(capy_chess_core ${CAPY_WARNINGS_AS_ERRORS})
if(CAPY_ENABLE_SANITIZERS)
    capy_chess_enable_sanitizers(capy_chess_core)
endif()

# -----------------------------------------------------------------------------
# Main Executable (Chess Engine with Raylib GUI)
# -----------------------------------------------------------------------------
if(BUILD_GUI)
    find_package(raylib QUIET)

    if(NOT raylib_FOUND AND NOT TARGET raylib)
        include(FetchContent)
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(
            raylib
            URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
        )
        FetchContent_MakeAvailable(raylib)
    endif()

    add_executable(chess_engine
        src/gui/gui.cpp
        src/main.cpp
    )

    target_compile_features(chess_engine PRIVATE cxx_std_23)
    target_link_libraries(chess_engine PRIVATE capy_chess_core)

    if(TARGET raylib)
        target_link_libraries(chess_engine PRIVATE raylib)
    else()
        target_link_libraries(chess_engine PRIVATE raylib GL m pthread dl rt X11)
    endif()

    capy_chess_set_project_warnings(chess_engine ${CAPY_WARNINGS_AS_ERRORS})
    if(CAPY_ENABLE_SANITIZERS)
        capy_chess_enable_sanitizers(chess_engine)
    endif()

    install(TARGETS chess_engine RUNTIME DESTINATION bin)
    install(DIRECTORY assets DESTINATION bin)
    install(DIRECTORY assets DESTINATION .)
    install(FILES README.md LICENSE DESTINATION .)
    install(FILES README.md LICENSE DESTINATION share/doc/chess_engine)
endif()

# -----------------------------------------------------------------------------
# Packaging (CPack)
# -----------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME "capy-chess-engine")
set(CPACK_PACKAGE_VENDOR "Nerver-zip")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Capy Chess Engine in C++23 with Raylib GUI")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_GENERATOR "TGZ;ZIP")
include(CPack)

# -----------------------------------------------------------------------------
# Test Suite
# -----------------------------------------------------------------------------
if(BUILD_TESTING)
    include(CTest)
    include(FetchContent)

    set(INSTALL_GTEST OFF CACHE BOOL "Disable GoogleTest installation" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "Disable GoogleMock until mocking is required" FORCE)
    set(gtest_build_tests OFF CACHE BOOL "Disable GoogleTest self-tests" FORCE)

    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/52eb8108c5bdec04579160ae17225d66034bd723.zip
    )
    FetchContent_MakeAvailable(googletest)

    add_subdirectory(tests)
endif()
