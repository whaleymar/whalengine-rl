#include "ShaderTranspiler.h"

#include <cmath>
#include <sstream>
#include <string>
#include <vector>

#include "Util/FileUtils.h"
#include "Util/Print.h"
#include "Util/String.h"
#include "Util/Types.h"
#include "raylib.h"

using std::string;
using std::vector;

namespace whal {

ShaderTranspiler::ShaderTranspiler() {}

bool isValidGLSLIdentifierChar(char c, bool firstChar) {
    if (firstChar) {
        return std::isalpha(c) || c == '_';  // Must start with a letter or underscore
    }
    return std::isalnum(c) || c == '_';  // Can contain letters, digits, or underscore
}

#ifdef __EMSCRIPTEN__
string toWebGl(const string& code, bool isFragment) {
    string layoutPrefix = "layout(location";
    bool isMrt = isFragment && contains(code, layoutPrefix);
    struct MRTEntry {
        string ix;
        string name;
    };
    MRTEntry mrtEntryTable[4];
    s32 mrtCount = 0;
    if (isMrt) {
        u64 pos = 0;
        auto prefixLen = layoutPrefix.size();
        // looks like `layout(location = 0) out vec4 FragColor;`
        while ((pos = code.find(layoutPrefix, pos)) != string::npos) {
            if (mrtCount > 3) {
                print("More than 4 render targets unsupported on WebGL. Ignoring the remainder");
                break;
            }
            auto nextCharIx = pos + prefixLen;
            u64 mrtIxLoc = code.find_first_of("0123", nextCharIx);
            u64 nameLoc = code.find("vec4 ", mrtIxLoc) + string("vec4 ").size();
            u64 eolLoc = code.find(";", nameLoc);
            string name = code.substr(nameLoc, eolLoc - nameLoc);
            mrtEntryTable[mrtCount] = MRTEntry{code.substr(mrtIxLoc, 1), name};
            mrtCount++;
            pos = eolLoc + 1;
        }
    }

    std::istringstream stream(code);
    std::stringstream newCode;
    string fragOutVar;
    string line;
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
            vector<string> tokens;
            string token;
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

    string final_code = newCode.str();
    if (isMrt) {
        for (s32 i = 0; i < mrtCount; i++) {
            u64 pos = 0;
            while ((pos = final_code.find(mrtEntryTable[i].name, pos)) != string::npos) {
                if (isValidGLSLIdentifierChar(final_code[pos - 1], false)) {
                    // don't greedily replace variables where this is a substring
                    pos += mrtEntryTable[i].name.length();
                    continue;
                }
                string ix = mrtEntryTable[i].ix;
                string accessor = string("gl_FragData[") + ix + string("]");
                final_code.replace(pos, mrtEntryTable[i].name.length(), accessor);
                pos += mrtEntryTable[i].name.length();
            }
        }
    } else if (!fragOutVar.empty()) {
        u64 pos;
        while ((pos = final_code.find(fragOutVar)) != string::npos) {
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
    Expected<string> vertexCode = vertexShaderPath == NULL ? string("") : readFile(vertexShaderPath);
    if (!vertexCode.isExpected()) {
        return vertexCode.error();
    }
    Expected<string> fragmentCode = fragmentShaderPath == NULL ? string("") : readFile(fragmentShaderPath);
    if (!fragmentCode.isExpected()) {
        return fragmentCode.error();
    }

    return compile(vertexCode.value(), fragmentCode.value());
}

Expected<rl::Shader> ShaderTranspiler::loadAndCompile(const char* unifiedShaderPath) {
    Expected<string> code = unifiedShaderPath == NULL ? string("") : readFile(unifiedShaderPath);
    if (!code.isExpected()) {
        return code.error();
    }

    auto eShader = compile(code.value());
    if (!eShader.isExpected()) {
        std::stringstream ss;
        ss << "Error compiling `" << unifiedShaderPath << "`:\n" << eShader.error();
        return Error(ss.str());
    } else {
        return eShader;
    }
}

Expected<rl::Shader> ShaderTranspiler::compile(const string& vertCode, const string& fragCode) {
    bool isDefaultVertex = vertCode == "";
    bool isDefaultFragment = fragCode == "";
    string vertexEdited;
    string fragmentEdited;
    if (!isDefaultVertex) {
        vertexEdited = preprocess(vertCode);
#ifdef __EMSCRIPTEN__
        vertexEdited = toWebGl(vertexEdited, false);
#endif
    }
    if (!isDefaultFragment) {
        fragmentEdited = preprocess(fragCode);
#ifdef __EMSCRIPTEN__
        fragmentEdited = toWebGl(fragmentEdited, true);
#endif
    }

    const char* vertexFinal = isDefaultVertex ? 0 : vertexEdited.c_str();
    const char* fragmentFinal = isDefaultFragment ? 0 : fragmentEdited.c_str();

    rl::Shader shader = rl::LoadShaderFromMemory(vertexFinal, fragmentFinal);
    // print(vertexFinal);  // TEMP
    if (rl::IsShaderValid(shader)) {
        return shader;
    }
    print("ERROR COMPILING SHADER!!!");
    print("\nVERTEX SHADER:");
    std::istringstream stream(vertexFinal);
    string line;

    s32 i = 1;
    while (std::getline(stream, line)) {
        print(i++, line);
    }

    print("\nFRAGMENT SHADER");
    stream = std::istringstream(fragmentFinal);
    i = 1;
    while (std::getline(stream, line)) {
        print(i++, line);
    }

    return Error("Invalid shader. Check raylib logging for details.");
}

Expected<rl::Shader> ShaderTranspiler::compile(const string& unifiedCode) {
    Expected<std::pair<string, string>> shaders = transpileUnifiedShader(unifiedCode);
    if (!shaders.isExpected()) {
        return shaders.error();
    }
    return compile(shaders->first, shaders->second);
}

Expected<std::pair<std::string, std::string>> ShaderTranspiler::transpileUnifiedShader(const std::string& unifiedShaderCode) {
    string code = preprocess(unifiedShaderCode);
    mState.clear();
    Expected<void> err = mState.parse(code);
    if (!err.isExpected()) {
        return err.error();
    }

    // RESEARCH: build symbol lists so vertex/fragment shaders don't include structs/functions they don't use
    std::pair<string, string> finalCode = {mState.getShaderString(true), mState.getShaderString(false)};
    return finalCode;
}

// preprocesses everything except for macros
string ShaderTranspiler::preprocess(const string& code) const {
    bool isSeekAccumulate = false;
    bool isAtLineStart = true;
    bool isAddNewLineAtNextNonWhitespace = false;
    u64 seekIx = 0;
    char prev = '\0';

    std::stringstream result;
    for (u64 i = 0; i < code.length(); ++i) {
        char c = code.at(i);
        isAtLineStart = prev == '\n';

        if (isAtLineStart && c == '\n') {
            // skip double newlines
            continue;
        }

        if (prev == ' ' && c == ' ') {
            // skip double spaces
            continue;
        }

        if (seekIx > 0) {
            if (i >= seekIx) {
                seekIx = 0;
            } else {
                if (isSeekAccumulate) {
                    if (isAddNewLineAtNextNonWhitespace) {
                        result << std::endl;
                        isAddNewLineAtNextNonWhitespace = false;
                    }
                    result << c;
                }
                prev = c;
                continue;
            }
        }

        if (isAtLineStart) {
            if (c == '#') {
                seekIx = code.find('\n', i) + 1;
                isSeekAccumulate = true;
            }
        }

        if (c == '/' && code.at(i + 1) == '/') {
            // regular comment, goto next line
            seekIx = code.find('\n', i + 2) + 1;
            isSeekAccumulate = false;
            // add a newline since we're not accumulating and will lose a line break (would break if next line is a macro)
            isAddNewLineAtNextNonWhitespace = true;
            continue;
        }

        if (c == '/' && code.at(i + 1) == '*') {
            seekIx = code.find("*/", i + 2) + 2;
            isSeekAccumulate = false;
            // add a newline since we're not accumulating and will lose a line break (would break if next line is a macro)
            isAddNewLineAtNextNonWhitespace = true;
            continue;
        }

        if (isAddNewLineAtNextNonWhitespace && !std::isspace(c)) {
            result << std::endl;
            isAddNewLineAtNextNonWhitespace = false;
        }
        result << c;

        prev = c;
    }

    return result.str();
}

ShaderMetaData ShaderTranspiler::getMetaData() const {
    return ShaderMetaData{
        .globalUniforms = mState.globalUniforms,
    };
}

static const char* SHADER_TYPENAMES[] = {
    "void",   "int",    "float",  "bool",   "uint",   "double", "vec2",  "vec3",  "vec4",  "bvec2",  "bvec3",  "bvec4",
    "ivec2",  "ivec3",  "ivec4",  "uvec2",  "uvec3",  "uvec4",  "dvec2", "dvec3", "dvec4", "mat2x2", "mat2x3", "mat2x4",
    "mat3x2", "mat3x3", "mat3x4", "mat4x2", "mat4x3", "mat4x4", "mat2",  "mat3",  "mat4",  "struct",
};

Expected<void> CompileState::parseGlobalLine(const string& line) {
    if (line.starts_with('#')) {
        return parseMacro(line);
    } else if (line.starts_with("in ")) {
        return parseInvar(line);
    } else if (line.starts_with("out ")) {
        return parseOutvar(line);
    } else if (line.starts_with("global uniform ")) {
        return parseGlobalUniform(line);
    } else if (line.starts_with("uniform ")) {
        return parseUniform(line);
    } else if (line.starts_with("const ")) {
        return parseConstant(line);
    } else if (line.starts_with("varying")) {
        return parseVarying(line);
    } else {
        for (const char* typeName : SHADER_TYPENAMES) {
            if (line.starts_with(typeName)) {
                return parseMutable(line);
            }
        }
        // ignore line
    }
    return Expected<void>();
}

Expected<void> CompileState::parse(const string& code) {
    u64 len = code.length();
    currentLineNo = 1;

    // state
    enum class Ctx { Global, Func, Struct, Macro };
    bool isSeeking = false;
    Ctx ctx = Ctx::Global;
    std::stringstream buf;
    u64 statementStart = 0;
    char seekTarget;
    s32 seekDepth = 0;
    char seekDepthIncrementor;
    bool hasSeekDepthIncrementor = false;

    for (u64 i = 0; i < len; i++) {
        char c = code.at(i);
        if (isSeeking) {
            buf << c;
            if (seekTarget == c && seekDepth == 0) {
                Expected<void> err;
                string sBuf = strip(buf.str());
                if (ctx == Ctx::Func) {
                    err = parseFunc(sBuf);
                } else if (ctx == Ctx::Struct) {
                    err = parseStruct(sBuf);
                } else if (ctx == Ctx::Macro) {
                    err = parseMacro(sBuf);
                }

                if (!err.isExpected()) {
                    std::stringstream ss;
                    ss << "Error on line " << std::to_string(currentLineNo) << std::endl;
                    ss << err.error();
                    return Error(ss.str());
                }

                isSeeking = false;
                ctx = Ctx::Global;
                buf.str("");  // clear the buffer
                buf.clear();  // clear the flags
                seekDepth = 0;
                seekTarget = '\0';
                seekDepthIncrementor = '\0';
                hasSeekDepthIncrementor = false;
                statementStart = i + 1;
            } else if (hasSeekDepthIncrementor && c == seekDepthIncrementor) {
                seekDepth++;
            } else if (seekTarget == c) {
                seekDepth--;
            }
        } else {
            if (i == statementStart) {
                // we're at the beginning of a new line. Check if it's a function or struct definition
                if (c == '\n' || c == '\r' || c == '\t' || c == ' ') {
                    // handle whitespace by advancing statementStart
                    statementStart++;

                } else if (c == '#') {
                    // macro definition
                    ctx = Ctx::Macro;
                    isSeeking = true;
                    seekTarget = '\n';
                    buf << c;

                } else if (code.substr(i).starts_with("struct ")) {
                    ctx = Ctx::Struct;
                    isSeeking = true;
                    seekTarget = '}';
                    seekDepthIncrementor = '{';
                    hasSeekDepthIncrementor = true;
                    seekDepth = -1;
                    buf << c;

                } else {
                    // check if function definition
                    string remaining = code.substr(i);
                    for (const char* typeName : SHADER_TYPENAMES) {
                        if (remaining.starts_with(typeName)) {
                            u64 openParen = remaining.find('(');
                            u64 closeParen = remaining.find(')');
                            u64 openBrace = remaining.find('{');
                            u64 semiColon = remaining.find(';');
                            if (openParen < closeParen && closeParen < openBrace && (openBrace < semiColon || semiColon == remaining.npos)) {
                                ctx = Ctx::Func;
                                isSeeking = true;
                                seekTarget = '}';
                                seekDepthIncrementor = '{';
                                hasSeekDepthIncrementor = true;
                                seekDepth = -1;
                                buf << c;
                                break;
                            }
                        }
                    }
                }
            } else if (c == ';') {
                // end of a global line
                string line = code.substr(statementStart, i + 1 - statementStart);
                auto err = parseGlobalLine(line);
                if (!err.isExpected()) {
                    std::stringstream ss;
                    ss << "Error on line " << std::to_string(currentLineNo) << std::endl;
                    ss << err.error();
                    return Error(ss.str());
                }
                statementStart = i + 1;
            }
        }

        if (c == '\n') {
            currentLineNo++;
        }
    }

    return Expected<void>();
}

Expected<void> CompileState::parseMacro(const string& line) {
    static constexpr std::string multipleRenderTargetMacro = "#pragma mrt ";
    constexpr u64 mrtPrefixLen = multipleRenderTargetMacro.size();
    if (line.starts_with(multipleRenderTargetMacro)) {
        if (currentLineNo != 1) {
            // If i don't enfore this, parsing out vars might get messed up
            return Error("`#pragma mrt` directive must appear on the first line");
        }
        isMrt = true;
        std::string outvarsSingleString = line.substr(mrtPrefixLen);
        strip_inplace(outvarsSingleString);
        std::vector<std::string> outVars = split(outvarsSingleString);
        s32 ix = 0;
        for (const auto& outVar : outVars) {
            frag.code << "layout(location = " << ix++ << ") out vec4 " << outVar << ";" << std::endl;
        }
    } else {
        vert.code << line;
        frag.code << line;
    }
    return Expected<void>();
}

Expected<void> CompileState::parseInvar(const string& line) {
    vert.code << line << std::endl;
    return Expected<void>();
}

Expected<void> CompileState::parseOutvar(const string& line) {
    if (isMrt) {
        return Error("Found a variable marked as `out`, but the `#pragma mrt` macro was used previously. Must use one or the other.");
    }
    frag.code << line << std::endl;
    return Expected<void>();
}

Expected<void> CompileState::parseVarying(const string& line) {
    varying.push_back(line);
    vert.code << replace(line, "varying", "out") << std::endl;
    frag.code << replace(line, "varying", "in") << std::endl;
    return Expected<void>();
}

Expected<void> CompileState::parseUniform(const string& line, bool isGlobal) {
    // TODO extension: default values and hints

    string name = strip(splitAndGet(line, ' ', 2));
    if (name.length() == 0) {
        return Error("Error parsing uniform declaration: " + line);
    }
    if (isGlobal) {
        name = name.substr(0, name.length() - 1);  // remove semicolon
        globalUniforms.push_back(name);
    }

    vert.code << line << std::endl;
    frag.code << line << std::endl;
    return Expected<void>();
}

Expected<void> CompileState::parseGlobalUniform(const string& line) {
    constexpr u64 prefixSize = 7;  // len("global ")
    return parseUniform(line.substr(prefixSize), true);
}

Expected<void> CompileState::parseConstant(const string& line) {
    u64 eqIx = line.find('=');
    if (eqIx == line.npos) {
        return Error("Definition of constant must assign a value");
    }

    string name = strip(splitAndGet(strip(line.substr(0, eqIx)), ' ', 2));
    if (name.length() == 0) {
        return Error("Error parsing constant declaration: " + line);
    }
    // allConstants[name] = line;
    vert.code << line << std::endl;
    frag.code << line << std::endl;
    return Expected<void>();
}

Expected<void> CompileState::parseMutable(const string& line) {
    u64 eqIx = line.find('=');
    if (eqIx == line.npos) {
        return Error("Definition of global must assign a value");
    }

    string name = strip(splitAndGet(strip(line.substr(0, eqIx)), ' ', 1));
    if (name.length() == 0) {
        return Error("Error parsing global declaration: " + line);
    }
    // allConstants[name] = line;
    vert.code << line << std::endl;
    frag.code << line << std::endl;
    return Expected<void>();
}

Expected<void> CompileState::parseStruct(const string& code) {
    u64 braceIx = code.find('{');
    if (braceIx == code.npos) {
        return Error("struct definition is missing a '{'");
    }

    string name = splitAndGet(strip(code.substr(0, braceIx)), ' ', 1);
    if (name.length() == 0) {
        return Error("Error parsing struct definition: " + code);
    }
    // allStructs[name] = code;
    vert.code << code << std::endl;
    frag.code << code << std::endl;
    return Expected<void>();
}

Expected<void> CompileState::parseFunc(const string& code) {
    u64 parenIx = code.find('(');
    if (parenIx == code.npos) {
        return Error("Function definition missing a '('");
    }

    string name = splitAndGet(strip(code.substr(0, parenIx)), ' ', 1);
    // allFuncs[name] = code;
    if (name == "vertex") {
        vert.isDefault = false;
        vert.code << replace(code, "vertex()", "main()") << std::endl;
    } else if (name == "fragment") {
        frag.isDefault = false;
        frag.code << replace(code, "fragment()", "main()") << std::endl;
    } else if (name == "main") {
        return Error("Cannot have function named `main`. Use `vertex` or `fragment`");
    } else {
        vert.code << code << std::endl;
        frag.code << code << std::endl;
    }

    return Expected<void>();
}

static const char* S_VERTEX_INVARS = R"(
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;
in vec4 vertexCustom0;
in vec4 vertexCustom1;
)";

static const char* S_VERTEX_UNIFORMS = R"(
uniform mat4 mvp;
uniform mat4 matModel;
)";

static const string S_DEFAULT_VERTEX = string("#version 330") + S_VERTEX_INVARS + S_VERTEX_UNIFORMS + R"(out vec2 fragTexCoord;
out vec4 fragColor;
out vec2 spriteSize;
void main() {
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    spriteSize = vertexCustom0.xy;
    gl_Position = mvp*vec4(vertexPosition, 1.0);
})";

