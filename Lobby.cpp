// ============================================================
//  THE LAST NIGHT - Lobby.cpp
//  The mansion lobby. Built from Objects::* builders placed with
//  object-coordinate transformations. Uses 4 light slots:
//    GL_LIGHT0 chandelier (warm, room center)
//    GL_LIGHT1 fireplace  (warm, flickering)
//    GL_LIGHT2 table lamp (soft corner fill)
//    GL_LIGHT3 cool moonlight wash (directional, through the front entrance)
//  GL_LIGHT7 stays reserved for the flashlight.
//
//  Room spans x [-8,+8], z [-7,+7]; floor y=0, ceiling y=4.2.
//  Front entrance in the north wall (z=-7), west-hall doors in the
//  south wall (z=+7),
//  fireplace on the west wall (x=-8).
// ============================================================
#include "Lobby.h"
#include "Objects.h"
#include "Texture.h"
#include "Collision.h"
#include "Interaction.h"
#include "Lighting.h"
#include <GL/freeglut.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace
{
    const float HX = 8.0f;
    const float HZ = 7.0f;
    const float WALL_H = 4.2f;
    const float WALL_T = 0.25f;

    const float FIRE_X  = -7.55f;   // fireplace center (west wall)
    const float LAMP_X  = 6.4f;
    const float LAMP_Z  = 5.0f;

    const float WIN_X0  = -2.6f;    // window opening (north wall)
    const float WIN_X1  =  2.6f;
    const float WIN_Y0  =  0.9f;
    const float WIN_Y1  =  3.5f;

    // The front exit is a separate exterior wall from the west hall passage.
    // Keeping it on the old shared lobby/hall opening hid the west doorway
    // behind the front leaves and made the escape trigger fire indoors.
    const float FRONT_X = 0.0f;
    const float FRONT_Z = -HZ;
    const float FRONT_HALF = 1.2f;

    const float DOOR_W  = 1.2f;     // door openings (south wall)
    const float DOOR1_X = -4.6f;
    const float DOOR2_X =  4.6f;

    void Box(const Material &m, const char *tex,
             float cx, float cy, float cz, float sx, float sy, float sz)
    {
        glPushMatrix();
        glTranslatef(cx, cy, cz);
        Objects::TexturedBox(m, tex, sx, sy, sz);
        glPopMatrix();
    }

    // ---- room shell -----------------------------------------------------
    void DrawFloor()
    {
        glPushMatrix();
        glTranslatef(0, -0.05f, 0);
        glScalef(2*HX + 1, 0.1f, 2*HZ + 1);
        Lighting_ApplyMaterial(Materials::WOOD);
        Textures::Bind("wood");
        glutSolidCube(1);
        glPopMatrix();
    }

    void DrawCeiling()
    {
        glPushMatrix();
        glTranslatef(0, WALL_H + 0.05f, 0);
        glScalef(2*HX + 1, 0.1f, 2*HZ + 1);
        Lighting_ApplyMaterial(Materials::FABRIC_C);
        Textures::Bind("plaster");
        glutSolidCube(1);
        glPopMatrix();

        // central ceiling rose / medallion under the chandelier mount
        glPushMatrix();
        glTranslatef(0, WALL_H - 0.06f, 0);
        Lighting_ApplyMaterial(Materials::GOLD);
        glutSolidSphere(0.42f, 20, 2);
        glPopMatrix();

        // decorative corner beams
        for (int sx = -1; sx <= 1; sx += 2)
            for (int sz = -1; sz <= 1; sz += 2)
            {
                glPushMatrix();
                glTranslatef(sx*(HX - 0.6f), WALL_H - 0.06f, sz*(HZ - 0.6f));
                Objects::TexturedBox(Materials::DARKWOOD, "darkwood", 1.2f, 0.12f, 0.22f);
                glPopMatrix();
            }
    }

    // garden door opening in the east wall (Phase 7)
    const float GDOOR_Z0 = -0.6f, GDOOR_Z1 = 0.6f;
    // front double-door swing (Phase 10) - shared with DrawDoorWall
    float g_frontAngle = 0.0f;

    void DrawSideWalls()
    {
        Box(Materials::FABRIC_C, "wall", -HX, WALL_H/2, 0, WALL_T, WALL_H, 2*HZ);
        // east wall: split around the garden-door opening (z -0.6..0.6)
        Box(Materials::FABRIC_C, "wall",  HX, WALL_H/2, (-HZ + GDOOR_Z0)/2,
            WALL_T, WALL_H, GDOOR_Z0 - (-HZ));
        Box(Materials::FABRIC_C, "wall",  HX, WALL_H/2, (GDOOR_Z1 + HZ)/2,
            WALL_T, WALL_H, HZ - GDOOR_Z1);
        Box(Materials::FABRIC_C, "wall",  HX, (2.9f + WALL_H)/2, 0,
            WALL_T, WALL_H - 2.9f, GDOOR_Z1 - GDOOR_Z0);
        // door frame (jamb + lintel)
        Box(Materials::DARKWOOD, "darkwood", HX, 1.45f, GDOOR_Z0 - 0.06f, 0.18f, 2.9f, 0.12f);
        Box(Materials::DARKWOOD, "darkwood", HX, 1.45f, GDOOR_Z1 + 0.06f, 0.18f, 2.9f, 0.12f);
        Box(Materials::DARKWOOD, "darkwood", HX, 2.92f, 0, 0.18f, 0.1f, 1.5f);

        // wooden wall panels (moldings)
        for (int i = 0; i < 3; ++i)
        {
            float z = -4.5f + i*4.5f;
            glPushMatrix();   // west wall, facing +X
            glTranslatef(-HX + WALL_T/2 + 0.03f, 1.95f, z);
            glRotatef(90, 0, 1, 0);
            Objects::TexturedBox(Materials::DARKWOOD, 0, 2.2f, 1.7f, 0.03f);
            glPopMatrix();
            glPushMatrix();   // east wall, facing -X
            glTranslatef(HX - WALL_T/2 - 0.03f, 1.95f, z);
            glRotatef(-90, 0, 1, 0);
            Objects::TexturedBox(Materials::DARKWOOD, 0, 2.2f, 1.7f, 0.03f);
            glPopMatrix();
        }
        // skirting boards
        Box(Materials::DARKWOOD, "darkwood", -HX + 0.17f, 0.09f, 0, 0.14f, 0.18f, 2*HZ - 0.4f);
        Box(Materials::DARKWOOD, "darkwood",  HX - 0.17f, 0.09f, 0, 0.14f, 0.18f, 2*HZ - 0.4f);
    }

    void DrawGardenBackdrop()
    {
        // Exterior seen through the window: night sky, moon, lawn, hedges.
        // Drawn WITHOUT lighting so it keeps its own evening colors
        // (cool blue sky, moon-lit lawn) instead of catching the warm
        // interior lights - and it also stays cheap to render.
        glPushMatrix();
        glTranslatef(0, 0, -HZ - 6.0f);   // 6 units beyond the window
        glDisable(GL_LIGHTING);
        glColor3f(1, 1, 1);

        // sky
        glEnable(GL_TEXTURE_2D);
        Textures::Bind("nightsky");
        glBegin(GL_QUADS);
        glNormal3f(0, 0, 1);
        glTexCoord2f(0, 0); glVertex3f(-17, 0, 0);
        glTexCoord2f(1, 0); glVertex3f( 17, 0, 0);
        glTexCoord2f(1, 1); glVertex3f( 17, 10, 0);
        glTexCoord2f(0, 1); glVertex3f(-17, 10, 0);
        glEnd();

        // moon
        glDisable(GL_TEXTURE_2D);
        glBegin(GL_TRIANGLE_FAN);
        glColor3f(0.92f, 0.90f, 0.78f);
        for (int i = 0; i <= 24; ++i)
        {
            float a = i * 2*M_PI/24;
            glVertex3f(3.0f + cosf(a)*0.55f, 7.2f + sinf(a)*0.55f, 0);
        }
        glEnd();

        // lawn stretching back toward the mansion
        glEnable(GL_TEXTURE_2D);
        Textures::Bind("grass");
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex3f(-17, -0.02f, 4.5f);
        glTexCoord2f(9, 0); glVertex3f( 17, -0.02f, 4.5f);
        glTexCoord2f(9, 9); glVertex3f( 17, -0.02f, -9.0f);
        glTexCoord2f(0, 9); glVertex3f(-17, -0.02f, -9.0f);
        glEnd();

        // hedges along the far side
        Textures::Bind("grass");
        for (int i = 0; i < 5; ++i)
        {
            glBegin(GL_QUADS);
            float x0 = -10.0f + i*5.0f - 1.3f, x1 = x0 + 2.6f;
            // front face
            glColor3f(0.55f, 0.75f, 0.5f);
            glTexCoord2f(0,0); glVertex3f(x0, 0.0f, -2.5f);
            glTexCoord2f(2,0); glVertex3f(x1, 0.0f, -2.5f);
            glTexCoord2f(2,1); glVertex3f(x1, 1.1f, -2.5f);
            glTexCoord2f(0,1); glVertex3f(x0, 1.1f, -2.5f);
            glEnd();
        }

        // small stone fountain silhouette (untextured stone gray)
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.45f, 0.42f, 0.40f);
        glBegin(GL_QUADS);
        // basin front
        glVertex3f(-0.8f, 0.0f, 1.3f); glVertex3f(0.8f, 0.0f, 1.3f);
        glVertex3f(0.8f, 0.5f, 1.3f);  glVertex3f(-0.8f, 0.5f, 1.3f);
        // pedestal front
        glVertex3f(-0.2f, 0.5f, 1.1f); glVertex3f(0.2f, 0.5f, 1.1f);
        glVertex3f(0.2f, 1.25f, 1.1f); glVertex3f(-0.2f, 1.25f, 1.1f);
        glEnd();
        glEnable(GL_TEXTURE_2D);

        glEnable(GL_LIGHTING);
        glPopMatrix();
    }

    void DrawWindowWall()
    {
        // Exterior wall and front entrance.  The old window opening let the
        // player walk through the glass plane; the solid side piers now
        // leave only the actual double-door passage open.
        const float jamb = FRONT_HALF + 0.14f;
        const float leftW = jamb - (-HX - WALL_T/2);
        const float rightW = (HX + WALL_T/2) - jamb;
        Box(Materials::FABRIC_C, "wall", (-HX - WALL_T/2 + jamb)/2,
            WALL_H/2, FRONT_Z, leftW, WALL_H, WALL_T);
        Box(Materials::FABRIC_C, "wall", (HX + WALL_T/2 + jamb)/2,
            WALL_H/2, FRONT_Z, rightW, WALL_H, WALL_T);
        Box(Materials::FABRIC_C, "wall", FRONT_X, (2.9f + WALL_H)/2,
            FRONT_Z, 2*jamb, WALL_H - 2.9f, WALL_T);

        Box(Materials::DARKWOOD, "darkwood", -jamb, 1.45f, FRONT_Z,
            0.12f, 2.9f, 0.20f);
        Box(Materials::DARKWOOD, "darkwood", jamb, 1.45f, FRONT_Z,
            0.12f, 2.9f, 0.20f);
        Box(Materials::DARKWOOD, "darkwood", FRONT_X, 2.96f, FRONT_Z,
            2*jamb + 0.08f, 0.14f, 0.20f);
        // Brass inlays and a transom plaque help the entrance stand out
        // against the dark wood and plaster, even from across the lobby.
        for (int side = -1; side <= 1; side += 2)
        {
            Box(Materials::GOLD, "white", side * 0.66f, 1.45f, FRONT_Z + 0.055f,
                0.035f, 1.12f, 0.035f);
            Box(Materials::GOLD, "white", side * 0.66f, 2.35f, FRONT_Z + 0.055f,
                0.78f, 0.035f, 0.035f);
        }
        Box(Materials::GOLD, "white", FRONT_X, 3.12f, FRONT_Z + 0.06f,
            1.25f, 0.16f, 0.035f);
        glPushMatrix();
        glTranslatef(FRONT_X, 3.42f, FRONT_Z + 0.10f);
        Objects::Crest(0.58f, 0.50f, 0.38f, 0.14f, 1.0f);
        glPopMatrix();

        const float frontLeft = FRONT_X - FRONT_HALF;
        const float frontRight = FRONT_X + FRONT_HALF;
        glPushMatrix();                                  // right leaf
        glTranslatef(frontRight, 0, FRONT_Z);
        glRotatef(-g_frontAngle, 0, 1, 0);              // swing outward (-z)
        glTranslatef(-FRONT_HALF, 0, 0);
        Objects::Door(FRONT_HALF, 2.9f);
        glPushMatrix();
        glTranslatef(-0.10f, 1.45f, -0.07f);
        Objects::TexturedBox(Materials::GOLD, 0, 0.05f, 0.34f, 0.05f);
        glPopMatrix();
        glPopMatrix();
        glPushMatrix();                                  // left leaf
        glTranslatef(frontLeft, 0, FRONT_Z);
        glRotatef(g_frontAngle, 0, 1, 0);
        glTranslatef(FRONT_HALF, 0, 0);
        glRotatef(180, 0, 1, 0);
        Objects::Door(FRONT_HALF, 2.9f);
        glPushMatrix();
        glTranslatef(0.10f, 1.45f, 0.07f);
        Objects::TexturedBox(Materials::GOLD, 0, 0.05f, 0.34f, 0.05f);
        glPopMatrix();
        glPopMatrix();

        // Path and low stone planters are real geometry outside the threshold.
        DrawGardenBackdrop();
        Box(Materials::STONE, "stone", 0.0f, -0.12f, FRONT_Z - 1.65f,
            3.8f, 0.22f, 3.4f);
        for (int side = -1; side <= 1; side += 2)
        {
            Box(Materials::STONE, "stone", side * 2.0f, 0.28f,
                FRONT_Z - 0.75f, 0.62f, 0.56f, 0.70f);
            glPushMatrix();
            glTranslatef(side * 2.0f, 0.56f, FRONT_Z - 0.75f);
            Objects::PottedPlant(0.72f);
            glPopMatrix();
        }
    }

    void DrawDoorWall()
    {
        // wall segments between/around the doors
        Box(Materials::FABRIC_C, "wall", (-HX - WALL_T/2 + (DOOR1_X - DOOR_W/2))/2, WALL_H/2, HZ,
            (DOOR1_X - DOOR_W/2) + HX + WALL_T/2, WALL_H, WALL_T);
        Box(Materials::FABRIC_C, "wall", 0, WALL_H/2, HZ,
            (DOOR2_X - DOOR_W/2) - (DOOR1_X + DOOR_W/2), WALL_H, WALL_T);
        Box(Materials::FABRIC_C, "wall", (HX + WALL_T/2 + (DOOR2_X + DOOR_W/2))/2, WALL_H/2, HZ,
            (HX + WALL_T/2) - (DOOR2_X + DOOR_W/2), WALL_H, WALL_T);
        // lintels above the doors
        Box(Materials::FABRIC_C, "wall", DOOR1_X, (2.8f + WALL_H)/2, HZ, DOOR_W + 0.2f, WALL_H - 2.8f, WALL_T);
        Box(Materials::FABRIC_C, "wall", DOOR2_X, (2.8f + WALL_H)/2, HZ, DOOR_W + 0.2f, WALL_H - 2.8f, WALL_T);

        // door frames; WEST panel is the real passage to the hall
        for (int i = 0; i < 2; ++i)
        {
            float dx = (i ? DOOR2_X : DOOR1_X);
            Box(Materials::DARKWOOD, "darkwood", dx - DOOR_W/2 - 0.05f, 1.4f, HZ, 0.1f, 2.8f, 0.18f);
            Box(Materials::DARKWOOD, "darkwood", dx + DOOR_W/2 + 0.05f, 1.4f, HZ, 0.1f, 2.8f, 0.18f);
            Box(Materials::DARKWOOD, "darkwood", dx, 2.82f, HZ, DOOR_W + 0.3f, 0.1f, 0.18f);
        }
        // The east opening remains the barred door, kept separate from the
        // hall passage on the west side.
        glPushMatrix();
        glTranslatef(DOOR2_X - DOOR_W/2, 0, HZ);      // hinge at the left edge
        Objects::Door(DOOR_W, 2.8f);
        glPopMatrix();
        // brass plate hints the east door is special (locked)
        Box(Materials::GOLD, 0, DOOR2_X + DOOR_W/2 - 0.08f, 1.05f, HZ - 0.11f, 0.05f, 0.14f, 0.03f);
    }

    // ---- furnishings ----------------------------------------------------
    void DrawFurniture(float timeSec)
    {
        // carpet under the sitting area
        glPushMatrix();
        glTranslatef(-3.0f, 0, 0.2f);
        Objects::Rug(4.4f, 3.8f);
        glPopMatrix();

        // sofa (west side of the carpet, facing east)
        glPushMatrix();
        glTranslatef(-3.05f, 0, -0.45f);
        glRotatef(90, 0, 1, 0);
        Objects::Sofa(2.1f, Materials::FABRIC);
        glPopMatrix();

        // armchairs facing the coffee table
        glPushMatrix();
        glTranslatef(-2.2f, 0, 1.55f);
        glRotatef(150, 0, 1, 0);
        Objects::Armchair(Materials::FABRIC_G);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(-3.9f, 0, 1.55f);
        glRotatef(210, 0, 1, 0);
        Objects::Armchair(Materials::FABRIC_G);
        glPopMatrix();

        // coffee table in the middle (an old letter rests on it)
        glPushMatrix();
        glTranslatef(-3.0f, 0, 0.0f);
        Objects::CoffeeTable();
        glPushMatrix();
        glTranslatef(0.25f, 0.505f, 0.12f);
        glRotatef(-28, 0, 1, 0);
        Objects::TexturedBox(Materials::FABRIC_C, 0, 0.02f, 0.28f, 0.2f);
        glPopMatrix();
        glPopMatrix();

        // side table + lamp + vase in the east corner
        glPushMatrix();
        glTranslatef(LAMP_X, 0, LAMP_Z);
        Objects::SideTable();
        glTranslatef(0, 0.62f, 0);
        Objects::TableLamp();
        glPopMatrix();
        glPushMatrix();
        glTranslatef(LAMP_X - 0.7f, 0.63f, LAMP_Z);
        Objects::VaseWithFlowers(1.0f);
        glPopMatrix();

        // grandfather clock (east wall, north corner)
        glPushMatrix();
        glTranslatef(7.5f, 0, -4.8f);
        glRotatef(-90, 0, 1, 0);
        Objects::GrandfatherClock(timeSec);
        glPopMatrix();

        // bookshelf (west wall, north of the fireplace)
        glPushMatrix();
        glTranslatef(-7.7f, 0, -3.6f);
        glRotatef(90, 0, 1, 0);
        Objects::Bookshelf(2.9f, 2.6f);
        glPopMatrix();

        // plants flanking the window
        glPushMatrix(); glTranslatef( 1.45f, 0, -6.4f); Objects::PottedPlant(1.0f);  glPopMatrix();
        glPushMatrix(); glTranslatef(-1.45f, 0, -6.4f); Objects::PottedPlant(1.15f); glPopMatrix();

        // candles on the mantle
        for (int i = 0; i < 2; ++i)
        {
            glPushMatrix();
            glTranslatef(-7.3f, 1.58f, -0.5f + i*1.0f);
            Objects::Candle(0.16f, true, timeSec + i*0.7f);
            glPopMatrix();
        }
    }

    void DrawPaintings()
    {
        // east wall: landscape + abstract
        glPushMatrix();
        glTranslatef(HX - WALL_T/2 - 0.04f, 2.25f, -1.5f);
        glRotatef(-90, 0, 1, 0);
        Objects::Painting(1.7f, 1.15f, "paint_land");
        glPopMatrix();
        glPushMatrix();
        glTranslatef(HX - WALL_T/2 - 0.04f, 2.05f, 1.8f);
        glRotatef(-90, 0, 1, 0);
        Objects::Painting(0.9f, 1.2f, "paint_abs");
        glPopMatrix();

        // south wall east of the medallion assembly: the night painting
        glPushMatrix();
        glTranslatef(1.6f, 2.15f, HZ - WALL_T/2 - 0.04f);
        glRotatef(180, 0, 1, 0);
        Objects::Painting(1.3f, 0.95f, "paint_night");
        glPopMatrix();

        // west wall above the bookshelf
        glPushMatrix();
        glTranslatef(-HX + WALL_T/2 + 0.04f, 2.6f, -3.6f);
        glRotatef(90, 0, 1, 0);
        Objects::PictureFrame(0.75f, 0.95f);
        glPopMatrix();

        // north wall piers: small frames
        glPushMatrix();
        glTranslatef(-5.4f, 3.1f, -HZ + WALL_T/2 + 0.04f);
        Objects::PictureFrame(0.6f, 0.8f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef( 5.4f, 3.1f, -HZ + WALL_T/2 + 0.04f);
        Objects::PictureFrame(0.6f, 0.8f);
        glPopMatrix();
    }

    void DrawFixtures(float timeSec)
    {
        // chandelier hangs from the ceiling center (rotates slowly)
        glPushMatrix();
        glTranslatef(0, WALL_H, 0);
        Objects::Chandelier(timeSec);
        glPopMatrix();

        // wall sconces on the window piers
        glPushMatrix(); glTranslatef(-5.4f, 2.75f, -HZ + WALL_T/2 + 0.05f); glRotatef(180, 0, 1, 0); Objects::WallSconce(); glPopMatrix();
        glPushMatrix(); glTranslatef( 5.4f, 2.75f, -HZ + WALL_T/2 + 0.05f); glRotatef(180, 0, 1, 0); Objects::WallSconce(); glPopMatrix();
    }
}

