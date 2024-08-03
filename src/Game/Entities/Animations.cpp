#include "Animations.h"

#include "Components/Animator.h"
#include "Util/Print.h"
#include "Util/String.h"

using namespace whal;

struct NameToAnimator {
    const char* name;
    Animator animator;
};

whal::Animator getAnimator(const char* name) {
    // table of basic animators that don't have complex logic
    static const NameToAnimator S_ANIMATORS[] = {
        {"effect/bluefire", Animator({{"effect/bluefire", 0, 4, 0.1}})},
        {"effect/explosion", Animator({{"effect/explosion", 0, 6, 0.5 / 6.0}}, false)},
        {"actor/magichat", Animator({{"actor/magichat", 0, 8, 0.2}})},
        {"actor/blast-crystal", Animator({{"actor/blast-crystal", 0, 2, 0.0}}, &basicAnimationUnsquish)},
        {"actor/aim-arrow", Animator({
                                {"actor/aim-arrow-straight", 0, 8, 0.125},
                                {"actor/aim-arrow-diagonal", 1, 8, 0.125},
                            })},
    };
    constexpr s32 NUM_ANIMATORS = sizeof(S_ANIMATORS) / sizeof(NameToAnimator);

    for (s32 i = 0; i < NUM_ANIMATORS; i++) {
        if (isEqualString(S_ANIMATORS[i].name, name)) {
            return S_ANIMATORS[i].animator;
        }
    }

    print("ERROR: Couldn't find animator with name: ", name);
    return Animator();
}
