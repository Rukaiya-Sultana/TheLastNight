#ifndef LIBRARY_H
#define LIBRARY_H

// ============================================================
//  THE LAST NIGHT - Library.h
//  The Grand Library (Phase 5), east of the Grand Hall and
//  connected to the Study through its north door (Phase 6).
//
//  World layout (continuous world, shared with all rooms):
//    Library:  x [8.25, 20.25],  z [13.25, 23.25]
//    Entrance: doorway in the west wall (z ~19.5) from the hall
//    Study door: locked, north wall (x = 14.25)
// ============================================================

namespace Library
{
    void Load();                        // collision + interactables (once)
    void UpdateLights(float timeSec);   // player-room lights L0/L1/L2
    void UpdateFill();                  // L6 second fill (library position)
    void Draw(float timeSec);

    // The study door (north wall): locked until the brass key is used.
    void SetStudyDoorOpen(bool open);
    bool IsStudyDoorOpen();
    void UpdateStudyDoor(float dt);     // animates + draws the panel

    // The safe door swings open after the correct code (Puzzle).
    bool IsSafeOpen();

    // Puzzle keypads / messages may need these anchors.
    float SafeX();                      // safe interaction point
    float SafeZ();
}

#endif // LIBRARY_H
