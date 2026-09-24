// ============================================================
//  THE LAST NIGHT - Objects.cpp
//  Reusable object builders constructed from multiple OpenGL
//  primitives with local-coordinate transformations. Textured
//  faces use a unit quad helper so surfaces get visible detail.
//  A shared GLU quadric provides smooth cylinders/disks/spheres.
// ============================================================
#include "Objects.h"
#include "Texture.h"
#include <GL/freeglut.h>
#include <cmath>
#include <cstddef>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace
{
    GLUquadric *g_quad = NULL;

    void EnsureQuad()
    {
        if (!g_quad) g_quad = gluNewQuadric();
    }

    void SetSurface(const Material &m, const char *tex)
    {
        Lighting_ApplyMaterial(m);
        if (tex) Textures::Bind(tex);
    }

    void QuadXYLocal()
    {
        glBegin(GL_QUADS);
        glNormal3f(0, 0, 1);
        glTexCoord2f(0, 0); glVertex3f(-0.5f, -0.5f, 0);
        glTexCoord2f(1, 0); glVertex3f( 0.5f, -0.5f, 0);
        glTexCoord2f(1, 1); glVertex3f( 0.5f,  0.5f, 0);
        glTexCoord2f(0, 1); glVertex3f(-0.5f,  0.5f, 0);
        glEnd();
    }

    void TexQuad(const Material &m, const char *tex, float w, float h)
    {
        glPushMatrix();
        glScalef(w, h, 1);
        SetSurface(m, tex);
        QuadXYLocal();
        glPopMatrix();
    }

    void PlainQuad(const Material &m, float w, float h)
    {
        glPushMatrix();
        glScalef(w, h, 1);
        SetSurface(m, 0);
        QuadXYLocal();
        glPopMatrix();
    }

    void SolidCylinder(float r, float h, int slices)
    {
        EnsureQuad();
        glPushMatrix();
        glTranslatef(0, 0, -h/2);
        glRotatef(-90, 1, 0, 0);       // +Z axis -> +Y axis
        gluCylinder(g_quad, r, r, h, slices, 2);
        glPopMatrix();
    }

    void SolidDisk(float r, int slices)   // faces +Z
    {
        EnsureQuad();
        gluDisk(g_quad, 0, r, slices, 1);
    }
}

namespace Objects
{
    void QuadXY() { QuadXYLocal(); }
    // ------------------------------------------------------------------
    //  Low-level helpers
    // ------------------------------------------------------------------
    void Box(float sx, float sy, float sz)
    {
        glPushMatrix();
        glScalef(sx, sy, sz);
        glutSolidCube(1.0f);
        glPopMatrix();
    }

    void TexturedBox(const Material &m, const char *tex,
                     float sx, float sy, float sz)
    {
        SetSurface(m, tex);
        Box(sx, sy, sz);
    }

    // ------------------------------------------------------------------
    //  Furniture
    // ------------------------------------------------------------------
    void Sofa(float w, const Material &fabric)
    {
        const float D  = 0.95f;
        const float SH = 0.42f;
        const float BH = 1.05f;
        const float AH = 0.62f;
        const float AW = 0.22f;

        SetSurface(fabric, 0);

        // seat base
        glPushMatrix();
        glTranslatef(0, SH/2, 0);
        Box(w, SH, D);
        glPopMatrix();

        // backrest
        glPushMatrix();
        glTranslatef(0, (BH+SH)/2 - 0.05f, -D/2 + 0.12f);
        Box(w, BH - 0.1f, 0.24f);
        glPopMatrix();

        // arms
        glPushMatrix(); glTranslatef(-(w/2 - AW/2), AH/2, 0); Box(AW, AH, D); glPopMatrix();
        glPushMatrix(); glTranslatef( (w/2 - AW/2), AH/2, 0); Box(AW, AH, D); glPopMatrix();

        // seat cushions
        SetSurface(Materials::FABRIC_C, 0);
        for (int i = 0; i < 2; ++i)
        {
            glPushMatrix();
            glTranslatef((i ? 1 : -1) * (w/4 - 0.02f), SH + 0.07f, 0.03f);
            Box(w/2 - 0.08f, 0.16f, D - 0.12f);
            glPopMatrix();
        }

        // wooden legs
        SetSurface(Materials::DARKWOOD, "darkwood");
        const float legIn = 0.14f, legH = 0.10f;
        for (int sx = -1; sx <= 1; sx += 2)
            for (int sz = -1; sz <= 1; sz += 2)
            {
                glPushMatrix();
                glTranslatef(sx*(w/2 - legIn), legH/2 - 0.02f, sz*(D/2 - legIn));
                Box(0.08f, legH, 0.08f);
                glPopMatrix();
            }
    }

