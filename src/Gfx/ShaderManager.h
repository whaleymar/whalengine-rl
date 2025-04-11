#pragma once

#include "Expected.h"
#include "Util/Singleton.h"

namespace rl {
typedef struct Shader Shader;
}

namespace whal {

class Shader;

class ShaderMgr {
    SINGLETON(ShaderMgr)
public:
    static Expected<void> loadShaders();
    static void unloadShaders();
    static void reloadShaders();

    // query string is the file name (without extension)
    static Shader& get(const std::string& name);

private:
    static Expected<void> loadShaderDir(const char* path);
};

}  // namespace whal