namespace Lobby
{
    // garden door animation state (Phase 7)
    float g_gardenAngle = 0.0f;
    const float GARDEN_OPEN_ANGLE = 104.0f;
    int   g_gardenSlot = -1;
    bool  g_gardenOpening = false;

    void SetGardenDoorOpen(bool open) { g_gardenOpening = open; }
    bool IsGardenDoorOpen()           { return g_gardenOpening; }

    // ---- Phase 10: the front entrance (double doors, south wall) ----
    const float FRONT_OPEN_ANGLE = 100.0f;
    int   g_frontSlotL = -1, g_frontSlotR = -1;
    bool  g_frontOpening = false;

    void SetFrontOpen(bool open) { g_frontOpening = open; }
    bool IsFrontOpen()           { return g_frontOpening; }

    // Animates the swinging garden door, draws the panel, and follows it
    // with collision (called from Game::Update, like the study door).
    void UpdateGardenDoor(float dt)
    {
        if (g_gardenOpening && g_gardenAngle < GARDEN_OPEN_ANGLE)
            g_gardenAngle += 140.0f * dt;
        else if (!g_gardenOpening && g_gardenAngle > 0.0f)
            g_gardenAngle -= 140.0f * dt;
        if (g_gardenAngle < 0) g_gardenAngle = 0;
        if (g_gardenAngle > GARDEN_OPEN_ANGLE) g_gardenAngle = GARDEN_OPEN_ANGLE;

        // draw the swinging panel (hinged at the opening's north edge,
        // opens INTO the lobby)
        glPushMatrix();
        glTranslatef(HX - 0.1f, 0, GDOOR_Z1);
        glRotatef(g_gardenAngle, 0, 1, 0);
        glTranslatef(0, 0, -(GDOOR_Z1 - GDOOR_Z0));
        Objects::Door(GDOOR_Z1 - GDOOR_Z0, 2.8f);
        glPopMatrix();

        if (g_gardenSlot >= 0)
        {
            if (g_gardenAngle >= GARDEN_OPEN_ANGLE - 2.0f)
            {
                Collision::SetDynamicBox(g_gardenSlot, false, 0,0,0,0);
            }
            else
            {
                float rad = g_gardenAngle * (float)M_PI / 180.0f;
                float hz = GDOOR_Z1;
                float px = HX - 0.1f - sinf(rad) * (GDOOR_Z1 - GDOOR_Z0);
                float pz = hz - cosf(rad) * (GDOOR_Z1 - GDOOR_Z0);
                float minX = (px < HX ? px : HX) - 0.10f;
                float maxX = (px > HX ? px : HX) + 0.10f;
                float minZ = (pz < hz ? pz : hz) - 0.10f;
                float maxZ = (pz > hz ? pz : hz) + 0.10f;
                Collision::SetDynamicBox(g_gardenSlot, true, minX, minZ, maxX, maxZ);
            }
        }
    }

