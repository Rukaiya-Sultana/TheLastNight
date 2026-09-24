// ============================================================
//  THE LAST NIGHT - Library.cpp
//  The Grand Library: tall walnut bookshelves filled with colorful
//  books, a reading table on a burgundy-gold rug, a small fireplace,
//  two lamps, the stopped grandfather clock, the founder painting
//  with its brass plate (the "7" clue), the pulled-out red book
//  (the secret journal), the locked north door to the Study and
//  the 3-digit safe. Warm, bright, colorful - blue evening windows.
//
//  Uses light slots L0 (chandelier), L1 (table lamps), L2 (fireplace)
//  while the player is here; L6 is the always-on fill for this room.
// ============================================================
#include "Library.h"
#include "Objects.h"
#include "Texture.h"
#include "Lighting.h"
#include "Collision.h"
#include "Interaction.h"
#include "Progress.h"
#include <GL/freeglut.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace
{
    const float X0 = 8.25f, X1 = 20.25f;   // room x extent
    const float Z0 = 13.25f, Z1 = 23.25f;  // room z extent
    const float WH = 4.0f;                 // wall height
    const float WT = 0.25f;                // wall thickness

    // doorway in the west wall (from the hall), z extent:
    // Broad shared entry aligned with the Grand Hall east-wall opening.
    const float ENTRY_Z0 = 18.35f, ENTRY_Z1 = 20.75f;
    // locked door to the study, in the north wall:
    const float STUDY_DOOR_X0 = 13.65f, STUDY_DOOR_X1 = 14.85f;
    // window opening in the south wall, x extent:
    const float WIN_X0 = 15.3f, WIN_X1 = 17.3f;
    const float WIN_Y0 = 1.0f, WIN_Y1 = 3.0f;

    // world-space wall centers
    const float WX_W = X0 - WT/2;   // west wall center x
    const float WX_E = X1 + WT/2;   // east
    const float WZ_S = Z0 - WT/2;   // south
    const float WZ_N = Z1 + WT/2;   // north

    // the 3-digit safe (east wall, near the north corner)
    const float SAFE_X = 19.0f, SAFE_Z = 22.5f;
    const float SAFE_W = 0.7f, SAFE_H = 0.8f;

    // safe door animation
    float g_safeAngle = 0.0f;
    const float SAFE_OPEN_ANGLE = 95.0f;
    bool  g_safeOpen = false;

    // --- palette -------------------------------------------------------
    const Material FABRIC_BURG = {{0.18f, 0.06f, 0.07f, 1}, {0.52f, 0.13f, 0.15f, 1}, {0.05f, 0.05f, 0.05f, 1}, 2.0f};

    void SetSurface(const Material &m, const char *tex)
    {
        Lighting_ApplyMaterial(m);
        if (tex) Textures::Bind(tex);
    }

    // centered, at the current origin (delegates to Objects::Box)
    void Box(float sx, float sy, float sz) { Objects::Box(sx, sy, sz); }

    void Box(const Material &m, const char *tex,
             float cx, float cy, float cz, float sx, float sy, float sz)
    {
        glPushMatrix();
        glTranslatef(cx, cy, cz);
        Objects::TexturedBox(m, tex, sx, sy, sz);
        glPopMatrix();
    }

    // ------------------------------------------------------------------
    //  A tall bookshelf. Frame centered at local x=0; books extend +z.
    //  The SEPARATE SHELF's row 1 (s==1) can skip a slot to leave room
    //  for the pulled-out secret book (or the brass key once taken).
    // ------------------------------------------------------------------
    void ShelfDraw(float w, float h, bool separateShelf, bool keyTaken)
    {
        const float D = 0.42f, T = 0.06f;
        const int   SHELVES = 5;

        SetSurface(Materials::DARKWOOD, "darkwood");
        glPushMatrix(); glTranslatef(-(w/2 - T/2), h/2, 0); Objects::Box(T, h, D); glPopMatrix();
        glPushMatrix(); glTranslatef( (w/2 - T/2), h/2, 0); Objects::Box(T, h, D); glPopMatrix();
        glPushMatrix(); glTranslatef(0, h/2, -D/2 + T/2); Objects::Box(w, h, T); glPopMatrix(); // back panel
        // decorative crown molding
        glPushMatrix(); glTranslatef(0, h - T/2 + 0.05f, 0); Objects::Box(w + 0.12f, T + 0.1f, D + 0.08f); glPopMatrix();
        glPushMatrix(); glTranslatef(0, T/2, 0); Objects::Box(w, T, D); glPopMatrix();

        const float innerH = (h - 2*T) / SHELVES;
        static const float bookHs[8] = {0.24f,0.28f,0.22f,0.26f,0.30f,0.24f,0.27f,0.23f};
        // near-white tints: let the books texture's own colorful palette
        // carry the hue (over-tinting turned every shelf teal)
        static const float bookTints[4][3] = {
            {1.00f,1.00f,1.00f},{0.96f,0.96f,1.00f},{1.00f,0.97f,0.94f},{0.95f,1.00f,0.97f}};

        for (int s = 0; s < SHELVES; ++s)
        {
            float sy = T + innerH*(s+1) - T/2;
            glPushMatrix();
            glTranslatef(0, sy, 0);
            Objects::Box(w - 2*T, T, D);
            glPopMatrix();

            float bx = -(w/2 - T) + 0.03f;
            float limit = (w/2 - T) - 0.06f;
            int i = 0;
            while (bx < limit && i < 14)
            {
                // the separate shelf leaves one gap in row 1
                if (separateShelf && s == 1 && i == 5)
                {
                    bx += 0.14f;
                    ++i;
                    continue;
                }
                float bw = 0.05f + 0.018f*((i*7+s*3)%3);
                float bh = bookHs[(i*5+s*2)%8] * (innerH/0.34f);
                const float *t = bookTints[(i*3+s*5)%4];
                Material bm = Materials::Tinted(Materials::BOOK, t[0], t[1], t[2]);
                SetSurface(bm, "books");
                float lean = ((i*7+s*11)%9 == 0) ? 0.10f : 0.0f; // occasional lean
                glPushMatrix();
                glTranslatef(bx + bw/2, sy + T/2 + bh/2, 0.03f);
                if (lean) glRotatef(lean * 57.3f, 0, 0, 1);
                Objects::Box(bw, bh, D - 0.10f);
                glPopMatrix();
                bx += bw + 0.012f;
                ++i;
            }
        }

        // THE SECRET: the dark-red book with the gold spine, pulled out.
        // Sits in the separate shelf, row 1 (s==1), at the reserved gap.
        if (separateShelf)
        {
            const float gapY = T + innerH*2 - T/2;      // bottom of row 1
            float by = gapY + innerH * 0.42f;
            float bx = -(w/2 - T) + 0.03f;
            for (int k = 0; k < 5; ++k)                  // width of 5 book slots
            {
                bx += 0.05f + 0.018f*((k*7+3)%3) + 0.012f;
            }
            // book (upright, pulled 0.14 toward +z)
            glPushMatrix();
            glTranslatef(bx, by, 0.16f);
            SetSurface(FABRIC_BURG, 0);
            Box(0.075f, 0.34f, 0.30f);
            // gold spine strip
            SetSurface(Materials::GOLD, 0);
            glPushMatrix(); glTranslatef(0, 0, 0.145f); Box(0.055f, 0.30f, 0.012f); glPopMatrix();
            glPopMatrix();
            // the brass key resting on the shelf once the safe is open
            if (keyTaken == false && Progress::Get(Progress::SAFE_OPEN))
            {
                glPushMatrix();
                glTranslatef(bx + 0.18f, gapY + 0.10f, 0.10f);
                glRotatef(35, 0, 1, 0);
                Objects::BrassKey();
                glPopMatrix();
            }
        }
    }

    // ------------------------------------------------------------------
    //  Room shell
    // ------------------------------------------------------------------
    void DrawFloor()
    {
        glPushMatrix();
        glTranslatef((X0+X1)/2, -0.05f, (Z0+Z1)/2);
        glScalef((X1-X0) + 1, 0.1f, (Z1-Z0) + 1);
        Lighting_ApplyMaterial(Materials::WOOD);
        Textures::Bind("wood");
        glutSolidCube(1);
        glPopMatrix();
    }

    void DrawCeiling()
    {
        glPushMatrix();
        glTranslatef((X0+X1)/2, WH + 0.05f, (Z0+Z1)/2);
        glScalef((X1-X0) + 1, 0.1f, (Z1-Z0) + 1);
        Lighting_ApplyMaterial(Materials::FABRIC_C);
        Textures::Bind("plaster");
        glutSolidCube(1);
        glPopMatrix();

        // coffered beams
        SetSurface(Materials::DARKWOOD, "darkwood");
        for (int i = 0; i < 4; ++i)
        {
            float x = X0 + 2.4f + i*2.4f;
            Box(Materials::DARKWOOD, "darkwood", x, WH - 0.11f, (Z0+Z1)/2, 0.28f, 0.22f, (Z1-Z0));
        }
    }

    void DrawWestWall()
    {
        // segments around the entry doorway
        float segS = (ENTRY_Z0) - Z0;
        float segN = Z1 - ENTRY_Z1;
        Box(Materials::FABRIC_C, "wall", WX_W, WH/2, Z0 + segS/2, WT, WH, segS);
        Box(Materials::FABRIC_C, "wall", WX_W, WH/2, Z1 - segN/2, WT, WH, segN);
        Box(Materials::FABRIC_C, "wall", WX_W, (2.9f + WH)/2,
            (ENTRY_Z0+ENTRY_Z1)/2, WT, WH - 2.9f, ENTRY_Z1 - ENTRY_Z0);

        // doorway frame
        const float cz = (ENTRY_Z0 + ENTRY_Z1) / 2;
        Box(Materials::DARKWOOD, "darkwood", WX_W, 1.45f, ENTRY_Z0 - 0.05f, 0.18f, 2.9f, 0.1f);
        Box(Materials::DARKWOOD, "darkwood", WX_W, 1.45f, ENTRY_Z1 + 0.05f, 0.18f, 2.9f, 0.1f);
        Box(Materials::DARKWOOD, "darkwood", WX_W, 2.92f, cz, 0.18f, 0.1f, ENTRY_Z1 - ENTRY_Z0 + 0.3f);

        // Two leaves are held open against the inside wall, making the
        // library entrance unmistakable without narrowing the walk-through.
        glPushMatrix();
        glTranslatef(WX_W + 0.08f, 0.02f, ENTRY_Z0 + 0.08f);
        Objects::Door(0.95f, 2.78f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(WX_W + 0.08f, 0.02f, ENTRY_Z1 - 0.08f);
        Objects::Door(0.95f, 2.78f);
        glPopMatrix();

        // Brass lintel plaque above the open leaves.
        Box(Materials::GOLD, "white", WX_W + 0.03f, 3.12f, cz,
            0.10f, 0.22f, 1.55f);
    }

    void DrawEastWall()
    {
        Box(Materials::FABRIC_C, "wall", WX_E, WH/2, (Z0+Z1)/2, WT, WH, (Z1-Z0));
        // dark lower paneling + top rail
        Box(Materials::DARKWOOD, "darkwood", WX_E - 0.03f, 0.55f, (Z0+Z1)/2, 0.06f, 1.1f, (Z1-Z0) - 0.3f);
        Box(Materials::DARKWOOD, "darkwood", WX_E - 0.03f, 1.14f, (Z0+Z1)/2, 0.10f, 0.07f, (Z1-Z0) - 0.3f);
        // skirting
        Box(Materials::DARKWOOD, "darkwood", WX_E - 0.02f, 0.09f, (Z0+Z1)/2, 0.05f, 0.18f, (Z1-Z0));

        // THE SAFE: dark steel box with a brass door + dial, east wall
        float sx = X1 - 0.20f;
        Box(Materials::METAL, 0, sx, SAFE_H/2, SAFE_Z, SAFE_W, SAFE_H, 0.6f);
        // safe door (hinged at its left edge; swings open after the code)
        glPushMatrix();
        glTranslatef(sx - SAFE_W/2, SAFE_H/2, SAFE_Z - 0.30f);
        glRotatef(-g_safeAngle, 0, 1, 0);
        SetSurface(Materials::METAL, 0);
        Box(SAFE_W, SAFE_H - 0.06f, 0.05f);
        // dial
        SetSurface(Materials::GOLD, 0);
        glPushMatrix(); glTranslatef(SAFE_W - 0.10f, 0, 0.04f);
        glutSolidTorus(0.02f, 0.05f, 8, 16); glPopMatrix();
        // handle
        glPushMatrix(); glTranslatef(SAFE_W - 0.10f, -0.18f, 0.03f);
        Box(0.05f, 0.10f, 0.03f); glPopMatrix();
        glPopMatrix();
    }

    void DrawNorthWall()
    {
        // segments around the study door
        float segW = (STUDY_DOOR_X0) - X0;
        float segE = X1 - STUDY_DOOR_X1;
        Box(Materials::FABRIC_C, "wall", X0 + segW/2, WH/2, WZ_N, segW, WH, WT);
        Box(Materials::FABRIC_C, "wall", X1 - segE/2, WH/2, WZ_N, segE, WH, WT);
        Box(Materials::FABRIC_C, "wall", (STUDY_DOOR_X0+STUDY_DOOR_X1)/2,
            (2.9f + WH)/2, WZ_N, STUDY_DOOR_X1 - STUDY_DOOR_X0, WH - 2.9f, WT);

        // door frame + molding
        Box(Materials::DARKWOOD, "darkwood", STUDY_DOOR_X0 - 0.05f, 1.45f, WZ_N, 0.1f, 2.9f, 0.18f);
        Box(Materials::DARKWOOD, "darkwood", STUDY_DOOR_X1 + 0.05f, 1.45f, WZ_N, 0.1f, 2.9f, 0.18f);
        Box(Materials::DARKWOOD, "darkwood", (STUDY_DOOR_X0+STUDY_DOOR_X1)/2, 2.92f, WZ_N, 0.3f, 0.1f, 0.18f);

        // paneling between door and corner
        Box(Materials::DARKWOOD, "darkwood", X0 + segW/2, 2.0f, WZ_N - 0.03f, segW - 0.6f, 1.6f, 0.04f);
        Box(Materials::DARKWOOD, "darkwood", X1 - segE/2, 2.0f, WZ_N - 0.03f, segE - 0.6f, 1.6f, 0.04f);
    }

    void DrawSouthWall()
    {
        // segments around the window
        float segW = (WIN_X0) - X0;
        float segE = X1 - WIN_X1;
        Box(Materials::FABRIC_C, "wall", X0 + segW/2, WH/2, WZ_S, segW, WH, WT);
        Box(Materials::FABRIC_C, "wall", X1 - segE/2, WH/2, WZ_S, segE, WH, WT);
        Box(Materials::FABRIC_C, "wall", (WIN_X0+WIN_X1)/2, (WIN_Y1 + WH)/2, WZ_S,
            WIN_X1 - WIN_X0, WH - WIN_Y1, WT);
        Box(Materials::FABRIC_C, "wall", (WIN_X0+WIN_X1)/2, WIN_Y0/2, WZ_S,
            WIN_X1 - WIN_X0, WIN_Y0, WT);

        // window frame + mullions
        const float fw = 0.08f;
        Box(Materials::WOOD, "wood", (WIN_X0+WIN_X1)/2, WIN_Y0 - fw/2, WZ_S, WIN_X1-WIN_X0+2*fw, fw, 0.14f);
        Box(Materials::WOOD, "wood", (WIN_X0+WIN_X1)/2, WIN_Y1 + fw/2, WZ_S, WIN_X1-WIN_X0+2*fw, fw, 0.14f);
        Box(Materials::WOOD, "wood", WIN_X0 - fw/2, (WIN_Y0+WIN_Y1)/2, WZ_S, fw, WIN_Y1-WIN_Y0+2*fw, 0.14f);
        Box(Materials::WOOD, "wood", WIN_X1 + fw/2, (WIN_Y0+WIN_Y1)/2, WZ_S, fw, WIN_Y1-WIN_Y0+2*fw, 0.14f);
        Box(Materials::WOOD, "wood", (WIN_X0+WIN_X1)/2, (WIN_Y0+WIN_Y1)/2, WZ_S, 0.05f, WIN_Y1-WIN_Y0, 0.07f);
        Box(Materials::WOOD, "wood", (WIN_X0+WIN_X1)/2, (WIN_Y0+WIN_Y1)/2, WZ_S, WIN_X1-WIN_X0, 0.05f, 0.07f);
        // sill
        Box(Materials::MARBLE, "marble", (WIN_X0+WIN_X1)/2, WIN_Y0 - 0.06f, WZ_S + 0.15f, 2.3f, 0.07f, 0.4f);

        // exterior: blue evening + lawn + trees (self-lit)
        glDisable(GL_LIGHTING);
        glDepthMask(GL_FALSE);
        Textures::Bind("nightsky");
        glBegin(GL_QUADS);
            glColor3f(1, 1, 1);
            glTexCoord2f(0, 0); glVertex3f(WIN_X0, WIN_Y0, WZ_S - 3.0f);
            glTexCoord2f(1, 0); glVertex3f(WIN_X1, WIN_Y0, WZ_S - 3.0f);
            glTexCoord2f(1, 1); glVertex3f(WIN_X1, WIN_Y1 + 2.5f, WZ_S - 3.0f);
            glTexCoord2f(0, 1); glVertex3f(WIN_X0, WIN_Y1 + 2.5f, WZ_S - 3.0f);
        glEnd();
        Textures::Bind("grass");
        glBegin(GL_QUADS);
            glColor3f(0.65f, 0.78f, 0.62f);
            glVertex3f(WIN_X0 - 1, 0, WZ_S - 3.0f);
            glVertex3f(WIN_X1 + 1, 0, WZ_S - 3.0f);
            glVertex3f(WIN_X1 + 1, 1.0f, WZ_S - 3.0f);
            glVertex3f(WIN_X0 - 1, 1.0f, WZ_S - 3.0f);
        glEnd();
        glColor3f(0.05f, 0.10f, 0.07f);
        for (int i = 0; i < 3; ++i)
        {
            float tx = WIN_X0 + 0.5f + i*0.7f;
            glBegin(GL_TRIANGLES);
            glVertex3f(tx - 0.5f, 1.0f, WZ_S - 3.0f);
            glVertex3f(tx + 0.5f, 1.0f, WZ_S - 3.0f);
            glVertex3f(tx, 3.2f + 0.3f*i, WZ_S - 3.0f);
            glEnd();
        }
        glDepthMask(GL_TRUE);
        glEnable(GL_LIGHTING);

        // faint glass
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glColor4f(0.75f, 0.85f, 0.95f, 0.10f);
        Lighting_ApplyMaterial(Materials::GLASS);
        glBegin(GL_QUADS);
            glNormal3f(0, 0, 1);
            glVertex3f(WIN_X0, WIN_Y0, WZ_S + 0.02f);
            glVertex3f(WIN_X1, WIN_Y0, WZ_S + 0.02f);
            glVertex3f(WIN_X1, WIN_Y1, WZ_S + 0.02f);
            glVertex3f(WIN_X0, WIN_Y1, WZ_S + 0.02f);
        glEnd();
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        // curtains + potted plants flanking the window
        glPushMatrix();
        glTranslatef((WIN_X0+WIN_X1)/2, 0, WZ_S + 0.2f);
        Objects::Curtains(WIN_X1 - WIN_X0 + 0.3f, WIN_Y1 - WIN_Y0 + 0.4f);
        glPopMatrix();
        glPushMatrix(); glTranslatef(WIN_X0 - 1.0f, 0, WZ_S + 0.6f); Objects::PottedPlant(1.0f); glPopMatrix();
        glPushMatrix(); glTranslatef(WIN_X1 + 1.0f, 0, WZ_S + 0.6f); Objects::PottedPlant(0.9f); glPopMatrix();
    }

    // ------------------------------------------------------------------
    //  Furniture
    // ------------------------------------------------------------------
    void DrawFurniture(float timeSec)
    {
        // ---- three tall shelves along the east wall (facing -x / west) --
        glPushMatrix(); glTranslatef(X1 - 0.24f, 0, 15.0f); glRotatef(-90, 0, 1, 0);
        ShelfDraw(3.0f, 3.2f, false, false); glPopMatrix();
        glPushMatrix(); glTranslatef(X1 - 0.24f, 0, 18.5f); glRotatef(-90, 0, 1, 0);
        ShelfDraw(3.0f, 3.2f, false, false); glPopMatrix();
        glPushMatrix(); glTranslatef(X1 - 0.24f, 0, 22.0f); glRotatef(-90, 0, 1, 0);
        ShelfDraw(2.6f, 3.2f, true, Progress::Get(Progress::HAS_BRASS_KEY)); glPopMatrix();

        // ---- one shelf on the west wall, south of the entry -------------
        glPushMatrix(); glTranslatef(X0 + 0.24f, 0, 16.0f); glRotatef(90, 0, 1, 0);
        ShelfDraw(3.0f, 3.2f, false, false); glPopMatrix();

        // ---- low side shelf on the south wall, west of the window -------
        glPushMatrix(); glTranslatef(11.5f, 0, Z0 + 0.24f);
        ShelfDraw(2.2f, 1.9f, false, false); glPopMatrix();

        // ---- reading table (center-south) + chairs + lamp + objects -----
        glPushMatrix();
        glTranslatef(12.6f, 0, 18.6f);
        // tabletop
        SetSurface(Materials::WOOD, "wood");
        glPushMatrix(); glTranslatef(0, 0.76f, 0); Box(2.4f, 0.08f, 1.3f); glPopMatrix();
        // decorative edge trim
        SetSurface(Materials::DARKWOOD, "darkwood");
        glPushMatrix(); glTranslatef(0, 0.70f, 0.64f); Box(2.4f, 0.10f, 0.05f); glPopMatrix();
        glPushMatrix(); glTranslatef(0, 0.70f, -0.64f); Box(2.4f, 0.10f, 0.05f); glPopMatrix();
        // legs
        for (int i = 0; i < 4; ++i)
        {
            float lx = (i % 2) ? 1.0f : -1.0f;
            float lz = (i / 2) ? 0.5f : -0.5f;
            glPushMatrix(); glTranslatef(lx, 0.36f, lz); Box(0.10f, 0.72f, 0.10f); glPopMatrix();
        }
        // lower stretcher
        glPushMatrix(); glTranslatef(0, 0.18f, 0); Box(2.0f, 0.08f, 0.08f); glPopMatrix();

        // ON THE TABLE (local coordinates):
        // open old book
        glPushMatrix(); glTranslatef(-0.55f, 0.80f, -0.15f); glRotatef(12, 0, 1, 0);
        SetSurface(Materials::Tinted(Materials::BOOK, 1.0f, 0.9f, 0.7f), "books");
        glPushMatrix(); glRotatef(-8, 0, 0, 1); Box(0.42f, 0.05f, 0.30f); glPopMatrix();
        glPushMatrix(); glTranslatef(0.41f, 0, 0); glRotatef(16, 0, 1, 0); Box(0.42f, 0.05f, 0.30f); glPopMatrix();
        glPopMatrix();
        // paper stack
        SetSurface(Materials::Tinted(Materials::BOOK, 1.1f, 1.08f, 1.0f), 0);
        glPushMatrix(); glTranslatef(0.35f, 0.815f, 0.20f); glRotatef(-14, 0, 1, 0);
        Box(0.30f, 0.025f, 0.22f); glPopMatrix();
        // ink bottle
        SetSurface(Materials::GLASS, 0);
        glPushMatrix(); glTranslatef(0.62f, 0.85f, -0.28f); glScalef(1, 1.6f, 1);
        glutSolidSphere(0.055f, 12, 10); glPopMatrix();
        // green banker lamp (light L1 sits just above its shade)
        glPushMatrix(); glTranslatef(0.85f, 0.80f, 0.42f); Objects::DeskLamp(timeSec); glPopMatrix();
        // brass candle holder (unlit - the lamps carry the light)
        glPushMatrix(); glTranslatef(-0.95f, 0.80f, 0.30f); Objects::Candle(0.14f, false, timeSec); glPopMatrix();
        glPopMatrix();

        // chairs around the table
        glPushMatrix(); glTranslatef(12.6f - 1.55f, 0, 18.6f - 0.1f); glRotatef(-90, 0, 1, 0);
        Objects::SideChair(FABRIC_BURG); glPopMatrix();
        glPushMatrix(); glTranslatef(12.6f + 1.55f, 0, 18.6f + 0.1f); glRotatef(90, 0, 1, 0);
        Objects::SideChair(Materials::FABRIC_G); glPopMatrix();
        glPushMatrix(); glTranslatef(12.6f, 0, 18.6f - 1.15f); glRotatef(180, 0, 1, 0);
        Objects::SideChair(FABRIC_BURG); glPopMatrix();

        // ---- grandfather clock (stopped at midnight), north-west corner -
        glPushMatrix(); glTranslatef(9.3f, 0, 22.6f); glRotatef(43, 0, 1, 0);
        Objects::StoppedClock(); glPopMatrix();

        // ---- small fireplace on the west wall, north of the entry -------
        glPushMatrix();
        glTranslatef(X0 + 0.05f, 0, 21.6f);
        glRotatef(-90, 0, 1, 0);
        Objects::Fireplace(true, timeSec);
        glPopMatrix();

        // ---- founder painting + brass plate (the "7" clue), south wall --
        glPushMatrix();
        glTranslatef(10.0f, 2.4f, WZ_S + 0.06f);
        Objects::Painting(1.3f, 1.7f, "paint_owner");
        glPopMatrix();
        SetSurface(Materials::GOLD, 0);
        glPushMatrix(); glTranslatef(10.0f, 1.42f, WZ_S + 0.05f);
        Box(0.42f, 0.10f, 0.02f);
        glPopMatrix();

        // ---- landscape painting, north wall ------------------------------
        glPushMatrix();
        glTranslatef(17.4f, 2.4f, WZ_N - 0.06f);
        glRotatef(180, 0, 1, 0);
        Objects::Painting(1.5f, 1.0f, "paint_land");
        glPopMatrix();

        // ---- globe on a stand (north-west area) --------------------------
        glPushMatrix();
        glTranslatef(10.6f, 0, 20.8f);
        SetSurface(Materials::DARKWOOD, "darkwood");
        glPushMatrix(); glTranslatef(0, 0.50f, 0); Box(0.05f, 1.0f, 0.05f); glPopMatrix();
        glPushMatrix(); glTranslatef(0, 0.02f, 0); Box(0.4f, 0.04f, 0.4f); glPopMatrix();
        SetSurface(Materials::FABRIC_G, "grass");
        glPushMatrix(); glTranslatef(0, 1.12f, 0); glRotatef(23, 0, 0, 1);
        glutSolidSphere(0.24f, 18, 14); glPopMatrix();
        glPopMatrix();

        // ---- rug under the reading area ----------------------------------
        glPushMatrix();
        glTranslatef(12.6f, 0.012f, 18.0f);
        Objects::Rug(4.4f, 3.4f);
        glPopMatrix();
    }

    // ------------------------------------------------------------------
    //  Fixtures: chandelier + two standing lamps
    // ------------------------------------------------------------------
    void DrawFixtures(float timeSec)
    {
        // chandelier center (rotates slowly - continuous animation)
        glPushMatrix();
        glTranslatef(14.0f, WH, 18.5f);
        Objects::Chandelier(timeSec);
        glPopMatrix();

        // standing lamps: east corner + near the north door
        glPushMatrix(); glTranslatef(19.5f, 0, 14.2f); Objects::TableLamp(); glPopMatrix();
        glPushMatrix(); glTranslatef(16.2f, 0, 22.7f); Objects::TableLamp(); glPopMatrix();
    }
}

namespace Library
{
    // study door animation state (swings open after unlocking)
    float g_studyAngle = 0.0f;
    const float STUDY_OPEN_ANGLE = 100.0f;
    int   g_studyDoorSlot = -1;
    bool  g_studyOpening = false;

    void SetStudyDoorOpen(bool open)  { g_studyOpening = open; }
    bool IsStudyDoorOpen()            { return g_studyOpening; }

    // Draws the swinging study door panel and updates its collision box.
    // Called from Game::Update so it animates even between frames.
    void UpdateStudyDoor(float dt)
    {
        if (g_studyOpening && g_studyAngle < STUDY_OPEN_ANGLE)
            g_studyAngle += 140.0f * dt;
        else if (!g_studyOpening && g_studyAngle > 0.0f)
            g_studyAngle -= 140.0f * dt;
        if (g_studyAngle < 0) g_studyAngle = 0;
        if (g_studyAngle > STUDY_OPEN_ANGLE) g_studyAngle = STUDY_OPEN_ANGLE;

        // draw the swinging panel (hinged at its west jamb, opens inward)
        glPushMatrix();
        glTranslatef(STUDY_DOOR_X0, 0, Z1);
        glRotatef(-g_studyAngle, 0, 1, 0);
        Objects::Door(STUDY_DOOR_X1 - STUDY_DOOR_X0, 2.9f);
        glPopMatrix();

        // collision follows the panel while it swings
        if (g_studyDoorSlot >= 0)
        {
            if (g_studyAngle >= STUDY_OPEN_ANGLE - 2.0f)
            {
                Collision::SetDynamicBox(g_studyDoorSlot, false, 0,0,0,0);
            }
            else
            {
                float rad = g_studyAngle * (float)M_PI / 180.0f;
                float hx = STUDY_DOOR_X0;
                float px = hx + cosf(rad) * (STUDY_DOOR_X1 - STUDY_DOOR_X0);
                float pz = Z1 - sinf(rad) * (STUDY_DOOR_X1 - STUDY_DOOR_X0);
                float minX = (px < hx ? px : hx) - 0.08f;
                float maxX = (px > hx ? px : hx) + 0.08f;
                float minZ = (pz < Z1 ? pz : Z1) - 0.08f;
                float maxZ = (pz > Z1 ? pz : Z1) + 0.08f;
                Collision::SetDynamicBox(g_studyDoorSlot, true, minX, minZ, maxX, maxZ);
            }
        }
    }

    void Load()
    {
        const float t = 0.25f;

        // west wall (entry opening between ENTRY_Z0..ENTRY_Z1)
        Collision::AddBox(X0 - t, Z0 - t, X0 + t, ENTRY_Z0);
        Collision::AddBox(X0 - t, ENTRY_Z1, X0 + t, Z1 + t);
        // east wall
        Collision::AddBox(X1 - t, Z0 - t, X1 + t, Z1 + t);
        // south wall (window)
        Collision::AddBox(X0 - t, Z0 - t, X1 + t, Z0 + t);
        // north wall (study doorway between STUDY_DOOR_X0..X1)
        Collision::AddBox(X0 - t, Z1 - t, STUDY_DOOR_X0, Z1 + t);
        Collision::AddBox(STUDY_DOOR_X1, Z1 - t, X1 + t, Z1 + t);

        // furniture
        Collision::AddBox(17.4f, 13.6f, 20.4f, 16.6f);   // shelf E1
        Collision::AddBox(17.4f, 17.1f, 20.4f, 19.9f);   // shelf E2
        Collision::AddBox(17.6f, 20.8f, 20.4f, 23.2f);   // shelf E3 (secret)
        Collision::AddBox(8.1f, 14.5f, 10.9f, 17.5f);    // shelf W
        // Low south-wall shelf: its rendered depth is 0.42 (z 13.28..13.70).
        // Keep the in-room blocking face tight to that footprint; the former
        // 14.6 maxZ created a 0.9-unit invisible extension across the founder
        // plate's only natural approach route.
        Collision::AddBox(10.4f, 13.5f, 12.6f, 13.70f);  // side shelf
        Collision::AddBox(11.4f, 17.9f, 13.8f, 19.3f);   // reading table
        Collision::AddBox(8.8f, 22.1f, 9.8f, 23.1f);     // clock
        Collision::AddBox(8.25f, 21.0f, 9.1f, 22.2f);    // fireplace
        Collision::AddBox(19.2f, 13.9f, 19.8f, 14.5f);   // lamp NE
        Collision::AddBox(15.9f, 22.4f, 16.5f, 23.0f);   // lamp NW
        Collision::AddBox(10.3f, 20.5f, 10.9f, 21.1f);   // globe
        Collision::AddBox(18.55f, 22.1f, 19.45f, 22.9f); // safe

        // closed study door panel: claimed as a dynamic slot (swings open)
        Collision::AddBox(STUDY_DOOR_X0 - 0.08f, Z1 - 0.08f,
                          STUDY_DOOR_X1 + 0.08f, Z1 + 0.08f);
        g_studyDoorSlot = Collision::ClaimDynamic();

        // ---- interactables ----
        // the secret red book (journal) on the separate shelf
        Interaction::Add(Interaction::ID_SECRETBOOK, 17.8f, 1.6f, 21.4f,
                         2.0f, "Examine Book");
        // the stopped clock
        Interaction::Add(Interaction::ID_STOPCLOCK, 9.3f, 1.3f, 22.6f,
                         1.8f, "Examine Clock");
        // the founder painting with its brass plate
        Interaction::Add(Interaction::ID_FOUNDER, 10.0f, 1.7f, Z0,
                         2.0f, "Examine Painting");
        // the safe
        Interaction::Add(Interaction::ID_SAFE, SAFE_X - 0.4f, 0.9f, SAFE_Z,
                         1.8f, "Use Safe");
        // the brass key (interactable only once the safe is open)
        Interaction::Add(Interaction::ID_BRASSKEY, 18.2f, 0.8f, 21.9f,
                         1.6f, "Pick Up Key");
        Interaction::SetEnabled(Interaction::ID_BRASSKEY, false);
        // the study door: locked/unlocked variants (toggled by state)
        Interaction::Add(Interaction::ID_STUDYDOOR,
                         (STUDY_DOOR_X0+STUDY_DOOR_X1)/2, 1.4f, Z1,
                         1.7f, "Open Door");
        Interaction::Add(Interaction::ID_STUDYCLOSE,
                         (STUDY_DOOR_X0+STUDY_DOOR_X1)/2, 1.4f, Z1,
                         1.7f, "Close Door");
        Interaction::SetEnabled(Interaction::ID_STUDYCLOSE, false);
    }

    void UpdateLights(float timeSec)
    {
        // L0: chandelier - warm center light (dominant)
        Lighting_SetPointLight(GL_LIGHT0, 14.0f, 3.2f, 18.5f,
                               1.0f, 0.88f, 0.62f, 1.25f);
        // L1: the banker lamp on the reading table
        Lighting_SetPointLight(GL_LIGHT1, 13.45f, 1.15f, 19.05f,
                               1.0f, 0.85f, 0.58f, 0.9f);
        // L2: library fireplace glow (gentle flicker)
        const float fl = 0.85f + 0.15f * Objects::FireFlicker(timeSec);
        Lighting_SetPointLight(GL_LIGHT2, X0 + 0.9f, 0.9f, 21.6f,
                               1.0f, 0.55f, 0.25f, 0.9f * fl);
    }

    void UpdateFill()
    {
        // L6: standing lamps' fill (north end of the library)
        Lighting_SetPointLight(GL_LIGHT6, 16.2f, 1.35f, 22.7f,
                               1.0f, 0.85f, 0.60f, 0.55f);
    }

    void Draw(float timeSec)
    {
        DrawFloor();
        DrawCeiling();
        DrawWestWall();
        DrawEastWall();
        DrawNorthWall();
        DrawSouthWall();
        DrawFurniture(timeSec);
        DrawFixtures(timeSec);

        // safe door swing after the correct code
        if (Progress::Get(Progress::SAFE_OPEN) && g_safeAngle < SAFE_OPEN_ANGLE)
            g_safeAngle += 90.0f * (1.0f/60.0f);
        if (g_safeAngle > SAFE_OPEN_ANGLE) g_safeAngle = SAFE_OPEN_ANGLE;
        g_safeOpen = g_safeAngle >= SAFE_OPEN_ANGLE - 1.0f;
    }

    bool IsSafeOpen()       { return g_safeOpen; }
    float SafeX()           { return SAFE_X; }
    float SafeZ()           { return SAFE_Z; }
}
