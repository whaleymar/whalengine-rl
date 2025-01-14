#pragma once

#ifndef NDEBUG

#include "ECS.h"
#include "Events/Events.h"
#include "Sys/IListen.h"
#include "Util/Vector.h"

namespace whal {

struct Transform;

class EditorUiSystem : public ecs::ISystem<Transform>,
                       public ecs::IMonitorSystem,
                       public IListen<evt::Input, true, InputEvent>,
                       public IListen<evt::EnginePause, true, bool>,
                       public IListen<evt::WindowResize, true, Vector2f> {
public:
    void activate();
    void deactivate();
    void dragSelectedEntities() const;
    void panCamera() const;
    void buildEntityTree() const;

    void onEvent(evt::Input, InputEvent input) override;
    void onEvent(evt::EnginePause, bool isPaused) override;
    void onEvent(evt::WindowResize, Vector2f scalar) override;
    void onAdd(ecs::Entity entity) override;
    void onRemove(ecs::Entity entity) override;
    void draw();

private:
    std::vector<ecs::Entity> mClickedEntities;
};

}  // namespace whal

#endif
