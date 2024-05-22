#include "Game.h"

int main() {
    Game& game = Game::instance();

    if (game.startup()) {
        return 1;
    }

    game.mainloop();
    game.end();

    return 0;
}
