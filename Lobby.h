#ifndef LOBBY_H
#define LOBBY_H

// ============================================================
//  THE LAST NIGHT - Lobby.h
//  The mansion lobby: first explorable room.
//  Coordinate system: room spans x [-8, +8], z [-7, +7],
//  floor at y = 0, ceiling at y = 4.2. Window wall: z = -7.
//  Fireplace wall: x = -8. Entrance wall: z = +7.
// ============================================================

namespace Lobby
{
    void Load();                        // register collision boxes (once)

    // Re-register the lobby's lights (fireplace flickers with time).
    // Must be called AFTER the camera view transform is applied.
    void UpdateLights(float timeSec, bool farAway);

    void Draw(float timeSec);           // draw the whole scene

    // --- Phase 7: the garden entrance door (east door) ---
    // Unlocked with the mansion key; swings open/closed (animated).
    void SetGardenDoorOpen(bool open);
    bool IsGardenDoorOpen();
    void UpdateGardenDoor(float dt);    // animates + updates collision

    // --- Phase 10: the front entrance (south wall, double doors) ---
    // Locked until the brass symbol mechanism is solved with the
    // master key. Doors swing on hinges; collision tracks them.
    void SetFrontOpen(bool open);
    bool IsFrontOpen();
    void UpdateFrontDoor(float dt);     // animates + updates collision

    // --- Phase 10 front-door medallions (visible brass dials) ---
    // Push the current dial state from Game so the drawn symbols match.
    void SetFrontDialState(const int dial[3], bool solved);
    void DrawFrontSymbols();            // call right after Lobby::Draw()
}

#endif // LOBBY_H
