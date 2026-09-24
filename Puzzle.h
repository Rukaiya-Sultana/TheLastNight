#ifndef PUZZLE_H
#define PUZZLE_H

// ============================================================
//  THE LAST NIGHT - Puzzle.h
//  The library safe's 3-digit keypad (Phase 5).
//  Code discovery loop: secret book journal -> stopped clock (12)
//  + founder painting plate (7) -> 127.
//  Rendering is a self-contained 2D overlay pass; Game routes
//  keyboard input here while the keypad is active.
// ============================================================

namespace Puzzle
{
    void Reset();                       // new game

    void Open();                        // show the keypad
    void Close();
    bool IsOpen();

    // Keyboard routing while the keypad is open (Game forwards 0-9, C, E).
    // Returns true if the key was consumed by the keypad.
    bool HandleKey(unsigned char key);

    // Per-frame feedback timing; call from the game update.
    void Update(float dt);

    // True exactly once after a correct code, for the safe animation.
    bool ConsumeJustOpened();

    // Draw the keypad overlay (2D pass; call after UI::Render).
    void Render(int windowWidth, int windowHeight);

    bool Solved();                      // code accepted at least once
}
#endif // PUZZLE_H
