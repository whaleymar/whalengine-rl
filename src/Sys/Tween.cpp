#include "Tween.h"

#include <algorithm>

namespace whal {

void TweenManager::onEvent(DeathEvent, ecs::Entity entity) {
    mKilledEntities.insert(entity);
}

void TweenManager::update() {
    // fastest way of removing finished tweens from the vector
    if (mTweens.size() > 0) {
        mTweens.erase(std::remove_if(mTweens.begin(), mTweens.end(),
                                     [this](std::shared_ptr<ITween>& pTween) {
                                         if (mKilledEntities.contains(pTween->getEntity())) {
                                             return true;
                                         }
                                         if (pTween->isCancelled()) {
                                             return true;
                                         }
                                         if (pTween->isDelayCondition()) {
                                             // waiting to start
                                             return false;
                                         }

                                         // check if tween started already. If not, init the start & end values
                                         const bool isStarted = pTween->isStarted();
                                         if (!isStarted) {
                                             pTween->init();
                                         }
                                         pTween->tick();

                                         // dispatch callback based on tween state
                                         if (!isStarted && pTween->isStarted()) {
                                             pTween->onStart();
                                             return false;
                                         } else if (pTween->isDone()) {
                                             pTween->onEnd();
                                             return true;
                                         } else {
                                             pTween->onUpdate();
                                             return false;
                                         }
                                     }),
                      mTweens.end());
    }

    mKilledEntities.clear();
}

void TweenManager::clear() {
    mTweens.clear();
    mKilledEntities.clear();
}

}  // namespace whal
