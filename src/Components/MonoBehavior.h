#pragma once

// TODO use corrade?
#include <memory>
#include "ECS.h"
#include "Sys/InputHandler.h"

namespace whal {
struct InputEvent;

namespace ecs {

class Entity;

}

// https://docs.unity3d.com/6000.0/Documentation/ScriptReference/MonoBehaviour.html
struct MonoBase {
    MonoBase() = default;
    MonoBase(const MonoBase& other) = default;
    MonoBase& operator=(MonoBase const& other) = default;

    virtual ~MonoBase() {}
    virtual void init(ecs::Entity self) {}    // runs the first frame after an entity is created
    virtual void start(ecs::Entity self) {}   // runs the first frame after an entity is activated
    virtual void update(ecs::Entity self) {}  // runs every frame
    // fixedUpdate?
    virtual void onCollisionEnter() {}  // runs when this entity collides with something
    // onCollisionStay?
    // onCollisionExit?
    virtual void onTriggerEnter() {}
    virtual void onTriggerStay() {}
    virtual void onTriggerExit() {}
    virtual void onDestroy(ecs::Entity self) {}  // runs when an entity is killed or deactivated
    // onDrawGizmo?
    virtual void onInput(ecs::Entity self, InputEvent input) {}

    virtual std::unique_ptr<MonoBase> clone() const = 0;

    // I could do a default onEditorRender in IMonoBehavior, but I have to define a ReflectionType
    // for derived classes anyway (aggregate types can't have virtual functions) so I might as well
    // save the compile time and write it in a source file
    virtual void onEditorRender() {};
};

// using CRTP to automatically define a clone method so that MonoBehavior is copy-constructible
template <typename T>
struct IMonoBehavior : MonoBase {
    std::unique_ptr<MonoBase> clone() const override { return std::unique_ptr<T>(new T(*static_cast<const T*>(this))); }
};

/*
Example Usage:
class PlayerScript : public IMonoBehavior<PlayerScript> {};
entity.add(MonoBehaviour{std::make_unique<PlayerScript>()});
*/
struct MonoBehavior {
    std::unique_ptr<MonoBase> pBehavior;
    void onEditorRender() { pBehavior->onEditorRender(); }

    // rule of five
    MonoBehavior(std::unique_ptr<MonoBase>&& behavior) : pBehavior(std::move(behavior)) {}
    ~MonoBehavior() = default;
    MonoBehavior(MonoBehavior const& other) : pBehavior(other.pBehavior->clone()) {}
    MonoBehavior(MonoBehavior&& other) = default;
    MonoBehavior& operator=(MonoBehavior const& other) {
        pBehavior = other.pBehavior->clone();
        return *this;
    }
    MonoBehavior& operator=(MonoBehavior&& other) = default;

    // I want to make this private to prevent dereferencing a nullptr, but friending the ECS stuff isn't working
    //     friend class ecs::ComponentArray<MonoBehavior>;
    //     friend class ecs::ComponentManager;
    //
    // private:
    MonoBehavior() = default;
};

}  // namespace whal
