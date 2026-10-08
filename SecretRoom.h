#ifndef SECRETROOM_H
#define SECRETROOM_H

// ============================================================
//  THE LAST NIGHT - SecretRoom.h
//  The hidden chamber behind the basement passage (Phase 9).
//  Layout: east of the passage (x 31.5..39.9), z -4.6..2.4,
//  floor shared with the basement (y = -3.35).
//  Contents: the great crest wall (3 rotating rings -> cabinet
//  unlock), investigation table (blueprint, map, photo, journal,
//  final letter), bookshelves, cabinet with the master key,
//  window with a garden view, warm lantern + candle light.
// ============================================================

namespace SecretRoom
{
    void Load();                        // collision + interactables (once)
    void Draw(float timeSec);
    void UpdateLights(float timeSec);   // slots 0..2 while the player is here
    void UpdateFill();                  // slot 6

    // secret door (passage <-> room), hinged swing + collision
    void SetDoorOpen(bool open);
    void UpdateDoor(float dt);
    bool IsDoorOpen();
    bool IsDoorOpening();               // requested state, even mid-animation

    // the crest puzzle: each call rotates one ring 90 deg
    void TurnRing(int which);           // 0 = outer, 1 = middle, 2 = inner
    bool CrestJustSolved();             // true exactly once, on alignment
    bool IsCrestSolved();

    void SetCabinetOpen(bool open);     // hinge animation after the solve
    void UpdateCabinet(float dt);
    bool IsCabinetOpen();

    void Reset();                       // new game: closed door, locked crest
}

#endif // SECRETROOM_H
