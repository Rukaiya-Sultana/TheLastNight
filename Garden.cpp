// ============================================================
//  THE LAST NIGHT - Garden.cpp
//  The mansion courtyard, EAST of the lobby. Entered through the
//  lobby's east door (opened with the mansion key). Stone patio,
//  main path, animated fountain with the engraved plaque, statue
//  with the hidden plate, flower beds, hedges, benches, lanterns,
//  trees, the perimeter wall and the cellar annex whose stairs
//  descend to the Basement.
//
//  World layout (continuous world):
//    Garden lawn:  x [8.4, 28.4],  z [-9.25, 9.25]
//    Mansion facade: x = 8.2 (lobby's east wall exterior)
//    Door opening: z [-0.6, 0.6]  (the lobby's garden door)
//    Fountain:     (16.5, 0);  Statue: (22.6, 4.6)
//    Cellar annex: x [25.4, 29.0], z [6.4, 9.0], floor y -0.35
//    Stairs:       x [25.6, 27.2], z [4.0, 8.8] down to y -3.35
//    Basement hall: x [13, 25], z [-4, 6], floor y -3.35
//
//  Light slots while the player is here: L0 fountain glow,
//  L1 path lanterns, L2 mansion warm spill, L6 cool moon fill.
// ============================================================
#include "Garden.h"
#include "Objects.h"
#include "Texture.h"
#include "Lighting.h"
#include "Collision.h"
#include "Interaction.h"
#include "Progress.h"
#include "Weather.h"
#include "Player.h"
#include <GL/freeglut.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace
{
    // ---- world extents ---------------------------------------------------
    const float GX0 = 8.4f,  GX1 = 28.4f;       // lawn
    const float GZ0 = -9.25f, GZ1 = 9.25f;
    const float WALL_H = 2.6f;                  // perimeter wall height

    const float PAT_X0 = 8.4f, PAT_X1 = 12.4f;  // stone patio
    const float PAT_Z0 = -3.0f, PAT_Z1 = 3.0f;

    const float FOUNT_X = 16.5f, FOUNT_Z = 0.0f;
    const float STAT_X  = 22.6f, STAT_Z  = 4.6f;

    const float MW_X = 8.2f;                    // mansion facade plane
    const float DOOR_Z0 = -0.6f, DOOR_Z1 = 0.6f;// garden door opening

    // cellar annex (north-east corner): a small sunken vestibule
    const float ANN_X0 = 25.4f, ANN_X1 = 29.0f;
    const float ANN_Z0 = 5.6f,  ANN_Z1 = 8.8f;
    const float ANN_FLOOR_Y = -0.35f;
    // Keep a comfortable player-width route from the garden, through the
    // annex, and down the stair trench.  The former 1.2-unit aperture left
    // only 0.5 units between the expanded wall colliders.
    const float CELL_D0 = 25.6f, CELL_D1 = 27.2f;   // door opening (x)

    // stair trench: continues straight south from the cellar door,
    // descending under the lawn to the basement hall
    const float ST_X0 = 25.6f, ST_X1 = 27.2f;
    const float ST_ZTOP = 5.6f, ST_ZBOT = 0.8f;     // run 4.8, drop 3.0
    const float BAS_FLOOR_Y = -3.35f;
    // basement hall footprint (under the lawn, entered at the trench bottom)
    const float BAS_X0 = 14.5f, BAS_X1 = 27.2f;
    const float BAS_Z0 = -4.5f, BAS_Z1 = 0.8f;

    float g_cellarAngle = 0.0f;
    const float CELLAR_OPEN_ANGLE = 100.0f;

    // front entrance (Phase 10): facade gap closed until the doors open.
    // The lobby's swinging leaves have their own dynamic slots; this one
    // blocks the gap in the garden-side wall while they are shut.
    int g_frontGapSlot = -1;
    int   g_cellarSlot = -1;
    bool  g_cellarOpening = false;

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
    //  Sky + moon + distant trees (self-lit; lightning brightens it)
    // ------------------------------------------------------------------
    void DrawSky()
    {
        const float f = Weather::FlashLevel();
        const float skyTop[3]  = {0.10f + 0.45f * f, 0.14f + 0.5f * f, 0.30f + 0.55f * f};
        const float skyHor[3]  = {0.30f + 0.5f * f,  0.38f + 0.5f * f, 0.62f + 0.35f * f};

        glDisable(GL_LIGHTING);
        glDepthMask(GL_FALSE);
        Textures::Bind("nightsky");
        glBegin(GL_QUADS);
            glColor3f(skyTop[0], skyTop[1], skyTop[2]);
            glVertex3f(GX0 - 24, 26, GZ0 - 26); glVertex3f(GX1 + 24, 26, GZ0 - 26);
            glColor3f(skyHor[0], skyHor[1], skyHor[2]);
            glVertex3f(GX1 + 24, 0, GZ0 - 26);  glVertex3f(GX0 - 24, 0, GZ0 - 26);
        glEnd();
        glBegin(GL_QUADS);
            glColor3f(skyTop[0], skyTop[1], skyTop[2]);
            glVertex3f(GX1 + 24, 26, GZ1 + 26); glVertex3f(GX0 - 24, 26, GZ1 + 26);
            glColor3f(skyHor[0], skyHor[1], skyHor[2]);
            glVertex3f(GX0 - 24, 0, GZ1 + 26);  glVertex3f(GX1 + 24, 0, GZ1 + 26);
        glEnd();
        glBegin(GL_QUADS);
            glColor3f(skyTop[0], skyTop[1], skyTop[2]);
            glVertex3f(GX0 - 24, 26, GZ1 + 26); glVertex3f(GX0 - 24, 26, GZ0 - 26);
            glColor3f(skyHor[0], skyHor[1], skyHor[2]);
            glVertex3f(GX0 - 24, 0, GZ0 - 26);  glVertex3f(GX0 - 24, 0, GZ1 + 26);
        glEnd();
        glBegin(GL_QUADS);
            glColor3f(skyTop[0], skyTop[1], skyTop[2]);
            glVertex3f(GX1 + 24, 26, GZ0 - 26); glVertex3f(GX1 + 24, 26, GZ1 + 26);
            glColor3f(skyHor[0], skyHor[1], skyHor[2]);
            glVertex3f(GX1 + 24, 0, GZ1 + 26);  glVertex3f(GX1 + 24, 0, GZ0 - 26);
        glEnd();
        glDisable(GL_TEXTURE_2D);

        // moon + halo, high over the lawn
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.75f, 0.82f, 1.0f, 0.20f);
        glPushMatrix(); glTranslatef(21.0f, 16.0f, -19.0f);
        glutSolidSphere(2.6f, 18, 14); glPopMatrix();
        glDisable(GL_BLEND);
        glColor3f(0.95f, 0.95f, 0.85f);
        glPushMatrix(); glTranslatef(21.0f, 16.0f, -19.0f);
        glutSolidSphere(1.5f, 18, 14); glPopMatrix();

        // distant treeline silhouettes (east + far south)
        glColor3f(0.05f, 0.09f, 0.07f);
        for (int i = 0; i < 11; ++i)
        {
            float tz = GZ0 - 8 + i * 4.2f;
            float th = 3.0f + 1.6f * ((i * 7) % 4);
            glBegin(GL_TRIANGLES);
            glVertex3f(GX1 + 18, 0, tz - 1.8f);
            glVertex3f(GX1 + 18, 0, tz + 1.8f);
            glVertex3f(GX1 + 18, th, tz);
            glEnd();
        }
        for (int i = 0; i < 6; ++i)
        {
            float tx = 14.0f + i * 4.0f;
            float th = 3.2f + 1.4f * ((i * 5) % 4);
            glBegin(GL_TRIANGLES);
            glVertex3f(tx - 1.8f, 0, GZ0 - 14);
            glVertex3f(tx + 1.8f, 0, GZ0 - 14);
            glVertex3f(tx, th, GZ0 - 14);
            glEnd();
        }
        glEnable(GL_TEXTURE_2D);
        glDepthMask(GL_TRUE);
        glEnable(GL_LIGHTING);
    }

    // ------------------------------------------------------------------
    //  Ground: lawn + variation patches + patio + stone paths
    // ------------------------------------------------------------------
    void DrawGround()
    {
        // lawn: two slabs with a gap for the open stair trench
        glPushMatrix();
        glTranslatef((GX0 - 1.0f + ST_X0) / 2, -0.06f, (GZ0 + GZ1) / 2);
        glScalef(ST_X0 - (GX0 - 1.0f), 0.1f, (GZ1 - GZ0) + 2);
        SetSurface(Materials::LEAF, "grass");
        glutSolidCube(1);
        glPopMatrix();
        glPushMatrix();
        glTranslatef((ST_X1 + GX1 + 1.0f) / 2, -0.06f, (GZ0 + GZ1) / 2);
        glScalef((GX1 + 1.0f) - ST_X1, 0.1f, (GZ1 - GZ0) + 2);
        SetSurface(Materials::LEAF, "grass");
        glutSolidCube(1);
        glPopMatrix();

        // lighter/darker grass patches
        for (int i = 0; i < 7; ++i)
        {
            float px = GX0 + 2.5f + i * 2.9f;
            if (px > 25.0f) px -= 3.4f;          // keep clear of the stair trench
            float pz = GZ0 + 1.5f + ((i * 5) % 7) * 2.3f;
            glPushMatrix();
            glTranslatef(px, 0.002f, pz);
            glRotatef(i * 37.0f, 0, 1, 0);
            Lighting_ApplyMaterial(Materials::Tinted(Materials::LEAF, 0.75f, 0.92f, 0.72f));
            Textures::Bind("grass");
            glBegin(GL_QUADS);
                glNormal3f(0, 1, 0);
                glTexCoord2f(0, 0); glVertex3f(-1.1f, 0, -0.8f);
                glTexCoord2f(2, 0); glVertex3f( 1.1f, 0, -0.8f);
                glTexCoord2f(2, 2); glVertex3f( 1.1f, 0,  0.8f);
                glTexCoord2f(0, 2); glVertex3f(-1.1f, 0,  0.8f);
            glEnd();
            glPopMatrix();
        }

        // patio
        glPushMatrix();
        glTranslatef((PAT_X0 + PAT_X1) / 2, 0.02f, (PAT_Z0 + PAT_Z1) / 2);
        glScalef(PAT_X1 - PAT_X0, 0.1f, PAT_Z1 - PAT_Z0);
        SetSurface(Materials::STONE, "stone");
        glutSolidCube(1);
        glPopMatrix();

        // main path: patio -> fountain -> toward the statue
        SetSurface(Materials::STONE, "stone");
        for (int i = 0; i < 4; ++i)               // patio to fountain
        {
            float x = 13.1f + i * 1.1f;
            glPushMatrix();
            glTranslatef(x, 0.035f, -0.3f + 0.15f * sinf(i * 1.3f));
            glRotatef(((i * 11) % 7 - 3) * 2.0f, 0, 1, 0);
            glScalef(1.0f, 0.08f, 1.5f + 0.2f * ((i * 5) % 3));
            glutSolidCube(1);
            glPopMatrix();
        }
        for (int i = 0; i < 7; ++i)               // fountain toward the statue
        {
            float x = 18.2f + i * 0.85f;
            float z = 0.6f + i * 0.72f;
            glPushMatrix();
            glTranslatef(x, 0.035f, z);
            glRotatef(((i * 13) % 7 - 3) * 2.5f, 0, 1, 0);
            glScalef(1.15f + 0.2f * ((i * 3) % 3), 0.08f, 0.95f + 0.2f * ((i * 7) % 3));
            glutSolidCube(1);
            glPopMatrix();
        }
    }

    // ------------------------------------------------------------------
    //  Mansion facade (east exterior of the lobby wall)
    // ------------------------------------------------------------------
    void DrawMansion()
    {
        const float H = 3.8f, t = 0.22f;
        // front-entrance gap (Phase 10): z -3.6..-1.2 in the facade
        const float FE_Z0 = -3.6f, FE_Z1 = -1.2f;
        // wall segments around the door openings
        Box(Materials::FABRIC_C, "wall", MW_X, H / 2, (GZ0 + FE_Z0) / 2,
            t, H, FE_Z0 - GZ0);
        Box(Materials::FABRIC_C, "wall", MW_X, H / 2, (FE_Z1 + DOOR_Z0) / 2,
            t, H, DOOR_Z0 - FE_Z1);
        Box(Materials::FABRIC_C, "wall", MW_X, (2.9f + H) / 2, (FE_Z0 + FE_Z1) / 2,
            t, H - 2.9f, FE_Z1 - FE_Z0);               // header over the gap
        Box(Materials::FABRIC_C, "wall", MW_X, H / 2, (DOOR_Z1 + GZ1) / 2,
            t, H, GZ1 - DOOR_Z1);
        Box(Materials::FABRIC_C, "wall", MW_X, (2.9f + H) / 2, 0,
            t, H - 2.9f, DOOR_Z1 - DOOR_Z0);
        // front-door frame (jambs + lintel) on the garden face
        Box(Materials::DARKWOOD, "darkwood", MW_X, 1.45f, FE_Z0 - 0.05f, 0.26f, 2.9f, 0.1f);
        Box(Materials::DARKWOOD, "darkwood", MW_X, 1.45f, FE_Z1 + 0.05f, 0.26f, 2.9f, 0.1f);
        Box(Materials::DARKWOOD, "darkwood", MW_X, 2.92f, (FE_Z0 + FE_Z1) / 2, 0.26f, 0.1f,
            (FE_Z1 - FE_Z0) + 0.2f);
        // carved crest above the main entrance
        glPushMatrix();
        glTranslatef(MW_X + 0.14f, 3.25f, (FE_Z0 + FE_Z1) / 2);
        glRotatef(90, 0, 1, 0);
        Objects::Crest(0.55f, 0.55f, 0.42f, 0.16f, 1.0f);
        glPopMatrix();

        // string course + plinth
        Box(Materials::STONE, "stone", MW_X + 0.06f, 2.4f, 0, 0.14f, 0.12f,
            (GZ1 - GZ0) * 0.98f);
        Box(Materials::STONE, "stone", MW_X + 0.08f, 0.18f, 0, 0.2f, 0.36f,
            (GZ1 - GZ0) * 0.98f);

        // two warm glowing windows (the lobby's lamp light seen outside)
        const float winZ[2] = { -4.8f, 4.8f };
        for (int i = 0; i < 2; ++i)
        {
            float wz = winZ[i];
            glDisable(GL_LIGHTING);
            glBegin(GL_QUADS);
            glColor3f(1.0f, 0.82f, 0.45f);
            glVertex3f(MW_X + 0.06f, 1.1f, wz - 0.9f);
            glVertex3f(MW_X + 0.06f, 1.1f, wz + 0.9f);
            glVertex3f(MW_X + 0.06f, 2.7f, wz + 0.9f);
            glVertex3f(MW_X + 0.06f, 2.7f, wz - 0.9f);
            glEnd();
            // mullion cross silhouette
            glColor3f(0.25f, 0.16f, 0.08f);
            glBegin(GL_QUADS);
            glVertex3f(MW_X + 0.08f, 1.86f, wz - 0.95f);
            glVertex3f(MW_X + 0.08f, 1.94f, wz + 0.95f);
            glVertex3f(MW_X + 0.08f, 1.94f, wz + 0.95f);
            glVertex3f(MW_X + 0.08f, 1.86f, wz - 0.95f);
            glEnd();
            glBegin(GL_QUADS);
            glVertex3f(MW_X + 0.08f, 1.1f, wz - 0.04f);
            glVertex3f(MW_X + 0.08f, 2.7f, wz - 0.04f);
            glVertex3f(MW_X + 0.08f, 2.7f, wz + 0.04f);
            glVertex3f(MW_X + 0.08f, 1.1f, wz + 0.04f);
            glEnd();
            glEnable(GL_LIGHTING);
            // frame + sill
            Box(Materials::DARKWOOD, "darkwood", MW_X + 0.07f, 1.06f, wz, 0.1f, 0.1f, 2.0f);
            Box(Materials::DARKWOOD, "darkwood", MW_X + 0.07f, 2.74f, wz, 0.1f, 0.1f, 2.0f);
            Box(Materials::STONE, "stone", MW_X + 0.14f, 1.0f, wz, 0.26f, 0.07f, 2.1f);
        }

        // door frame (panel itself is drawn by Lobby on its hinge)
        Box(Materials::DARKWOOD, "darkwood", MW_X + 0.02f, 1.45f, DOOR_Z0 - 0.05f,
            0.24f, 2.9f, 0.1f);
        Box(Materials::DARKWOOD, "darkwood", MW_X + 0.02f, 1.45f, DOOR_Z1 + 0.05f,
            0.24f, 2.9f, 0.1f);
        Box(Materials::DARKWOOD, "darkwood", MW_X + 0.02f, 2.92f, 0,
            0.24f, 0.1f, 1.3f);

        // crest carved above the door (story continuity)
        glPushMatrix();
        glTranslatef(MW_X + 0.13f, 3.25f, 0);
        glRotatef(90, 0, 1, 0);
        Objects::Crest(0.5f, 0.2f, 0.17f, 0.13f, 1.0f);
        glPopMatrix();

        // warm light spilling through the open doorway onto the patio
        if (Progress::Get(Progress::GARDEN_DOOR_OPEN))
        {
            glDisable(GL_LIGHTING);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            glColor4f(1.0f, 0.85f, 0.5f, 0.16f);
            glBegin(GL_QUADS);
            glVertex3f(MW_X + 1.7f, 0.03f, -0.75f);
            glVertex3f(MW_X + 1.7f, 0.03f,  0.75f);
            glVertex3f(MW_X - 0.05f, 0.03f,  0.55f);
            glVertex3f(MW_X - 0.05f, 0.03f, -0.55f);
            glEnd();
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
            glEnable(GL_LIGHTING);
        }

        // climbing vines
        SetSurface(Materials::LEAF, "grass");
        for (int i = 0; i < 5; ++i)
        {
            float vz = -7.2f + i * 3.4f;
            if (vz > DOOR_Z0 - 0.8f && vz < DOOR_Z1 + 0.8f) continue;
            for (int k = 0; k < 4; ++k)
            {
                glPushMatrix();
                glTranslatef(MW_X + 0.15f, 0.5f + k * 0.75f + 0.2f * ((i + k) % 3),
                             vz + 0.18f * k);
                glutSolidSphere(0.16f, 8, 6);
                glPopMatrix();
            }
        }
    }

    // ------------------------------------------------------------------
    //  Perimeter walls (gaps keep the library/lobby window views open)
    // ------------------------------------------------------------------
    void DrawPerimeter()
    {
        // east wall (full)
        Box(Materials::STONE, "stone", GX1 + 0.4f, WALL_H / 2, 0, 0.4f, WALL_H,
            (GZ1 - GZ0) + 1.0f);
        Box(Materials::MARBLE, "marble", GX1 + 0.4f, WALL_H + 0.06f, 0, 0.5f, 0.12f,
            (GZ1 - GZ0) + 1.0f);
        // north wall: only x 19..GX1 (gap 10..19 = the library's view)
        Box(Materials::STONE, "stone", (19.0f + GX1 + 0.6f) / 2, WALL_H / 2, GZ1 + 0.2f,
            (GX1 + 0.6f) - 19.0f, WALL_H, 0.4f);
        Box(Materials::MARBLE, "marble", (19.0f + GX1 + 0.6f) / 2, WALL_H + 0.06f,
            GZ1 + 0.2f, (GX1 + 0.6f) - 19.0f, 0.12f, 0.5f);
        // south wall: only x 14..GX1 (gap toward the lobby's window view)
        Box(Materials::STONE, "stone", (14.0f + GX1 + 0.6f) / 2, WALL_H / 2, GZ0 - 0.2f,
            (GX1 + 0.6f) - 14.0f, WALL_H, 0.4f);
        Box(Materials::MARBLE, "marble", (14.0f + GX1 + 0.6f) / 2, WALL_H + 0.06f,
            GZ0 - 0.2f, (GX1 + 0.6f) - 14.0f, 0.12f, 0.5f);
        // hedges guard both gaps (and block the player)
        glPushMatrix(); glTranslatef(14.5f, 0, 7.6f); Objects::Hedge(9.0f, 1.1f); glPopMatrix();
        glPushMatrix(); glTranslatef(18.5f, 0, -7.6f); Objects::Hedge(11.0f, 1.1f); glPopMatrix();
        glPushMatrix(); glTranslatef(11.2f, 0, -7.9f); Objects::Hedge(5.0f, 1.1f); glPopMatrix();
        // east hedge row
        glPushMatrix(); glTranslatef(27.4f, 0, -2.0f); glRotatef(90, 0, 1, 0);
        Objects::Hedge(9.0f, 1.1f); glPopMatrix();
    }

    // ------------------------------------------------------------------
    //  Fountain + statue + flora + benches + lanterns
    // ------------------------------------------------------------------
    void DrawFountain(float timeSec)
    {
        glPushMatrix();
        glTranslatef(FOUNT_X, 0, FOUNT_Z);
        Objects::Fountain(timeSec);

        // the old plaque on the west rim (faces the path from the mansion)
        glPushMatrix();
        glTranslatef(-1.12f, 0.55f, 0);
        glRotatef(98, 0, 1, 0);
        SetSurface(Materials::STONE, "stone");
        Box(0.55f, 0.4f, 0.06f);
        glPushMatrix();
        glTranslatef(0, 0.02f, 0.035f);
        Objects::Crest(0.30f, 0.22f, 0.19f, 0.15f, 1.0f);
        glPopMatrix();
        SetSurface(Materials::Tinted(Materials::BOOK, 0.4f, 0.36f, 0.3f), 0);
        glPushMatrix(); glTranslatef(0, -0.10f, 0.037f); Box(0.36f, 0.018f, 0.004f); glPopMatrix();
        glPushMatrix(); glTranslatef(0, -0.145f, 0.037f); Box(0.28f, 0.018f, 0.004f); glPopMatrix();
        glPopMatrix();
        glPopMatrix();
    }

    void DrawStatue()
    {
        glPushMatrix();
        glTranslatef(STAT_X, 0, STAT_Z);
        glRotatef(-125, 0, 1, 0);      // faces the path
        glScalef(1.05f, 1.05f, 1.05f);
        Objects::Statue(1.0f);
        if (Progress::Get(Progress::STATUE_FOUND))
        {
            glPushMatrix();
            glTranslatef(0, 0.24f, 0.345f);
            SetSurface(Materials::GOLD, 0);
            Box(0.3f, 0.16f, 0.02f);
            glPushMatrix();
            glTranslatef(0, 0, 0.014f);
            Objects::Crest(0.12f, 0.25f, 0.18f, 0.06f, 1.0f);
            glPopMatrix();
            glPopMatrix();
        }
        glPopMatrix();
    }

    void DrawFlora(float timeSec)
    {
        glPushMatrix(); glTranslatef(13.1f, 0, -4.6f); Objects::FlowerBed(2.6f, 1.7f, 3); glPopMatrix();
        glPushMatrix(); glTranslatef(13.1f, 0,  4.6f); Objects::FlowerBed(2.6f, 1.7f, 8); glPopMatrix();
        glPushMatrix(); glTranslatef(23.2f, 0, -5.3f); Objects::FlowerBed(3.0f, 2.0f, 5); glPopMatrix();
        glPushMatrix(); glTranslatef(16.2f, 0,  6.4f); Objects::FlowerBed(2.4f, 1.6f, 11); glPopMatrix();

        for (int i = 0; i < 5; ++i)
        {
            glPushMatrix();
            glTranslatef(GX0 + 1.6f + i * 3.4f, 0, GZ1 - 1.5f - ((i * 3) % 3) * 0.6f);
            Objects::Bush(1.0f + 0.15f * (i % 3), i + 2);
            glPopMatrix();
        }
        for (int i = 0; i < 4; ++i)
        {
            glPushMatrix();
            glTranslatef(GX0 + 2.6f + i * 4.1f, 0, GZ0 + 1.6f);
            Objects::Bush(1.1f + 0.12f * (i % 3), i + 9);
            glPopMatrix();
        }

        struct T { float x, z, s; };
        const T trees[6] = { {10.5f, 7.6f, 1.0f}, {19.8f, 8.1f, 0.9f},
                             {27.5f, -7.8f, 1.05f}, {11.2f, -7.9f, 0.9f},
                             {20.4f, -8.2f, 0.95f}, {27.9f, 3.4f, 0.95f} };
        for (int i = 0; i < 6; ++i)
        {
            glPushMatrix();
            glTranslatef(trees[i].x, 0, trees[i].z);
            glScalef(trees[i].s, trees[i].s, trees[i].s);
            Objects::Tree(1.0f, timeSec + i * 1.7f);
            glPopMatrix();
        }
    }

    void DrawBenchesAndLamps(float timeSec)
    {
        struct B { float x, z, a; };
        const B benches[3] = { {18.6f, 2.3f, 150.0f}, {14.8f, -2.6f, -20.0f}, {22.8f, -3.4f, 90.0f} };
        for (int i = 0; i < 3; ++i)
        {
            glPushMatrix();
            glTranslatef(benches[i].x, 0, benches[i].z);
            glRotatef(benches[i].a, 0, 1, 0);
            Objects::Bench();
            glPopMatrix();
        }

        struct L { float x, z; };
        const L lamps[7] = { {13.4f, 1.7f}, {15.6f, 2.6f}, {17.8f, 1.7f},
                             {13.4f, -2.4f}, {17.8f, -2.4f}, {21.4f, -1.9f},
                             {28.2f, 5.4f} };
        for (int i = 0; i < 7; ++i)
        {
            glPushMatrix();
            glTranslatef(lamps[i].x, 0, lamps[i].z);
            Objects::GardenLamp(timeSec);
            glPopMatrix();
        }
    }

    // ------------------------------------------------------------------
    //  Cellar annex + the descending stair trench
    // ------------------------------------------------------------------
    void DrawAnnex(float timeSec)
    {
        // sunken floor
        glPushMatrix();
        glTranslatef((ANN_X0 + ANN_X1) / 2, ANN_FLOOR_Y - 0.05f, (ANN_Z0 + ANN_Z1) / 2);
        glScalef((ANN_X1 - ANN_X0) + 0.4f, 0.1f, (ANN_Z1 - ANN_Z0) + 0.4f);
        SetSurface(Materials::STONE, "stone");
        glutSolidCube(1);
        glPopMatrix();

        // walls (door opening in the south wall at CELL_D0..CELL_D1)
        Box(Materials::STONE, "stone", ANN_X0 - 0.1f, 1.3f + ANN_FLOOR_Y,
            (ANN_Z0 + ANN_Z1) / 2, 0.2f, 3.3f, (ANN_Z1 - ANN_Z0) + 0.4f);
        Box(Materials::STONE, "stone", (ANN_X0 + ANN_X1) / 2, 1.3f + ANN_FLOOR_Y,
            ANN_Z1 + 0.1f, (ANN_X1 - ANN_X0) + 0.4f, 3.3f, 0.2f);
        Box(Materials::STONE, "stone", ANN_X1 + 0.1f, 1.3f + ANN_FLOOR_Y,
            (ANN_Z0 + ANN_Z1) / 2, 0.2f, 3.3f, (ANN_Z1 - ANN_Z0) + 0.4f);
        Box(Materials::STONE, "stone", (ANN_X0 + CELL_D0) / 2, 1.3f + ANN_FLOOR_Y,
            ANN_Z0, (CELL_D0 - ANN_X0) + 0.2f, 3.3f, 0.2f);
        Box(Materials::STONE, "stone", (CELL_D1 + ANN_X1) / 2, 1.3f + ANN_FLOOR_Y,
            ANN_Z0, (ANN_X1 - CELL_D1) + 0.2f, 3.3f, 0.2f);
        Box(Materials::STONE, "stone", (CELL_D0 + CELL_D1) / 2, (2.9f + 3.3f) / 2 + ANN_FLOOR_Y,
            ANN_Z0, (CELL_D1 - CELL_D0), 3.3f - 2.9f, 0.2f);

        // roof
        Box(Materials::STONE, "stone", (ANN_X0 + ANN_X1) / 2, 2.95f + ANN_FLOOR_Y,
            (ANN_Z0 + ANN_Z1) / 2, (ANN_X1 - ANN_X0) + 0.4f, 0.15f, (ANN_Z1 - ANN_Z0) + 0.4f);

        // crest carved above the outer door face
        glPushMatrix();
        glTranslatef((CELL_D0 + CELL_D1) / 2, 2.1f + ANN_FLOOR_Y, ANN_Z0 - 0.13f);
        Objects::Crest(0.44f, 0.2f, 0.17f, 0.13f, 1.0f);
        glPopMatrix();

        // door frame + swinging door (hinged at the west jamb, opens inward)
        Box(Materials::DARKWOOD, "darkwood", CELL_D0 - 0.05f, 1.45f + ANN_FLOOR_Y,
            ANN_Z0, 0.1f, 2.9f, 0.2f);
        Box(Materials::DARKWOOD, "darkwood", CELL_D1 + 0.05f, 1.45f + ANN_FLOOR_Y,
            ANN_Z0, 0.1f, 2.9f, 0.2f);
        Box(Materials::DARKWOOD, "darkwood", (CELL_D0 + CELL_D1) / 2, 2.92f + ANN_FLOOR_Y,
            ANN_Z0, 1.2f, 0.1f, 0.2f);
        glPushMatrix();
        glTranslatef(CELL_D1, ANN_FLOOR_Y, ANN_Z0);
        // Objects::Door extends along +X from this hinge.  A positive yaw
        // sends that leaf south across the stair approach; negative yaw
        // correctly swings it north into the annex.
        glRotatef(-g_cellarAngle, 0, 1, 0);
        glTranslatef(-(CELL_D1 - CELL_D0), 0, 0);
        Objects::Door(CELL_D1 - CELL_D0, 2.9f);
        glPopMatrix();

        // candles inside the annex (warm welcome)
        glPushMatrix();
        glTranslatef(28.4f, ANN_FLOOR_Y + 0.02f, 8.2f);
        Objects::CandleGroup(timeSec);
        glPopMatrix();

        // ---- the stair trench (open-air, framed by low stone walls) ----
        SetSurface(Materials::STONE, "stone");
        Box(Materials::STONE, "stone", ST_X0 - 0.15f, -1.7f, (ST_ZBOT + ST_ZTOP) / 2,
            0.3f, 3.7f, (ST_ZTOP - ST_ZBOT) + 0.6f);
        Box(Materials::STONE, "stone", ST_X1 + 0.15f, -1.7f, (ST_ZBOT + ST_ZTOP) / 2,
            0.3f, 3.7f, (ST_ZTOP - ST_ZBOT) + 0.6f);
        // steps (tops match GroundHeight's linear descent)
        for (int i = 0; i < 10; ++i)
        {
            float z = ST_ZTOP - 0.48f * i - 0.24f;
            float y = ANN_FLOOR_Y - 0.30f * (i + 1) - 0.15f;
            glPushMatrix();
            glTranslatef((ST_X0 + ST_X1) / 2, y, z);
            glScalef(ST_X1 - ST_X0, 0.3f, 0.48f);
            glutSolidCube(1);
            glPopMatrix();
        }
        // south cap of the trench (the basement's outer north wall face)
        Box(Materials::STONE, "stone", (BAS_X0 + BAS_X1) / 2, -1.85f, BAS_Z1 + 0.1f,
            (BAS_X1 - BAS_X0) + 0.4f, 3.4f, 0.2f);
    }
}

