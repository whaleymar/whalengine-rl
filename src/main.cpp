#include "Game.h"

#include "Settings.h"  // define macros

int main() {
    Game& game = Game::instance();

    if (game.startup()) {
        return 1;
    }

    game.mainloop();
    game.end();

    return 0;
}