    void Armchair(const Material &fabric)
    {
        Sofa(1.05f, fabric);
    }

    void CoffeeTable()
    {
        const float W = 1.5f, D = 0.85f, TH = 0.06f, LEG = 0.42f;
        glPushMatrix();
        glTranslatef(0, LEG + TH/2, 0);
        TexturedBox(Materials::WOOD, "wood", W, TH, D);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(0, 0.14f, 0);
        TexturedBox(Materials::DARKWOOD, "darkwood", W - 0.2f, 0.04f, D - 0.15f);
        glPopMatrix();
        SetSurface(Materials::DARKWOOD, "darkwood");
        for (int sx = -1; sx <= 1; sx += 2)
            for (int sz = -1; sz <= 1; sz += 2)
            {
                glPushMatrix();
                glTranslatef(sx*(W/2 - 0.09f), LEG/2, sz*(D/2 - 0.09f));
                Box(0.07f, LEG, 0.07f);
                glPopMatrix();
            }
        // decorations: a book and a bowl
        glPushMatrix();
        glTranslatef(-0.35f, LEG + TH + 0.025f, 0.05f);
        glRotatef(12, 0, 1, 0);
        TexturedBox(Materials::BOOK, "books", 0.30f, 0.05f, 0.22f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(0.42f, LEG + TH + 0.05f, -0.05f);
        SetSurface(Materials::MARBLE, "marble");
        glScalef(0.22f, 0.09f, 0.22f);
        glutSolidSphere(0.5f, 18, 10);
        glPopMatrix();
    }

    void SideTable()
    {
        const float W = 0.55f, H = 0.6f;
        glPushMatrix();
        glTranslatef(0, H, 0);
        TexturedBox(Materials::WOOD, "wood", W, 0.05f, W);
        glPopMatrix();
        SetSurface(Materials::DARKWOOD, "darkwood");
        for (int sx = -1; sx <= 1; sx += 2)
            for (int sz = -1; sz <= 1; sz += 2)
            {
                glPushMatrix();
                glTranslatef(sx*(W/2 - 0.06f), H/2, sz*(W/2 - 0.06f));
                Box(0.06f, H, 0.06f);
                glPopMatrix();
            }
    }

    void GrandfatherClock(float timeSec)
    {
        const float W = 0.62f, D = 0.38f, H = 2.15f;
        // body, crown, plinth
        glPushMatrix();
        glTranslatef(0, H/2, 0);
        TexturedBox(Materials::DARKWOOD, "darkwood", W, H, D);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(0, H + 0.07f, 0);
        TexturedBox(Materials::DARKWOOD, "darkwood", W + 0.1f, 0.14f, D + 0.05f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(0, 0.06f, 0.03f);
        TexturedBox(Materials::DARKWOOD, "darkwood", W + 0.06f, 0.12f, D + 0.08f);
        glPopMatrix();

        // clock face (round, slightly proud of the body)
        glPushMatrix();
        glTranslatef(0, H - 0.38f, D/2 + 0.005f);
        SetSurface(Materials::MARBLE, "marble");
        SolidDisk(0.24f, 36);
        // hour markers
        SetSurface(Materials::DARKWOOD, 0);
        for (int i = 0; i < 12; ++i)
        {
            float a = i * 2*M_PI/12;
            glPushMatrix();
            glTranslatef(cosf(a)*0.20f, sinf(a)*0.20f, 0.004f);
            Box(0.022f, 0.05f, 0.002f);
            glPopMatrix();
        }
        // hands
        float minA  = (fmodf(timeSec, 3600.0f) / 3600.0f) * 2*M_PI;
        float hourA = fmodf(timeSec / 43200.0f, 1.0f) * 2*M_PI;
        glPushMatrix();
        glRotatef(-minA * 180/M_PI, 0, 0, 1);
        glTranslatef(0, 0.10f, 0.006f);
        Box(0.016f, 0.20f, 0.003f);
        glPopMatrix();
        glPushMatrix();
        glRotatef(-hourA * 180/M_PI, 0, 0, 1);
        glTranslatef(0, 0.065f, 0.008f);
        Box(0.02f, 0.13f, 0.003f);
        glPopMatrix();
        glPopMatrix();

        // pendulum window (glass) + swinging pendulum
        glPushMatrix();
        glTranslatef(0, 0.95f, D/2 + 0.004f);
        PlainQuad(Materials::GLASS, 0.34f, 0.9f);
        glPopMatrix();
        float pendA = sinf(timeSec * M_PI) * 22.0f;
        glPushMatrix();
        glTranslatef(0, 1.42f, D/2 - 0.02f);
        glRotatef(pendA, 0, 0, 1);
        SetSurface(Materials::GOLD, 0);
        glPushMatrix();
        glTranslatef(0, -0.28f, 0);
        Box(0.014f, 0.56f, 0.012f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(0, -0.60f, 0);
        glScalef(0.09f, 0.09f, 0.02f);
        glutSolidSphere(0.5f, 20, 12);
        glPopMatrix();
        glPopMatrix();
    }

    void Bookshelf(float w, float h)
    {
        const float D = 0.32f;
        const float T = 0.05f;
        const int   SHELVES = 5;

        SetSurface(Materials::DARKWOOD, "darkwood");
        glPushMatrix(); glTranslatef(-(w/2 - T/2), h/2, 0); Box(T, h, D); glPopMatrix();
        glPushMatrix(); glTranslatef( (w/2 - T/2), h/2, 0); Box(T, h, D); glPopMatrix();
        glPushMatrix(); glTranslatef(0, h/2, -D/2 + T/2); Box(w, h, T); glPopMatrix();
        glPushMatrix(); glTranslatef(0, h - T/2, 0); Box(w, T, D); glPopMatrix();
        glPushMatrix(); glTranslatef(0, T/2, 0);     Box(w, T, D); glPopMatrix();

        // shelves + colorful books
        const float innerH = (h - 2*T) / SHELVES;
        static const float bookHs[8] = {0.24f,0.28f,0.22f,0.26f,0.30f,0.24f,0.27f,0.23f};
        static const float bookTints[8][3] = {
            {1.0f,0.45f,0.45f},{0.5f,0.7f,1.05f},{0.55f,1.0f,0.6f},{1.05f,0.85f,0.45f},
            {0.8f,0.55f,1.05f},{1.0f,0.75f,0.5f},{0.55f,0.95f,1.0f},{1.0f,0.55f,0.8f}};

        for (int s = 0; s < SHELVES; ++s)
        {
            float sy = T + innerH*(s+1) - T/2;
            glPushMatrix();
            glTranslatef(0, sy, 0);
            Box(w - 2*T, T, D);
            glPopMatrix();

            float bx = -(w/2 - T) + 0.03f;
            float limit = (w/2 - T) - 0.06f;
            int i = 0;
            while (bx < limit && i < 12)
            {
                float bw = 0.045f + 0.02f*((i*7+s*3)%3);
                float bh = bookHs[(i*5+s*2)%8] * (innerH/0.34f);
                const float *t = bookTints[(i*3+s*5)%8];
                Material bm = Materials::Tinted(Materials::BOOK, t[0], t[1], t[2]);
                SetSurface(bm, "books");
                glPushMatrix();
                glTranslatef(bx + bw/2, sy + T/2 + bh/2, 0.02f);
                Box(bw, bh, D - 0.08f);
                glPopMatrix();
                bx += bw + 0.012f;
                ++i;
            }
        }
    }

    void Door(float w, float h)
    {
        // Hinged at the LOCAL x = -w/2 edge: callers translate to the hinge,
        // rotate for open/close, then call this.
        glPushMatrix();
        glTranslatef(w/2, h/2, 0);
        TexturedBox(Materials::WOOD, "wood", w, h, 0.06f);
        SetSurface(Materials::DARKWOOD, 0);
        glPushMatrix(); glTranslatef(0,  h*0.22f, 0.035f); Box(w*0.55f, h*0.28f, 0.012f); glPopMatrix();
        glPushMatrix(); glTranslatef(0, -h*0.16f, 0.035f); Box(w*0.55f, h*0.34f, 0.012f); glPopMatrix();
        glPushMatrix();
        glTranslatef(w*0.38f, 0, 0.05f);
        SetSurface(Materials::GOLD, 0);
        glPushMatrix(); glTranslatef(0.03f, 0, 0); Box(0.09f, 0.03f, 0.03f); glPopMatrix();
        glPushMatrix(); Box(0.03f, 0.09f, 0.03f); glPopMatrix();
        glPopMatrix();
        glPopMatrix();
    }

    // ------------------------------------------------------------------
    //  Fireplace
    // ------------------------------------------------------------------
    float FireFlicker(float timeSec)
    {
        return 0.95f + 0.10f * sinf(timeSec*9.0f) * sinf(timeSec*3.7f+1.3f)
                     + 0.06f * sinf(timeSec*23.0f);
    }

    namespace
    {
        void FlameLayer(float w, float h, float r, float g, float b, float timeSec, float phase)
        {
            float sway = sinf(timeSec*7.0f + phase) * 0.06f;
            float hs   = 1.0f + 0.18f*sinf(timeSec*11.0f + phase*2.0f);
            glBegin(GL_TRIANGLES);
            glColor4f(r, g, b, 0.85f);
            glVertex3f(-w/2, 0, 0);
            glVertex3f( w/2, 0, 0);
            glVertex3f(sway * w, h * hs, 0);
            glEnd();
        }
    }

    void Fireplace(bool lit, float timeSec)
    {
        const float W = 2.2f, H = 1.45f, D = 0.65f;
        const float openW = 1.15f, openH = 0.75f;

        // brick surround: pillars + lintel + mantle + hearth
        SetSurface(Materials::BRICK, "brick");
        float pw = (W - openW)/2 + 0.1f;
        glPushMatrix(); glTranslatef(-(openW/2 + pw/2 - 0.05f), H/2, 0); Box(pw, H, D); glPopMatrix();
        glPushMatrix(); glTranslatef( (openW/2 + pw/2 - 0.05f), H/2, 0); Box(pw, H, D); glPopMatrix();
        glPushMatrix(); glTranslatef(0, openH + (H-openH)/2, 0); Box(W, H-openH, D); glPopMatrix();
        glPushMatrix(); glTranslatef(0, H + 0.05f, 0.05f);
        TexturedBox(Materials::DARKWOOD, "darkwood", W + 0.3f, 0.1f, D + 0.15f);
        glPopMatrix();
        glPushMatrix(); glTranslatef(0, 0.045f, 0.12f);
        TexturedBox(Materials::MARBLE, "marble", W + 0.25f, 0.09f, D + 0.3f);
        glPopMatrix();

        // fire chamber interior
        const float dark[4] = {0.05f, 0.04f, 0.035f, 1};
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, dark);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dark);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, dark);
        glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 2);
        glPushMatrix(); glTranslatef(0, openH/2, -D/2 + 0.06f); Box(openW - 0.06f, openH - 0.04f, 0.02f); glPopMatrix();
        SetSurface(Materials::STONE, 0);
        glPushMatrix(); glTranslatef(-(openW/2 - 0.01f), openH/2, 0); Box(0.04f, openH, D); glPopMatrix();
        glPushMatrix(); glTranslatef( (openW/2 - 0.01f), openH/2, 0); Box(0.04f, openH, D); glPopMatrix();

