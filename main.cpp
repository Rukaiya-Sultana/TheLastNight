// ============================================================
//  THE LAST NIGHT
//  A 3D first-person mystery/adventure game.
//  C++ / OpenGL / FreeGLUT (Code::Blocks, Windows)
//
//  Phase 0: project foundation and game loop.
// ============================================================

#include "Game.h"

int main(int argc, char **argv)
{
    if (!Game::Init(argc, argv))
        return 1;

    Game::Run();
    return 0;
}
