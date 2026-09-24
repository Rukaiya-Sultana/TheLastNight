#ifndef BASEMENT_H
#define BASEMENT_H

// ============================================================
//  THE LAST NIGHT - Basement.h
//  The underground cellar hall (Phase 8), reached from the
//  garden's stair trench. Stone vaults, wine store, work table,
//  and the crest mechanism: three rotating dials that open the
//  hidden stone panel to a short passage and the locked
//  secret door (the next phase's entrance).
//
//  World layout (continuous world, under the garden lawn):
//    Hall:      x [14.5, 27.2], z [-4.5, 0.8], floor y = -3.35
//    Ceiling:   y = -0.55 (the lawn sits above at y = 0)
//    Passage:   x [27.2, 31.3], z [-1.3, 0.3] (behind the panel)
//    Stairs up: x [26.0, 27.2] (top at z 0.8, see Garden.cpp)
// ============================================================

namespace Basement
{
    void Load();                        // collision + interactables (once)
    void UpdateLights(float timeSec);   // player-room lights L0/L1/L2
    void UpdateFill();                  // L6 fill (basement position)
    void Draw(float timeSec, float playerX, float playerZ);

    // The hidden stone panel: CLOSED -> OPENING (slide) -> OPEN.
    void SetPanelOpen(bool open);
    bool IsPanelOpen();
    void UpdatePanel(float dt);         // animates + collision

    // The crest mechanism: each TurnDial rotates one dial 90 deg.
    void Reset();                       // new game: dials home, panel closed
    // MechanismJustSolved() returns true exactly once when all three
    // align - the caller opens the panel and shows the feedback.
    void TurnDial(int which);           // 0 = left, 1 = middle, 2 = right
    bool MechanismJustSolved();
    bool DialAligned(int which);        // for the glow state

    // The secret door at the passage's end (opens in Phase 10 once the
    // player holds the master key). Swing animation only - the static
    // collision box is skipped at Load when the door stands open.
    void SetSecretDoorOpen(bool open);
    void UpdateSecretDoor(float dt);
}
#endif // BASEMENT_H