// ======================================================================
//  Garden namespace
// ======================================================================
// ======================================================================
//  Garden namespace
// ==================================================================
namespace Garden
{
    // ---- Phase 10: the front yard (main-entrance courtyard) ------------
    // Stone path + hedges + the final family portrait on the facade.
    void DrawFrontYard()
    {
        // stone path running south from the entrance
        SetSurface(Materials::STONE, "stone");
        for (int i = 0; i < 5; ++i)
        {
            glPushMatrix();
            glTranslatef(MW_X + 1.1f, -0.02f, -2.4f - 1.05f * i);
            glRotatef((i * 37) % 20 - 10, 0, 1, 0);
            glScalef(1.15f, 0.05f, 0.85f);
            glutSolidCube(1);
            glPopMatrix();
        }
        // hedge pair framing the path
        SetSurface(Materials::LEAF, "grass");
        for (int s = -1; s <= 1; s += 2)
        {
            glPushMatrix();
            glTranslatef(MW_X + 1.1f + 1.3f * s, 0.35f, -3.0f);
            glutSolidSphere(0.55f, 10, 8);
            glPopMatrix();
            glPushMatrix();
            glTranslatef(MW_X + 1.1f + 1.3f * s, 0.32f, -5.2f);
            glutSolidSphere(0.5f, 10, 8);
            glPopMatrix();
        }
        // two lanterns flanking the way out
        glPushMatrix();
        glTranslatef(MW_X + 1.1f, 0, -2.0f);
        glRotatef(-90, 0, 1, 0);
        Objects::GardenLamp(0);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(MW_X + 1.1f, 0, -6.2f);
        glRotatef(-90, 0, 1, 0);
        Objects::GardenLamp(0);
        glPopMatrix();

        // the final family portrait (facade wall, by the entrance)
        glPushMatrix();
        glTranslatef(MW_X + 0.14f, 1.9f, -4.4f);
        glRotatef(90, 0, 1, 0);
        Objects::Painting(1.0f, 0.8f, "portrait");
        glPopMatrix();
        // carved crest beside it
        glPushMatrix();
        glTranslatef(MW_X + 0.14f, 1.7f, -2.6f);
        glRotatef(90, 0, 1, 0);
        Objects::Crest(0.45f, 0.5f, 0.38f, 0.14f, 1.0f);
        glPopMatrix();
    }

