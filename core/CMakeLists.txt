cmake_minimum_required(VERSION 3.16)
project(engine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11) # required by raylib

if (EMSCRIPTEN)
    set(CFLAGS "$CFLAGS -D__GLIBC_USE\(...\)=0")
elseif (WIN32)
    set(CMAKE_CXX_COMPILER "clang-cl.exe") # uses clang w/ MSVC compiler
elseif(UNIX)
    set(CMAKE_CXX_COMPILER "clang++")
endif()

set(CMAKE_GENERATOR_PLATFORM "x64")

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_DEBUG_POSTFIX d)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SOURCES_BASE ${PROJECT_SOURCE_DIR}/../src/*.hpp ${PROJECT_SOURCE_DIR}/../src/*.h)

add_executable(${PROJECT_NAME} ${SOURCES_BASE} "Engine.hpp" "main.cpp")
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

# target_link_libraries(${PROJECT_NAME} "${PROJECT_SOURCE_DIR}/../build/libengined.a")
set(FMOD_DIR "${PROJECT_SOURCE_DIR}/../lib/fmod")

target_include_directories(${PROJECT_NAME} 
    PUBLIC "${PROJECT_SOURCE_DIR}/../src"
    PUBLIC "${PROJECT_SOURCE_DIR}/../lib/RapidXML"
    PUBLIC "${PROJECT_SOURCE_DIR}/../lib/json"
    PUBLIC "${PROJECT_SOURCE_DIR}/../lib/reflect-cpp/include"
    PUBLIC "${FMOD_DIR}/api/core/inc"
    PUBLIC "${FMOD_DIR}/api/studio/inc"
    PUBLIC "${PROJECT_SOURCE_DIR}/../lib/magnum-singles"
)

include_directories("../lib")

target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:${-g}>")
target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:RELEASE>:${-O2}>")

if (WIN32)
    target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/../lib/raylib/src")
    target_link_libraries(${PROJECT_NAME} "${PROJECT_SOURCE_DIR}/../lib/raylib.lib")
elseif(UNIX)
    find_package(raylib 3.0 REQUIRED) # Requires at least version 3.0
    target_link_libraries(${PROJECT_NAME} raylib)

endif()