static const char* S_DEFAULT_FRAGMENT = R"(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
in vec2 spriteSize;
uniform sampler2D texture0;
out vec4 finalColor;
void main() {
    vec4 texelColor = texture(texture0, fragTexCoord);
    finalColor = texelColor*fragColor;
})";

static const string S_DEFAULT_VERTEX_MRT = string("#version 330") + S_VERTEX_INVARS + S_VERTEX_UNIFORMS + R"(
out vec2 fragTexCoord;
out vec4 fragColor;
out vec2 spriteSize;
out float fragDepth;
out float isUI;
out float isMask;
out vec2 maskTexCoord;
out float isSilhouette;
out float isMaskBlendAdditive;
#ifdef PLATFORM_WEB
float extractBit(int value, int bitPos) {
    return 0.;
}
#else
float extractBit(uint intData, int bitPosition) {
    uint bitPos = 0x1u << (uint(bitPosition) - 1u);
    if ((intData & bitPos) != 0u) {
        return 1.0;
    } else {
        return 0.0;
    }
}
#endif
void main() {
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    spriteSize = vertexCustom0.rg;
    #ifdef PLATFORM_WEB
    int bitData = int(floor(vertexNormal.r + 0.5));
    fragDepth = 0.;
    #else
    uint bitData = floatBitsToUint(vertexNormal.r);
    fragDepth = float(bitData & 0xFFu) / 255.0;
    #endif
    isUI = extractBit(bitData, 10);
    isMask = extractBit(bitData, 11);
    isSilhouette = extractBit(bitData, 12);
    isMaskBlendAdditive = extractBit(bitData, 13);
    maskTexCoord = vertexNormal.gb + vertexTexCoord;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
})";

