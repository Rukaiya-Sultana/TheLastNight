// ============================================================
//  THE LAST NIGHT - Weather.cpp
//  Rain was removed at the owner's request (it read as visual
//  noise rather than atmosphere). The lightning system is kept
//  but permanently idle (g_flash stays 0), so the night sky,
//  moon and lamps carry the garden on their own. The API is
//  unchanged so callers keep compiling untouched.
// ============================================================
#include "Weather.h"
#include <GL/freeglut.h>

namespace
{
    // lightning state machine (kept for the FlashLevel() API; idle)
    float g_flash      = 0.0f;      // current envelope 0..1
    float g_timer      = 14.0f;     // seconds until the next flash
    float g_holdTimer  = 0.0f;      // remaining hold at peak
    bool  g_rising     = false;
}

namespace Weather
{
    void Reset()
    {
        g_flash = 0;
        g_timer = 14.0f;
        g_holdTimer = 0;
        g_rising = false;
    }

    void Update(float dt, float px, float pz)
    {
        // Rain removed: no drops are spawned or drawn, and the lightning
        // state machine stays idle (g_flash stays 0 -> the sky, moon and
        // garden lamps carry the outdoor mood). One small timer advance
        // is kept so the call is never fully dead code.
        (void)px; (void)pz;
        if (g_timer > 0.0f) g_timer -= dt;
        (void)g_holdTimer; (void)g_rising;
    }

    void Draw(float px, float pz)
    {
        // Rain removed - nothing to draw. Kept as a no-op so the garden
        // pass and the API stay unchanged.
        (void)px; (void)pz;
    }

    float FlashLevel() { return g_flash; }
    bool  Flashing()   { return g_flash > 0.02f; }
}
