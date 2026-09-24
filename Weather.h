#ifndef WEATHER_H
#define WEATHER_H

// ============================================================
//  THE LAST NIGHT - Weather.h
//  Garden weather (Phase 7, revised): the rain particles were
//  removed after playtesting (visual noise, not atmosphere);
//  the lightning hooks remain but stay idle. The sky, moon and
//  warm lamps carry the outdoor mood instead.
// ============================================================

namespace Weather
{
    void Reset();                       // new game

    void Update(float dt, float px, float pz);  // advance rain + lightning

    // Draw the rain (translucent streaks; call inside the garden pass,
    // after opaque geometry). Rain is drawn around the player position.
    void Draw(float px, float pz);

    // 0..1 lightning envelope for lights/sky (0 = no flash).
    float FlashLevel();

    // True while a flash is currently brightening the scene.
    bool Flashing();
}
#endif // WEATHER_H
