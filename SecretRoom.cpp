// ============================================================
//  THE LAST NIGHT - SecretRoom.cpp
//  The hidden chamber (Phase 9). Warm, rich and clearly visible:
//  cream walls with walnut panelling, the great bronze crest with
//  three rotating rings, an investigation table covered in story
//  objects, bookshelves, the locked cabinet (master key), and a
//  window looking onto the garden's blue evening.
// ============================================================
#include "SecretRoom.h"
#include "Objects.h"
#include "Lighting.h"
#include "Collision.h"
#include "Interaction.h"
#include "Progress.h"
#include "Texture.h"
#include <GL/freeglut.h>
#include <cmath>

namespace
{
    // shell ------------------------------------------------------------
    const float X0 = 31.5f, X1 = 39.9f;      // east of the passage
    const float Z0 = -4.6f, Z1 =  2.4f;
    const float FLOOR = -3.35f;              // basement level
    const float H    =  3.2f;                // ceiling height above floor
    const float CEIL = FLOOR + H;

    // door in the WEST wall (from the passage), hinged at its +z edge
    const float DOOR_Z0 = -0.9f, DOOR_Z1 = 0.3f;
    const float DOOR_OPEN_ANGLE = 100.0f;    // swings INTO the room

    // window on the NORTH wall (x span), sill at +1.0, header at +2.9
    const float WX0 = 32.8f, WX1 = 34.4f;

    // furniture anchors
    const float TABLE_X = 35.7f, TABLE_Z = -1.1f;
    const float CAB_X   = 38.4f, CAB_Z   =  1.5f;   // SE corner cabinet

    // crest puzzle (east wall, facing the entrance)
    float g_ringAngle[3] = {0, 0, 0};
    bool  g_ringSolved[3] = {false, false, false};
    const float RING_TARGET[3] = {90.0f, 270.0f, 180.0f};
    bool  g_crestSolved = false;

    // door + cabinet animation
    float g_doorAngle = 0.0f;
    bool  g_doorOpening = false;
    float g_cabAngle = 0.0f;
    bool  g_cabOpening = false;
    int   g_doorSlot = -1;

