// ============================================================
//  THE LAST NIGHT - GrandHall.cpp
//  A large dramatic hall north of the lobby: walnut floor, big
//  patterned carpet, cream walls with moldings, six columns,
//  a grand rotating chandelier, portrait paintings, side tables
//  with decorative objects, potted plants, wall sconces and two
//  windows with the blue evening exterior.
//
//  Visual storytelling: a crooked portrait, a stopped clock,
//  an old letter on the writing desk (the first clue).
// ============================================================
#include "GrandHall.h"
#include "Objects.h"
#include "Texture.h"
#include "Lighting.h"
#include "Collision.h"
#include "Interaction.h"
#include <GL/freeglut.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace
{
    const float HX = 8.0f;        // hall spans x [-8, +8]
    const float Z0 = 7.25f;       // shared wall (north face)
    const float Z1 = 21.25f;      // far wall
    const float HALL_L = Z1 - Z0; // 14 units deep
    const float WALL_H = 5.2f;    // taller than the lobby (4.2)
    const float WALL_T = 0.25f;

    const float DOOR1_X = -4.6f;  // doorway centers (match the lobby doors)
    const float DOOR2_X =  4.6f;
    const float DOOR_W  = 1.2f;

    const float FIRE_X  = -7.55f; // lobby fireplace (for light spill)

    // --- door animation state (west doorway) ---
    float g_doorAngle = 0.0f;     // 0 = closed, ~100 = open
    bool  g_doorOpening = false;
    const float DOOR_OPEN_ANGLE = 100.0f;
    int   g_doorSlot  = -1;       // collision slot for the swinging panel

    void Box(const Material &m, const char *tex,
             float cx, float cy, float cz, float sx, float sy, float sz)
    {
        glPushMatrix();
        glTranslatef(cx, cy, cz);
        Objects::TexturedBox(m, tex, sx, sy, sz);
        glPopMatrix();
    }

    // ---- floor, ceiling, walls -------------------------------------------
    void DrawFloor()
    {
        glPushMatrix();
        glTranslatef(0, -0.05f, (Z0 + Z1) / 2);
        glScalef(2*HX + 1, 0.1f, HALL_L + 1);
        Lighting_ApplyMaterial(Materials::WOOD);
        Textures::Bind("hallwood");
        glutSolidCube(1);
        glPopMatrix();
    }

    void DrawCeiling()
    {
        // flat border ceiling; the chandelier well is a raised center dome
        glPushMatrix();
        glTranslatef(0, WALL_H + 0.05f, (Z0 + Z1) / 2);
        glScalef(2*HX + 1, 0.1f, HALL_L + 1);
        Lighting_ApplyMaterial(Materials::FABRIC_C);
        Textures::Bind("plaster");
        glutSolidCube(1);
        glPopMatrix();

        // raised center coffer above the chandelier
        glPushMatrix();
        glTranslatef(0, WALL_H + 0.42f, (Z0 + Z1) / 2);
        glScalef(5.0f, 0.75f, 5.0f);
        Lighting_ApplyMaterial(Materials::FABRIC_C);
        Textures::Bind("plaster");
        glutSolidCube(1);
        glPopMatrix();

        // beams across the hall
        for (int i = 0; i < 5; ++i)
        {
            float z = Z0 + 1.7f + i * (HALL_L - 3.4f) / 4.0f;
            glPushMatrix();
            glTranslatef(0, WALL_H - 0.12f, z);
            glScalef(2*HX, 0.24f, 0.34f);
            Lighting_ApplyMaterial(Materials::DARKWOOD);
            Textures::Bind("darkwood");
            glutSolidCube(1);
            glPopMatrix();
        }
    }

    // Doorway with upper lintel; used on the shared wall.
    void DrawWallWithDoorway(float doorX)
    {
        // left/right segments + lintel above
        float segL = (doorX - DOOR_W/2) - (-HX - WALL_T/2);
        float segR = (HX + WALL_T/2) - (doorX + DOOR_W/2);
        Box(Materials::FABRIC_C, "wall",
            (-HX - WALL_T/2 + segL/2), WALL_H/2, Z0,
            segL, WALL_H, WALL_T);
        Box(Materials::FABRIC_C, "wall",
            (HX + WALL_T/2 - segR/2), WALL_H/2, Z0,
            segR, WALL_H, WALL_T);
        Box(Materials::FABRIC_C, "wall",
            doorX, (2.9f + WALL_H)/2, Z0,
            DOOR_W + 0.2f, WALL_H - 2.9f, WALL_T);

        // frame
        Box(Materials::DARKWOOD, "darkwood", doorX - DOOR_W/2 - 0.05f, 1.45f, Z0, 0.1f, 2.9f, 0.18f);
        Box(Materials::DARKWOOD, "darkwood", doorX + DOOR_W/2 + 0.05f, 1.45f, Z0, 0.1f, 2.9f, 0.18f);
        Box(Materials::DARKWOOD, "darkwood", doorX, 2.92f, Z0, DOOR_W + 0.3f, 0.1f, 0.18f);
    }

    void DrawSideWall(bool west)   // west: x=-HX, east: x=+HX
    {
        float wx = west ? -HX : HX;
        float sw = 6.8f;          // window opening width
        float wy0 = 1.0f, wy1 = 3.9f;

        // solid segments: above, below, and the two piers beside the window
        float cs = (Z0 + Z1) / 2;
        Box(Materials::FABRIC_C, "wall", wx, (wy1 + WALL_H)/2, cs, WALL_T, WALL_H - wy1, HALL_L);
        Box(Materials::FABRIC_C, "wall", wx, wy0/2, cs, WALL_T, wy0, HALL_L);
        float segS = (cs - sw/2) - Z0;
        Box(Materials::FABRIC_C, "wall", wx, WALL_H/2, Z0 + segS/2, WALL_T, WALL_H, segS);

        // north pier: on the EAST wall it carries the library entrance
        if (!west)
        {
            const float dZ0 = 18.35f, dZ1 = 20.75f; // matches the library entry
            float s1 = dZ0 - (cs + sw/2);
            float s2 = Z1 - dZ1;
            if (s1 > 0.01f)
                Box(Materials::FABRIC_C, "wall", wx, WALL_H/2, cs + sw/2 + s1/2, WALL_T, WALL_H, s1);
            if (s2 > 0.01f)
                Box(Materials::FABRIC_C, "wall", wx, WALL_H/2, Z1 - s2/2, WALL_T, WALL_H, s2);
            Box(Materials::FABRIC_C, "wall", wx, (2.9f + WALL_H)/2, (dZ0+dZ1)/2,
                WALL_T, WALL_H - 2.9f, dZ1 - dZ0);
            // door frame trim (open passage to the library)
            Box(Materials::DARKWOOD, "darkwood", wx, 1.45f, dZ0 - 0.05f, 0.18f, 2.9f, 0.1f);
            Box(Materials::DARKWOOD, "darkwood", wx, 1.45f, dZ1 + 0.05f, 0.18f, 2.9f, 0.1f);
            Box(Materials::DARKWOOD, "darkwood", wx, 2.92f, (dZ0+dZ1)/2, 0.18f, 0.1f, dZ1 - dZ0 + 0.3f);
        }
        else
        {
            float segN = Z1 - (cs + sw/2);
            Box(Materials::FABRIC_C, "wall", wx, WALL_H/2, Z1 - segN/2, WALL_T, WALL_H, segN);
        }

        // window frame + mullions (in the wall plane)
        const float fw = 0.09f;
        Box(Materials::WOOD, "wood", wx, wy0 - fw/2, (Z0+Z1)/2, 0.14f, fw, sw + 2*fw);
        Box(Materials::WOOD, "wood", wx, wy1 + fw/2, (Z0+Z1)/2, 0.14f, fw, sw + 2*fw);
        Box(Materials::WOOD, "wood", wx, (wy0+wy1)/2, (Z0+Z1)/2 - (sw/2 + fw), 0.14f, wy1-wy0+2*fw, fw);
        Box(Materials::WOOD, "wood", wx, (wy0+wy1)/2, (Z0+Z1)/2 + (sw/2 + fw), 0.14f, wy1-wy0+2*fw, fw);
        Box(Materials::WOOD, "wood", wx, (wy0+wy1)/2, (Z0+Z1)/2, 0.07f, wy1-wy0, 0.05f);
        Box(Materials::WOOD, "wood", wx, (wy0+wy1)/2, (Z0+Z1)/2, 0.07f, 0.05f, sw);
        // sill
        Box(Materials::MARBLE, "marble", wx + (west ? 0.15f : -0.15f), wy0 - 0.06f, (Z0+Z1)/2, 0.4f, 0.07f, sw + 0.3f);

        // exterior backdrop: a simple blue-evening garden silhouette, self-lit
        glDisable(GL_LIGHTING);
        glDepthMask(GL_FALSE);
        Textures::Bind("nightsky");
        glBegin(GL_QUADS);
            glColor3f(1, 1, 1);
            glTexCoord2f(0, 0); glVertex3f(wx + (west ? -6.0f : 6.0f), wy0, (Z0+Z1)/2 - sw/2 - 1);
            glTexCoord2f(1, 0); glVertex3f(wx + (west ? -6.0f : 6.0f), wy0, (Z0+Z1)/2 + sw/2 + 1);
            glTexCoord2f(1, 1); glVertex3f(wx + (west ? -6.0f : 6.0f), wy1 + 2.0f, (Z0+Z1)/2 + sw/2 + 1);
            glTexCoord2f(0, 1); glVertex3f(wx + (west ? -6.0f : 6.0f), wy1 + 2.0f, (Z0+Z1)/2 - sw/2 - 1);
        glEnd();
        // distant lawn strip
        Textures::Bind("grass");
        glBegin(GL_QUADS);
            glColor3f(0.7f, 0.8f, 0.7f);
            glVertex3f(wx + (west ? -6.0f : 6.0f), 0, (Z0+Z1)/2 - sw/2 - 1);
            glVertex3f(wx + (west ? -6.0f : 6.0f), 0, (Z0+Z1)/2 + sw/2 + 1);
            glVertex3f(wx + (west ? -6.0f : 6.0f), 0.9f, (Z0+Z1)/2 + sw/2 + 1);
            glVertex3f(wx + (west ? -6.0f : 6.0f), 0.9f, (Z0+Z1)/2 - sw/2 - 1);
        glEnd();
        // tree silhouettes
        glColor3f(0.05f, 0.10f, 0.07f);
        for (int i = 0; i < 3; ++i)
        {
            float z = (Z0+Z1)/2 - 4.0f + i * 4.0f;
            float tx = wx + (west ? -3.2f : 3.2f);
            glBegin(GL_TRIANGLES);
                glVertex3f(tx, 0.8f, z - 0.9f);
                glVertex3f(tx, 0.8f, z + 0.9f);
                glVertex3f(tx, 3.0f + i * 0.5f, z);
            glEnd();
            glBegin(GL_QUADS);
                glVertex3f(tx + (west ? 0.1f : -0.1f), 0, z - 0.08f);
                glVertex3f(tx + (west ? 0.1f : -0.1f), 0, z + 0.08f);
                glVertex3f(tx + (west ? 0.1f : -0.1f), 0.9f, z + 0.08f);
                glVertex3f(tx + (west ? 0.1f : -0.1f), 0.9f, z - 0.08f);
            glEnd();
        }
        glDepthMask(GL_TRUE);
        glEnable(GL_LIGHTING);

        // faint tinted glass over the opening
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glColor4f(0.72f, 0.84f, 0.96f, 0.10f);
        Lighting_ApplyMaterial(Materials::GLASS);
        glBegin(GL_QUADS);
            glNormal3f(west ? 1.0f : -1.0f, 0, 0);
            glVertex3f(wx, wy0, (Z0+Z1)/2 - sw/2);
            glVertex3f(wx, wy0, (Z0+Z1)/2 + sw/2);
            glVertex3f(wx, wy1, (Z0+Z1)/2 + sw/2);
            glVertex3f(wx, wy1, (Z0+Z1)/2 - sw/2);
        glEnd();
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        // curtains at both sides of the window
        for (int s = -1; s <= 1; s += 2)
        {
            glPushMatrix();
            glTranslatef(wx, (wy0 + wy1)/2 + 0.2f, (Z0+Z1)/2 + s * (sw/2 + 0.35f));
            glRotatef(90, 0, 1, 0);
            Objects::Curtains(1.4f, wy1 - wy0 + 0.4f);
            glPopMatrix();
        }
    }

    void DrawFarWall()
    {
        Box(Materials::FABRIC_C, "wall", 0, WALL_H/2, Z1 + WALL_T/2, 2*HX + 2*WALL_T, WALL_H, WALL_T);

        // the big crooked portrait of the mansion owner (mystery object)
        glPushMatrix();
        glTranslatef(2.8f, 2.6f, Z1 - WALL_T/2 - 0.06f);
        glRotatef(180, 0, 1, 0);
        glRotatef(-3.5f, 0, 0, 1);              // hangs slightly crooked
        Objects::Painting(1.6f, 2.1f, "paint_owner");
        glPopMatrix();

        // forest painting on the other side
        glPushMatrix();
        glTranslatef(-3.2f, 2.7f, Z1 - WALL_T/2 - 0.06f);
        glRotatef(180, 0, 1, 0);
        Objects::Painting(1.5f, 1.05f, "paint_forest");
        glPopMatrix();

        // moldings
        Box(Materials::DARKWOOD, "darkwood", 0, 0.14f, Z1 + WALL_T/2 - 0.02f, 2*HX, 0.28f, 0.06f);
        Box(Materials::DARKWOOD, "darkwood", 0, 3.1f, Z1 + WALL_T/2 - 0.02f, 2*HX, 0.12f, 0.05f);
    }

    void DrawColumns()
    {
        // four marble columns supporting the coffer, two per row
        for (int i = 0; i < 2; ++i)
        {
            float z = Z0 + 3.2f + i * 5.6f;
            for (int s = -1; s <= 1; s += 2)
            {
                glPushMatrix();
                glTranslatef(s * 5.6f, 0, z);
                Objects::Column(WALL_H - 0.4f, 0.30f);
                glPopMatrix();
            }
        }
    }

    void DrawFurniture(float timeSec)
    {
        // large formal carpet
        glPushMatrix();
        glTranslatef(0, 0, (Z0 + Z1)/2 + 0.5f);
        Objects::Rug(5.6f, 8.2f);
        glPopMatrix();

        // writing desk with the old letter (the first clue)
        glPushMatrix();
        glTranslatef(-6.4f, 0, 13.0f);
        glRotatef(90, 0, 1, 0);
        Objects::CoffeeTable();     // reused as a writing table
        // the letter on top
        glPushMatrix();
        glTranslatef(0.05f, 0.505f, 0.05f);
        glRotatef(18, 0, 1, 0);
        Objects::TexturedBox(Materials::FABRIC_C, 0, 0.02f, 0.3f, 0.22f);
        glPopMatrix();
        glPopMatrix();

        // candlestick on the desk (small light source visual)
        glPushMatrix();
        glTranslatef(-6.3f, 0.5f, 13.35f);
        Objects::Candle(0.2f, true, timeSec);
        glPopMatrix();

        // stopped grandfather clock (storytelling: hands frozen at 12)
        glPushMatrix();
         glTranslatef(5.8f, 0, 18.6f);
        glRotatef(-90, 0, 1, 0);
        Objects::GrandfatherClock(0.0f);   // frozen at midnight
        glPopMatrix();

        // side tables with vases near the far wall
        glPushMatrix();
        glTranslatef(-6.9f, 0, 20.0f);
        Objects::SideTable();
        glTranslatef(0, 0.62f, 0);
        Objects::VaseWithFlowers(1.0f);
        glPopMatrix();
        glPushMatrix();
         glTranslatef(4.8f, 0, 20.0f);
        Objects::SideTable();
        glTranslatef(0, 0.62f, 0);
        Objects::VaseWithFlowers(0.85f);
        glPopMatrix();

        // bookshelf on the east wall (extends the mansion's library hint)
        glPushMatrix();
        glTranslatef(7.7f, 0, 9.3f);
        glRotatef(-90, 0, 1, 0);
        Objects::Bookshelf(2.9f, 2.6f);
        glPopMatrix();

        // potted plants flanking the doorways
        glPushMatrix(); glTranslatef(-3.3f, 0, 7.9f); Objects::PottedPlant(1.1f); glPopMatrix();
        glPushMatrix(); glTranslatef( 3.3f, 0, 7.9f); Objects::PottedPlant(1.0f); glPopMatrix();
        glPushMatrix(); glTranslatef(-6.9f, 0, 8.6f);  Objects::PottedPlant(1.2f); glPopMatrix();

        // candlesticks on the side tables
        glPushMatrix();
        glTranslatef(-6.9f, 0.65f, 20.35f);
        Objects::Candle(0.18f, true, timeSec + 0.4f);
        glPopMatrix();
    }

    void DrawPaintingsSide()
    {
        // landscape painting on the shared wall pier (between doorways)
        glPushMatrix();
        glTranslatef(0, 2.6f, Z0 + WALL_T/2 + 0.05f);
        Objects::Painting(1.5f, 1.0f, "paint_land");
        glPopMatrix();

        // abstract on the east wall pier (north of the window)
        glPushMatrix();
        glTranslatef(HX - WALL_T/2 - 0.04f, 2.6f, 19.45f);
        glRotatef(-90, 0, 1, 0);
        Objects::Painting(1.1f, 1.4f, "paint_abs");
        glPopMatrix();
    }

    void DrawFixtures(float timeSec)
    {
        // grand chandelier from the raised coffer, slow continuous rotation
        glPushMatrix();
        glTranslatef(0, WALL_H + 0.1f, (Z0 + Z1)/2);
        glScalef(1.35f, 1.25f, 1.35f);
        Objects::Chandelier(timeSec);
        glPopMatrix();

        // wall sconces near the paintings
        glPushMatrix(); glTranslatef(0, 3.6f, Z0 + WALL_T/2 + 0.06f); Objects::WallSconce(); glPopMatrix();
        glPushMatrix(); glTranslatef(-3.2f, 3.8f, Z1 - WALL_T/2 - 0.06f); glRotatef(180, 0, 1, 0); Objects::WallSconce(); glPopMatrix();
        glPushMatrix(); glTranslatef( 2.8f, 4.3f, Z1 - WALL_T/2 - 0.06f); glRotatef(180, 0, 1, 0); Objects::WallSconce(); glPopMatrix();
        glPushMatrix(); glTranslatef(HX - WALL_T/2 - 0.06f, 3.6f, 19.45f); glRotatef(-90, 0, 1, 0); Objects::WallSconce(); glPopMatrix();
    }

    void DrawDoors(float dt)
    {
        // WEST doorway: interactive door (hinge at its left jamb).
        if (g_doorOpening && g_doorAngle < DOOR_OPEN_ANGLE)
            g_doorAngle += 140.0f * dt;
        else if (!g_doorOpening && g_doorAngle > 0.0f)
            g_doorAngle -= 140.0f * dt;
        if (g_doorAngle < 0) g_doorAngle = 0;
        if (g_doorAngle > DOOR_OPEN_ANGLE) g_doorAngle = DOOR_OPEN_ANGLE;

        glPushMatrix();
        glTranslatef(DOOR1_X - DOOR_W/2, 0, Z0);
        glRotatef(g_doorAngle, 0, 1, 0);
        Objects::Door(DOOR_W, 2.9f);
        glPopMatrix();

        // EAST doorway: stays open (empty frame = free passage)
    }

    // Collision for the west door panel; recomputed while it swings.
    // The box was claimed as a dynamic slot in Load(). Fully-open doors
    // block nothing (the panel rests flat against the wall).
    void UpdateDoorCollision()
    {
        if (g_doorSlot < 0) return;
        if (g_doorAngle >= DOOR_OPEN_ANGLE - 2.0f)
        {
            Collision::SetDynamicBox(g_doorSlot, false, 0, 0, 0, 0);
            return;
        }
        float rad = g_doorAngle * (float)M_PI / 180.0f;

        // door panel: local +X along the panel, hinged at (hx, Z0)
        float hx = DOOR1_X - DOOR_W/2;
        float px = hx + cosf(rad) * DOOR_W;      // far edge of the panel
        float pz = Z0 + sinf(rad) * DOOR_W;
        // AABB around the swung panel
        float minX = (px < hx ? px : hx) - 0.08f;
        float maxX = (px > hx ? px : hx) + 0.08f;
        float minZ = (pz < Z0 ? pz : Z0) - 0.08f;
        float maxZ = (pz > Z0 ? pz : Z0) + 0.08f;

        Collision::SetDynamicBox(g_doorSlot, true, minX, minZ, maxX, maxZ);
    }
}