    void Load()
    {
        Collision::Reset();
        Interaction::Reset();
        const float t = 0.25f;

        // exterior front wall (two solid piers around the double-door gap)
        Collision::AddBox(-HX - t, FRONT_Z - t, FRONT_X - FRONT_HALF, FRONT_Z + t);
        Collision::AddBox(FRONT_X + FRONT_HALF, FRONT_Z - t, HX + t, FRONT_Z + t);
        // south wall (two door openings)
        Collision::AddBox(-HX - t, HZ - t, DOOR1_X - DOOR_W/2, HZ + t);
        Collision::AddBox(DOOR1_X + DOOR_W/2, HZ - t, DOOR2_X - DOOR_W/2, HZ + t);
        Collision::AddBox(DOOR2_X + DOOR_W/2, HZ - t,  HX + t, HZ + t);
        // west & east walls (east split around the garden-door opening)
        Collision::AddBox(-HX - t, -HZ, -HX + t, HZ);
        Collision::AddBox( HX - t, -HZ,  HX + t, GDOOR_Z0);
        Collision::AddBox( HX - t, GDOOR_Z1, HX + t, HZ);

        // furniture (matches Draw placement, kept generous)
        Collision::AddBox(-3.6f, -1.5f, -2.5f,  0.65f);  // sofa
        Collision::AddBox(-2.75f, 1.0f, -1.65f, 2.1f);   // armchair 1
        Collision::AddBox(-4.45f, 1.0f, -3.35f, 2.1f);   // armchair 2
        Collision::AddBox(-3.8f, -0.48f, -2.2f, 0.48f);  // coffee table
        Collision::AddBox( 6.1f,  4.7f,  6.7f,  5.3f);   // side table
        Collision::AddBox( 7.2f, -5.2f,  7.8f, -4.4f);   // grandfather clock
        Collision::AddBox(-8.0f, -4.1f, -7.4f, -1.1f);   // bookshelf
        Collision::AddBox(-8.0f, -1.2f, -6.95f, 1.2f);   // fireplace
        Collision::AddBox( 1.3f, -6.6f,  1.6f, -6.2f);   // plant right
        Collision::AddBox(-1.6f, -6.6f, -1.3f, -6.2f);   // plant left

        // the south DOOR2 panel stays as a fixed blocker (the "barred" door)
        Collision::AddBox(DOOR2_X - DOOR_W/2, HZ - 0.1f,
                          DOOR2_X + DOOR_W/2, HZ + 0.1f);
        // front entrance: two dynamic slots on its own exterior wall
        Collision::AddBox(FRONT_X, FRONT_Z - 0.08f, FRONT_X + FRONT_HALF, FRONT_Z + 0.08f);
        g_frontSlotL = Collision::ClaimDynamic();
        Collision::AddBox(FRONT_X - FRONT_HALF, FRONT_Z - 0.08f, FRONT_X, FRONT_Z + 0.08f);
        g_frontSlotR = Collision::ClaimDynamic();
        // the garden door panel blocks the east opening while closed
        Collision::AddBox(HX - 0.15f, GDOOR_Z0 - 0.08f,
                          HX + 0.15f, GDOOR_Z1 + 0.08f);
        g_gardenSlot = Collision::ClaimDynamic();

        // ---- interactables ----
        Interaction::Add(Interaction::ID_LOBBYLETTER, -3.0f, 0.6f, 0.0f,
                         1.6f, "Examine Letter");
        Interaction::Add(Interaction::ID_LOBBYDOOR, DOOR2_X, 1.4f, HZ,
                         1.6f, "Try Door");
        // the garden door (unlocked with the mansion key)
        Interaction::Add(Interaction::ID_GARDENDOOR, HX - 0.2f, 1.4f, 0,
                         1.8f, "Open the Garden Door");
        // the final front door + its three brass symbols (Phase 10)
        Interaction::Add(Interaction::ID_FRONTDOOR, FRONT_X, 1.5f, FRONT_Z + 0.15f,
                         1.8f, "Open the Front Door");
        // Medallions and plaque sit on the west pier beside the front door.
        Interaction::Add(Interaction::ID_SYMBOL1, -4.4f, 1.85f, FRONT_Z + 0.15f,
                         1.6f, "Turn the Moon Symbol");
        Interaction::Add(Interaction::ID_SYMBOL2, -3.65f, 1.85f, FRONT_Z + 0.15f,
                         1.6f, "Turn the Star Symbol");
        Interaction::Add(Interaction::ID_SYMBOL3, -2.9f, 1.85f, FRONT_Z + 0.15f,
                         1.6f, "Turn the Sun Symbol");
        Interaction::Add(Interaction::ID_MEDPLAQUE, -5.25f, 1.85f, FRONT_Z + 0.15f,
                         1.7f, "Read the Brass Plaque");
    }

