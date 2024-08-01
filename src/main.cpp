#include "Game.h"

#include "Engine.h"

// windows-only way of coercing dedicated GPU usage...
// #ifdef __cplusplus
// extern "C" {
// #endif
//
// __declspec(dllexport) unsigned long NvOptimusEnablement = 1;
// __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
//
// #ifdef __cplusplus
// }
// #endif

int main() {
    whal::Engine<Game> engine;

    if (engine.start()) {
        return 1;
    }

    engine.mainloop();
    engine.end();

    return 0;
}
