#include "ShaderManager.h"

#include <cassert>
#include <cstring>
#include <raylib.h>
#include "Events/Events.h"
#include "Gfx/Shader.h"
#include "Gfx/ShaderTranspiler.h"
#include "Settings.h"
#include "Sys/System.h"

#include "Util/Print.h"

namespace whal {

struct Uniforms {
    enum flags : u32 {
        None = 0,
        TimeStamp = 1,
        Resolution = 1 << 1,
        VirtualResolution = 1 << 2,  // mutually exclusive w/ Resolution
    };

    bool isSet(flags flag) { return (uniformFlags & flag) > 0; }

    u32 uniformFlags = None;
    int iTime;
    int iResolution;
};

struct RegisteredShader {
    std::string name;
    Shader shader;
};

static std::vector<RegisteredShader> S_SHADERS;
static const char* S_ENGINE_SHADER_DIR = "whalengine/data/shaders";

Expected<void> ShaderMgr::loadShaders() {
    unloadShaders();

    Expected<void> err = loadShaderDir(S_ENGINE_SHADER_DIR);
    if (!err.isExpected()) {
        return err;
    }
    return loadShaderDir(SHADER_DIR);
}

void ShaderMgr::unloadShaders() {
    S_SHADERS.clear();
}

void ShaderMgr::reloadShaders() {
    Expected<void> err = loadShaders();
    if (!err.isExpected()) {
        print(err.error());
    }
    Event.emit<evt::ShaderReload>();
}

Shader& ShaderMgr::get(const std::string& sName) {
    for (auto& [name, shader] : S_SHADERS) {
        if (name == sName) {
            return shader;
        }
    }
    print("ERROR: Shader named", sName, "not found. Returning random shader");
    return S_SHADERS[0].shader;
}

Expected<void> ShaderMgr::loadShaderDir(const char* path) {
    ShaderTranspiler transpiler;
    rl::FilePathList files = rl::LoadDirectoryFiles(path);  // paths include the parent path
    for (u32 i = 0; i < files.count; i++) {
        if (!rl::IsPathFile(files.paths[i])) {
            continue;
        }
        const char* filepath = files.paths[i];
        Expected<rl::Shader> eShader = transpiler.loadAndCompile(filepath);
        if (!eShader.isExpected()) {
            ShaderMgr::unloadShaders();
            return eShader.error();
        }

        S_SHADERS.emplace_back(rl::GetFileNameWithoutExt(filepath), Shader(eShader.value(), filepath));
        ShaderMetaData meta = transpiler.getMetaData();
        for (const auto& gUniformName : meta.globalUniforms) {
            Graphics.globalUniformSubscribe(gUniformName, S_SHADERS.back().shader);
        }
    }

    rl::UnloadDirectoryFiles(files);
    return {};
}

}  // namespace whal
