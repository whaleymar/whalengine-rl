#pragma once

#include "whalECS/src/ECS.h"

typedef struct Shader Shader;

namespace whal {

struct PointLight;
struct Transform2D;

class PointLightSystem : public ecs::ISystem<Transform2D, PointLight> {
public:
    void update() override;
    void setShader(Shader* shader) { mShaderPtr = shader; }
    void setPositionUniform(int id) { mPositionUniform = id; }

private:
    Shader* mShaderPtr = nullptr;
    int mPositionUniform;
};

}  // namespace whal
