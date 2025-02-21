#pragma once

#include <string>
#include "Expected.h"
#include "raylib.h"

namespace whal {

/*
 * For now, this converts GLSL 3.3.0 shaders into WebGL for web builds, and does nothing otherwise.
 * Eventually I want it to work on a gdshader-like language.
 * It can also output metadata about the shader, like which global uniforms it needs (e.g. Time)
 */
class ShaderTranspiler {
public:
    ShaderTranspiler();

    Expected<rl::Shader> loadAndCompile(const char* vertexShaderPath, const char* fragmentShaderCode);
    Expected<rl::Shader> compile(const std::string& vertexShaderCode, const std::string& fragmentShaderCode);

private:
    std::string preprocess(const std::string& code) const;
};

}  // namespace whal