    void SetCellarDoorOpen(bool open) { g_cellarOpening = open; }
    bool IsCellarDoorOpen()           { return g_cellarOpening; }

    void UpdateCellarDoor(float dt)
    {
        if (g_cellarOpening && g_cellarAngle < CELLAR_OPEN_ANGLE)
            g_cellarAngle += 140.0f * dt;
        else if (!g_cellarOpening && g_cellarAngle > 0.0f)
            g_cellarAngle -= 140.0f * dt;
        if (g_cellarAngle < 0) g_cellarAngle = 0;
        if (g_cellarAngle > CELLAR_OPEN_ANGLE) g_cellarAngle = CELLAR_OPEN_ANGLE;

        if (g_cellarSlot >= 0)
        {
            // The panel swings inward, away from the garden-side approach.
            // Do not retain the full closed-door AABB while its visual has
            // started opening; that invisible blocker covered the doorway.
            if (g_cellarOpening)
                Collision::SetDynamicBox(g_cellarSlot, false, 0, 0, 0, 0);
            else
                Collision::SetDynamicBox(g_cellarSlot, true,
                    CELL_D0 - 0.08f, ANN_Z0 - 0.12f, CELL_D1 + 0.08f, ANN_Z0 + 0.12f);
        }
    }

    float GroundHeight(float x, float z)
    {
        // stair trench: linear descent from the annex floor to the basement
        if (x > ST_X0 && x < ST_X1 && z > ST_ZBOT && z < ST_ZTOP)
        {
            float t = (ST_ZTOP - z) / (ST_ZTOP - ST_ZBOT);
            float y = ANN_FLOOR_Y - t * 3.0f;
            if (y < BAS_FLOOR_Y) y = BAS_FLOOR_Y;
            return y;
        }
        // bottom landing (trench bottom -> basement hall, continuous)
        if (x > ST_X0 && x < ST_X1 && z > BAS_Z1 && z < ST_ZTOP)
            return BAS_FLOOR_Y;
        // the basement hall under the lawn (room proper is in Basement.cpp;
        // this keeps the floor continuous for the player sampler)
        if (x > BAS_X0 && x < BAS_X1 && z > BAS_Z0 && z < BAS_Z1)
            return BAS_FLOOR_Y;
        // the hidden passage behind the mechanism panel
        if (x > 27.2f && x < 31.3f && z > -1.3f && z < 0.3f)
            return BAS_FLOOR_Y;
        // the secret room (Phase 9) - same level as the basement
        if (x > 31.3f && x < 40.0f && z > -4.7f && z < 2.5f)
            return BAS_FLOOR_Y;
        // annex step bands in the doorway
        if (x > CELL_D0 && x < CELL_D1 && z > 5.4f && z < 5.7f) return -0.18f;
        if (x > CELL_D0 && x < CELL_D1 && z > 5.7f && z < 6.0f) return -0.27f;
        // annex open floor (sunken vestibule)
        if (x > ANN_X0 + 0.2f && x < ANN_X1 && z > 6.0f && z < ANN_Z1)
            return ANN_FLOOR_Y;
        return 0.0f;
    }

