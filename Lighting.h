#ifndef LIGHTING_H
#define LIGHTING_H

// ============================================================
//  THE LAST NIGHT - Lighting.h
//  Central place for light and material property definitions.
//  Phase 0: material presets and OpenGL fixed-function light setup.
// ============================================================

struct Material
{
    float ambient[4];
    float diffuse[4];
    float specular[4];
    float shininess;
};

namespace Materials
{
    // Presets shared by all game objects (Phase 3+ extends these).
    const Material WOOD    = {{0.2f, 0.12f, 0.05f, 1}, {0.55f, 0.38f, 0.22f, 1}, {0.3f, 0.25f, 0.2f, 1}, 12.0f};
    const Material DARKWOOD= {{0.12f, 0.07f, 0.03f, 1}, {0.35f, 0.22f, 0.12f, 1}, {0.2f, 0.16f, 0.12f, 1}, 10.0f};
    const Material FABRIC  = {{0.18f, 0.08f, 0.08f, 1}, {0.5f, 0.12f, 0.12f, 1}, {0.05f, 0.05f, 0.05f, 1}, 2.0f};
    const Material FABRIC_G= {{0.06f, 0.10f, 0.05f, 1}, {0.16f, 0.30f, 0.14f, 1}, {0.05f, 0.05f, 0.05f, 1}, 2.0f};
    const Material FABRIC_C= {{0.22f, 0.19f, 0.13f, 1}, {0.72f, 0.64f, 0.48f, 1}, {0.05f, 0.05f, 0.05f, 1}, 2.0f};
    const Material METAL   = {{0.25f, 0.25f, 0.28f, 1}, {0.7f, 0.7f, 0.75f, 1}, {0.9f, 0.9f, 0.95f, 1}, 80.0f};
    const Material GLASS   = {{0.1f, 0.15f, 0.2f, 1}, {0.4f, 0.5f, 0.6f, 1}, {0.95f, 0.95f, 1.0f, 1}, 90.0f};
    const Material MARBLE  = {{0.25f, 0.25f, 0.24f, 1}, {0.8f, 0.8f, 0.78f, 1}, {0.6f, 0.6f, 0.6f, 1}, 40.0f};
    const Material GOLD    = {{0.35f, 0.25f, 0.05f, 1}, {0.85f, 0.65f, 0.2f, 1}, {0.9f, 0.8f, 0.4f, 1}, 60.0f};
    const Material STONE   = {{0.18f, 0.16f, 0.14f, 1}, {0.58f, 0.54f, 0.48f, 1}, {0.25f, 0.24f, 0.22f, 1}, 8.0f};
    const Material BRICK   = {{0.20f, 0.12f, 0.09f, 1}, {0.62f, 0.42f, 0.32f, 1}, {0.15f, 0.14f, 0.12f, 1}, 6.0f};
    const Material LEAF    = {{0.05f, 0.10f, 0.04f, 1}, {0.16f, 0.34f, 0.14f, 1}, {0.05f, 0.08f, 0.04f, 1}, 3.0f};
    const Material POT     = {{0.14f, 0.08f, 0.05f, 1}, {0.48f, 0.28f, 0.18f, 1}, {0.15f, 0.12f, 0.10f, 1}, 6.0f};
    const Material FIRE    = {{0.4f, 0.1f, 0.02f, 1}, {1.0f, 0.55f, 0.15f, 1}, {0.0f, 0.0f, 0.0f, 1}, 1.0f};
    const Material BULB    = {{0.4f, 0.35f, 0.2f, 1}, {1.0f, 0.92f, 0.65f, 1}, {0.8f, 0.75f, 0.5f, 1}, 20.0f};
    const Material BOOK    = {{0.2f, 0.2f, 0.2f, 1}, {0.75f, 0.75f, 0.75f, 1}, {0.06f, 0.06f, 0.06f, 1}, 4.0f};

    // Tinted copies for colorful variations (fabric, books, etc.)
    Material Tinted(const Material &m, float r, float g, float b);
}

// OpenGL light management (GL_LIGHT0..GL_LIGHT7).
void Lighting_Init();
void Lighting_Apply();

// Register a light slot with world position and warm/cool color.
void Lighting_SetPointLight(int glLightId, float x, float y, float z,
                            float r, float g, float b, float intensity);
void Lighting_SetDirectionalLight(int glLightId,
                                  float dx, float dy, float dz,
                                  float r, float g, float b, float intensity);

// Convenience: apply a Material struct to the current OpenGL state.
void Lighting_ApplyMaterial(const Material &m);

// Per-room global ambient boost (values are ADDED to the built-in warm
// ambient). Call once per frame from the current room's light update;
// reset to 0 in every other room. Lets darker interiors (cellar, night
// garden) stay clearly visible without inflating every light slot.
void Lighting_SetAmbientBoost(float r, float g, float b);
void Lighting_DisableAll();

// Flashlight: an OpenGL spot light that rides with the camera.
// Pass the camera's forward vector; call after the view transform is set.
void Lighting_SetFlashlight(bool on, float px, float py, float pz,
                            float dx, float dy, float dz);

#endif // LIGHTING_H
