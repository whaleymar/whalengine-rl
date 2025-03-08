#pragma once

#include <sstream>
#include <string>
#include <vector>

#include "Expected.h"
#include "Util/Types.h"
#include "raylib.h"

namespace whal {

struct ShaderTracker {
    std::stringstream code;
    bool isDefault = true;

    void clear() {
        code.str("");
        code.clear();
        isDefault = true;
    }
};

struct CompileState {
    std::vector<std::string> input;
    std::vector<std::string> varying;
    std::vector<std::string> globalUniforms;
    // std::unordered_map<string, string> allUniforms;   // name: definition
    // std::unordered_map<string, string> allConstants;  // name: definition
    // std::unordered_map<string, string> allStructs;    // name: definition
    // std::unordered_map<string, string> allFuncs;      // name: definition

    std::string outFrag;
    ShaderTracker vert;
    ShaderTracker frag;
    s32 currentLineNo = 1;
    bool isMrt = false;  // Multiple Render Targets flag

    Expected<void> parse(const std::string& unifiedShaderCode);
    Expected<void> parseGlobalLine(const std::string& line);
    Expected<void> parseMacro(const std::string& line);
    Expected<void> parseInvar(const std::string& line);
    Expected<void> parseOutvar(const std::string& line);
    Expected<void> parseVarying(const std::string& line);
    Expected<void> parseUniform(const std::string& line, bool isGlobal = false);
    Expected<void> parseGlobalUniform(const std::string& line);
    Expected<void> parseConstant(const std::string& line);
    Expected<void> parseMutable(const std::string& line);
    Expected<void> parseStruct(const std::string& line);
    Expected<void> parseFunc(const std::string& line);

    std::string getShaderString(bool isVertex) const;

    void clear() {
        input.clear();
        varying.clear();
        globalUniforms.clear();
        outFrag.clear();
        vert.clear();
        frag.clear();
        isMrt = false;
    }
};

struct ShaderMetaData {
    std::vector<std::string> globalUniforms;
};

/*
 * For now, this converts GLSL 3.3.0 shaders into WebGL for web builds, and does nothing otherwise.
 * Eventually I want it to work on a gdshader-like language.
 * It can also output metadata about the shader, like which global uniforms it needs (e.g. Time)
 */
class ShaderTranspiler {
public:
    ShaderTranspiler();

    Expected<rl::Shader> loadAndCompile(const char* vertexShaderPath, const char* fragmentShaderPath);
    Expected<rl::Shader> loadAndCompile(const char* unifiedShaderPath);
    Expected<rl::Shader> compile(const std::string& vertexShaderCode, const std::string& fragmentShaderCode);
    Expected<rl::Shader> compile(const std::string& unifiedShaderCode);
    Expected<std::pair<std::string, std::string>> transpileUnifiedShader(const std::string& unifiedShaderCode);

    // returns metadata from most recent shader compilation
    ShaderMetaData getMetaData() const;

private:
    std::string preprocess(const std::string& code) const;
    CompileState mState;
};

}  // namespace whal
