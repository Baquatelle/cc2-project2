cmake_minimum_required(VERSION 3.20)

project(LibraryManagementSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LIBRARY_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

# ---------------------------------------------------------------------------
# Shared warning configuration.
#
# An INTERFACE target so that EVERY target gets the same strictness. Setting
# these PRIVATE on the library alone would leave the demo and the tests --
# where the trickiest code lives -- compiling with default warnings.
# ---------------------------------------------------------------------------
add_library(library_warnings INTERFACE)

if(MSVC)
    target_compile_options(library_warnings INTERFACE
        /W4
        /permissive-
        $<$<BOOL:${LIBRARY_WARNINGS_AS_ERRORS}>:/WX>
    )
else()
    target_compile_options(library_warnings INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        $<$<BOOL:${LIBRARY_WARNINGS_AS_ERRORS}>:-Werror>
    )
endif()

# ---------------------------------------------------------------------------
# Core library
# ---------------------------------------------------------------------------
add_library(library_core
    src/Book.cpp
    src/Member.cpp
    src/InMemoryBookRepository.cpp
    src/Results.cpp
)

target_include_directories(library_core PUBLIC include)
target_compile_features(library_core PUBLIC cxx_std_20)
target_link_libraries(library_core PRIVATE library_warnings)

# ---------------------------------------------------------------------------
# Demo
# ---------------------------------------------------------------------------
add_executable(library_demo app/main.cpp)
target_link_libraries(library_demo PRIVATE library_core library_warnings)
