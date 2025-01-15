#pragma once

#ifndef NDEBUG

#include "ECS.h"
#include "Events/Events.h"
#include "Sys/IListen.h"
#include "Util/ImguiUtil.h"

namespace whal {

struct Transform;

class EditorUiSystem : public ecs::ISystem<Transform>,
                       public ecs::IMonitorSystem,
                       public IListen<evt::Input, true, InputEvent>,
                       public IListen<evt::EnginePause, true, bool>,
                       public IRenderDebug {
public:
    void activate();
    void deactivate();
    void dragSelectedEntities() const;
    void panCamera() const;

    void onEvent(evt::Input, InputEvent input) override;
    void onEvent(evt::EnginePause, bool isPaused) override;
    void onAdd(ecs::Entity entity) override;
    void onRemove(ecs::Entity entity) override;

    void drawWorld() const;
    void draw() override;

private:
    std::vector<ecs::Entity> mClickedEntities;
};

}  // namespace whal

#endif
