cmake_minimum_required(VERSION 3.15)
project(engine CXX C)

set(CMAKE_DEBUG_POSTFIX d)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(raylib 3.0 REQUIRED) # Requires at least version 3.0

set(CMAKE_C_STANDARD 11) # Requires C11 standard

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SOURCES_BASE ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/src/*.c ${PROJECT_SOURCE_DIR}/src/*.hpp ${PROJECT_SOURCE_DIR}/src/*.h ${PROJECT_SOURCE_DIR}/lib/whalECS/src/*)

add_executable(${PROJECT_NAME} ${SOURCES_BASE})
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

target_include_directories(${PROJECT_NAME} 
    PUBLIC src
    # PUBLIC "${PROJECT_SOURCE_DIR}/lib/whalECS"
    PUBLIC "${PROJECT_SOURCE_DIR}/lib/RapidXML"
    PUBLIC "${PROJECT_SOURCE_DIR}/lib/json"
    # PUBLIC "${PROJECT_SOURCE_DIR}/lib/SDL/include"
    PUBLIC "${SDL2_INCLUDE_DIRS}"
    )

target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:${-g}>")
target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:RELEASE>:${-O2}>")
target_compile_options(${PROJECT_NAME} PRIVATE -fno-rtti -fno-exceptions)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wdeprecated)
target_compile_options(${PROJECT_NAME} PRIVATE -Wno-unused-parameter -Wno-unused-private-field -Wno-macro-redefined)
target_compile_options(${PROJECT_NAME} PRIVATE -fno-strict-aliasing)
target_compile_options(${PROJECT_NAME} PRIVATE -Wno-invalid-offsetof)

include_directories(lib)

target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${SDL2_MIXER_LIBRARIES})
target_link_libraries(${PROJECT_NAME} raylib)

# Checks if OSX and links appropriate frameworks (only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()
