cmake_minimum_required(VERSION 3.16)
project(engineWrapper CXX)

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

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SOURCES_BASE ${PROJECT_SOURCE_DIR}/../src/*.hpp ${PROJECT_SOURCE_DIR}/../src/*.h)

# compiling this on its own, can assume we're dynamic linking the game
add_compile_definitions(DYNLIB)

add_executable(${PROJECT_NAME} ${SOURCES_BASE} "Engine.hpp" "main.cpp")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

set(FMOD_DIR "${PROJECT_SOURCE_DIR}/../lib/fmod")

target_include_directories(${PROJECT_NAME} 
    PUBLIC "${PROJECT_SOURCE_DIR}/../src"
    PUBLIC "${PROJECT_SOURCE_DIR}/../lib/json"
    PUBLIC "${FMOD_DIR}/api/core/inc"
    PUBLIC "${FMOD_DIR}/api/studio/inc"
    PUBLIC "${PROJECT_SOURCE_DIR}/../lib/magnum-singles"
    PUBLIC "${PROJECT_SOURCE_DIR}/../lib/raylib/src"
)

include_directories("../lib")

target_compile_options(${PROJECT_NAME} PRIVATE "-g")

if (WIN32)
    target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/../lib/raylib/src")
    target_link_libraries(${PROJECT_NAME} "${PROJECT_SOURCE_DIR}/bin/windows/raylib.lib") 
elseif(UNIX)
    # need to dynamically link so there's only one OpenGL context
    target_link_libraries(${PROJECT_NAME} "${PROJECT_SOURCE_DIR}/bin/linux/libraylib.so")

    # Hard-code the library path into the executable
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
    set(CMAKE_BUILD_RPATH "$ORIGIN")
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
endif()