static const char* S_DEFAULT_FRAGMENT_MRT = R"(#version 330
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 Depth;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec2 spriteSize;
in float fragDepth;
in float isOccluder;
in float isUI;
in float isMask;
in vec2 maskTexCoord;
in float isSilhouette;
in float isMaskBlendAdditive;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
void main() {
    vec4 texelColor = texture(texture0, fragTexCoord);
    if (isMask > 0.) {
        vec4 maskColor = texture(texture0, maskTexCoord);
        if (isMaskBlendAdditive > 0.) {
            texelColor = vec4((texelColor + texelColor * maskColor).xyz, texelColor.a);
        } else {
            texelColor *= maskColor;
        }
    }
    if (isSilhouette > 0.) {
        FragColor = fragColor * vec4(1., 1., 1., texelColor.a);
    } else {
        FragColor = fragColor * texelColor;
    }
    // To make things more visible when debugging, scale the colors
    // During release, this can just be 1.0
    const float scalar = 20.0;
    if (isUI < 0.5) {
        Depth = vec4(fragDepth * scalar, 0., 0., texelColor.a);
    }
})";

string CompileState::getShaderString(bool isVertex) const {
    if (isVertex && vert.isDefault) {
        if (isMrt) {
            return S_DEFAULT_VERTEX_MRT;
        } else {
            return S_DEFAULT_VERTEX;
        }
    } else if (!isVertex && frag.isDefault) {
        if (isMrt) {
            return S_DEFAULT_FRAGMENT_MRT;
        } else {
            return S_DEFAULT_FRAGMENT;
        }
    }

    using std::endl;
    std::stringstream ss;
    ss << "#version 330" << endl;
    if (isVertex) {
        ss << S_VERTEX_INVARS << endl;
        ss << S_VERTEX_UNIFORMS << endl;
        ss << vert.code.str();
    } else {
        ss << frag.code.str();
    }

    return ss.str();
}

}  // namespace whal