    // --- Phase 10: the three brass medallions beside the front door ----
    // Rendered here (Lobby.cpp owns the door) with state pushed in from
    // Game each frame, so the engraved symbol always matches the dial.
    namespace {
        const float MED_Y = 1.85f;   // medallion center height
        int   g_medDial[3] = {0, 0, 0};   // copy of Game's g_frontDial
        bool  g_medSolved = false;

        void Medallion(float x, int pos, int glyph)
        {
            // glyph: 0=crescent  1=star  2=sun
            const float R = 0.16f;
            glPushMatrix();
            glTranslatef(x, MED_Y, FRONT_Z + 0.24f);
            // Existing medallion geometry faces local -Z. Mirror that local
            // axis so the same detailed mesh faces the room on the -Z wall.
            glScalef(1.0f, 1.0f, -1.0f);

            // mounting plate
            glDisable(GL_CULL_FACE);
            Lighting_ApplyMaterial(Materials::GOLD);
            glNormal3f(0, 0, -1);
            glBegin(GL_QUADS);
            glVertex3f(-0.22f, -0.22f, 0.0f); glVertex3f(0.22f, -0.22f, 0.0f);
            glVertex3f(0.22f, 0.22f, 0.0f);   glVertex3f(-0.22f, 0.22f, 0.0f);
            glEnd();

            // brass medallion disc (billboard: -z faces the player)
            const float rad = pos * 1.5707963f;   // current quarter, radians
            glNormal3f(0, 0, -1);
            glBegin(GL_TRIANGLE_FAN);
            glVertex3f(0.0f, 0.0f, 0.0f);
            for (int i = 0; i <= 20; ++i)
            {
                const float a = i * (3.14159265f * 2.0f / 20.0f);
                glVertex3f(cosf(a) * R, sinf(a) * R, 0.0f);
            }
            glEnd();

        // engraved symbol, rotating with the dial (small z offset so
        // it sits in front of the disc). DARKWOOD reads as a dark
        // carving against the polished brass disc.
        glPushMatrix();
        glRotatef(rad * 57.2957795f, 0, 0, 1);
        Lighting_ApplyMaterial(Materials::DARKWOOD);
        glNormal3f(0, 0, -1);
        glBegin(GL_TRIANGLES);
            if (glyph == 0)          // crescent moon: two horns
            {
                glVertex3f( 0.000f, 0.075f, -0.012f);
                glVertex3f(-0.062f, -0.055f, -0.012f);
                glVertex3f(-0.018f, -0.040f, -0.012f);
                glVertex3f( 0.000f, 0.075f, -0.012f);
                glVertex3f( 0.018f, -0.040f, -0.012f);
                glVertex3f( 0.062f, -0.055f, -0.012f);
            }
            else if (glyph == 1)     // four-point star
            {
                glVertex3f( 0.000f,  0.085f, -0.012f);
                glVertex3f(-0.028f,  0.000f, -0.012f);
                glVertex3f( 0.028f,  0.000f, -0.012f);
                glVertex3f( 0.000f, -0.085f, -0.012f);
                glVertex3f(-0.028f,  0.000f, -0.012f);
                glVertex3f( 0.028f,  0.000f, -0.012f);
                glVertex3f(-0.085f,  0.000f, -0.012f);
                glVertex3f(-0.020f, -0.020f, -0.012f);
                glVertex3f(-0.020f,  0.020f, -0.012f);
                glVertex3f( 0.085f,  0.000f, -0.012f);
                glVertex3f( 0.020f,  0.020f, -0.012f);
                glVertex3f( 0.020f, -0.020f, -0.012f);
            }
            else                     // sun: disc + rays
            {
                glVertex3f( 0.000f,  0.085f, -0.012f);
                glVertex3f(-0.045f, -0.035f, -0.012f);
                glVertex3f( 0.045f, -0.035f, -0.012f);
                glVertex3f(-0.085f, -0.020f, -0.012f);
                glVertex3f(-0.045f, -0.035f, -0.012f);
                glVertex3f( 0.000f,  0.030f, -0.012f);
                glVertex3f( 0.085f, -0.020f, -0.012f);
                glVertex3f( 0.045f, -0.035f, -0.012f);
                glVertex3f( 0.000f,  0.030f, -0.012f);
            }
            glEnd();
        glPopMatrix();

        // progress pips: four notches around the rim; the active
        // quarter's pip glows warm gold, the rest stay dark metal
        for (int p = 0; p < 4; ++p)
        {
            const float a = p * 1.5707963f;
            const float px = cosf(a) * (R + 0.055f);
            const float py = sinf(a) * (R + 0.055f);
            if (p == pos)
            {
                Lighting_ApplyMaterial(Materials::GOLD);
                glColor3f(1.0f, 0.85f, 0.35f);
            }
            else
            {
                Lighting_ApplyMaterial(Materials::METAL);
                glColor3f(0.35f, 0.28f, 0.16f);
            }
            glBegin(GL_TRIANGLES);
            glVertex3f(px - 0.022f, py - 0.030f, -0.010f);
            glVertex3f(px + 0.022f, py - 0.030f, -0.010f);
            glVertex3f(px,          py + 0.035f, -0.010f);
            glEnd();
        }
        Lighting_ApplyMaterial(Materials::GOLD);   // restore
        glEnable(GL_CULL_FACE);
        glPopMatrix();
    }
    }