        // logs
        SetSurface(Materials::DARKWOOD, 0);
        for (int i = 0; i < 3; ++i)
        {
            glPushMatrix();
            glTranslatef((i-1)*0.16f, 0.10f + i*0.015f, -0.02f + 0.02f*i);
            glRotatef(90, 0, 1, 0);
            glRotatef(i*24 - 20, 1, 0, 0);
            glScalef(0.09f, 0.55f, 0.09f);
            glutSolidSphere(0.5f, 10, 8);
            glPopMatrix();
        }

        if (lit)
        {
            glDisable(GL_LIGHTING);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            glPushMatrix();
            glTranslatef(0, 0.12f, 0.05f);
            FlameLayer(0.85f, 0.55f, 0.72f, 0.10f, 0.02f, timeSec,       0.0f);
            FlameLayer(0.62f, 0.48f, 1.00f, 0.38f, 0.05f, timeSec*1.13f, 2.1f);
            FlameLayer(0.38f, 0.34f, 1.00f, 0.78f, 0.16f, timeSec*0.87f, 4.2f);
            glPopMatrix();
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
            glEnable(GL_LIGHTING);
        }
    }

    // ------------------------------------------------------------------
    //  Lighting fixtures
    // ------------------------------------------------------------------
    void Chandelier(float timeSec)
    {
        glPushMatrix();
        glRotatef(timeSec * 8.0f, 0, 1, 0);   // slow continuous rotation

        SetSurface(Materials::GOLD, 0);
        glPushMatrix(); glTranslatef(0, -0.18f, 0); Box(0.03f, 0.36f, 0.03f); glPopMatrix();
        glPushMatrix(); glTranslatef(0, -0.48f, 0); glutSolidSphere(0.09f, 16, 12); glPopMatrix();
        glPushMatrix();
        glTranslatef(0, -0.44f, 0);
        glRotatef(90, 0, 1, 0);
        glScalef(0.4f, 0.05f, 0.4f);
        glutSolidTorus(0.4f, 0.5f, 10, 20);
        glPopMatrix();

        for (int i = 0; i < 6; ++i)
        {
            glPushMatrix();
            glRotatef(i * 60.0f, 0, 1, 0);
            glPushMatrix();
            glTranslatef(0, -0.50f, 0);
            glRotatef(-55, 0, 0, 1);
            glScalef(0.03f, 0.42f, 0.03f);
            glutSolidCube(1);
            glPopMatrix();
            glPushMatrix();
            glTranslatef(0, -0.44f, 0.34f);
            glPushMatrix(); glTranslatef(0, -0.03f, 0); glScalef(0.1f, 0.06f, 0.1f);
            glutSolidSphere(0.5f, 12, 8); glPopMatrix();
            glPushMatrix(); glTranslatef(0, 0.05f, 0);
            SetSurface(Materials::BULB, 0);
            glutSolidSphere(0.055f, 12, 10);
            glPopMatrix();
            glPopMatrix();
            glPopMatrix();   // arm orientation (was missing: leaked 6
                             // matrix frames per chandelier per frame,
                             // overflowing the modelview stack)
        }

        for (int i = 0; i < 8; ++i)
        {
            glPushMatrix();
            glRotatef(i * 45.0f + 22.5f, 0, 1, 0);
            glTranslatef(0, -0.62f, 0.30f);
            SetSurface(Materials::GLASS, 0);
            glPushMatrix(); glTranslatef(0, -0.07f, 0); glScalef(0.02f, 0.16f, 0.02f);
            glutSolidCube(1); glPopMatrix();
            glPopMatrix();
        }
        glPopMatrix();
    }

    // A grandfather clock frozen at midnight - the library clue.
    // No pendulum, no moving hands: its stillness IS the message.
    void StoppedClock()
    {
        const float W = 0.62f, H = 2.1f, D = 0.38f;
        SetSurface(Materials::DARKWOOD, "darkwood");
        // body: base plinth, waist, head
        glPushMatrix(); glTranslatef(0, 0.10f, 0); Box(W + 0.1f, 0.2f, D + 0.06f); glPopMatrix();
        glPushMatrix(); glTranslatef(0, H * 0.45f, 0); Box(W, H * 0.7f, D); glPopMatrix();
        glPushMatrix(); glTranslatef(0, H - 0.26f, 0); Box(W + 0.08f, 0.52f, D + 0.04f); glPopMatrix();
        // decorative crown
        SetSurface(Materials::GOLD, 0);
        glPushMatrix(); glTranslatef(0, H - 0.02f, 0); Box(W + 0.14f, 0.05f, D + 0.1f); glPopMatrix();
        // clock face (frozen at midnight: both hands straight up)
        glPushMatrix();
        glTranslatef(0, H - 0.26f, D/2 + 0.021f);
        Lighting_ApplyMaterial(Materials::MARBLE);
        glBegin(GL_QUADS);
            glNormal3f(0, 0, 1);
            glVertex3f(-0.20f, -0.20f, 0); glVertex3f(0.20f, -0.20f, 0);
            glVertex3f( 0.20f,  0.20f, 0); glVertex3f(-0.20f,  0.20f, 0);
        glEnd();
        // hands: minute at 12, hour at 12
        SetSurface(Materials::METAL, 0);
        glBegin(GL_TRIANGLES);
            glNormal3f(0, 0, 1);
            glVertex3f(-0.018f, 0.01f, 0.01f); glVertex3f(0.018f, 0.01f, 0.01f); glVertex3f(0, 0.15f, 0.01f);
            glVertex3f(-0.024f, 0.01f, 0.02f); glVertex3f(0.024f, 0.01f, 0.02f); glVertex3f(0, 0.10f, 0.02f);
        glEnd();
        glPopMatrix();
        // front glass (the polished pane from the letter's hint)
        glPushMatrix();
        glTranslatef(0, H * 0.45f, D/2 + 0.021f);
        glColor4f(0.7f, 0.8f, 0.9f, 0.15f);
        Lighting_ApplyMaterial(Materials::GLASS);
        glBegin(GL_QUADS);
            glNormal3f(0, 0, 1);
            glVertex3f(-W/2+0.04f, -H*0.32f, 0); glVertex3f(W/2-0.04f, -H*0.32f, 0);
            glVertex3f( W/2-0.04f,  H*0.32f, 0); glVertex3f(-W/2+0.04f, H*0.32f, 0);
        glEnd();
        glPopMatrix();
    }

    void SideChair(const Material &fabric)
    {
        SetSurface(Materials::DARKWOOD, "darkwood");
        // four legs
        for (int i = 0; i < 4; ++i)
        {
            float lx = (i % 2) ? 0.17f : -0.17f;
            float lz = (i / 2) ? 0.17f : -0.17f;
            glPushMatrix(); glTranslatef(lx, 0.22f, lz); Box(0.045f, 0.44f, 0.045f); glPopMatrix();
        }
        // seat frame + fabric seat
        glPushMatrix(); glTranslatef(0, 0.46f, 0); Box(0.42f, 0.05f, 0.42f); glPopMatrix();
        SetSurface(fabric, 0);
        glPushMatrix(); glTranslatef(0, 0.50f, 0); Box(0.38f, 0.05f, 0.38f); glPopMatrix();
        // backrest
        SetSurface(Materials::DARKWOOD, "darkwood");
        glPushMatrix(); glTranslatef(-0.17f, 0.75f, -0.19f); Box(0.05f, 0.56f, 0.045f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.17f, 0.75f, -0.19f); Box(0.05f, 0.56f, 0.045f); glPopMatrix();
        SetSurface(fabric, 0);
        glPushMatrix(); glTranslatef(0, 0.83f, -0.19f); Box(0.34f, 0.40f, 0.04f); glPopMatrix();
    }

    // Banker-style desk lamp: brass base + stem + green-gold shade + bulb.
    void DeskLamp(float timeSec)
    {
        SetSurface(Materials::GOLD, 0);
        glPushMatrix(); glTranslatef(0, 0.015f, 0); glScalef(0.11f, 0.03f, 0.11f);
        glutSolidSphere(0.5f, 16, 8); glPopMatrix();
        glPushMatrix(); glTranslatef(0, 0.16f, 0); Box(0.02f, 0.30f, 0.02f); glPopMatrix();
        // arm tilts the shade toward the desk surface
        glPushMatrix();
        glTranslatef(0, 0.31f, 0.03f);
        glRotatef(-30.0f + 1.2f * sinf(timeSec * 0.7f), 1, 0, 0);
        SetSurface(Materials::FABRIC_G, 0);
        glutSolidCone(0.13f, 0.14f, 18, 2);
        SetSurface(Materials::BULB, 0);
        glPushMatrix(); glTranslatef(0, -0.05f, 0); glutSolidSphere(0.035f, 10, 8); glPopMatrix();
        glPopMatrix();
    }

    // Small brass key: bow + blade with two teeth. Sits at the origin,
    // lying flat; a slow spin is added by the caller while it waits.
    void BrassKey()
    {
        SetSurface(Materials::GOLD, 0);
        // bow (ring)
        glPushMatrix(); glTranslatef(-0.05f, 0, 0); glRotatef(90, 0, 1, 0);
        glScalef(1.0f, 1.0f, 0.35f);
        glutSolidTorus(0.012f, 0.028f, 8, 16);
        glPopMatrix();
        // blade
        glPushMatrix(); glTranslatef(0.035f, 0, 0); Box(0.09f, 0.012f, 0.008f); glPopMatrix();
        // teeth
        glPushMatrix(); glTranslatef(0.068f, -0.012f, 0); Box(0.012f, 0.02f, 0.008f); glPopMatrix();
        glPushMatrix(); glTranslatef(0.046f, -0.010f, 0); Box(0.012f, 0.016f, 0.008f); glPopMatrix();
    }

    void TableLamp()
    {
        SetSurface(Materials::GOLD, 0);
        glPushMatrix(); glTranslatef(0, 0.015f, 0); glScalef(0.16f, 0.03f, 0.16f);
        glutSolidSphere(0.5f, 16, 8); glPopMatrix();
        glPushMatrix(); glTranslatef(0, 0.16f, 0); Box(0.025f, 0.28f, 0.025f); glPopMatrix();
        glPushMatrix(); glTranslatef(0, 0.34f, 0);
        SetSurface(Materials::FABRIC_C, 0);
        glutSolidCone(0.14f, 0.22f, 20, 2);
        glPopMatrix();
    }

    void WallSconce()
    {
        SetSurface(Materials::GOLD, 0);
        glPushMatrix(); Box(0.04f, 0.16f, 0.04f); glPopMatrix();
        glPushMatrix(); glTranslatef(0, 0.10f, 0.05f);
        SetSurface(Materials::BULB, 0);
        glutSolidSphere(0.05f, 12, 10);
        glPopMatrix();
    }

    void Column(float height, float radius)
    {
        EnsureQuad();
        // square plinth
        glPushMatrix();
        glTranslatef(0, 0.09f, 0);
        TexturedBox(Materials::MARBLE, "marble", radius*2.6f, 0.18f, radius*2.6f);
        glPopMatrix();
        // shaft (smooth quadric cylinder)
        glPushMatrix();
        glTranslatef(0, 0.18f, 0);
        Lighting_ApplyMaterial(Materials::MARBLE);
        Textures::Bind("marble");
        glPushMatrix();
        glTranslatef(0, 0, 0);
        glRotatef(-90, 1, 0, 0);
        gluCylinder(g_quad, radius, radius*1.04f, height - 0.40f, 16, 2);
        glPopMatrix();
        glPopMatrix();
        // capital
        glPushMatrix();
        glTranslatef(0, height - 0.13f, 0);
        TexturedBox(Materials::MARBLE, "marble", radius*2.8f, 0.26f, radius*2.8f);
        glPopMatrix();
    }

    // ------------------------------------------------------------------
    //  Decorations
    // ------------------------------------------------------------------
    void Painting(float w, float h, const char *texName)
    {
        glPushMatrix();
        glTranslatef(0, 0, 0.012f);
        TexQuad(Materials::FABRIC, texName, w, h);
        glPopMatrix();

        SetSurface(Materials::DARKWOOD, "darkwood");
        const float T = 0.06f;
        glPushMatrix(); glTranslatef(0,  h/2 + T/2, 0.02f); Box(w + 2*T, T, 0.05f); glPopMatrix();
        glPushMatrix(); glTranslatef(0, -h/2 - T/2, 0.02f); Box(w + 2*T, T, 0.05f); glPopMatrix();
        glPushMatrix(); glTranslatef(-w/2 - T/2, 0, 0.02f); Box(T, h, 0.05f); glPopMatrix();
        glPushMatrix(); glTranslatef( w/2 + T/2, 0, 0.02f); Box(T, h, 0.05f); glPopMatrix();
    }

    void PictureFrame(float w, float h)
    {
        glPushMatrix();
        glTranslatef(0, 0, 0.012f);
        TexQuad(Materials::FABRIC_C, "paint_land", w, h);
        glPopMatrix();
        SetSurface(Materials::GOLD, 0);
        const float T = 0.035f;
        glPushMatrix(); glTranslatef(0,  h/2 + T/2, 0.015f); Box(w + 2*T, T, 0.03f); glPopMatrix();
        glPushMatrix(); glTranslatef(0, -h/2 - T/2, 0.015f); Box(w + 2*T, T, 0.03f); glPopMatrix();
        glPushMatrix(); glTranslatef(-w/2 - T/2, 0, 0.015f); Box(T, h, 0.03f); glPopMatrix();
        glPushMatrix(); glTranslatef( w/2 + T/2, 0, 0.015f); Box(T, h, 0.03f); glPopMatrix();
    }

    void PottedPlant(float scale)
    {
        EnsureQuad();
        glPushMatrix();
        glScalef(scale, scale, scale);

        // pot: tapered cylinder
        glPushMatrix();
        glTranslatef(0, 0.17f, 0);
        glRotatef(-90, 1, 0, 0);
        SetSurface(Materials::POT, 0);
        gluCylinder(g_quad, 0.15f, 0.115f, 0.17f, 16, 2);
        glPopMatrix();
        // soil
        glPushMatrix();
        glTranslatef(0, 0.30f, 0);
        SetSurface(Materials::DARKWOOD, 0);
        glPushMatrix(); glScalef(0.13f, 0.02f, 0.13f); glutSolidSphere(0.5f, 14, 6); glPopMatrix();
        glPopMatrix();

        // stems + leaves
        SetSurface(Materials::LEAF, 0);
        for (int i = 0; i < 7; ++i)
        {
            float a = i * 51.0f;
            float lean = 18.0f + 9.0f * ((i*13)%5);
            glPushMatrix();
            glRotatef(a, 0, 1, 0);
            glRotatef(lean, 1, 0, 0);
            glTranslatef(0, 0, 0.10f);
            glPushMatrix();
            glTranslatef(0, 0.30f, 0);
            Box(0.025f, 0.60f, 0.025f);
            glPopMatrix();
            glPushMatrix();
            glTranslatef(0, 0.62f, 0.10f);
            glScalef(0.09f, 0.16f, 0.035f);
            glutSolidSphere(0.5f, 10, 8);
            glPopMatrix();
            glPopMatrix();
        }
        glPopMatrix();
    }

    void VaseWithFlowers(float scale)
    {
        glPushMatrix();
        glScalef(scale, scale, scale);
        glPushMatrix();
        glTranslatef(0, 0.14f, 0);
        SetSurface(Materials::MARBLE, "marble");
        glPushMatrix(); glScalef(0.11f, 0.28f, 0.11f); glutSolidSphere(0.5f, 16, 12); glPopMatrix();
        glPopMatrix();
        glPushMatrix();
        glTranslatef(0, 0.30f, 0);
        SetSurface(Materials::MARBLE, "marble");
        SolidCylinder(0.045f, 0.10f, 14);
        glPopMatrix();

        const float cols[5][3] = {{0.85f,0.25f,0.30f},{0.95f,0.75f,0.25f},{0.55f,0.65f,0.90f},{0.90f,0.55f,0.75f},{0.95f,0.95f,0.92f}};
        for (int i = 0; i < 5; ++i)
        {
            glPushMatrix();
            glRotatef(i*72.0f, 0, 1, 0);
            glRotatef(24, 1, 0, 0);
            glPushMatrix();
            glTranslatef(0, 0.42f, 0);
            SetSurface(Materials::LEAF, 0);
            Box(0.015f, 0.16f, 0.015f);
            glPopMatrix();
            glPushMatrix();
            glTranslatef(0, 0.52f, 0.03f);
            const float *c = cols[i];
            const Material fm = Materials::Tinted(Materials::FABRIC, c[0]*2.2f, c[1]*2.2f, c[2]*2.2f);
            SetSurface(fm, 0);
            glutSolidSphere(0.045f, 10, 8);
            glPopMatrix();
            glPopMatrix();
        }
        glPopMatrix();
    }

    void Candle(float h, bool lit, float timeSec)
    {
        SetSurface(Materials::FABRIC_C, 0);
        glPushMatrix(); glTranslatef(0, h/2, 0); Box(0.035f, h, 0.035f); glPopMatrix();
        if (lit)
        {
            glDisable(GL_LIGHTING);
            float f = 1.0f + 0.25f*sinf(timeSec*13.0f);
            glBegin(GL_TRIANGLES);
            glColor4f(1.0f, 0.85f, 0.35f, 0.9f);
            glVertex3f(-0.012f, h, 0);
            glVertex3f( 0.012f, h, 0);
            glVertex3f(0, h + 0.045f*f, 0);
            glEnd();
            glEnable(GL_LIGHTING);
        }
    }

    void Curtains(float w, float h)
    {
        SetSurface(Materials::GOLD, 0);
        glPushMatrix(); glTranslatef(0, h/2 + 0.06f, 0.10f); Box(w + 0.5f, 0.035f, 0.035f); glPopMatrix();
        glPushMatrix(); glTranslatef(-(w/2 + 0.25f), h/2 + 0.02f, 0.10f); glScalef(0.05f, 0.05f, 0.05f);
        glutSolidSphere(0.5f, 12, 8); glPopMatrix();
        glPushMatrix(); glTranslatef( (w/2 + 0.25f), h/2 + 0.02f, 0.10f); glScalef(0.05f, 0.05f, 0.05f);
        glutSolidSphere(0.5f, 12, 8); glPopMatrix();

        SetSurface(Materials::FABRIC, "carpet"); // burgundy fabric
        const int FOLDS = 7;
        const float PW = 0.34f;
        for (int side = -1; side <= 1; side += 2)
        {
            for (int i = 0; i < FOLDS; ++i)
            {
                float fx = side * (w/2 + 0.06f + i*0.035f);
                float depth = (i % 2) ? 0.10f : 0.16f;
                glPushMatrix();
                glTranslatef(fx, h/2, -depth/2 + 0.12f);
                Box(PW * 0.62f, h + 0.04f, depth);
                glPopMatrix();
            }
        }
    }

    void Rug(float w, float d)
    {
        glPushMatrix();
        glTranslatef(0, 0.012f, 0);
        glRotatef(-90, 1, 0, 0);
        glScalef(w, d, 1);
        Lighting_ApplyMaterial(Materials::FABRIC);
        Textures::Bind("carpet");
        QuadXYLocal();
        glPopMatrix();
    }
#include "ObjectsGarden.inc"

} // namespace Objects

