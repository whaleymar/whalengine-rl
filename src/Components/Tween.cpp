#include "Tween.h"

namespace whal {

void TweenManager::onEvent(DeathEvent, ecs::Entity entity) {
    mTweens.erase(entity);
}

template <typename T>
static void updateTweens(ecs::Entity entity, std::vector<Tween<T>>& tweens) {
    for (auto it = tweens.begin(); it != tweens.end();) {
        (*it).tick(entity);
        if ((*it).isDone()) {
            (*it).onEnd(entity);
            it = tweens.erase(it);
        } else {
            (*it).onUpdate(entity);
            ++it;
        }
    }
}

void TweenManager::update() {
    std::vector<ecs::Entity> toRemove;
    for (auto& [entity, tweenLists] : mTweens) {
        updateTweens(entity, tweenLists.floats);
        updateTweens(entity, tweenLists.ints);
        updateTweens(entity, tweenLists.vec2fs);
        updateTweens(entity, tweenLists.vec2is);
        updateTweens(entity, tweenLists.colors);
        if (tweenLists.isEmpty()) {
            toRemove.push_back(entity);
        }
    }

    for (auto e : toRemove) {
        mTweens.erase(e);
    }
}

}  // namespace whal