    void Load()
    {
        // perimeter (gaps left unblocked but fenced by hedges; add hidden
        // fences at the wall gaps so the player cannot wander out)
        Collision::AddBox(GX1 + 0.2f, GZ0 - 1.0f, GX1 + 0.8f, GZ1 + 1.0f);   // east
        Collision::AddBox(19.0f, GZ1, GX1 + 0.8f, GZ1 + 0.6f);               // north E
        Collision::AddBox(GX0 - 0.8f, GZ1, 10.0f, GZ1 + 0.6f);               // north W
        Collision::AddBox(10.0f, GZ1 - 0.1f, 19.0f, GZ1 + 0.5f);             // fence in gap
        Collision::AddBox(14.0f, GZ0 - 0.6f, GX1 + 0.8f, GZ0 - 0.1f);        // south E
        Collision::AddBox(GX0 - 0.8f, GZ0 - 0.6f, 14.0f, GZ0 - 0.1f);        // fence in gap

        // mansion facade (solid except the doorway)
        Collision::AddBox(MW_X - 0.25f, GZ0 - 1.0f, MW_X + 0.25f, DOOR_Z0);
        Collision::AddBox(MW_X - 0.25f, DOOR_Z1, MW_X + 0.25f, GZ1 + 1.0f);
        // front-entrance gap in the facade (leaves handled by lobby's slots);
        // closed leaves + open-gap are re-set every frame by UpdateFrontDoor
        Collision::AddBox(MW_X - 0.25f, GZ0 - 1.0f, MW_X + 0.25f, -3.6f);
        Collision::AddBox(MW_X - 0.25f, -1.2f, MW_X + 0.25f, GZ0);
        Collision::AddBox(MW_X - 0.25f, -3.6f, MW_X + 0.25f, -1.2f);
        g_frontGapSlot = Collision::ClaimDynamic();

        // fountain, statue, benches, lanterns, trees
        Collision::AddBox(FOUNT_X - 1.15f, FOUNT_Z - 1.15f, FOUNT_X + 1.15f, FOUNT_Z + 1.15f);
        // The rendered pedestal is 0.66 wide (scaled to 0.693); keep its
        // blocker tight so the player can inspect its visible front.
        Collision::AddBox(STAT_X - 0.36f, STAT_Z - 0.36f, STAT_X + 0.36f, STAT_Z + 0.36f);
        Collision::AddBox(17.7f, 1.9f, 19.5f, 2.7f);      // bench 1
        Collision::AddBox(13.9f, -3.0f, 15.7f, -2.2f);    // bench 2
        Collision::AddBox(22.3f, -3.9f, 23.3f, -2.9f);    // bench 3
        Collision::AddBox(13.2f, 1.5f, 13.6f, 1.9f);
        Collision::AddBox(15.4f, 2.4f, 15.8f, 2.8f);
        Collision::AddBox(17.6f, 1.5f, 18.0f, 1.9f);
        Collision::AddBox(13.2f, -2.6f, 13.6f, -2.2f);
        Collision::AddBox(17.6f, -2.6f, 18.0f, -2.2f);
        Collision::AddBox(21.2f, -2.1f, 21.6f, -1.7f);
        // Annex lamp sits beside the stair entrance, not in its 1.2-unit
        // trench.  Keeping this collider in the stair mouth left less than
        // the player's radius of usable passage between the trench walls.
        Collision::AddBox(28.0f, 5.2f, 28.4f, 5.6f);
        Collision::AddBox(10.1f, 7.2f, 10.9f, 8.0f);      // trees
        Collision::AddBox(19.4f, 7.7f, 20.2f, 8.5f);
        Collision::AddBox(27.1f, -8.2f, 27.9f, -7.4f);
        Collision::AddBox(10.8f, -8.3f, 11.6f, -7.5f);
        Collision::AddBox(20.0f, -8.6f, 20.8f, -7.8f);
        Collision::AddBox(27.5f, 3.0f, 28.3f, 3.8f);

        // annex + stair trench (side walls flank the whole descent)
        Collision::AddBox(ANN_X0 - 0.3f, ANN_Z0 - 0.3f, ANN_X0, ANN_Z1 + 0.3f);
        Collision::AddBox(ANN_X0 - 0.3f, ANN_Z1, ANN_X1 + 0.3f, ANN_Z1 + 0.3f);
        Collision::AddBox(ANN_X1, ANN_Z0 - 0.3f, ANN_X1 + 0.3f, ANN_Z1 + 0.3f);
        Collision::AddBox(ANN_X0 - 0.3f, ANN_Z0 - 0.3f, CELL_D0, ANN_Z0);
        Collision::AddBox(CELL_D1, ANN_Z0 - 0.3f, ANN_X1 + 0.3f, ANN_Z0);
        Collision::AddBox(ST_X0 - 0.3f, BAS_Z1 - 0.3f, ST_X0, ST_ZTOP + 0.3f);   // stair W
        Collision::AddBox(ST_X1, BAS_Z1 - 0.3f, ST_X1 + 0.3f, ST_ZTOP + 0.3f);   // stair E

        // cellar door panel: dynamic slot
        Collision::AddBox(CELL_D0 - 0.08f, ANN_Z0 - 0.12f, CELL_D1 + 0.08f, ANN_Z0 + 0.12f);
        g_cellarSlot = Collision::ClaimDynamic();

        // ---- interactables ----
        Interaction::Add(Interaction::ID_FOUNTAIN, FOUNT_X, 0.8f, FOUNT_Z,
                         2.4f, "Examine Fountain");
        Interaction::Add(Interaction::ID_FOUNTPLAQ, FOUNT_X - 1.2f, 0.7f, FOUNT_Z,
                         1.7f, "Examine Plaque");
        // The statue faces south-west after its -125 degree rotation; place
        // the interaction on that visible front instead of inside its base.
        Interaction::Add(Interaction::ID_STATUE, STAT_X - 0.28f, 1.2f,
                         STAT_Z - 0.20f, 1.3f, "Inspect the Garden Statue");
        // The hidden plate is drawn at local +Z on the rotated pedestal.
        Interaction::Add(Interaction::ID_GPLATE, STAT_X - 0.30f, 0.5f,
                         STAT_Z - 0.21f, 1.2f, "Read the Hidden Plate");
        Interaction::SetEnabled(Interaction::ID_GPLATE, false);
        Interaction::Add(Interaction::ID_BENCH, 18.6f, 0.6f, 2.3f, 1.6f, "Rest a Moment");
        // cellar door open/close
        Interaction::Add(Interaction::ID_CELLARDOOR, (CELL_D0 + CELL_D1) / 2,
                         1.3f + ANN_FLOOR_Y, ANN_Z0, 1.8f, "Open the Cellar Door");
        Interaction::Add(Interaction::ID_CELLARCLOSE, (CELL_D0 + CELL_D1) / 2,
                         1.3f + ANN_FLOOR_Y, ANN_Z0, 1.8f, "Close the Cellar Door");
        Interaction::SetEnabled(Interaction::ID_CELLARCLOSE, false);
        Interaction::Add(Interaction::ID_STAIRDOWN, (ST_X0 + ST_X1) / 2,
                         BAS_FLOOR_Y + 1.2f, BAS_Z1 - 0.4f, 1.9f, "Descend the Stairs");
        // The physical stair shaft remains in the scene, but this accessible
        // garden-side control guarantees progression if its narrow geometry
        // cannot be traversed on a particular approach.
        Interaction::Add(Interaction::ID_ENTERBASEMENT, (CELL_D0 + CELL_D1) / 2,
                         1.1f, ANN_Z0 - 0.85f, 1.8f, "Enter Basement");
        Interaction::SetEnabled(Interaction::ID_ENTERBASEMENT, false);
        // garden entrance door (outside face)
        Interaction::Add(Interaction::ID_GARDENDOOR, MW_X + 0.4f, 1.4f, 0, 1.9f,
                         "Enter the Mansion");
        Interaction::Add(Interaction::ID_GARDENCLOSE, MW_X + 0.4f, 1.4f, 0, 1.9f,
                         "Close the Door");
        Interaction::SetEnabled(Interaction::ID_GARDENCLOSE, false);
    }

