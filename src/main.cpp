#include "Engine.h"
#include "Game/Game.h"

int main() {
    whal::Engine<Game> engine;

    if (engine.start()) {
        return 1;
    }

    engine.mainloop();
    engine.end();

    return 0;
}
