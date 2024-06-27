cmake_minimum_required(VERSION 3.16)
project(engine CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11) # required by raylib

set(CMAKE_CXX_COMPILER "clang-cl.exe") # uses clang w/ MSVC compiler

set(CMAKE_GENERATOR_PLATFORM "x64")

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_DEBUG_POSTFIX d)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# find_package(raylib 3.0 REQUIRED) # Requires at least version 3.0

file(GLOB_RECURSE SOURCES_BASE ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/src/*.c ${PROJECT_SOURCE_DIR}/src/*.hpp ${PROJECT_SOURCE_DIR}/src/*.h ${PROJECT_SOURCE_DIR}/lib/whalECS/src/*)

add_executable(${PROJECT_NAME} ${SOURCES_BASE})
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

set(FMOD_DIR "${PROJECT_SOURCE_DIR}/lib/fmod")

target_include_directories(${PROJECT_NAME} 
    PUBLIC src
    PUBLIC "${PROJECT_SOURCE_DIR}/lib/RapidXML"
    PUBLIC "${PROJECT_SOURCE_DIR}/lib/json"
    PUBLIC "${FMOD_DIR}/api/core/inc"
    PUBLIC "${FMOD_DIR}/api/studio/inc"
    PUBLIC "${PROJECT_SOURCE_DIR}/lib/magnum-singles"
    PUBLIC "${PROJECT_SOURCE_DIR}/lib/raylib/src"
)

include_directories(lib)

target_link_libraries(${PROJECT_NAME} "${PROJECT_SOURCE_DIR}/lib/fmod/api/core/lib/x86_64/fmod_vc.lib")
target_link_libraries(${PROJECT_NAME} "${PROJECT_SOURCE_DIR}/lib/fmod/api/core/lib/x86_64/fmodL_vc.lib")
target_link_libraries(${PROJECT_NAME} "${PROJECT_SOURCE_DIR}/lib/raylib.lib")