    // state push from Game (per frame, before Lobby::Draw)
    void SetFrontDialState(const int dial[3], bool solved)
    {
        for (int i = 0; i < 3; ++i) g_medDial[i] = dial[i];
        g_medSolved = solved;
    }

    void DrawFrontSymbols()
    {
        // Brass is untextured: every earlier draw leaves some texture
        // bound and GL_TEXTURE_2D enabled, which would tint (or even
        // texture-map) these flat parts if left active.
        glDisable(GL_TEXTURE_2D);
        Lighting_ApplyMaterial(Materials::GOLD);
        Medallion(-4.4f, g_medDial[0], 0);    // Moon
        Medallion(-3.65f, g_medDial[1], 1);   // Star
        Medallion(-2.9f, g_medDial[2], 2);    // Sun

        // the instructions plaque (ID_MEDPLAQUE) - left of the medallions,
        // engraved brass with darker text lines
        glPushMatrix();
        glTranslatef(-5.25f, MED_Y, FRONT_Z + 0.24f);
        glScalef(1.0f, 1.0f, -1.0f);
        glDisable(GL_CULL_FACE);
        Lighting_ApplyMaterial(Materials::GOLD);
        glNormal3f(0, 0, -1);
        glBegin(GL_QUADS);
        glVertex3f(-0.34f, -0.24f, 0.0f); glVertex3f(0.34f, -0.24f, 0.0f);
        glVertex3f(0.34f, 0.24f, 0.0f);   glVertex3f(-0.34f, 0.24f, 0.0f);
        glEnd();
        Lighting_ApplyMaterial(Materials::DARKWOOD);
        glNormal3f(0, 0, -1);
        glBegin(GL_TRIANGLES);                 // "I  II  III" markers
        glVertex3f(-0.24f, -0.16f, -0.008f); glVertex3f(-0.24f, -0.10f, -0.008f);
        glVertex3f(-0.14f, -0.16f, -0.008f);
        glVertex3f(-0.24f, -0.10f, -0.008f); glVertex3f(-0.14f, -0.10f, -0.008f);
        glVertex3f(-0.14f, -0.16f, -0.008f);
        glVertex3f(-0.05f, -0.16f, -0.008f); glVertex3f(-0.05f, -0.10f, -0.008f);
        glVertex3f( 0.05f, -0.16f, -0.008f);
        glVertex3f(-0.05f, -0.10f, -0.008f); glVertex3f( 0.05f, -0.10f, -0.008f);
        glVertex3f( 0.05f, -0.16f, -0.008f);
        glVertex3f( 0.14f, -0.16f, -0.008f); glVertex3f( 0.14f, -0.10f, -0.008f);
        glVertex3f( 0.14f, -0.16f, -0.008f);
        glEnd();
        Lighting_ApplyMaterial(Materials::GOLD);
        glEnable(GL_CULL_FACE);
        glPopMatrix();

        // solved feedback: a warm glow panel behind the three medallions
        if (g_medSolved)
        {
            glDisable(GL_LIGHTING);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            glColor4f(1.0f, 0.85f, 0.4f, 0.14f);
            glBegin(GL_QUADS);
            glVertex3f(-5.55f, MED_Y - 0.55f, FRONT_Z + 0.26f);
            glVertex3f(-2.55f, MED_Y - 0.55f, FRONT_Z + 0.26f);
            glVertex3f(-2.55f, MED_Y + 0.55f, FRONT_Z + 0.26f);
            glVertex3f(-5.55f, MED_Y + 0.55f, FRONT_Z + 0.26f);
            glEnd();
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
            glEnable(GL_LIGHTING);
        }

        glEnable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);       // restore the textured 3D pipeline
    }

    // Animates the double front doors, draws the swinging panels, and
    // follows them with collision (called from Game::Update).
    void UpdateFrontDoor(float dt)
    {
        if (g_frontOpening && g_frontAngle < FRONT_OPEN_ANGLE)
            g_frontAngle += 110.0f * dt;
        else if (!g_frontOpening && g_frontAngle > 0.0f)
            g_frontAngle -= 110.0f * dt;
        if (g_frontAngle < 0) g_frontAngle = 0;
        if (g_frontAngle > FRONT_OPEN_ANGLE) g_frontAngle = FRONT_OPEN_ANGLE;

        // collision follows each leaf while it swings
        const float w = 1.2f;
        auto Track = [&](int slot, float hx, bool right)
        {
            if (slot < 0) return;
            if (g_frontAngle >= FRONT_OPEN_ANGLE - 2.0f)
            {
                Collision::SetDynamicBox(slot, false, 0, 0, 0, 0);
            }
            else
            {
                const float rad = g_frontAngle * (float)M_PI / 180.0f;
                // right leaf extends -x from its hinge (rotated by +ang),
                // left leaf extends +x from its hinge (rotated by -ang)
                const float dx = right ? -cosf(rad) * w : cosf(rad) * w;
                const float dz = sinf(rad) * w;   // both leaves swing outward
                const float px = hx + dx, pz = FRONT_Z - dz;
                const float minX = (px < hx ? px : hx) - 0.10f;
                const float maxX = (px > hx ? px : hx) + 0.10f;
                const float minZ = (pz < FRONT_Z ? pz : FRONT_Z) - 0.10f;
                const float maxZ = (pz > FRONT_Z ? pz : FRONT_Z) + 0.10f;
                Collision::SetDynamicBox(slot, true, minX, minZ, maxX, maxZ);
            }
        };
        Track(g_frontSlotL, FRONT_X + FRONT_HALF, true);
        Track(g_frontSlotR, FRONT_X - FRONT_HALF, false);
    }

    void UpdateLights(float timeSec, bool farAway)
    {
        // LIGHT 0: chandelier — main warm light in the room center
        Lighting_SetPointLight(GL_LIGHT0, 0.0f, 3.0f, 0.0f,
                               1.0f, 0.88f, 0.62f, 0.85f);

        // LIGHT 1: fireplace — flickering warm glow
        const float fl = Objects::FireFlicker(timeSec);
        Lighting_SetPointLight(GL_LIGHT1, -6.95f, 0.9f, 0.0f,
                               1.0f, 0.55f, 0.22f, 0.9f * fl);

        // LIGHT 2: table lamp — soft corner fill (local only: the slot is
        // claimed by whichever room the player is actually in)
        if (!farAway)
            Lighting_SetPointLight(GL_LIGHT2, LAMP_X, 1.05f, LAMP_Z,
                                   1.0f, 0.85f, 0.60f, 0.55f);

        // LIGHT 3: cool moonlight wash — only while the player is upstairs;
        // it otherwise floods the below-ground rooms blue (no distance
        // attenuation: a directional light reaches everywhere)
        if (!farAway)
        Lighting_SetDirectionalLight(GL_LIGHT3, 0.25f, -0.7f, 0.65f,
                                     0.45f, 0.55f, 0.80f, 0.35f);
    }

    void Draw(float timeSec)
    {
        DrawFloor();
        DrawCeiling();
        DrawSideWalls();
        DrawWindowWall();
        DrawDoorWall();
        DrawFurniture(timeSec);
        DrawPaintings();
        DrawFrontSymbols();   // brass medallions + plaque (must draw after the wall)
        DrawFixtures(timeSec);

        // fireplace on the west wall, opening facing +X (into the room)
        glPushMatrix();
        glTranslatef(FIRE_X, 0, 0);
        glRotatef(90, 0, 1, 0);
        Objects::Fireplace(true, timeSec);
        glPopMatrix();
    }
}
