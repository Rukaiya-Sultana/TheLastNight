// ============================================================
//  THE LAST NIGHT - Lighting.cpp
//  Each scene file (Lobby.cpp, Garden.cpp, ...) registers its
//  lights here in later phases. GL_LIGHT0..7 are the only lights
//  available in fixed-function OpenGL, so the registry keeps
//  ownership of all eight slots in one place.
// ============================================================
#include "Lighting.h"
#include <GL/freeglut.h>

namespace
{
    struct LightState
    {
        float color[4];
        float position[4]; // w = 0 -> directional, w = 1 -> point light
        bool  enabled;
    };

    LightState g_lights[8];
    bool g_initialized = false;

    // Soft warm-neutral global ambient: keeps the whole mansion visible
    // even in unlit corners (the game must never be pitch black).
    const float GLOBAL_AMBIENT[4] = {0.36f, 0.34f, 0.31f, 1.0f};
}

void Lighting_Init()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, GLOBAL_AMBIENT);

    for (int i = 0; i < 8; ++i)
    {
        g_lights[i].enabled   = false;
        g_lights[i].color[0]  = 0.0f;
        g_lights[i].color[1]  = 0.0f;
        g_lights[i].color[2]  = 0.0f;
        g_lights[i].color[3]  = 1.0f;
        g_lights[i].position[0] = 0.0f;
        g_lights[i].position[1] = 1.0f;
        g_lights[i].position[2] = 0.0f;
        g_lights[i].position[3] = 0.0f;
    }
    g_initialized = true;
}

void Lighting_SetPointLight(int glLightId, float x, float y, float z,
                            float r, float g, float b, float intensity)
{
    if (!g_initialized) Lighting_Init();

    int idx = glLightId - GL_LIGHT0;
    if (idx < 0 || idx >= 8) return;

    g_lights[idx].color[0] = r * intensity;
    g_lights[idx].color[1] = g * intensity;
    g_lights[idx].color[2] = b * intensity;
    g_lights[idx].color[3] = 1.0f;
    g_lights[idx].position[0] = x;
    g_lights[idx].position[1] = y;
    g_lights[idx].position[2] = z;
    g_lights[idx].position[3] = 1.0f; // point light
    g_lights[idx].enabled = true;
}

void Lighting_SetDirectionalLight(int glLightId, float dx, float dy, float dz,
                                  float r, float g, float b, float intensity)
{
    if (!g_initialized) Lighting_Init();

    int idx = glLightId - GL_LIGHT0;
    if (idx < 0 || idx >= 8) return;

    g_lights[idx].color[0] = r * intensity;
    g_lights[idx].color[1] = g * intensity;
    g_lights[idx].color[2] = b * intensity;
    g_lights[idx].color[3] = 1.0f;
    g_lights[idx].position[0] = dx;
    g_lights[idx].position[1] = dy;
    g_lights[idx].position[2] = dz;
    g_lights[idx].position[3] = 0.0f; // directional
    g_lights[idx].enabled = true;
}

void Lighting_ApplyMaterial(const Material &m)
{
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  m.ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  m.diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, m.specular);
    glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, m.shininess);
}

namespace
{
    float g_ambientBoost[3] = {0.0f, 0.0f, 0.0f};
}

void Lighting_DisableAll()
{
    for (int i = 0; i < 8; ++i)
        g_lights[i].enabled = false;
}

void Lighting_SetAmbientBoost(float r, float g, float b)
{
    g_ambientBoost[0] = r;
    g_ambientBoost[1] = g;
    g_ambientBoost[2] = b;
    const float a[4] = {0.36f + r, 0.34f + g, 0.31f + b, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, a);
}

Material Materials::Tinted(const Material &m, float r, float g, float b)
{
    Material out = m;
    out.ambient[0]  = m.ambient[0]*r;  out.ambient[1]  = m.ambient[1]*g;  out.ambient[2]  = m.ambient[2]*b;
    out.diffuse[0]  = m.diffuse[0]*r;  out.diffuse[1]  = m.diffuse[1]*g;  out.diffuse[2]  = m.diffuse[2]*b;
    out.specular[0] = m.specular[0]*r; out.specular[1] = m.specular[1]*g; out.specular[2] = m.specular[2]*b;
    return out;
}

// ---- flashlight: GL_LIGHT7 as a camera-attached spot light --------------
void Lighting_SetFlashlight(bool on, float px, float py, float pz,
                            float dx, float dy, float dz)
{
    const int id = GL_LIGHT7;
    if (!on)
    {
        glDisable(id);
        return;
    }

    // The flashlight is camera-attached, so it is specified directly in
    // EYE space: the lens sits at the eye origin and the beam points
    // straight down the view axis (-z). World-space coordinates would
    // have to be counter-rotated by the view matrix (the world forward
    // vector is only the eye forward at yaw 0 - at other yaws the beam
    // pointed behind the camera and the light seemed dead). Eye-space
    // constants need no math and cannot drift out of sync with the view;
    // pitch is included automatically because the view looks along its
    // own forward.
    (void)px; (void)py; (void)pz; (void)dx; (void)dy; (void)dz;
    const float pos[4]   = { 0.0f, 0.0f, 0.0f, 1.0f };  // at the eyes
    const float dir[3]   = { 0.0f, 0.0f, -1.0f };       // along the view
    const float warm[4]  = { 1.0f, 0.97f, 0.86f, 1.0f };
    const float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

    // Same bracket as Lighting_Apply: glLightfv transforms GL_POSITION
    // (and the spot direction) by the CURRENT modelview. The game passes
    // world coordinates, so they must be applied while the modelview is
    // identity - otherwise the camera view matrix relocates the beam and
    // the flashlight appears to do nothing.
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glLightfv(id, GL_POSITION,     pos);
    glLightfv(id, GL_SPOT_DIRECTION, dir);
        // This engine lights per-vertex (Gouraud) and the wall geometry is
    // coarse, so a narrow spotlight cone can fall entirely between
    // vertices: every corner evaluates to zero and the whole quad stays
    // dark (the flashlight "did nothing" bug). A wide cone + gentle
    // exponent guarantees nearby vertices catch the beam and the
    // interpolation reads as a realistic spill.
    glLightf (id, GL_SPOT_CUTOFF,   45.0f);
    glLightf (id, GL_SPOT_EXPONENT, 6.0f);
    glLightf (id, GL_CONSTANT_ATTENUATION,  0.50f);
    glLightf (id, GL_LINEAR_ATTENUATION,    0.07f);
    glLightf (id, GL_QUADRATIC_ATTENUATION, 0.015f);
    glLightfv(id, GL_DIFFUSE,  warm);
    glLightfv(id, GL_SPECULAR, warm);
    glLightfv(id, GL_AMBIENT,  black);
    glEnable(id);

    glPopMatrix();
}

void Lighting_Apply()
{
    // Light positions are specified in WORLD coordinates, but OpenGL
    // transforms GL_POSITION by the current modelview matrix at the time
    // of the call. Apply them while the modelview is IDENTITY (projection
    // only) so world == eye; the game calls this right after
    // Camera::setView() as a separate matrix-mode bracket.
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    for (int i = 0; 1; ++i)
    {
        if (i >= 8) break;
        int id = GL_LIGHT0 + i;
        if (g_lights[i].enabled)
        {
            glLightfv(id, GL_POSITION, g_lights[i].position);
            glLightfv(id, GL_DIFFUSE,  g_lights[i].color);
            glLightfv(id, GL_SPECULAR, g_lights[i].color);
            glEnable(id);
        }
        else
        {
            glDisable(id);
        }
    }

    glPopMatrix();
}
