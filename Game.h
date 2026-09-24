#ifndef GAME_H
#define GAME_H

// ============================================================
//  THE LAST NIGHT - Game.h
//  Application core: owns the camera, frame timing, input state
//  and the current scene's update/render entry points.
//  Phase 0: foundation + smoke-test scene.
// ============================================================

namespace Game
{
    bool Init(int argc, char **argv);   // create window, register callbacks
    bool Run();                         // enter the FreeGLUT main loop
}

#endif // GAME_H
