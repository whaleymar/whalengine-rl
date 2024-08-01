#include "Game.h"

#include "Settings.h"  // define macros

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
    Game& game = Game::instance();

    if (game.startup()) {
        return 1;
    }

    game.mainloop();
    game.end();

    return 0;
}
