// ============================================================
//  THE LAST NIGHT - Basement.cpp
//  The old wine cellar beneath the lawn. Warm amber candlelight
//  everywhere - bright, colorful, lived-in. Contents: stone
//  vaults with beams, wine racks + barrels + storage shelves,
//  a work table with candles, copper pipes, the carved crest
//  with THREE rotating dials (the mechanism), the sliding stone
//  panel, a short passage with the note + second photograph,
//  and the locked secret door with the mansion crest.
//
//  Light slots while the player is here: L0 chandelier lamp,
//  L1 candle clusters, L2 work-table lamp, L6 cool fill.
// ============================================================
#include "Basement.h"
#include "Objects.h"
#include "Texture.h"
#include "Lighting.h"
#include "Collision.h"
#include "Interaction.h"
#include "Progress.h"
#include "Weather.h"
#include <GL/freeglut.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace
{
    // ---- world extents (must match Garden.cpp) ---------------------------
    const float X0 = 14.5f, X1 = 27.2f;      // hall
    const float Z0 = -4.5f, Z1 =  0.8f;
    const float FLOOR = -3.35f;
    const float CEIL  = -0.55f;              // 2.8 m clear height
    const float H = CEIL - FLOOR;

    // passage (behind the hidden panel)
    const float P_X0 = 27.2f, P_X1 = 31.3f;
    // Align the passage with the panel opening in the east basement wall
    // (-2.75..-0.95), leaving a player-width connection after it opens.
    const float P_Z0 = -2.6f, P_Z1 =  0.3f;
    // Matches the Secret Room's west-wall doorway.
    const float SECRET_DOOR_Z0 = -0.9f, SECRET_DOOR_Z1 = 0.3f;

    // secret door swing (Phase 10; drawn by DrawPassage via Progress flag)
    float g_secretAngle = 0.0f;
    bool  g_secretOpening = false;
    int   g_secretDoorSlot = -1;

    // the mechanism wall (east wall of the hall)
    const float MECH_X = X1;
    const float DIAL_R = 2.2f;               // interaction radius (consistent reach)

    // dial states (each press rotates 90 deg; alignment opens the panel)
    float g_dialAngle[3] = {0, 0, 0};        // 0, 90, 180, 270
    bool  g_dialSolved[3] = {false, false, false};
    const float DIAL_TARGET[3] = {180.0f, 90.0f, 270.0f};
    bool  g_mechSolved = false;

    // hidden panel slide (x offset when open)
    float g_panelX = 0.0f;                   // 0 = closed .. 1.35 = open
    bool  g_panelOpening = false;
    int   g_panelSlot = -1;

    // ---- layout anchors ---------------------------------------------------
    const float TABLE_X = 18.5f, TABLE_Z = -2.0f;   // work table
    const float RACK_X  = 22.0f, RACK_Z  = Z0 + 0.45f; // wine racks (north wall)
    const float LAMP_X  = (X0 + X1) / 2, LAMP_Z = (Z0 + Z1) / 2;

    void SetSurface(const Material &m, const char *tex)
    {
        Lighting_ApplyMaterial(m);
        if (tex) Textures::Bind(tex);
    }

    void Box(const Material &m, const char *tex,
             float cx, float cy, float cz, float sx, float sy, float sz)
    {
        glPushMatrix();
        glTranslatef(cx, cy, cz);
        Objects::TexturedBox(m, tex, sx, sy, sz);
        glPopMatrix();
    }
    void Box(float sx, float sy, float sz) { Objects::Box(sx, sy, sz); }

    // ------------------------------------------------------------------
    //  Shell: floor, vaulted ceiling with beams, stone walls
    // ------------------------------------------------------------------
    void DrawShell()
    {
        // stone slab floor
        glPushMatrix();
        glTranslatef((X0 + X1) / 2, FLOOR - 0.05f, (Z0 + Z1) / 2);
        glScalef((X1 - X0) + 0.4f, 0.1f, (Z1 - Z0) + 0.4f);
        SetSurface(Materials::STONE, "stone");
        glutSolidCube(1);
        glPopMatrix();

        // floor tile variation (a few darker inlay slabs)
        for (int i = 0; i < 6; ++i)
        {
            float tx = X0 + 1.8f + i * 2.1f;
            float tz = Z0 + 1.4f + ((i * 3) % 3) * 1.1f;
            Box(Materials::Tinted(Materials::STONE, 0.8f, 0.78f, 0.75f), "stone",
                tx, FLOOR + 0.005f, tz, 1.0f, 0.02f, 0.9f);
        }

        // ceiling slab
        Box(Materials::STONE, "stone", (X0 + X1) / 2, CEIL + 0.12f, (Z0 + Z1) / 2,
            (X1 - X0) + 0.4f, 0.24f, (Z1 - Z0) + 0.4f);

        // heavy walnut beams across the ceiling
        for (int i = 0; i < 4; ++i)
        {
            glPushMatrix();
            glTranslatef(X0 + 2.4f + i * 3.2f, CEIL - 0.13f, (Z0 + Z1) / 2);
            Objects::SupportBeam((Z1 - Z0) + 0.3f);
            glPopMatrix();
        }

        // walls: west, north, south (east is the mechanism wall)
        Box(Materials::STONE, "stone", X0 - 0.1f, FLOOR + H / 2, (Z0 + Z1) / 2,
            0.2f, H, (Z1 - Z0) + 0.4f);
        Box(Materials::STONE, "stone", (X0 + X1) / 2, FLOOR + H / 2, Z0 - 0.1f,
            (X1 - X0) + 0.4f, H, 0.2f);
        Box(Materials::STONE, "stone", (X0 + P_X0) / 2, FLOOR + H / 2, Z1 + 0.1f,
            (P_X0 - X0) + 0.3f, H, 0.2f);      // south wall west of the stairs
        // (the stair opening occupies x 26.0..27.2 on the south side)

        // the mechanism wall (east): stone face with the carved crest
        Box(Materials::STONE, "stone", X1 + 0.1f, FLOOR + H / 2, (Z0 + Z1) / 2,
            0.2f, H, (Z1 - Z0) + 0.4f);

        // the big carved symbol above the dials
        glPushMatrix();
        glTranslatef(X1 - 0.02f, FLOOR + 2.2f, -1.85f);
        glRotatef(-90, 0, 1, 0);
        Objects::Crest(0.8f, 0.16f, 0.13f, 0.10f, 1.0f);
        glPopMatrix();

        // ---- copper pipes along the north wall ----
        SetSurface(Materials::Tinted(Materials::METAL, 0.85f, 0.55f, 0.3f), 0);
        for (int i = 0; i < 2; ++i)
        {
            float py = FLOOR + 2.3f + i * 0.28f;
            glPushMatrix();
            glTranslatef((X0 + X1) / 2 - 2.0f, py, Z0 + 0.16f);
            glRotatef(90, 1, 0, 0);
            glutSolidCylinder(0.045f, (X1 - X0) - 4.0f, 10, 3);
            glPopMatrix();
        }
        // vertical feed pipe + elbow
        glPushMatrix();
        glTranslatef(X0 + 2.6f, FLOOR + 1.2f, Z0 + 0.16f);
        glutSolidCylinder(0.045f, 2.2f, 10, 3);
        glPopMatrix();
    }

    // ------------------------------------------------------------------
    //  Furnishings: table, racks, shelves, barrels, candles
    // ------------------------------------------------------------------
    void DrawFurniture(float timeSec)
    {
        // ---- work table + objects ----
        glPushMatrix();
        glTranslatef(TABLE_X, 0, TABLE_Z);
        SetSurface(Materials::WOOD, "wood");
        glPushMatrix(); glTranslatef(0, FLOOR + 0.78f, 0); Box(2.0f, 0.08f, 1.1f); glPopMatrix();
        for (int i = 0; i < 4; ++i)
        {
            float lx = (i % 2) ? 0.85f : -0.85f;
            float lz = (i / 2) ? 0.42f : -0.42f;
            glPushMatrix(); glTranslatef(lx, FLOOR + 0.37f, lz);
            Box(0.10f, 0.74f, 0.10f); glPopMatrix();
        }
        // drawer front
        SetSurface(Materials::DARKWOOD, "darkwood");
        glPushMatrix(); glTranslatef(0, FLOOR + 0.55f, 0.56f); Box(0.7f, 0.22f, 0.05f); glPopMatrix();
        glPushMatrix(); glTranslatef(0, FLOOR + 0.55f, 0.60f);
        SetSurface(Materials::GOLD, 0); Box(0.12f, 0.04f, 0.03f); glPopMatrix();

        // ON the table: papers, open book, bottle, candle group (the lamp)
        glPushMatrix(); glTranslatef(-0.5f, FLOOR + 0.82f, 0.1f); glRotatef(-18, 0, 1, 0);
        SetSurface(Materials::Tinted(Materials::BOOK, 1.05f, 1.0f, 0.9f), 0);
        Box(0.32f, 0.02f, 0.24f); glPopMatrix();
        glPushMatrix(); glTranslatef(0.4f, FLOOR + 0.83f, -0.15f); glRotatef(24, 0, 1, 0);
        SetSurface(Materials::Tinted(Materials::BOOK, 0.95f, 0.85f, 0.65f), "books");
        Box(0.36f, 0.06f, 0.26f); glPopMatrix();
        glPushMatrix(); glTranslatef(0.15f, FLOOR + 0.86f, 0.25f);
        Objects::Bottle(1); glPopMatrix();
        glPushMatrix(); glTranslatef(0.55f, FLOOR + 0.80f, 0.3f);
        Objects::CandleGroup(timeSec);
        glPopMatrix();
        glPopMatrix();

        // ---- wine racks along the north wall (colorful bottles) ----
        SetSurface(Materials::DARKWOOD, "darkwood");
        for (int i = 0; i < 2; ++i)
        {
            glPushMatrix();
            glTranslatef(RACK_X + i * 2.6f, FLOOR, RACK_Z);
            // diamond wine rack: frame + rows of bottles
            Box(2.2f, 0.08f, 0.5f);   // base
            glPushMatrix(); glTranslatef(-1.05f, 0.9f, 0); Box(0.08f, 1.8f, 0.5f); glPopMatrix();
            glPushMatrix(); glTranslatef( 1.05f, 0.9f, 0); Box(0.08f, 1.8f, 0.5f); glPopMatrix();
            glPushMatrix(); glTranslatef(0, 1.84f, 0); Box(2.2f, 0.08f, 0.5f); glPopMatrix();
            // bottle rows lying in the rack
            for (int row = 0; row < 3; ++row)
            {
                float ry = 0.45f + row * 0.52f;
                for (int b = 0; b < 5; ++b)
                {
                    float bx = -0.75f + b * 0.38f + 0.04f * ((row + b) % 3);
                    glPushMatrix();
                    glTranslatef(bx, ry, 0);
                    glRotatef(90, 0, 0, 1);
                    glScalef(1.3f, 1.3f, 1.0f);
                    Objects::Bottle((row * 5 + b + i) % 4);
                    glPopMatrix();
                }
            }
            glPopMatrix();
        }

        // ---- storage shelves (west wall) ----
        glPushMatrix();
        glTranslatef(X0 + 0.30f, FLOOR, -2.6f);
        glRotatef(90, 0, 1, 0);
        Objects::StorageShelf(2.6f, 2.0f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(X0 + 0.30f, FLOOR, 0.6f);
        glRotatef(90, 0, 1, 0);
        Objects::StorageShelf(2.2f, 1.8f);
        glPopMatrix();

        // ---- barrels (south-west corner) ----
        glPushMatrix(); glTranslatef(16.0f, FLOOR, Z1 - 0.75f); Objects::Barrel(0.32f, 0.85f); glPopMatrix();
        glPushMatrix(); glTranslatef(16.8f, FLOOR, Z1 - 0.7f);  glRotatef(14, 0, 1, 0);
        Objects::Barrel(0.30f, 0.8f); glPopMatrix();
        glPushMatrix(); glTranslatef(16.3f, FLOOR + 0.85f, Z1 - 0.75f);
        Objects::Barrel(0.28f, 0.7f); glPopMatrix();

        // ---- candle clusters (wall + floor) ----
        glPushMatrix(); glTranslatef(20.8f, FLOOR + 0.02f, 0.2f); Objects::CandleGroup(timeSec); glPopMatrix();
        glPushMatrix(); glTranslatef(25.0f, FLOOR + 0.02f, -3.4f); Objects::CandleGroup(timeSec + 0.9f); glPopMatrix();
        // wall sconce candles near the mechanism
        SetSurface(Materials::GOLD, 0);
        glPushMatrix(); glTranslatef(X1 - 0.12f, FLOOR + 1.5f, -3.4f);
        Objects::CandleGroup(timeSec + 1.7f); glPopMatrix();
        glPushMatrix(); glTranslatef(X1 - 0.12f, FLOOR + 1.5f, -0.2f);
        Objects::CandleGroup(timeSec + 2.4f); glPopMatrix();

        // ---- hanging lamp above the center (the L0 fixture) ----
        glPushMatrix();
        glTranslatef(LAMP_X, CEIL, LAMP_Z);
        SetSurface(Materials::Tinted(Materials::METAL, 0.4f, 0.32f, 0.22f), 0);
        glPushMatrix(); glTranslatef(0, -0.5f, 0); Box(0.03f, 1.0f, 0.03f); glPopMatrix();
        glPushMatrix(); glTranslatef(0, -1.05f, 0);
        SetSurface(Materials::BULB, 0);
        glutSolidSphere(0.12f, 12, 9);
        SetSurface(Materials::Tinted(Materials::METAL, 0.4f, 0.32f, 0.22f), 0);
        glPushMatrix(); glTranslatef(0, 0.16f, 0); glutSolidTorus(0.02f, 0.14f, 8, 16); glPopMatrix();
        glPopMatrix();
        glPopMatrix();
    }

    // ------------------------------------------------------------------
    //  The mechanism: three crest dials + the sliding stone panel
    // ------------------------------------------------------------------
    void DrawMechanism()
    {
        // three dials in a row on the east wall
        const float dz[3] = { -2.6f, -1.85f, -1.1f };
        for (int i = 0; i < 3; ++i)
        {
            glPushMatrix();
            glTranslatef(X1 - 0.045f, FLOOR + 1.45f, dz[i]);
            glRotatef(-90, 0, 1, 0);
            Objects::CrestDial(0.52f, g_dialAngle[i],
                               g_mechSolved || g_dialSolved[i]);
            glPopMatrix();
        }
        // a small brass plaque above: "three marks as one"
        glPushMatrix();
        glTranslatef(X1 - 0.03f, FLOOR + 2.05f, -1.85f);
        glRotatef(-90, 0, 1, 0);
        SetSurface(Materials::GOLD, 0);
        Box(0.6f, 0.14f, 0.02f);
        glPopMatrix();

        // ---- the hidden stone panel (slides west into the wall) ----
        // Closed: flush at x = X1. Open: slid 1.35 into the hall.
        float px = X1 - 0.06f - g_panelX;
        // panel blocks the passage doorway (z -0.65..0.65 region of the wall)
        Box(Materials::STONE, "stone", px, FLOOR + 1.45f, -1.85f,
            0.12f, 2.9f, 1.5f);
        // seam + faint crest engraved on the panel
        glPushMatrix();
        glTranslatef(px - 0.065f, FLOOR + 1.45f, -1.85f);
        glRotatef(-90, 0, 1, 0);
        Objects::Crest(0.5f, g_panelX > 0.05f ? 0.4f : 0.14f,
                       g_panelX > 0.05f ? 0.32f : 0.12f,
                       g_panelX > 0.05f ? 0.10f : 0.09f, 1.0f);
        glPopMatrix();
    }

    // ------------------------------------------------------------------
    //  The passage: note + photograph + the locked secret door
    // ------------------------------------------------------------------
    void DrawPassage(float timeSec)
    {
        if (g_panelX < 0.05f) return;       // hidden until the panel opens

        // passage shell (stone)
        glPushMatrix();
        glTranslatef((P_X0 + P_X1) / 2, FLOOR - 0.05f, (P_Z0 + P_Z1) / 2);
        glScalef((P_X1 - P_X0), 0.1f, (P_Z1 - P_Z0));
        SetSurface(Materials::STONE, "stone");
        glutSolidCube(1);
        glPopMatrix();
        Box(Materials::STONE, "stone", (P_X0 + P_X1) / 2, CEIL + 0.12f, (P_Z0 + P_Z1) / 2,
            (P_X1 - P_X0), 0.24f, (P_Z1 - P_Z0));
        Box(Materials::STONE, "stone", (P_X0 + P_X1) / 2, FLOOR + H / 2, P_Z0 - 0.1f,
            (P_X1 - P_X0), H, 0.2f);
        Box(Materials::STONE, "stone", (P_X0 + P_X1) / 2, FLOOR + H / 2, P_Z1 + 0.1f,
            (P_X1 - P_X0), H, 0.2f);
        Box(Materials::STONE, "stone", P_X1 + 0.1f, FLOOR + H / 2, (P_Z0 + P_Z1) / 2,
            0.2f, H, (P_Z1 - P_Z0) + 0.4f);

        // small table with the note + the second photograph
        glPushMatrix();
        glTranslatef(30.2f, FLOOR, -0.5f);
        SetSurface(Materials::DARKWOOD, "darkwood");
        glPushMatrix(); glTranslatef(0, 0.74f, 0); Box(1.0f, 0.06f, 0.6f); glPopMatrix();
        for (int i = 0; i < 4; ++i)
        {
            float lx = (i % 2) ? 0.4f : -0.4f;
            float lz = (i / 2) ? 0.2f : -0.2f;
            glPushMatrix(); glTranslatef(lx, 0.37f, lz); Box(0.07f, 0.74f, 0.07f); glPopMatrix();
        }
        // the note (cream paper)
        glPushMatrix(); glTranslatef(-0.15f, 0.78f, 0.05f); glRotatef(16, 0, 1, 0);
        SetSurface(Materials::Tinted(Materials::BOOK, 1.08f, 1.04f, 0.92f), 0);
        Box(0.26f, 0.015f, 0.19f); glPopMatrix();
        // framed photograph leaning against the wall
        glPushMatrix(); glTranslatef(0.25f, 0.87f, -0.18f); glRotatef(-70, 1, 0, 0);
        Objects::PictureFrame(0.26f, 0.2f); glPopMatrix();
        // candle on the table
        glPushMatrix(); glTranslatef(0.05f, 0.77f, 0.18f); Objects::Candle(0.12f, true, timeSec); glPopMatrix();
        glPopMatrix();

        // candles on passage brackets
        glPushMatrix(); glTranslatef(29.0f, FLOOR + 1.4f, P_Z0 + 0.15f);
        Objects::CandleGroup(timeSec + 0.5f); glPopMatrix();

        // ---- the SECRET DOOR at the passage's west end ----
        // dark oak, iron bands, the mansion crest in gold. When the player
        // has the master key (Phase 10 returns them here with it), the door
        // stands open: the panel swings into the secret room on a hinge at
        // its north edge.
        if (Progress::Get(Progress::SECRETDOOR_OPEN))
        {
            glPushMatrix();
            glTranslatef(P_X1, FLOOR, P_Z1 - 0.05f);
            glRotatef(90.0f - g_secretAngle, 0, 1, 0);
            glRotatef(90, 0, 1, 0);
            Objects::Door(1.4f, 2.9f);
            glPopMatrix();
        }
        else
        {
            Box(Materials::DARKWOOD, "darkwood", P_X1 + 0.05f, FLOOR + 1.45f, (P_Z0 + P_Z1) / 2,
                0.1f, 2.9f, 1.4f);
            SetSurface(Materials::Tinted(Materials::METAL, 0.35f, 0.3f, 0.25f), 0);
            glPushMatrix(); glTranslatef(P_X1 - 0.005f, FLOOR + 0.55f, (P_Z0 + P_Z1) / 2);
            Box(0.03f, 0.1f, 1.3f); glPopMatrix();
            glPushMatrix(); glTranslatef(P_X1 - 0.005f, FLOOR + 2.3f, (P_Z0 + P_Z1) / 2);
            Box(0.03f, 0.1f, 1.3f); glPopMatrix();
            glPushMatrix();
            glTranslatef(P_X1 - 0.02f, FLOOR + 1.5f, (P_Z0 + P_Z1) / 2);
            glRotatef(-90, 0, 1, 0);
            Objects::Crest(0.42f, 0.75f, 0.55f, 0.18f, 1.0f);
            glPopMatrix();
        }
    }
}

// ======================================================================
//  Basement namespace
// ======================================================================
namespace Basement
{
    void SetSecretDoorOpen(bool open) { g_secretOpening = open; }

    void UpdateSecretDoor(float dt)
    {
        if (g_secretOpening && g_secretAngle < 100.0f)
            g_secretAngle += 120.0f * dt;
        else if (!g_secretOpening && g_secretAngle > 0.0f)
            g_secretAngle -= 120.0f * dt;
        if (g_secretAngle < 0) g_secretAngle = 0;
        if (g_secretAngle > 100.0f) g_secretAngle = 100.0f;

        if (g_secretDoorSlot >= 0)
        {
            if (g_secretAngle > 95.0f)
                Collision::SetDynamicBox(g_secretDoorSlot, false, 0, 0, 0, 0);
            else
                Collision::SetDynamicBox(g_secretDoorSlot, true,
                    P_X1 - 0.1f, SECRET_DOOR_Z0, P_X1 + 0.2f, SECRET_DOOR_Z1);
        }
    }
    void SetPanelOpen(bool open) { g_panelOpening = open; }
    bool IsPanelOpen()           { return g_panelX > 1.3f; }

    void TurnDial(int which)
    {
        if (which < 0 || which > 2) return;
        if (g_mechSolved) return;            // already open - no relocking
        g_dialAngle[which] += 90.0f;
        if (g_dialAngle[which] >= 360.0f) g_dialAngle[which] -= 360.0f;
        g_dialSolved[which] =
            (g_dialAngle[which] == DIAL_TARGET[which]);
    }

    bool DialAligned(int which)
    {
        if (which < 0 || which > 2) return false;
        return g_dialSolved[which];
    }

    void Reset()
    {
        for (int i = 0; i < 3; ++i) { g_dialAngle[i] = 0.0f; g_dialSolved[i] = false; }
        g_mechSolved = false;
        g_panelX = 0.0f; g_panelOpening = false;
        g_secretAngle = 0.0f; g_secretOpening = false;
    }

    bool MechanismJustSolved()
    {
        if (g_mechSolved) return false;
        if (g_dialSolved[0] && g_dialSolved[1] && g_dialSolved[2])
        {
            g_mechSolved = true;
            return true;
        }
        return false;
    }

    void UpdatePanel(float dt)
    {
        if (g_panelOpening && g_panelX < 1.35f)
            g_panelX += 0.55f * dt;      // slow, heavy stone slide
        else if (!g_panelOpening && g_panelX > 0.0f)
            g_panelX -= 0.55f * dt;
        if (g_panelX < 0) g_panelX = 0;
        if (g_panelX > 1.35f) g_panelX = 1.35f;

        // panel collision: solid while not fully open
        if (g_panelSlot >= 0)
        {
            if (g_panelX > 1.25f)
                Collision::SetDynamicBox(g_panelSlot, false, 0, 0, 0, 0);
            else
                Collision::SetDynamicBox(g_panelSlot, true,
                    X1 - 0.25f - g_panelX, -2.75f, X1 + 0.1f, -0.95f);
        }
    }

    void Load()
    {
        const float t = 0.25f;
        // west wall
        Collision::AddBox(X0 - t, Z0 - t, X0 + t, Z1 + t);
        // north wall
        Collision::AddBox(X0 - t, Z0 - t, X1 + t, Z0 + t);
        // south wall west of the stair opening
        // Match Garden's widened stair trench so the physical descent joins
        // the basement floor without a narrow collision pinch point.
        Collision::AddBox(X0 - t, Z1 - t, 25.6f, Z1 + t);
        // east wall (mechanism) north of the panel / south of the panel
        Collision::AddBox(X1 - t, Z0 - t, X1 + t, -2.75f);
        Collision::AddBox(X1 - t, -0.95f, X1 + t, Z1 + t);
        // hidden panel (solid while closed) - dynamic slot
        Collision::AddBox(X1 - 0.25f, -2.75f, X1 + 0.1f, -0.95f);
        g_panelSlot = Collision::ClaimDynamic();
        // Secret Room door: dynamic blocker, disabled with its release
        // animation instead of leaving an invisible static wall behind.
        Collision::AddBox(P_X1 - 0.1f, SECRET_DOOR_Z0,
                          P_X1 + 0.2f, SECRET_DOOR_Z1);
        g_secretDoorSlot = Collision::ClaimDynamic();
        // passage walls
        Collision::AddBox(P_X0, P_Z0 - 0.3f, P_X1 + 0.4f, P_Z0);
        Collision::AddBox(P_X0, P_Z1, P_X1 + 0.4f, P_Z1 + 0.3f);

        // furniture
        Collision::AddBox(TABLE_X - 1.0f, TABLE_Z - 0.55f, TABLE_X + 1.0f, TABLE_Z + 0.55f);
        Collision::AddBox(RACK_X - 1.15f, RACK_Z - 0.3f, RACK_X + 3.75f, RACK_Z + 0.3f);
        Collision::AddBox(X0 + 0.2f, -3.9f, X0 + 0.75f, -1.3f);   // shelves W
        Collision::AddBox(15.6f, Z1 - 1.0f, 17.2f, Z1 - 0.4f);    // barrels

        // ---- interactables ----
        Interaction::Add(Interaction::ID_DIAL1, X1 - 0.4f, FLOOR + 1.45f, -2.6f,
                         DIAL_R, "Turn the Left Dial");
        Interaction::Add(Interaction::ID_DIAL2, X1 - 0.4f, FLOOR + 1.45f, -1.85f,
                         DIAL_R, "Turn the Middle Dial");
        Interaction::Add(Interaction::ID_DIAL3, X1 - 0.4f, FLOOR + 1.45f, -1.1f,
                         DIAL_R, "Turn the Right Dial");
        // Interaction focus is evaluated in XZ only. The crest is directly
        // above the middle dial at the same XZ position, so register it
        // after the dial; equal-distance focus now correctly favors the
        // actionable middle dial rather than this flavor examination.
        Interaction::Add(Interaction::ID_SYMBOL, X1 - 0.4f, FLOOR + 2.2f, -1.85f,
                         DIAL_R, "Examine the Symbol");
        // The crest is decorative and directly above the middle dial. Focus
        // selection is XZ-only, so leaving this enabled can mask the dial
        // regardless of render height. Keep the puzzle controls unambiguous.
        Interaction::SetEnabled(Interaction::ID_SYMBOL, false);
        Interaction::Add(Interaction::ID_CANDLES, TABLE_X + 0.55f, FLOOR + 0.95f,
                         TABLE_Z + 0.3f, 1.6f, "Examine Candles");
        Interaction::Add(Interaction::ID_BARRELS, 16.4f, FLOOR + 0.5f, Z1 - 0.7f,
                         1.7f, "Examine Barrels");
        Interaction::Add(Interaction::ID_WINE, RACK_X + 1.3f, FLOOR + 1.0f, RACK_Z,
                         1.8f, "Examine the Wine");
        Interaction::Add(Interaction::ID_PASNOTE, 30.05f, FLOOR + 0.85f, -0.45f,
                         1.6f, "Read the Note");
        Interaction::Add(Interaction::ID_PASPHOTO, 30.45f, FLOOR + 0.95f, -0.68f,
                         1.6f, "Examine Photograph");
        Interaction::Add(Interaction::ID_SECRETDOOR, P_X1 - 0.3f, FLOOR + 1.4f,
                         (P_Z0 + P_Z1) / 2, 1.8f, "Examine the Door");
        // Matches the Garden-side fallback entry and returns the player to
        // the accessible approach just outside the cellar door.
        Interaction::Add(Interaction::ID_RETURNGARDEN, 26.4f, FLOOR + 1.2f,
                         0.2f, 1.8f, "Return to Garden");
    }

    void UpdateLights(float timeSec)
    {
        // L0: hanging lamp - warm golden center
        Lighting_SetPointLight(GL_LIGHT0, LAMP_X, CEIL - 1.1f, LAMP_Z,
                               1.0f, 0.85f, 0.55f, 1.1f);
        // L1: candle clusters (gentle flicker)
        const float fl = 0.9f + 0.1f * Objects::FireFlicker(timeSec);
        Lighting_SetPointLight(GL_LIGHT1, 22.5f, FLOOR + 1.2f, -1.4f,
                               1.0f, 0.68f, 0.34f, 0.85f * fl);
        // L2: work-table candle glow
        Lighting_SetPointLight(GL_LIGHT2, TABLE_X + 0.55f, FLOOR + 1.0f, TABLE_Z + 0.3f,
                               1.0f, 0.72f, 0.38f, 0.8f * fl);
    }

    void UpdateFill()
    {
        // L6: soft cool fill (old window slits high on the west wall)
        Lighting_SetPointLight(GL_LIGHT6, X0 + 1.5f, CEIL - 0.4f, -2.0f,
                               0.55f, 0.65f, 0.9f, 0.55f);
    }

    void Draw(float timeSec, float, float)
    {
        DrawShell();
        DrawFurniture(timeSec);
        DrawMechanism();
        DrawPassage(timeSec);
    }
}
