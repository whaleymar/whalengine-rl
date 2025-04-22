#pragma once

#ifndef NDEBUG

#include "Components/Transform.h"
#include "ECS.h"
#include "Events/Events.h"
#include "Gfx/IRender.h"
#include "Sys/IListen.h"

namespace whal {

class EditorUiSystem : public ecs::ISystem<Transform>,
                       public ecs::IMonitorSystem,
                       public IListen<evt::Input, true, InputEvent>,
                       public IListen<evt::EnginePause, true, bool>,
                       public IListen<evt::Restart, true, bool>,
                       public IRenderDebug {
public:
    void activate();
    void deactivate();
    void dragSelectedEntities() const;
    void panCamera() const;

    void onEvent(evt::Input, InputEvent input) override;
    void onEvent(evt::EnginePause, bool isPaused) override;
    void onEvent(evt::Restart, bool resetPlayers) override;
    void onAdd(ecs::Entity entity) override;
    void onRemove(ecs::Entity entity) override;

    void drawDebug() override;
    void drawEditor() override;

    void drawHierarchy(ecs::Entity entity);  // draws the hierarchy of entities that parent/are children of `entity`

private:
    void drawHierarchyRecursive(ecs::Entity rootEntity, const std::vector<ecs::Entity>& openEntities);

    std::vector<ecs::Entity> mClickedEntities;
    ecs::Entity mHierachySelectionMouseDown;  // the entity clicked in the hierarchy with MouseDown
    f32 mLastClickTime = 0;
};

}  // namespace whal

#endif
