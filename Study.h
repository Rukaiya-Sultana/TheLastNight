#ifndef STUDY_H
#define STUDY_H

// ============================================================
//  THE LAST NIGHT - Study.h
//  The private Study (Phase 6), north of the Library.
//
//  World layout (continuous world):
//    Study:    x [12.25, 16.25],  z [23.5, 29.5]
//    Entrance: doorway in the south wall (x 13.65..14.85)
//    from the Library. Small, personal, warm.
// ============================================================

namespace Study
{
    void Load();                        // collision + interactables (once)
    void UpdateLights(float timeSec);   // player-room lights L0/L1/L2
    void UpdateFill();                  // L6 fill (study position)
    void Draw(float timeSec);

    // Desk drawer animation (opens after examining the photograph).
    void SetDrawerOpen(bool open);
    bool IsDrawerOpen();
}

#endif // STUDY_H
