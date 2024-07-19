# Installation Requirements (Windows and Linux)
- CMake 
- Clang (any version that supports C++20)
- Ninja

# Building for Linux
In addition to the above requirements, the current configuration requires Raylib 3.0+ to be installed via your package manager.

```
git clone https://github.com/whaleymar/whalengine-rl.git
cd whalengine-rl 
git submodule update --init --recursive
make
```

# Building for Windows
Requires Microsoft Visual C++ (MSVC) compiler. This project wasn't built for Visual Studio and I have no idea how that IDE works, so instead you have to open `x64 Native Tools Command Prompt` (find using Windows search) to compile. Confirm you have a working MSVC compiler by typing `cl`, which should list information about the compiler. Next, confirm you have clang by running `clang-cl -v` (should see similar output). 

*From here you have a couple options:*

## VSCode 
From the `x64 Native Tools Command Prompt` type `code .` and hit enter. This should open VSCode. Now, open the project. Make sure you have the following extensions:
1. C/C++ IntelliSense, debugging, and code browing (Microsoft)
2. CMake (twxs)
3. CMake Language Support (Jose Torres)
4. CMake Tools (Microsoft)

(I have no idea if some of the CMake extensions are redundant).

Hit `ctrl+shift+p` to open the command listing, search CMake, and click `CMake: Build`

## CMake (from command line)
`cd` to the project directory and run `make`
