#pragma once

namespace whal {

namespace gfx {
struct RenderContext;
struct EntityRenderInfo;
class RenderQueue;
}  // namespace gfx

class IRender {
public:
    // Draw a single entity.
    virtual void draw(const gfx::EntityRenderInfo& entityInfo, const gfx::RenderContext& ctx) const = 0;

    // Adds all entities to the draw queue. Culling is performed automatically.
    virtual void addToQueue(gfx::RenderQueue& queue) const = 0;

    virtual ~IRender();

protected:
    IRender();
};

class IRenderLight {
public:
    virtual void draw(const gfx::RenderContext& ctx) const = 0;
    virtual ~IRenderLight();

protected:
    IRenderLight();
};

#ifndef NDEBUG
// make sure an empty class definition exists for inheritance reasons
class IRenderDebug {
public:
    virtual void drawDebug() {}
    virtual void drawEditor() {}
    virtual ~IRenderDebug();

protected:
    IRenderDebug();
};
#endif

}  // namespace whal
