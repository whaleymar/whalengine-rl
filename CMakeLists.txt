cmake_minimum_required(VERSION 3.16)
project(engine CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11) # required by raylib

if (WIN32)
    set(CMAKE_CXX_COMPILER "clang-cl.exe") # uses clang w/ MSVC compiler
elseif(UNIX)
    set(CMAKE_CXX_COMPILER "clang++")
endif()

set(CMAKE_GENERATOR_PLATFORM "x64")

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# for measuring compile times per file (do with single threaded compile)
# set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_DEBUG_POSTFIX d)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SOURCES_BASE ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/src/*.c ${PROJECT_SOURCE_DIR}/src/*.hpp ${PROJECT_SOURCE_DIR}/src/*.h ${PROJECT_SOURCE_DIR}/lib/whalECS/src/*)

add_executable(${PROJECT_NAME} ${SOURCES_BASE})
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

set(FMOD_DIR "${PROJECT_SOURCE_DIR}/lib/fmod")

target_include_directories(${PROJECT_NAME} 
    PUBLIC src
    PUBLIC "${PROJECT_SOURCE_DIR}/lib/RapidXML"
    PUBLIC "${PROJECT_SOURCE_DIR}/lib/json"
    PUBLIC "${FMOD_DIR}/api/core/inc"
    PUBLIC "${FMOD_DIR}/api/studio/inc"
    PUBLIC "${PROJECT_SOURCE_DIR}/lib/magnum-singles"
    # PUBLIC "${PROJECT_SOURCE_DIR}/lib/raylib/src"
)

include_directories(lib)

target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:${-g}>")
target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:RELEASE>:${-O2}>")

if (WIN32)
    target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/lib/raylib/src")

    target_link_libraries(${PROJECT_NAME} "${PROJECT_SOURCE_DIR}/lib/fmod/api/core/lib/x86_64/fmod_vc.lib")
    target_link_libraries(${PROJECT_NAME} "${PROJECT_SOURCE_DIR}/lib/fmod/api/core/lib/x86_64/fmodL_vc.lib")
    target_link_libraries(${PROJECT_NAME} "${PROJECT_SOURCE_DIR}/lib/raylib.lib")
elseif(UNIX)
    find_package(raylib 3.0 REQUIRED) # Requires at least version 3.0

    # these don't seem to work with clang-cl 
    target_compile_options(${PROJECT_NAME} PRIVATE -fno-rtti -fno-exceptions)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wdeprecated)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-unused-parameter -Wno-unused-private-field -Wno-macro-redefined)
    target_compile_options(${PROJECT_NAME} PRIVATE -fno-strict-aliasing)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-invalid-offsetof)

    target_precompile_headers(${PROJECT_NAME}
        PUBLIC "${PROJECT_SOURCE_DIR}/lib/whalECS/src/ECS.h"
        PUBLIC "${PROJECT_SOURCE_DIR}/src/Sys/System.h"
        PUBLIC "${PROJECT_SOURCE_DIR}/src/Sys/Event.h"
    )

    target_link_libraries(${PROJECT_NAME} "${PROJECT_SOURCE_DIR}/lib/fmod/api/core/lib/x86_64/libfmod.so")
    target_link_libraries(${PROJECT_NAME} "${PROJECT_SOURCE_DIR}/lib/fmod/api/core/lib/x86_64/libfmodL.so")
    target_link_libraries(${PROJECT_NAME} raylib)

endif()
