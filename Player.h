#ifndef PLAYER_H
#define PLAYER_H

// ============================================================
//  THE LAST NIGHT - Player.h
//  The first-person player: body state, movement, flashlight.
//  The camera is attached to the player's eyes (see Camera).
// ============================================================

#include "Camera.h"

class Player
{
public:
    Player();

    // --- configuration (tweakable) ---
    static const float WALKING_SPEED;   // units / second
    static const float RUNNING_SPEED;   // units / second
    static const float PLAYER_HEIGHT;   // feet on the floor
    static const float CAMERA_HEIGHT;   // eyes above the feet
    static const float COLLISION_RADIUS;// body radius for future collision
    static const float GROUND_HEIGHT;   // default floor plane y

    // Floor sampler: the game can supply a world height function so the
    // player can walk down stairs / up slopes (basement, ramps). nullptr
    // means a flat world at GROUND_HEIGHT.
    typedef float (*GroundFn)(float x, float z);
    static void SetGroundSampler(GroundFn fn);

    // --- per-frame update ---
    void handleKeyboard(unsigned char key, bool down);
    void handleSpecial(int key, bool down);
    void applyMouseDelta(int dx, int dy);         // yaw/pitch from cursor delta
    void update(float dt);

    // --- actions ---
    void toggleFlashlight();             // toggles + prints the HUD line
    bool flashlightOn() const           { return m_flashlightOn; }
    // One-shot HUD notification, e.g. "FLASHLIGHT ON" (cleared by UI). Empty
    // string clears it without toggling.
    static const char *FlashHud();
    static void ClearFlashHud();

    // Dev helpers (unattended screenshot poses)
    void SetYaw(float yaw);             // defined in Player.cpp
    void SetPos(float x, float z);      // re-samples ground height (stairs)

    // --- state queries ---
    float x() const                     { return m_x; }
    float y() const                     { return m_y; }   // feet height
    float z() const                     { return m_z; }
    float eyeY() const                  { return m_y + CAMERA_HEIGHT; }
    float yaw() const                   { return m_yaw; }
    float pitch() const                 { return m_pitch; }
    bool  isRunning() const             { return m_running; }

    // Attempt to move; returns the final applied position.
    // Uses Collision system to slide along blockers (walls, furniture).
    void applyMovement(float dx, float dz);

    // Attach camera to the eyes (position + orientation).
    void updateCamera(Camera &cam) const;

private:
    // Body position (feet on the floor)
    float m_x, m_y, m_z;

    // View orientation
    float m_yaw;                        // degrees, unlimited (turn around)
    float m_pitch;                      // degrees, clamped [-89, +89]

    // Movement flags set by key press/release (never auto-repeat dependent)
    bool  m_forward, m_back, m_left, m_right, m_running;
    bool  m_turnLeft, m_turnRight;      // arrow-key fallback look

    bool  m_flashlightOn;
};

#endif // PLAYER_H
