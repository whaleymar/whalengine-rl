#pragma once

#include "Expected.h"

namespace rl {
typedef struct Shader Shader;
}

namespace whal {

class Shader;

class ShaderMgr {
public:
    static ShaderMgr& instance() {
        static ShaderMgr instance_;
        return instance_;
    }

    static Expected<void> loadShaders();
    static void unloadShaders();
    static void reloadShaders();

    // query string is the file name (without extension)
    static Shader& get(const std::string& name);

private:
    ShaderMgr() = default;
    ShaderMgr(const ShaderMgr&) = delete;
    void operator=(const ShaderMgr&) = delete;

    static Expected<void> loadShaderDir(const char* path);
};

}  // namespace whal
