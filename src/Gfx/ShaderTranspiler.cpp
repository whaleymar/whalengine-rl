#include "ShaderTranspiler.h"

#include <sstream>
#include <vector>
#include "Util/FileUtils.h"
#include "Util/Print.h"
#include "Util/String.h"
#include "Util/Types.h"
#include "raylib.h"

namespace whal {

ShaderTranspiler::ShaderTranspiler() {}

bool isValidGLSLIdentifierChar(char c, bool firstChar) {
    if (firstChar) {
        return std::isalpha(c) || c == '_';  // Must start with a letter or underscore
    }
    return std::isalnum(c) || c == '_';  // Can contain letters, digits, or underscore
}

#ifdef __EMSCRIPTEN__
std::string toWebGl(const std::string& code, bool isFragment) {
    std::string layoutPrefix = "layout(location";
    bool isMrt = isFragment && contains(code, layoutPrefix);
    struct MRTEntry {
        std::string ix;
        std::string name;
    };
    MRTEntry mrtEntryTable[4];
    s32 mrtCount = 0;
    if (isMrt) {
        u64 pos = 0;
        auto prefixLen = layoutPrefix.size();
        // looks like `layout(location = 0) out vec4 FragColor;`
        while ((pos = code.find(layoutPrefix, pos)) != std::string::npos) {
            if (mrtCount > 3) {
                print("More than 4 render targets unsupported on WebGL. Ignoring the remainder");
                break;
            }
            auto nextCharIx = pos + prefixLen;
            u64 mrtIxLoc = code.find_first_of("0123", nextCharIx);
            u64 nameLoc = code.find("vec4 ", mrtIxLoc) + std::string("vec4 ").size();
            u64 eolLoc = code.find(";", nameLoc);
            std::string name = code.substr(nameLoc, eolLoc - nameLoc);
            mrtEntryTable[mrtCount] = MRTEntry{code.substr(mrtIxLoc, 1), name};
            mrtCount++;
            pos = eolLoc + 1;
        }
    }

    std::istringstream stream(code);
    std::stringstream newCode;
    std::string fragOutVar;
    std::string line;
    while (std::getline(stream, line)) {
        line = strip(line);

        if (line.empty() || startsWith(line, "//")) {
            continue;
        } else if (startsWith(line, "#version")) {
            newCode << "#version 100\n";
            if (isMrt) {
                newCode << "#extension GL_EXT_draw_buffers : require\n";
            }
            newCode << "precision mediump float;\n";
            newCode << "#define PLATFORM_WEB\n";
        } else if (startsWith(line, "in ")) {
            if (isFragment) {
                newCode << line.replace(0, 3, "varying ") << "\n";
            } else {
                newCode << line.replace(0, 3, "attribute ") << "\n";
            }
        } else if (!isMrt && isFragment && startsWith(line, "out ")) {
            // Expecting pattern: "out vec4 variableName;"
            std::istringstream token_stream(line);
            std::vector<std::string> tokens;
            std::string token;
            while (token_stream >> token) {
                tokens.push_back(token);
            }
            if (tokens.size() >= 3) {
                fragOutVar = tokens[2];
                fragOutVar.erase(fragOutVar.find(';'));
            }
            continue;
        } else if (!isFragment && startsWith(line, "out ")) {
            newCode << line.replace(0, 4, "varying ") << "\n";
        } else if (isMrt && startsWith(line, "layout(")) {
            continue;
        } else if (contains(line, "texture(")) {
            u64 pos = line.find("texture(");
            line.replace(pos, 8, "texture2D(");
            newCode << line << "\n";
        } else {
            newCode << line << "\n";
        }
    }

    std::string final_code = newCode.str();
    if (isMrt) {
        for (s32 i = 0; i < mrtCount; i++) {
            u64 pos = 0;
            while ((pos = final_code.find(mrtEntryTable[i].name, pos)) != std::string::npos) {
                if (isValidGLSLIdentifierChar(final_code[pos - 1], false)) {
                    // don't greedily replace variables where this is a substring
                    pos += mrtEntryTable[i].name.length();
                    continue;
                }
                std::string ix = mrtEntryTable[i].ix;
                std::string accessor = std::string("gl_FragData[") + ix + std::string("]");
                final_code.replace(pos, mrtEntryTable[i].name.length(), accessor);
                pos += mrtEntryTable[i].name.length();
            }
        }
    } else if (!fragOutVar.empty()) {
        u64 pos;
        while ((pos = final_code.find(fragOutVar)) != std::string::npos) {
            if (isValidGLSLIdentifierChar(final_code[pos - 1], false)) {
                // don't greedily replace variables where this is a substring
                pos += fragOutVar.length();
                continue;
            }
            final_code.replace(pos, fragOutVar.length(), "gl_FragColor");
        }
    }

    return final_code;
}
#endif

Expected<rl::Shader> ShaderTranspiler::loadAndCompile(const char* vertexShaderPath, const char* fragmentShaderPath) {
    Expected<std::string> vertexCode = vertexShaderPath == NULL ? std::string("") : readFile(vertexShaderPath);
    if (!vertexCode.isExpected()) {
        return vertexCode.error();
    }
    Expected<std::string> fragmentCode = fragmentShaderPath == NULL ? std::string("") : readFile(fragmentShaderPath);
    if (!fragmentCode.isExpected()) {
        return fragmentCode.error();
    }

    return compile(vertexCode.value(), fragmentCode.value());
}

Expected<rl::Shader> ShaderTranspiler::compile(const std::string& vertex, const std::string& fragment) {
    bool isDefaultVertex = vertex == "";
    bool isDefaultFragment = fragment == "";
    std::string vertexEdited;
    std::string fragmentEdited;
    if (!isDefaultVertex) {
        vertexEdited = preprocess(vertex);
#ifdef __EMSCRIPTEN__
        vertexEdited = toWebGl(vertexEdited, false);
#endif
    }
    if (!isDefaultFragment) {
        fragmentEdited = preprocess(fragment);
#ifdef __EMSCRIPTEN__
        fragmentEdited = toWebGl(fragmentEdited, true);
#endif
    }

    const char* vertexFinal = isDefaultVertex ? 0 : vertexEdited.c_str();
    const char* fragmentFinal = isDefaultFragment ? 0 : fragmentEdited.c_str();

    rl::Shader shader = rl::LoadShaderFromMemory(vertexFinal, fragmentFinal);
    if (rl::IsShaderValid(shader)) {
        return shader;
    }
    print("Vertex Shader:\n", vertexFinal, "\n\n", "Fragment Shader:\n", fragmentFinal);
    return Error("Invalid shader. Check raylib logging for details.");
}

std::string ShaderTranspiler::preprocess(const std::string& code) const {
    return code;  // stubbed
}

}  // namespace whal
