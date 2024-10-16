#pragma once

#include <concepts>

namespace whal {

struct Scene;

// A game must implement the IGame interface and satisfy the Singleton and StaticUpdate concepts
class IGame {
public:
    virtual bool start() = 0;
    virtual void end() = 0;
    virtual Scene& getScene() = 0;
};

template <typename T>
concept Singleton = requires {
    { T::instance() } -> std::same_as<T&>;  // Checks that T::instance() returns T&
};

template <typename T>
concept StaticUpdate = requires {
    { T::update() } -> std::same_as<void>;  // checks that T::update exists and returns nothing
};

}  // namespace whal
