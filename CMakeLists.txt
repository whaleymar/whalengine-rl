cmake_minimum_required(VERSION 3.16)
project(engine CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11) # required by raylib

if (EMSCRIPTEN)
    # set(CMAKE_CXX_COMPILER "/usr/bin/g++")
    set(CFLAGS "$CFLAGS -D__GLIBC_USE\(...\)=0")
else()
# set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_DEBUG_POSTFIX d)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(raylib 3.0 REQUIRED) # Requires at least version 3.0


file(GLOB_RECURSE SOURCES_BASE ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/src/*.c ${PROJECT_SOURCE_DIR}/src/*.hpp ${PROJECT_SOURCE_DIR}/src/*.h ${PROJECT_SOURCE_DIR}/lib/whalECS/src/*)

add_executable(${PROJECT_NAME} ${SOURCES_BASE})
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

if (EMSCRIPTEN)
    set(FMOD_DIR "${PROJECT_SOURCE_DIR}/lib/fmod-web")
else()
    set(FMOD_DIR "${PROJECT_SOURCE_DIR}/lib/fmod-linux")
endif()

target_include_directories(${PROJECT_NAME} 
    PUBLIC src
    PUBLIC "${PROJECT_SOURCE_DIR}/lib/RapidXML"
    PUBLIC "${PROJECT_SOURCE_DIR}/lib/json"
    PUBLIC "${FMOD_DIR}/api/core/inc"
    PUBLIC "${FMOD_DIR}/api/studio/inc"
    PUBLIC "${PROJECT_SOURCE_DIR}/lib/magnum-singles"
)

target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:${-g}>")
target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:RELEASE>:${-O2}>")
target_compile_options(${PROJECT_NAME} PRIVATE -fno-rtti -fno-exceptions)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wdeprecated)
target_compile_options(${PROJECT_NAME} PRIVATE -Wno-unused-parameter -Wno-unused-private-field -Wno-macro-redefined)
target_compile_options(${PROJECT_NAME} PRIVATE -fno-strict-aliasing)
target_compile_options(${PROJECT_NAME} PRIVATE -Wno-invalid-offsetof)

include_directories(lib)
target_precompile_headers(${PROJECT_NAME}
    PUBLIC "${PROJECT_SOURCE_DIR}/lib/whalECS/src/ECS.h"
    PUBLIC "${PROJECT_SOURCE_DIR}/src/Systems/System.h"
    PUBLIC "${PROJECT_SOURCE_DIR}/src/Systems/Event.h"
)

if (EMSCRIPTEN)
    # target_link_libraries(${PROJECT_NAME} "${FMOD_DIR}/api/core/lib/")
    # target_link_libraries(${PROJECT_NAME} "${FMOD_DIR}/api/core/lib/")
    # set(FMOD_JS "${FMOD_DIR}/api/core/lib/fastcomp/wasm/fmod.js")
    # set(FMOD_WASM "${FMOD_DIR}/api/core/lib/fastcomp/wasm/fmod.wasm")
    set(FMOD_JS "${FMOD_DIR}/api/core/lib/fastcomp/asm.js/fmod.js")
    # set(FMOD_JS "${FMOD_DIR}/api/core/lib/upstream/js/fmod.js")
    # set(FMOD_JS "${FMOD_DIR}/api/core/lib/upstream/wasm/fmod.js")
    # set(FMOD_JS2 "${FMOD_DIR}/api/core/lib/fastcomp/asm.js/fmodL.js")
    # set(FMOD_JS2 "${FMOD_DIR}/api/core/lib/fastcomp/asm.js/fmod_reduced.js")
    set(FMOD_BC "${FMOD_DIR}/api/core/lib/fastcomp/bitcode/fmod.bc")
    # set(FMOD_BC "${FMOD_DIR}/api/core/lib/fastcomp/bitcode/fmod_reduced.bc")
    # set(FMOD_BC "${FMOD_DIR}/api/core/lib/fastcomp/bitcode/fmodL.bc")
    # set(FMOD_BC "${FMOD_DIR}/api/core/lib/fastcomp/bitcode/fmod_bindings.bc")
    # target_link_libraries(${PROJECT_NAME} ${FMOD_JS})
    # target_link_libraries(${PROJECT_NAME} ${FMOD_BC})
else()
    target_link_libraries(${PROJECT_NAME} "${FMOD_DIR}/api/core/lib/x86_64/libfmod.so")
    target_link_libraries(${PROJECT_NAME} "${FMOD_DIR}/api/core/lib/x86_64/libfmodL.so")
endif()

target_link_libraries(${PROJECT_NAME} raylib)

# Checks if OSX and links appropriate frameworks (only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()

if (EMSCRIPTEN)
    # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lidbfs.js -s USE_GLFW=3 -s ASYNCIFY --shell-file ${CMAKE_CURRENT_LIST_DIR}/web/minshell.html --preload-file ${CMAKE_CURRENT_LIST_DIR}/resources/@resources/ -s GL_ENABLE_GET_PROC_ADDRESS=1 --js-library ${FMOD_JS} ${FMOD_BC}")
    # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lidbfs.js -s USE_GLFW=3 -s ASYNCIFY --shell-file ${CMAKE_CURRENT_LIST_DIR}/web/minshell.html --preload-file ${CMAKE_CURRENT_LIST_DIR}/resources/@resources/ -s GL_ENABLE_GET_PROC_ADDRESS=1")
    # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lidbfs.js -s USE_GLFW=3 -s ASYNCIFY --shell-file ${CMAKE_CURRENT_LIST_DIR}/web/minshell.html --preload-file ${CMAKE_CURRENT_LIST_DIR}/resources/@resources/ -s GL_ENABLE_GET_PROC_ADDRESS=1 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -o ${FMOD_JS} ${FMOD_BC}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lidbfs.js -s USE_GLFW=3 -s ASYNCIFY --shell-file ${CMAKE_CURRENT_LIST_DIR}/web/minshell.html --preload-file ${CMAKE_CURRENT_LIST_DIR}/resources/@resources/ -s GL_ENABLE_GET_PROC_ADDRESS=1 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1")
    set(CMAKE_EXECUTABLE_SUFFIX ".html") # This line is used to set your executable to build with the emscripten html template so that you can directly open it.
endif ()
