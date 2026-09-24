#ifndef GRANDHALL_H
#define GRANDHALL_H

// ============================================================
//  THE LAST NIGHT - GrandHall.h
//  The Grand Hall: second explorable room (Phase 4), connected
//  to the Lobby through two doorways (no loading screens).
//
//  World layout (continuous world, shared with the Lobby):
//    Lobby:  x [-8, +8],  z [-7, +7]
//    Hall:   x [-8, +8],  z [7.25, 21.25]
//    Shared wall at z = +7 (lobby side) / z = 7.25 (hall side)
//    with doorways at x = -4.6 and x = +4.6.
// ============================================================

namespace GrandHall
{
    void Load();                        // register collision + interactables
    void UpdateLights(float timeSec, bool farAway);
    void Draw(float timeSec);           // render hall contents

    // Door open/close (player operates the WEST doorway door via E).
    // The east doorway stays open (unobstructed passage).
    void SetWestDoorOpen(bool open);
    bool IsWestDoorOpen();
    void Reset();                       // restore closed-door state for a new game
}

#endif // GRANDHALL_H