    // ------------------------------------------------------------------
    //  helpers
    // ------------------------------------------------------------------
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
    //  shell: floor, ceiling, walls, window, panelling
    // ------------------------------------------------------------------
    void DrawShell()
    {
        // floor: polished walnut + large burgundy-gold rug
        Box(Materials::DARKWOOD, "darkwood", (X0 + X1) / 2, FLOOR - 0.05f,
            (Z0 + Z1) / 2, (X1 - X0) + 0.4f, 0.1f, (Z1 - Z0) + 0.4f);
        glPushMatrix();
        glTranslatef((X0 + X1) / 2, FLOOR + 0.012f, (Z0 + Z1) / 2);
        Objects::Rug(4.6f, 3.2f);
        glPopMatrix();

        // ceiling + beams
        Box(Materials::DARKWOOD, "darkwood", (X0 + X1) / 2, CEIL + 0.10f,
            (Z0 + Z1) / 2, (X1 - X0) + 0.4f, 0.2f, (Z1 - Z0) + 0.4f);
        for (int i = 0; i < 2; ++i)
            Box(Materials::DARKWOOD, "darkwood", 33.9f + i * 3.6f, CEIL - 0.14f,
                (Z0 + Z1) / 2, 0.22f, 0.28f, (Z1 - Z0));

        // west wall with the door opening (z DOOR_Z0..DOOR_Z1)
        Box(Materials::FABRIC_C, "wall", X0 - 0.1f, FLOOR + H / 2, (Z0 + DOOR_Z0) / 2,
            0.2f, H, (DOOR_Z0 - Z0));
        Box(Materials::FABRIC_C, "wall", X0 - 0.1f, FLOOR + H / 2, (DOOR_Z1 + Z1) / 2,
            0.2f, H, (Z1 - DOOR_Z1));
        Box(Materials::FABRIC_C, "wall", X0 - 0.1f, (2.9f + FLOOR + H) / 2, (DOOR_Z0 + DOOR_Z1) / 2,
            0.2f, (FLOOR + H) - 2.9f, (DOOR_Z1 - DOOR_Z0));
        // east wall
        Box(Materials::FABRIC_C, "wall", X1 + 0.1f, FLOOR + H / 2, (Z0 + Z1) / 2,
            0.2f, H, (Z1 - Z0) + 0.4f);
        // south wall
        Box(Materials::FABRIC_C, "wall", (X0 + X1) / 2, FLOOR + H / 2, Z1 + 0.1f,
            (X1 - X0) + 0.4f, H, 0.2f);

        // north wall: piers, sill, header around the window (x WX0..WX1)
        Box(Materials::FABRIC_C, "wall", (X0 + WX0) / 2, FLOOR + H / 2, Z0 - 0.1f,
            (WX0 - X0), H, 0.2f);
        Box(Materials::FABRIC_C, "wall", (WX1 + X1) / 2, FLOOR + H / 2, Z0 - 0.1f,
            (X1 - WX1), H, 0.2f);
        Box(Materials::FABRIC_C, "wall", (WX0 + WX1) / 2, FLOOR + 0.5f, Z0 - 0.1f,
            (WX1 - WX0), 1.0f, 0.2f);
        Box(Materials::FABRIC_C, "wall", (WX0 + WX1) / 2, (2.9f + FLOOR + H) / 2, Z0 - 0.1f,
            (WX1 - WX0), (FLOOR + H) - 2.9f, 0.2f);

        // window frame + mullions
        Box(Materials::DARKWOOD, "darkwood", (WX0 + WX1) / 2, FLOOR + 2.0f, Z0 - 0.16f,
            (WX1 - WX0) + 0.2f, 0.12f, 0.14f);                 // header
        Box(Materials::DARKWOOD, "darkwood", WX0 - 0.05f, FLOOR + 1.95f, Z0 - 0.16f,
            0.1f, 1.9f, 0.14f);
        Box(Materials::DARKWOOD, "darkwood", WX1 + 0.05f, FLOOR + 1.95f, Z0 - 0.16f,
            0.1f, 1.9f, 0.14f);
        Box(Materials::DARKWOOD, "darkwood", (WX0 + WX1) / 2, FLOOR + 1.95f, Z0 - 0.16f,
            0.06f, 1.9f, 0.12f);                               // center mullion

        // the view: unlit evening garden (grass lawn + night sky + moon)
        glPushMatrix();
        glDisable(GL_LIGHTING);
        glColor3f(1, 1, 1);
        glEnable(GL_TEXTURE_2D);
        // sky
        Textures::Bind("nightsky");
        glBegin(GL_QUADS);
        glNormal3f(0, 0, 1);
        glTexCoord2f(0, 0); glVertex3f(29.0f, FLOOR + 2.6f, Z0 - 1.2f);
        glTexCoord2f(1, 0); glVertex3f(38.5f, FLOOR + 2.6f, Z0 - 1.2f);
        glTexCoord2f(1, 1); glVertex3f(38.5f, FLOOR + 9.0f, Z0 - 1.2f);
        glTexCoord2f(0, 1); glVertex3f(29.0f, FLOOR + 9.0f, Z0 - 1.2f);
        glEnd();
        // moon
        glDisable(GL_TEXTURE_2D);
        glBegin(GL_TRIANGLE_FAN);
        glColor3f(0.92f, 0.90f, 0.78f);
        for (int i = 0; i <= 20; ++i)
        {
            float a = i * 2 * 3.1415926f / 20;
            glVertex3f(33.4f + cosf(a) * 0.4f, FLOOR + 6.2f + sinf(a) * 0.4f, Z0 - 1.1f);
        }
        glEnd();
        // lawn receding away
        glEnable(GL_TEXTURE_2D);
        Textures::Bind("grass");
        glBegin(GL_QUADS);
        glColor3f(0.55f, 0.72f, 0.55f);
        glTexCoord2f(0, 0); glVertex3f(29.0f, FLOOR + 0.01f, Z0 - 0.25f);
        glTexCoord2f(8, 0); glVertex3f(38.5f, FLOOR + 0.01f, Z0 - 0.25f);
        glTexCoord2f(8, 8); glVertex3f(38.5f, FLOOR + 0.01f, Z0 - 1.2f);
        glTexCoord2f(0, 8); glVertex3f(29.0f, FLOOR + 0.01f, Z0 - 1.2f);
        glEnd();
        glEnable(GL_LIGHTING);
        glPopMatrix();

        // curtains
        glPushMatrix();
        glTranslatef((WX0 + WX1) / 2, FLOOR + 1.0f, Z0 - 0.02f);
        glRotatef(180, 0, 1, 0);
        Objects::Curtains((WX1 - WX0) + 0.5f, 2.1f);
        glPopMatrix();

        // dark lower panelling all round (visual rhythm)
        const float py = FLOOR + 0.5f;
        Box(Materials::DARKWOOD, "darkwood", (X0 + X1) / 2, py, Z1 - 0.02f, (X1 - X0), 1.0f, 0.04f);
        Box(Materials::DARKWOOD, "darkwood", (X0 + X1) / 2, py, Z0 + 0.02f, (X1 - X0), 1.0f, 0.04f);
        Box(Materials::DARKWOOD, "darkwood", X0 + 0.02f, py, (Z0 + Z1) / 2, 0.04f, 1.0f, (Z1 - Z0));
        Box(Materials::DARKWOOD, "darkwood", X1 - 0.02f, py, (Z0 + Z1) / 2, 0.04f, 1.0f, (Z1 - Z0));
    }

