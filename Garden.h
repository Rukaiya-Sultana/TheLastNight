#ifndef GARDEN_H
#define GARDEN_H

// ============================================================
//  THE LAST NIGHT - Garden.h
//  The mansion courtyard (Phase 7), west of the Lobby. Entered
//  through the lobby's east-door opening; the cellar door in the
//  north-west annex leads down to the Basement (Phase 8).
//
//  World layout (continuous world):
//    Garden lawn:  x [8.4, 28.4],  z [-9.25, 9.25]
//    Mansion facade: x = 8.2 (lobby's east wall exterior)
//    Door opening: z [-0.6, 0.6]  (the lobby's garden door)
//    Fountain:     (16.5, 0);  Statue: (22.6, 4.6)
//    Cellar annex: x [25.4, 29.0], z [5.6, 8.8], floor y -0.35
//    Stair trench: x [26.0, 27.2], z [0.8, 5.6] down to y -3.35
// ============================================================

namespace Garden
{
    void Load();                        // collision + interactables (once)
    void UpdateLights(float timeSec);   // player-room lights L0/L1/L2
    void UpdateFill();                  // L6 fill (garden position)
    void Draw(float timeSec, float playerX, float playerZ);

    // Ground height for the player sampler (0 on grass/patio; the
    // annex floor is slightly lower; the stone path is flush).
    float GroundHeight(float x, float z);

    // The cellar door (annex) swings open after being found.
    void SetCellarDoorOpen(bool open);
    bool IsCellarDoorOpen();
    void UpdateCellarDoor(float dt);    // animates + draws + collision

    // The lobby's east door (garden entrance) is animated by Lobby.
    // The garden draws its own warm door glow so the entrance reads
    // as open from outside.
}
#endif // GARDEN_H