namespace GrandHall
{
    void Reset()
    {
        g_doorAngle = 0.0f;
        g_doorOpening = false;
        // Load() has already claimed this slot before a new game can begin.
        // Reinstall the closed panel immediately instead of waiting for a
        // render/update frame, so the player cannot pass through it on reset.
        UpdateDoorCollision();
    }

    void SetWestDoorOpen(bool open)
    {
        g_doorOpening = open;
    }

    void Load()
    {
        const float t = 0.25f;

        // shared wall (south face of the hall): two doorways
        Collision::AddBox(-HX - t, Z0 - t, DOOR1_X - DOOR_W/2, Z0 + t);
        Collision::AddBox(DOOR1_X + DOOR_W/2, Z0 - t, DOOR2_X - DOOR_W/2, Z0 + t);
        Collision::AddBox(DOOR2_X + DOOR_W/2, Z0 - t, HX + t, Z0 + t);
        // far wall
        Collision::AddBox(-HX - t, Z1, HX + t, Z1 + t);
        // side walls (east wall has the library doorway at z 18.9..20.1)
        Collision::AddBox(-HX - t, Z0, -HX + t, Z1);
        Collision::AddBox( HX - t, Z0,  HX + t, 18.35f);
        Collision::AddBox( HX - t, 20.75f, HX + t, Z1);

        // furniture
        Collision::AddBox(-7.2f, 12.4f, -5.6f, 13.6f);   // writing desk
        Collision::AddBox( 5.3f, 18.0f,  6.3f, 19.2f);   // stopped clock
        Collision::AddBox(-7.3f, 19.5f, -6.5f, 20.5f);   // side table west
        Collision::AddBox( 4.4f, 19.5f,  5.2f, 20.5f);   // side table east, clear of library entry
        Collision::AddBox( 7.2f,  9.6f,  8.0f, 11.2f);   // bookshelf
        Collision::AddBox(-3.55f, 7.65f, -3.05f, 8.15f); // plants
        Collision::AddBox( 3.05f, 7.65f,  3.55f, 8.15f);
        Collision::AddBox(-7.15f, 8.35f, -6.65f, 8.85f);
        // columns
        for (int i = 0; i < 2; ++i)
        {
            float z = Z0 + 3.2f + i * 5.6f;
            Collision::AddBox(5.6f - 0.45f, z - 0.45f, 5.6f + 0.45f, z + 0.45f);
            Collision::AddBox(-5.6f - 0.45f, z - 0.45f, -5.6f + 0.45f, z + 0.45f);
        }

        // closed west door panel: claimed as the DYNAMIC box so
        // SetDynamicBox can re-implace it every frame as it swings
        Collision::AddBox(DOOR1_X - DOOR_W/2 - 0.08f, Z0 - 0.08f,
                          DOOR1_X + DOOR_W/2 + 0.08f, Z0 + 0.08f);
        g_doorSlot = Collision::ClaimDynamic();

        // ---- interactables ----
        // the old letter on the writing desk
        Interaction::Add(Interaction::ID_LETTER, -6.4f, 0.6f, 13.0f,
                         1.8f, "Examine Letter");
        // the crooked portrait (mystery object)
        Interaction::Add(Interaction::ID_PORTRAIT, 2.8f, 2.6f, Z1,
                         2.2f, "Examine Portrait");
        // the stopped clock (storytelling)
        Interaction::Add(Interaction::ID_CLOCK, 5.8f, 1.4f, 18.6f,
                         1.8f, "Examine Clock");
        // the west hall door: two prompts, toggled by state
        Interaction::Add(Interaction::ID_HALLDOOR, DOOR1_X, 1.4f, Z0,
                         1.7f, "Open West Hall Door");
        Interaction::Add(Interaction::ID_CLOSEDOOR, DOOR1_X, 1.4f, Z0,
                         1.7f, "Close West Hall Door");
        Interaction::SetEnabled(Interaction::ID_CLOSEDOOR, false);
    }

