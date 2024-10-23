#include "Engine.h"

int main() {
    whal::Engine engine;

    if (engine.start() || engine.loadGame()) {
        return 1;
    }

    engine.mainloop();
    engine.unloadGame();
    engine.end();

    return 0;
}