    void UpdateLights(float timeSec)
    {
        // L0: fountain glow (cool silver-blue; lightning surges it)
        if (Weather::Flashing())
        {
            const float f = Weather::FlashLevel();
            Lighting_SetPointLight(GL_LIGHT0, FOUNT_X, 7.0f, FOUNT_Z,
                                   0.9f, 0.95f, 1.0f, 0.85f + 1.5f * f);
        }
        else
        {
            Lighting_SetPointLight(GL_LIGHT0, FOUNT_X, 1.7f, FOUNT_Z,
                                   0.72f, 0.85f, 1.0f, 0.9f);
        }
        // L1: path lanterns (warm), centered between the lamp rows
        Lighting_SetPointLight(GL_LIGHT1, 15.6f, 1.9f, 0.2f,
                               1.0f, 0.75f, 0.42f, 0.9f);
        // L2: warm spill from the mansion door/windows
        Lighting_SetPointLight(GL_LIGHT2, MW_X + 1.6f, 2.2f, 0,
                               1.0f, 0.80f, 0.48f, 0.85f);
    }

    void UpdateFill()
    {
        // L6: cool moon fill over the whole lawn (lightning boosts it)
        Lighting_SetDirectionalLight(GL_LIGHT6, 0.45f, -1.0f, 0.35f,
                                     0.55f, 0.65f, 0.9f,
                                     0.75f + 1.1f * Weather::FlashLevel());
    }

    void Draw(float timeSec, float playerX, float playerZ)
    {
        DrawSky();
        DrawGround();
        DrawMansion();
        DrawPerimeter();
        DrawFountain(timeSec);
        DrawStatue();
        DrawFlora(timeSec);
        DrawBenchesAndLamps(timeSec);
        DrawAnnex(timeSec);
        DrawFrontYard();

        // rain removed - the clear night carries the garden instead
        (void)Weather::FlashLevel();   // lightning hooks stay wired (idle)
    }
}