    // ------------------------------------------------------------------
    //  the great crest wall + rotating rings + the cabinet
    // ------------------------------------------------------------------
    void DrawCrestWall(float timeSec)
    {
        (void)timeSec;
        // wood backing panel on the east wall
        Box(Materials::DARKWOOD, "darkwood", X1 - 0.03f, FLOOR + 1.8f, -1.1f, 0.06f, 2.6f, 3.2f);

        // the fixed crest carving above the rings (gold once solved)
        glPushMatrix();
        glTranslatef(X1 - 0.13f, FLOOR + 2.65f, -1.1f);
        glRotatef(-90, 0, 1, 0);
        if (g_crestSolved) Objects::Crest(0.85f, 1.0f, 0.85f, 0.35f, 1.0f);
        else               Objects::Crest(0.85f, 0.16f, 0.14f, 0.12f, 1.0f);
        glPopMatrix();

        // three rings (rotate around their own axes on E)
        for (int i = 0; i < 3; ++i)
        {
            glPushMatrix();
            glTranslatef(X1 - 0.08f, FLOOR + 1.55f, -2.15f + 1.05f * i);
            glRotatef(-90, 0, 1, 0);
            Objects::CrestDial(0.55f, g_ringAngle[i], g_crestSolved);
            glPopMatrix();
            if (i < 2)
                Box(Materials::DARKWOOD, "darkwood", X1 - 0.08f, FLOOR + 1.55f,
                    -1.625f + 1.05f * i, 0.05f, 0.9f, 0.06f);
        }
        // engraved caption line
        Box(Materials::Tinted(Materials::GOLD, 0.9f, 0.82f, 0.6f), 0,
            X1 - 0.10f, FLOOR + 0.85f, -1.1f, 0.03f, 0.06f, 2.6f);

        // ---- the cabinet (SE corner) ----
        glPushMatrix();
        glTranslatef(CAB_X, FLOOR, CAB_Z);
        glRotatef(-90, 0, 1, 0);
        Objects::SmallCabinet();
        // door: left panel fixed, right panel swings (or stays shut)
        glPushMatrix();
        glTranslatef(-0.355f, 0.60f, 0.225f);
        SetSurface(Materials::DARKWOOD, "darkwood");
        Box(0.68f, 0.92f, 0.04f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(0.355f, 0.60f, 0.225f);
        if (g_cabAngle > 0.0f)
        {
            glTranslatef(0.34f, 0, 0);
            glRotatef(g_cabAngle, 0, 1, 0);
            glTranslatef(-0.34f, 0, 0);
        }
        SetSurface(Materials::DARKWOOD, "darkwood");
        Box(0.68f, 0.92f, 0.04f);
        SetSurface(Materials::GOLD, 0);
        glPushMatrix(); glTranslatef(-0.30f, 0, 0.035f); Box(0.04f, 0.10f, 0.03f); glPopMatrix();
        glPopMatrix();
        // interior hint: the master key waits inside once open
        if (g_cabOpening && !Progress::Get(Progress::HAS_MASTER_KEY))
        {
            glPushMatrix();
            glTranslatef(0.20f, 0.28f, 0.05f);
            glRotatef(90, 0, 1, 0);
            Objects::BrassKey();
            glPopMatrix();
        }
        glPopMatrix();
    }

    // ------------------------------------------------------------------
    //  the investigation table and its story objects
    // ------------------------------------------------------------------
    void DrawTable(float timeSec)
    {
        glPushMatrix();
        glTranslatef(TABLE_X, FLOOR, TABLE_Z);
        Objects::DrawingTable();

        // mansion blueprint (large, pale blue paper)
        glPushMatrix();
        glTranslatef(-0.55f, 0.84f, 0.10f);
        glRotatef(-8, 0, 1, 0);
        SetSurface(Materials::Tinted(Materials::BOOK, 0.78f, 0.86f, 1.05f), 0);
        Box(1.05f, 0.015f, 0.75f);
        SetSurface(Materials::Tinted(Materials::GOLD, 0.8f, 0.85f, 1.0f), 0);
        glPushMatrix(); glTranslatef(0, 0.012f, 0.02f); Box(0.5f, 0.012f, 0.35f); glPopMatrix();
        glPushMatrix(); glTranslatef(-0.12f, 0.012f, -0.18f); Box(0.34f, 0.012f, 0.24f); glPopMatrix();
        glPopMatrix();

        // old grounds map (aged cream) with the red marked location
        glPushMatrix();
        glTranslatef(0.62f, 0.84f, -0.28f);
        glRotatef(21, 0, 1, 0);
        SetSurface(Materials::Tinted(Materials::BOOK, 1.12f, 1.02f, 0.80f), 0);
        Box(0.85f, 0.015f, 0.60f);
        SetSurface(Materials::Tinted(Materials::BOOK, 1.2f, 0.35f, 0.30f), 0);
        glPushMatrix(); glTranslatef(0.08f, 0.012f, 0.05f); Box(0.16f, 0.012f, 0.12f); glPopMatrix();
        glPopMatrix();

        // the important photograph (framed, face up)
        glPushMatrix();
        glTranslatef(-0.15f, 0.85f, -0.42f);
        glRotatef(12, 0, 1, 0);
        Objects::PictureFrame(0.30f, 0.24f);
        glPopMatrix();

        // journal (closed, leather, strap)
        glPushMatrix();
        glTranslatef(0.30f, 0.86f, 0.32f);
        glRotatef(-28, 0, 1, 0);
        SetSurface(Materials::FABRIC, 0);
        Box(0.34f, 0.05f, 0.26f);
        SetSurface(Materials::GOLD, 0);
        glPushMatrix(); glTranslatef(0, 0.005f, 0); Box(0.05f, 0.055f, 0.27f); glPopMatrix();
        glPopMatrix();

        // fountain pen + ink bottle
        SetSurface(Materials::GOLD, 0);
        glPushMatrix(); glTranslatef(0.75f, 0.855f, 0.30f); glRotatef(35, 0, 1, 0);
        Box(0.30f, 0.02f, 0.025f); glPopMatrix();
        SetSurface(Materials::Tinted(Materials::GLASS, 0.4f, 0.4f, 0.5f), 0);
        glPushMatrix(); glTranslatef(0.92f, 0.87f, 0.18f); glutSolidCylinder(0.045f, 0.09f, 8, 4); glPopMatrix();

        // lit candle (amber glow over the papers)
        glPushMatrix(); glTranslatef(-0.85f, 0.84f, -0.40f);
        Objects::Candle(0.22f, true, timeSec); glPopMatrix();

        // magnifying glass
        SetSurface(Materials::GOLD, 0);
        glPushMatrix(); glTranslatef(0.55f, 0.85f, 0.48f); glRotatef(-90, 1, 0, 0);
        glutSolidTorus(0.012f, 0.055f, 6, 14); glPopMatrix();
        glPopMatrix();
    }

    // ------------------------------------------------------------------
    //  bookshelves + decor
    // ------------------------------------------------------------------
    void DrawFurnishings(float timeSec)
    {
        (void)timeSec;
        // two tall shelves flanking the north window + one on the south wall
        glPushMatrix();
        glTranslatef(32.6f, FLOOR, Z0 + 0.45f);
        glRotatef(180, 0, 1, 0);
        Objects::Bookshelf(2.0f, 2.9f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(38.9f, FLOOR, Z0 + 0.45f);
        glRotatef(180, 0, 1, 0);
        Objects::Bookshelf(1.7f, 2.9f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(36.2f, FLOOR, Z1 - 0.45f);
        Objects::Bookshelf(1.8f, 2.7f);
        glPopMatrix();

        // potted plant + globe
        glPushMatrix();
        glTranslatef(39.0f, FLOOR, 1.4f);
        Objects::PottedPlant(1.1f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(38.6f, FLOOR, -3.6f);
        Objects::Globe(1.0f);
        glPopMatrix();

        // three framed pictures on the west and south walls
        glPushMatrix();
        glTranslatef(X0 + 0.06f, FLOOR + 2.1f, -3.2f);
        glRotatef(90, 0, 1, 0);
        Objects::Painting(0.9f, 0.7f, "landscape");
        glPopMatrix();
        glPushMatrix();
        glTranslatef(X0 + 0.06f, FLOOR + 2.1f, 1.6f);
        glRotatef(90, 0, 1, 0);
        Objects::Painting(0.8f, 0.65f, "portrait");
        glPopMatrix();
        glPushMatrix();
        glTranslatef(34.6f, FLOOR + 2.0f, Z1 - 0.06f);
        glRotatef(180, 0, 1, 0);
        Objects::Painting(0.85f, 0.65f, "garden");
        glPopMatrix();
    }

    // ------------------------------------------------------------------
    //  the secret door (west wall opening)
    // ------------------------------------------------------------------
    void DrawDoor()
    {
        // frame
        Box(Materials::DARKWOOD, "darkwood", X0, FLOOR + 1.45f, DOOR_Z0 - 0.06f, 0.16f, 2.9f, 0.12f);
        Box(Materials::DARKWOOD, "darkwood", X0, FLOOR + 1.45f, DOOR_Z1 + 0.06f, 0.16f, 2.9f, 0.12f);
        Box(Materials::DARKWOOD, "darkwood", X0, FLOOR + 2.92f, (DOOR_Z0 + DOOR_Z1) / 2, 0.16f, 0.12f, (DOOR_Z1 - DOOR_Z0) + 0.24f);
        // the panel: hinged at (X0, DOOR_Z1), lying along -z when closed,
        // swinging INTO the room (+x). 90deg lays the +x panel into the
        // wall plane; subtracting the angle swings it open.
        glPushMatrix();
        glTranslatef(X0, FLOOR, DOOR_Z1);
        glRotatef(90.0f - g_doorAngle, 0, 1, 0);
        Objects::Door(DOOR_Z1 - DOOR_Z0, 2.9f);
        // the crest, carved into the room face
        glPushMatrix();
        glTranslatef(0.55f, 1.55f, -0.045f);
        glRotatef(180, 0, 1, 0);
        Objects::Crest(0.30f, 0.75f, 0.6f, 0.25f, 1.0f);
        glPopMatrix();
        glPopMatrix();
    }

    // ------------------------------------------------------------------
    //  load: collision + interactables
    // ------------------------------------------------------------------
    void LoadImpl()
    {
        // perimeter walls
        Collision::AddBox(X0 - 0.3f, Z0 - 0.3f, X1 + 0.3f, Z0);            // north
        Collision::AddBox(X0 - 0.3f, Z1, X1 + 0.3f, Z1 + 0.3f);            // south
        Collision::AddBox(X1, Z0 - 0.3f, X1 + 0.3f, Z1 + 0.3f);            // east
        // west wall, north and south of the door opening
        Collision::AddBox(X0 - 0.3f, Z0 - 0.3f, X0, DOOR_Z0);
        Collision::AddBox(X0 - 0.3f, DOOR_Z1, X0, Z1 + 0.3f);

        // furniture
        Collision::AddBox(TABLE_X - 1.4f, TABLE_Z - 0.8f, TABLE_X + 1.4f, TABLE_Z + 0.8f);
        Collision::AddBox(CAB_X - 0.8f, CAB_Z - 0.35f, CAB_X + 0.8f, CAB_Z + 0.35f);
        Collision::AddBox(31.6f, Z0, 33.6f, Z0 + 0.9f);                    // shelf NW
        Collision::AddBox(38.0f, Z0, 39.8f, Z0 + 0.9f);                    // shelf NE
        Collision::AddBox(35.3f, Z1 - 0.9f, 37.1f, Z1);                    // shelf S
        Collision::AddBox(38.4f, 0.8f, 39.6f, 2.0f);                       // plant

        // secret door panel: dynamic slot (open = disable)
        Collision::AddBox(X0 - 0.08f, DOOR_Z0, X0 + 0.08f, DOOR_Z1);
        g_doorSlot = Collision::ClaimDynamic();

        // ---- interactables ----
        Interaction::Add(Interaction::ID_SRDOOR, X0 + 0.1f, FLOOR + 1.4f, (DOOR_Z0 + DOOR_Z1) / 2,
                         1.8f, "Open / Close the Door");
        Interaction::SetEnabled(Interaction::ID_SRDOOR, false);
        Interaction::Add(Interaction::ID_BLUEPRINT, TABLE_X - 0.55f, FLOOR + 0.9f, TABLE_Z + 0.1f,
                         1.7f, "Examine the Blueprint");
        Interaction::Add(Interaction::ID_SMAP, TABLE_X + 0.62f, FLOOR + 0.9f, TABLE_Z - 0.28f,
                         1.7f, "Examine the Map");
        Interaction::Add(Interaction::ID_SPHOTO, TABLE_X - 0.15f, FLOOR + 0.92f, TABLE_Z - 0.42f,
                         1.7f, "Examine the Photograph");
        Interaction::Add(Interaction::ID_SJOURNAL, TABLE_X + 0.30f, FLOOR + 0.92f, TABLE_Z + 0.32f,
                         1.7f, "Read the Journal");
        Interaction::Add(Interaction::ID_CREST, X1 - 0.3f, FLOOR + 2.6f, -1.1f,
                         6.5f, "Examine the Crest");
        Interaction::Add(Interaction::ID_RING1, X1 - 0.3f, FLOOR + 1.55f, -2.15f,
                         2.2f, "Turn the Outer Ring");
        Interaction::Add(Interaction::ID_RING2, X1 - 0.3f, FLOOR + 1.55f, -1.10f,
                         2.2f, "Turn the Middle Ring");
        Interaction::Add(Interaction::ID_RING3, X1 - 0.3f, FLOOR + 1.55f, -0.05f,
                         2.2f, "Turn the Inner Ring");
        Interaction::Add(Interaction::ID_CABINET, CAB_X - 0.1f, FLOOR + 0.9f, CAB_Z - 0.35f,
                         1.7f, "Open the Cabinet");
        Interaction::Add(Interaction::ID_CABKEY, CAB_X - 0.1f, FLOOR + 0.45f, CAB_Z - 0.1f,
                         1.4f, "Take the Master Key");
        Interaction::SetEnabled(Interaction::ID_CABKEY, false);
        Interaction::Add(Interaction::ID_SLETTER, TABLE_X + 0.05f, FLOOR + 0.88f, TABLE_Z + 0.05f,
                         1.8f, "Read the Sealed Letter");
        Interaction::Add(Interaction::ID_GLOBE, 38.6f, FLOOR + 0.7f, -3.6f,
                         1.5f, "Spin the Globe");
    }
}

// ======================================================================
//  SecretRoom namespace
// ======================================================================
namespace SecretRoom
{
    void Load() { LoadImpl(); }

    void Draw(float timeSec)
    {
        DrawShell();
        DrawCrestWall(timeSec);
        DrawTable(timeSec);
        DrawFurnishings(timeSec);
        DrawDoor();
    }

    void UpdateLights(float timeSec)
    {
        // L0: warm lantern (center) - the dark walnut needs a strong source
        Lighting_SetPointLight(GL_LIGHT0, 35.7f, CEIL - 0.8f, -1.1f,
                               1.0f, 0.85f, 0.55f, 1.65f);
        // L1: table candle (flicker) - lights the papers
        const float fl = 0.9f + 0.1f * Objects::FireFlicker(timeSec);
        Lighting_SetPointLight(GL_LIGHT1, TABLE_X - 0.85f, FLOOR + 1.15f, TABLE_Z - 0.40f,
                               1.0f, 0.70f, 0.36f, 1.15f * fl);
        // L2: warm sconces washing the crest wall (gold flood once solved)
        if (g_crestSolved)
            Lighting_SetPointLight(GL_LIGHT2, X1 - 0.6f, FLOOR + 1.8f, -1.1f,
                                   1.0f, 0.9f, 0.6f, 1.35f);
        else
            Lighting_SetPointLight(GL_LIGHT2, X1 - 0.6f, FLOOR + 1.8f, -1.1f,
                                   1.0f, 0.85f, 0.55f, 1.15f);
    }

    void UpdateFill()
    {
        // L6: cool moonlight through the window
        Lighting_SetDirectionalLight(GL_LIGHT6, 0.4f, -1.0f, 0.5f,
                                     0.55f, 0.65f, 0.9f, 0.5f);
    }

    // ---- door --------------------------------------------------------
    void SetDoorOpen(bool open) { g_doorOpening = open; }
    bool IsDoorOpen()           { return g_doorAngle > 95.0f; }
    bool IsDoorOpening()        { return g_doorOpening; }

    void UpdateDoor(float dt)
    {
        if (g_doorOpening && g_doorAngle < DOOR_OPEN_ANGLE)
            g_doorAngle += 130.0f * dt;
        else if (!g_doorOpening && g_doorAngle > 0.0f)
            g_doorAngle -= 130.0f * dt;
        if (g_doorAngle < 0) g_doorAngle = 0;
        if (g_doorAngle > DOOR_OPEN_ANGLE) g_doorAngle = DOOR_OPEN_ANGLE;

        if (g_doorSlot >= 0)
        {
            if (g_doorAngle > 95.0f)
            {
                Collision::SetDynamicBox(g_doorSlot, false, 0, 0, 0, 0);
            }
            else
            {
                // far edge of the swinging panel (hinge at z=DOOR_Z1)
                const float w = DOOR_Z1 - DOOR_Z0;
                const float rad = g_doorAngle * 3.1415926f / 180.0f;
                const float fx = X0 + sinf(rad) * w;
                const float fz = DOOR_Z1 - cosf(rad) * w;
                const float minX = (fx < X0 ? fx : X0) - 0.08f;
                const float maxX = (fx > X0 ? fx : X0) + 0.08f;
                const float minZ = (fz < DOOR_Z1 ? fz : DOOR_Z1) - 0.08f;
                const float maxZ = (fz > DOOR_Z1 ? fz : DOOR_Z1) + 0.08f;
                Collision::SetDynamicBox(g_doorSlot, true, minX, minZ, maxX, maxZ);
            }
        }
    }

    // ---- crest puzzle --------------------------------------------------
    void TurnRing(int which)
    {
        if (which < 0 || which > 2) return;
        if (g_crestSolved) return;
        g_ringAngle[which] += 90.0f;
        if (g_ringAngle[which] >= 360.0f) g_ringAngle[which] -= 360.0f;
        g_ringSolved[which] = (g_ringAngle[which] == RING_TARGET[which]);
    }

    bool CrestJustSolved()
    {
        if (g_crestSolved) return false;
        if (g_ringSolved[0] && g_ringSolved[1] && g_ringSolved[2])
        {
            g_crestSolved = true;
            return true;
        }
        return false;
    }

    bool IsCrestSolved() { return g_crestSolved; }

    // ---- cabinet -------------------------------------------------------
    void SetCabinetOpen(bool open) { g_cabOpening = open; }
    bool IsCabinetOpen()           { return g_cabAngle > 95.0f; }

    void UpdateCabinet(float dt)
    {
        if (g_cabOpening && g_cabAngle < 105.0f)
            g_cabAngle += 120.0f * dt;
        else if (!g_cabOpening && g_cabAngle > 0.0f)
            g_cabAngle -= 120.0f * dt;
        if (g_cabAngle < 0) g_cabAngle = 0;
        if (g_cabAngle > 105.0f) g_cabAngle = 105.0f;
    }

    void Reset()
    {
        for (int i = 0; i < 3; ++i) { g_ringAngle[i] = 0; g_ringSolved[i] = false; }
        g_crestSolved = false;
        g_doorAngle = 0.0f;  g_doorOpening = false;
        g_cabAngle = 0.0f;   g_cabOpening = false;
    }
}