    void UpdateLights(float timeSec, bool farAway)
    {
        // LIGHT 4: grand chandelier — big warm light high in the hall
        Lighting_SetPointLight(GL_LIGHT4, 0.0f, WALL_H - 0.6f, (Z0 + Z1)/2,
                               1.0f, 0.87f, 0.60f, 1.0f);

        // LIGHT 5: second warm fill near the far wall (keeps the hall's
        // far end comfortable and shows the portrait)
        Lighting_SetPointLight(GL_LIGHT5, 0.0f, 3.4f, Z1 - 1.2f,
                               1.0f, 0.85f, 0.62f, 0.55f);

        // LIGHT 6: candle glow on the writing desk (the clue's spotlight)
        const float fl = 0.9f + 0.1f * sinf(timeSec * 9.0f) * sinf(timeSec * 3.7f);
        if (!farAway)
        Lighting_SetPointLight(GL_LIGHT6, -6.3f, 1.0f, 13.3f,
                               1.0f, 0.75f, 0.4f, 0.6f * fl);

        // (GL_LIGHT3 stays owned by the lobby: cool moonlight wash, which
        //  also reaches the hall through the two windows.)
    }

    void Draw(float timeSec)
    {
        DrawFloor();
        DrawCeiling();
        DrawWallWithDoorway(DOOR1_X);
        DrawWallWithDoorway(DOOR2_X);
        DrawSideWall(true);
        DrawSideWall(false);
        DrawFarWall();
        DrawColumns();
        DrawFurniture(timeSec);
        DrawPaintingsSide();
        DrawFixtures(timeSec);

        // doors animate with delta time derived from the game elapsed
        static float s_last = timeSec;
        float dt = timeSec - s_last;
        if (dt < 0) dt = 0;
        if (dt > 0.1f) dt = 0.1f;
        s_last = timeSec;
        DrawDoors(dt);
        UpdateDoorCollision();
    }

    bool IsWestDoorOpen()
    {
        return g_doorOpening;
    }
}
