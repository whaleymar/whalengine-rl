cmake_minimum_required(VERSION 3.1.2)

project(crunch VERSION 0.12 LANGUAGES C CXX)

find_package(ZLIB REQUIRED)

add_executable(crunch 
    crunch/main.cpp
    
    crunch/third_party/lodepng.cpp
    crunch/third_party/MaxRectsBinPack.cpp
    crunch/third_party/Rect.cpp
    crunch/third_party/aseprite.cpp
    
    crunch/binary.cpp
    crunch/bitmap.cpp
    crunch/cli.cpp
    crunch/hash.cpp
    crunch/options.cpp
    crunch/packer.cpp
    )

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
target_compile_features(crunch PUBLIC cxx_std_20)
if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g") 
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2") 
endif()
target_link_libraries(crunch PRIVATE ZLIB::ZLIB)
