// ============================================================
//  THE LAST NIGHT - Study.cpp
//  The private Study: a big antique desk (with a hidden drawer),
//  a leather chair, bookshelves, a small fireplace, a large window
//  with the blue evening outside, and the story clues: the old
//  photograph (scratched-out figure + crest), the journal, and the
//  hidden note pointing to the garden.
//
//  Light slots while the player is here: L0 ceiling, L1 desk lamp,
//  L2 fireplace, L6 standing-lamp fill.
// ============================================================
#include "Study.h"
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
    const float X0 = 12.25f, X1 = 16.25f;   // room x extent
    const float Z0 = 23.5f,  Z1 = 29.5f;    // room z extent
    const float WH = 3.4f;                  // lower, cozier ceiling
    const float WT = 0.25f;

    const float DOOR_X0 = 13.65f, DOOR_X1 = 14.85f;  // south doorway
    const float WIN_X0 = 13.4f,  WIN_X1 = 15.1f;     // north window
    const float WIN_Y0 = 1.0f,   WIN_Y1 = 2.8f;

    const float WX_W = X0 - WT/2, WX_E = X1 + WT/2;
    const float WZ_S = Z0 - WT/2, WZ_N = Z1 + WT/2;

    const float DESK_X = 14.25f, DESK_Z = 26.6f;

    // chair faces the desk; fireplace on the west wall
    const float FIRE_X = X0 + 0.05f, FIRE_Z = 27.8f;

    // drawer slide state
    float g_drawerOut = 0.0f;               // 0 = closed .. 1 = open
    bool  g_drawerOpening = false;
    const float DRAWER_SLIDE = 0.32f;

    // map closet door angle (east wall)
    float g_closetAngle = 0.0f;
    const float CLOSET_OPEN_ANGLE = 95.0f;

    const Material LEATHER_BURG = {{0.16f, 0.06f, 0.06f, 1}, {0.48f, 0.14f, 0.12f, 1}, {0.12f, 0.08f, 0.06f, 1}, 8.0f};

    void SetSurface(const Material &m, const char *tex)
    {
        Lighting_ApplyMaterial(m);
        if (tex) Textures::Bind(tex);
    }

    void Box(float sx, float sy, float sz) { Objects::Box(sx, sy, sz); }

    void Box(const Material &m, const char *tex,
             float cx, float cy, float cz, float sx, float sy, float sz)
    {
        glPushMatrix();
        glTranslatef(cx, cy, cz);
        Objects::TexturedBox(m, tex, sx, sy, sz);
        glPopMatrix();
    }

    // The recurring mansion crest: a circle + triangle + night-sky moon,
    // built from flat quads - carved into wood, printed on paper, etc.
    void Crest(float size, const float ink[4])
    {
        const float s = size;
        glColor4f(ink[0], ink[1], ink[2], ink[3]);
        // circle (fan of triangles)
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(0, 0, 0);
        for (int i = 0; i <= 16; ++i)
        {
            float a = i * 2 * M_PI / 16;
            glVertex3f(cosf(a) * s * 0.55f, sinf(a) * s * 0.55f, 0);
        }
        glEnd();
        // triangle inside
        glBegin(GL_TRIANGLES);
        glVertex3f(0, s * 0.34f, 0.001f);
        glVertex3f(-s * 0.30f, -s * 0.22f, 0.001f);
        glVertex3f( s * 0.30f, -s * 0.22f, 0.001f);
        glEnd();
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
        Textures::Bind("hallwood");
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
    }

    void DrawWalls()
    {
        // south wall with the doorway
        float segW = DOOR_X0 - (X0 - WT);
        float segE = (X1 + WT) - DOOR_X1;
        Box(Materials::FABRIC_C, "wall", X0 - WT/2 + segW/2, WH/2, WZ_S, segW, WH, WT);
        Box(Materials::FABRIC_C, "wall", X1 + WT/2 - segE/2, WH/2, WZ_S, segE, WH, WT);
        Box(Materials::FABRIC_C, "wall", (DOOR_X0+DOOR_X1)/2, (2.9f + WH)/2, WZ_S,
            DOOR_X1 - DOOR_X0, WH - 2.9f, WT);
        // frame
        Box(Materials::DARKWOOD, "darkwood", DOOR_X0 - 0.05f, 1.45f, WZ_S, 0.1f, 2.9f, 0.18f);
        Box(Materials::DARKWOOD, "darkwood", DOOR_X1 + 0.05f, 1.45f, WZ_S, 0.1f, 2.9f, 0.18f);
        Box(Materials::DARKWOOD, "darkwood", (DOOR_X0+DOOR_X1)/2, 2.92f, WZ_S, 0.3f, 0.1f, 0.18f);

        // west wall (fireplace wall) - full
        Box(Materials::FABRIC_C, "wall", WX_W, WH/2, (Z0+Z1)/2, WT, WH, (Z1-Z0));
        // east wall - full
        Box(Materials::FABRIC_C, "wall", WX_E, WH/2, (Z0+Z1)/2, WT, WH, (Z1-Z0));

        // north wall with the window
        Box(Materials::FABRIC_C, "wall", X0 + (WIN_X0 - X0)/2, WH/2, WZ_N, WIN_X0 - X0, WH, WT);
        Box(Materials::FABRIC_C, "wall", X1 - (X1 - WIN_X1)/2, WH/2, WZ_N, X1 - WIN_X1, WH, WT);
        Box(Materials::FABRIC_C, "wall", (WIN_X0+WIN_X1)/2, (WIN_Y1 + WH)/2, WZ_N,
            WIN_X1 - WIN_X0, WH - WIN_Y1, WT);
        Box(Materials::FABRIC_C, "wall", (WIN_X0+WIN_X1)/2, WIN_Y0/2, WZ_N,
            WIN_X1 - WIN_X0, WIN_Y0, WT);

        // window frame + mullions + sill
        const float fw = 0.08f;
        Box(Materials::WOOD, "wood", (WIN_X0+WIN_X1)/2, WIN_Y0 - fw/2, WZ_N, WIN_X1-WIN_X0+2*fw, fw, 0.14f);
        Box(Materials::WOOD, "wood", (WIN_X0+WIN_X1)/2, WIN_Y1 + fw/2, WZ_N, WIN_X1-WIN_X0+2*fw, fw, 0.14f);
        Box(Materials::WOOD, "wood", WIN_X0 - fw/2, (WIN_Y0+WIN_Y1)/2, WZ_N, fw, WIN_Y1-WIN_Y0+2*fw, 0.14f);
        Box(Materials::WOOD, "wood", WIN_X1 + fw/2, (WIN_Y0+WIN_Y1)/2, WZ_N, fw, WIN_Y1-WIN_Y0+2*fw, 0.14f);
        Box(Materials::WOOD, "wood", (WIN_X0+WIN_X1)/2, (WIN_Y0+WIN_Y1)/2, WZ_N, 0.05f, WIN_Y1-WIN_Y0, 0.07f);
        Box(Materials::WOOD, "wood", (WIN_X0+WIN_X1)/2, (WIN_Y0+WIN_Y1)/2, WZ_N, WIN_X1-WIN_X0, 0.05f, 0.07f);
        Box(Materials::MARBLE, "marble", (WIN_X0+WIN_X1)/2, WIN_Y0 - 0.06f, WZ_N - 0.15f, 2.0f, 0.07f, 0.4f);

        // exterior backdrop: blue evening, lawn, trees (self-lit)
        glDisable(GL_LIGHTING);
        glDepthMask(GL_FALSE);
        Textures::Bind("nightsky");
        glBegin(GL_QUADS);
            glColor3f(1, 1, 1);
            glTexCoord2f(0, 0); glVertex3f(WIN_X0, WIN_Y0, WZ_N + 3.0f);
            glTexCoord2f(1, 0); glVertex3f(WIN_X1, WIN_Y0, WZ_N + 3.0f);
            glTexCoord2f(1, 1); glVertex3f(WIN_X1, WIN_Y1 + 2.0f, WZ_N + 3.0f);
            glTexCoord2f(0, 1); glVertex3f(WIN_X0, WIN_Y1 + 2.0f, WZ_N + 3.0f);
        glEnd();
        Textures::Bind("grass");
        glBegin(GL_QUADS);
            glColor3f(0.65f, 0.78f, 0.62f);
            glVertex3f(WIN_X0 - 1, 0, WZ_N + 3.0f);
            glVertex3f(WIN_X1 + 1, 0, WZ_N + 3.0f);
            glVertex3f(WIN_X1 + 1, 1.0f, WZ_N + 3.0f);
            glVertex3f(WIN_X0 - 1, 1.0f, WZ_N + 3.0f);
        glEnd();
        glColor3f(0.05f, 0.10f, 0.07f);
        for (int i = 0; i < 2; ++i)
        {
            float tx = WIN_X0 + 0.6f + i*0.9f;
            glBegin(GL_TRIANGLES);
            glVertex3f(tx - 0.4f, 1.0f, WZ_N + 3.0f);
            glVertex3f(tx + 0.4f, 1.0f, WZ_N + 3.0f);
            glVertex3f(tx, 3.0f + 0.3f*i, WZ_N + 3.0f);
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
            glVertex3f(WIN_X0, WIN_Y0, WZ_N - 0.02f);
            glVertex3f(WIN_X1, WIN_Y0, WZ_N - 0.02f);
            glVertex3f(WIN_X1, WIN_Y1, WZ_N - 0.02f);
            glVertex3f(WIN_X0, WIN_Y1, WZ_N - 0.02f);
        glEnd();
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        // curtains
        glPushMatrix();
        glTranslatef((WIN_X0+WIN_X1)/2, 0, WZ_N - 0.2f);
        glRotatef(180, 0, 1, 0);
        Objects::Curtains(WIN_X1 - WIN_X0 + 0.3f, WIN_Y1 - WIN_Y0 + 0.4f);
        glPopMatrix();
    }

    // ------------------------------------------------------------------
    //  Furniture and clues
    // ------------------------------------------------------------------
    void DrawDesk(float timeSec)
    {
        glPushMatrix();
        glTranslatef(DESK_X, 0, DESK_Z);

        // ---- desk body: top, side panels, back panel -------------------
        SetSurface(Materials::DARKWOOD, "darkwood");
        glPushMatrix(); glTranslatef(0, 0.78f, 0); Box(2.2f, 0.09f, 1.1f); glPopMatrix();
        glPushMatrix(); glTranslatef(-1.02f, 0.39f, 0); Box(0.08f, 0.78f, 1.0f); glPopMatrix();
        glPushMatrix(); glTranslatef( 1.02f, 0.39f, 0); Box(0.08f, 0.78f, 1.0f); glPopMatrix();
        glPushMatrix(); glTranslatef(0, 0.39f, -0.48f); Box(2.0f, 0.78f, 0.06f); glPopMatrix();
        // decorative top trim
        glPushMatrix(); glTranslatef(0, 0.72f, 0.53f); Box(2.2f, 0.10f, 0.05f); glPopMatrix();

        // ---- drawer stack on the player side (+z face) -----------------
        // two false drawers + one REAL opening drawer (top)
        const float dw = 0.56f, dh = 0.20f;
        // top drawer: slides out by DRAWER_SLIDE * g_drawerOut
        glPushMatrix();
        glTranslatef(0.0f, 0.60f, 0.48f + g_drawerOut * DRAWER_SLIDE);
        SetSurface(Materials::WOOD, "wood");
        Box(dw, dh, 0.42f);
        // brass handle
        SetSurface(Materials::GOLD, 0);
        glPushMatrix(); glTranslatef(0, 0, 0.225f); Box(0.16f, 0.035f, 0.03f); glPopMatrix();
        // inside: the hidden note (visible when open), lying in the tray
        if (g_drawerOut > 0.5f && !Progress::Get(Progress::NOTE_TAKEN))
        {
            glPushMatrix();
            glTranslatef(0, 0.06f, 0.05f);
            SetSurface(Materials::Tinted(Materials::BOOK, 1.1f, 1.05f, 0.85f), 0);
            Box(0.30f, 0.015f, 0.20f);
            glPopMatrix();
        }
        glPopMatrix();
        // two lower false drawers
        SetSurface(Materials::WOOD, "wood");
        glPushMatrix(); glTranslatef(0, 0.36f, 0.48f); Box(dw, dh, 0.03f); glPopMatrix();
        SetSurface(Materials::GOLD, 0);
        glPushMatrix(); glTranslatef(0, 0.36f, 0.50f); Box(0.16f, 0.035f, 0.02f); glPopMatrix();
        SetSurface(Materials::WOOD, "wood");
        glPushMatrix(); glTranslatef(0, 0.13f, 0.48f); Box(dw, dh, 0.03f); glPopMatrix();
        SetSurface(Materials::GOLD, 0);
        glPushMatrix(); glTranslatef(0, 0.13f, 0.50f); Box(0.16f, 0.035f, 0.02f); glPopMatrix();

        // ---- ON THE DESK ------------------------------------------------
        // the old photograph (interactable) - slightly angled
        glPushMatrix();
        glTranslatef(-0.55f, 0.83f, -0.10f);
        glRotatef(-18, 0, 1, 0);
        SetSurface(Materials::Tinted(Materials::BOOK, 0.95f, 0.9f, 0.8f), 0);
        Box(0.24f, 0.012f, 0.18f);
        // faded image hint: a soft rectangle "family"
        glColor4f(0.35f, 0.33f, 0.30f, 1.0f);
        glPushMatrix(); glTranslatef(0, 0.008f, 0); Box(0.18f, 0.004f, 0.12f); glPopMatrix();
        glPopMatrix();
        // the mansion crest embossed on the photo's back? No - on the wall,
        // behind the desk (see DrawDecor). Here: journal + pen + ink + box.
        // journal (interactable), leather-bound
        glPushMatrix();
        glTranslatef(0.15f, 0.85f, 0.18f);
        glRotatef(24, 0, 1, 0);
        SetSurface(LEATHER_BURG, 0);
        Box(0.34f, 0.06f, 0.26f);
        SetSurface(Materials::GOLD, 0);
        glPushMatrix(); glTranslatef(0, 0.033f, 0); Box(0.30f, 0.006f, 0.05f); glPopMatrix();
        glPopMatrix();
        // fountain pen
        SetSurface(Materials::METAL, 0);
        glPushMatrix(); glTranslatef(-0.15f, 0.835f, 0.30f); glRotatef(65, 0, 1, 0);
        Box(0.22f, 0.012f, 0.012f); glPopMatrix();
        // ink bottle
        SetSurface(Materials::GLASS, 0);
        glPushMatrix(); glTranslatef(0.48f, 0.86f, 0.30f); glScalef(1, 1.5f, 1);
        glutSolidSphere(0.05f, 12, 10); glPopMatrix();
        // closed book
        SetSurface(Materials::Tinted(Materials::BOOK, 0.5f, 0.7f, 1.05f), "books");
        glPushMatrix(); glTranslatef(0.80f, 0.85f, -0.05f); glRotatef(-9, 0, 1, 0);
        Box(0.30f, 0.07f, 0.22f); glPopMatrix();
        // envelope
        SetSurface(Materials::Tinted(Materials::BOOK, 1.12f, 1.06f, 0.88f), 0);
        glPushMatrix(); glTranslatef(-0.80f, 0.835f, 0.28f); glRotatef(40, 0, 1, 0);
        Box(0.22f, 0.012f, 0.15f); glPopMatrix();
        // decorative wooden box
        SetSurface(Materials::WOOD, "wood");
        glPushMatrix(); glTranslatef(0.62f, 0.87f, -0.32f);
        Box(0.22f, 0.10f, 0.16f); glPopMatrix();
        // desk lamp (light L1 anchor just above the shade)
        glPushMatrix(); glTranslatef(-0.85f, 0.825f, -0.28f); Objects::DeskLamp(timeSec); glPopMatrix();

        glPopMatrix(); // desk
    }

    void DrawChair()
    {
        // the master's leather chair, behind the desk (facing -z)
        glPushMatrix();
        glTranslatef(DESK_X, 0, DESK_Z + 1.05f);
        SetSurface(LEATHER_BURG, 0);
        // seat
        glPushMatrix(); glTranslatef(0, 0.50f, 0); Box(0.55f, 0.10f, 0.55f); glPopMatrix();
        // high back
        glPushMatrix(); glTranslatef(0, 1.02f, 0.24f); Box(0.55f, 0.95f, 0.12f); glPopMatrix();
        // arms
        glPushMatrix(); glTranslatef(-0.26f, 0.68f, 0); Box(0.06f, 0.30f, 0.50f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.26f, 0.68f, 0); Box(0.06f, 0.30f, 0.50f); glPopMatrix();
        // wooden legs
        SetSurface(Materials::DARKWOOD, "darkwood");
        for (int i = 0; i < 4; ++i)
        {
            float lx = (i % 2) ? 0.22f : -0.22f;
            float lz = (i / 2) ? 0.22f : -0.22f;
            glPushMatrix(); glTranslatef(lx, 0.22f, lz); Box(0.06f, 0.44f, 0.06f); glPopMatrix();
        }
        glPopMatrix();
    }

    void DrawDecor(float timeSec)
    {
        // ---- two bookshelves on the east wall ---------------------------
        glPushMatrix(); glTranslatef(X1 - 0.24f, 0, 24.8f); glRotatef(-90, 0, 1, 0);
        // reuse the plain builder (not the library's special one)
        Objects::Bookshelf(2.4f, 2.6f);
        glPopMatrix();
        glPushMatrix(); glTranslatef(X1 - 0.24f, 0, 27.6f); glRotatef(-90, 0, 1, 0);
        Objects::Bookshelf(2.4f, 2.6f);
        glPopMatrix();

        // ---- fireplace, west wall (opening faces +x into the room) ------
        glPushMatrix();
        glTranslatef(FIRE_X, 0, FIRE_Z);
        glRotatef(-90, 0, 1, 0);
        Objects::Fireplace(true, timeSec);
        glPopMatrix();

        // ---- the mansion CREST on the west wall, above the fireplace ----
        glPushMatrix();
        glTranslatef(FIRE_X + 0.30f, 2.9f, FIRE_Z);
        glRotatef(90, 0, 1, 0);
        const float ink[4] = {0.55f, 0.42f, 0.18f, 1.0f};
        Crest(0.45f, ink);
        glPopMatrix();

        // ---- framed photograph (flavor) + family painting ---------------
        glPushMatrix();
        glTranslatef(X0 + 0.06f, 2.2f, 25.0f);
        glRotatef(90, 0, 1, 0);
        Objects::PictureFrame(0.6f, 0.75f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(15.7f, 2.3f, Z1 - 0.06f);
        glRotatef(180, 0, 1, 0);
        Objects::Painting(1.1f, 0.85f, "paint_night");
        glPopMatrix();

        // ---- potted plants ----------------------------------------------
        glPushMatrix(); glTranslatef(X0 + 0.6f, 0, Z1 - 0.7f); Objects::PottedPlant(1.0f); glPopMatrix();
        glPushMatrix(); glTranslatef(X1 - 0.6f, 0, Z0 + 0.7f); Objects::PottedPlant(0.85f); glPopMatrix();

        // ---- small rug under the desk ------------------------------------
        glPushMatrix();
        glTranslatef(DESK_X, 0.012f, DESK_Z - 0.4f);
        Objects::Rug(2.8f, 2.0f);
        glPopMatrix();

        // ---- the map closet (east wall): opens with the mansion key ------
        glPushMatrix();
        glTranslatef(X1 - 0.30f, 0, 25.9f);
        glRotatef(-90, 0, 1, 0);
        // closet body
        SetSurface(Materials::DARKWOOD, "darkwood");
        glPushMatrix(); glTranslatef(0, 1.05f, 0); Box(1.1f, 2.1f, 0.5f); glPopMatrix();
        // closet door: hinged at the local -z edge, swings after unlock
        glPushMatrix();
        glTranslatef(0, 1.05f, 0.26f);
        glRotatef(g_closetAngle, 0, 1, 0);
        SetSurface(Materials::WOOD, "wood");
        Box(1.06f, 1.9f, 0.045f);
        SetSurface(Materials::GOLD, 0);
        glPushMatrix(); glTranslatef(0.42f, 0, 0.04f); Box(0.05f, 0.12f, 0.03f); glPopMatrix();
        glPopMatrix();
        // inside: rolled maps on the shelf (visible when open)
        if (g_closetAngle > 40.0f)
        {
            for (int m = 0; m < 3; ++m)
            {
                glPushMatrix();
                glTranslatef(-0.25f + m*0.24f, 1.42f, 0.02f);
                glRotatef(80, 0, 0, 1);
                SetSurface(Materials::Tinted(Materials::BOOK, 1.0f, 0.92f, 0.72f), 0);
                Objects::Box(0.05f, 0.42f, 0.05f);
                glPopMatrix();
            }
        }
        glPopMatrix();

        // ---- standing lamp (L6 fill anchor) ------------------------------
        glPushMatrix(); glTranslatef(X0 + 0.5f, 0, Z0 + 0.6f); Objects::TableLamp(); glPopMatrix();
    }

    void DrawFixtures(float timeSec)
    {
        // ceiling fixture: a modest brass lantern
        glPushMatrix();
        glTranslatef(DESK_X, WH, (Z0+Z1)/2);
        glRotatef(timeSec * 6.0f, 0, 1, 0);   // slow continuous rotation
        SetSurface(Materials::GOLD, 0);
        glPushMatrix(); glTranslatef(0, -0.18f, 0); Box(0.03f, 0.36f, 0.03f); glPopMatrix();
        glPushMatrix(); glTranslatef(0, -0.42f, 0); glutSolidSphere(0.08f, 12, 10); glPopMatrix();
        SetSurface(Materials::BULB, 0);
        glPushMatrix(); glTranslatef(0, -0.50f, 0); glutSolidSphere(0.055f, 10, 8); glPopMatrix();
        glPopMatrix();
    }
}

namespace Study
{
    void Load()
    {
        const float t = 0.25f;

        // south wall (doorway between DOOR_X0..DOOR_X1)
        Collision::AddBox(X0 - t, Z0 - t, DOOR_X0, Z0 + t);
        Collision::AddBox(DOOR_X1, Z0 - t, X1 + t, Z0 + t);
        // west / east walls
        Collision::AddBox(X0 - t, Z0 - t, X0 + t, Z1 + t);
        Collision::AddBox(X1 - t, Z0 - t, X1 + t, Z1 + t);
        // north wall (window, no opening)
        Collision::AddBox(X0 - t, Z1 - t, X1 + t, Z1 + t);

        // furniture
        Collision::AddBox(DESK_X - 1.15f, DESK_Z - 0.6f, DESK_X + 1.15f, DESK_Z + 0.6f); // desk
        Collision::AddBox(X0 + 0.2f, 24.1f, X1 - 0.2f, 24.8f);   // shelf south (z 24.8 edge)
        Collision::AddBox(13.9f, 27.0f, 16.1f, 28.2f);           // shelf north
        Collision::AddBox(FIRE_X - 0.2f, FIRE_Z - 0.75f, FIRE_X + 0.6f, FIRE_Z + 0.75f); // fireplace
        Collision::AddBox(13.4f, 25.6f, 14.4f, 26.2f);           // closet front
        Collision::AddBox(X0 + 0.2f, Z0 + 0.3f, X0 + 0.8f, Z0 + 0.9f);  // standing lamp

        // ---- interactables ----
        Interaction::Add(Interaction::ID_PHOTO, DESK_X - 0.55f, 1.0f, DESK_Z - 0.1f,
                         1.7f, "Examine Photograph");
        Interaction::Add(Interaction::ID_JOURNAL, DESK_X + 0.15f, 1.0f, DESK_Z + 0.18f,
                         1.7f, "Examine Journal");
        Interaction::Add(Interaction::ID_DRAWER, DESK_X, 0.8f, DESK_Z + 0.7f,
                         1.6f, "Open Drawer");
        Interaction::Add(Interaction::ID_DRAWEROPEN, DESK_X, 0.8f, DESK_Z + 0.7f,
                         1.6f, "Take Note");
        Interaction::SetEnabled(Interaction::ID_DRAWEROPEN, false);
        Interaction::Add(Interaction::ID_NOTE, DESK_X, 0.8f, DESK_Z + 0.9f,
                         1.5f, "Read Note");
        Interaction::SetEnabled(Interaction::ID_NOTE, false);
        Interaction::Add(Interaction::ID_CLOSET, X1 - 0.4f, 1.1f, 25.9f,
                         1.6f, "Open Closet");
        Interaction::Add(Interaction::ID_STUDYFIRE, FIRE_X + 0.5f, 0.8f, FIRE_Z,
                         1.5f, "Look Into Fire");
    }

    void UpdateLights(float timeSec)
    {
        // L0: brass ceiling lantern
        Lighting_SetPointLight(GL_LIGHT0, DESK_X, WH - 0.6f, (Z0+Z1)/2,
                               1.0f, 0.88f, 0.62f, 0.85f);
        // L1: desk lamp - pools warm light on the papers
        Lighting_SetPointLight(GL_LIGHT1, DESK_X - 0.85f, 1.15f, DESK_Z - 0.28f,
                               1.0f, 0.85f, 0.55f, 0.85f);
        // L2: fireplace flicker
        const float fl = 0.85f + 0.15f * Objects::FireFlicker(timeSec);
        Lighting_SetPointLight(GL_LIGHT2, FIRE_X + 0.8f, 0.9f, FIRE_Z,
                               1.0f, 0.55f, 0.25f, 0.75f * fl);
    }

    void UpdateFill()
    {
        // L6: standing lamp fill near the door
        Lighting_SetPointLight(GL_LIGHT6, X0 + 0.5f, 1.35f, Z0 + 0.6f,
                               1.0f, 0.85f, 0.60f, 0.5f);
    }

    void Draw(float timeSec)
    {
        // drawer slide animation
        if (g_drawerOpening && g_drawerOut < 1.0f)
            g_drawerOut += (1.0f/60.0f) * 2.2f;
        else if (!g_drawerOpening && g_drawerOut > 0.0f)
            g_drawerOut -= (1.0f/60.0f) * 2.2f;
        if (g_drawerOut < 0) g_drawerOut = 0;
        if (g_drawerOut > 1) g_drawerOut = 1;

        // closet door swings only after the mansion key has been used on it
        if (Progress::Get(Progress::MAP_CLOSET_OPEN) && g_closetAngle < CLOSET_OPEN_ANGLE)
            g_closetAngle += 120.0f * (1.0f/60.0f);
        if (g_closetAngle > CLOSET_OPEN_ANGLE) g_closetAngle = CLOSET_OPEN_ANGLE;

        DrawFloor();
        DrawCeiling();
        DrawWalls();
        DrawDesk(timeSec);
        DrawChair();
        DrawDecor(timeSec);
        DrawFixtures(timeSec);
    }

    void SetDrawerOpen(bool open) { g_drawerOpening = open; }
    bool IsDrawerOpen()           { return g_drawerOut > 0.9f; }
}
