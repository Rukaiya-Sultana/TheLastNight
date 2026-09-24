// ============================================================
//  THE LAST NIGHT - Puzzle.cpp
//  The 3-digit safe keypad. Entry is keyboard-only (0-9, C, E/Enter);
//  ESC closes. A wrong code shows "Nothing happened." and keeps the
//  digits so the player can correct a single typo. The correct code
//  shows "Click." and "The lock opened." and flips Progress::SAFE_OPEN.
// ============================================================
#include "Puzzle.h"
#include "Progress.h"
#include <GL/freeglut.h>
#include <cstdio>
#include <cstring>

namespace
{
    const int CODE_DIGITS[3] = {1, 2, 7};   // clock=12, painting plate=7

    bool g_open   = false;
    bool g_solved = false;
    char g_digits[3] = {'_', '_', '_'};

    // feedback line
    enum Feedback { FB_NONE, FB_WRONG, FB_OPENING, FB_OPENED };
    Feedback g_fb = FB_NONE;
    float    g_fbTime = 0.0f;   // seconds since feedback started
    bool     g_justOpened = false;  // one-shot for the safe animation

    void DrawCentered(float x, float y, void *font, const char *text)
    {
        float width = 0.0f;
        for (const char *c = text; *c; ++c)
            width += glutBitmapWidth(font, *c);
        glRasterPos2f(x - width / 2.0f, y);
        for (const char *c = text; *c; ++c)
            glutBitmapCharacter(font, *c);
    }

    void DrawBox(float x0, float y0, float w, float h,
                 const float fill[4], const float edge[4], float lw)
    {
        glColor4f(fill[0], fill[1], fill[2], fill[3]);
        glBegin(GL_QUADS);
            glVertex2f(x0, y0); glVertex2f(x0 + w, y0);
            glVertex2f(x0 + w, y0 + h); glVertex2f(x0, y0 + h);
        glEnd();
        glLineWidth(lw);
        glColor4f(edge[0], edge[1], edge[2], edge[3]);
        glBegin(GL_LINE_LOOP);
            glVertex2f(x0, y0); glVertex2f(x0 + w, y0);
            glVertex2f(x0 + w, y0 + h); glVertex2f(x0, y0 + h);
        glEnd();
        glLineWidth(1.0f);
    }

    void Submit()
    {
        if (g_digits[0] == '_' || g_digits[1] == '_' || g_digits[2] == '_')
            return;
        if (g_digits[0] - '0' == CODE_DIGITS[0] &&
            g_digits[1] - '0' == CODE_DIGITS[1] &&
            g_digits[2] - '0' == CODE_DIGITS[2])
        {
            g_fb = FB_OPENING;
            g_justOpened = true;
            if (!g_solved)
            {
                g_solved = true;
                Progress::Set(Progress::SAFE_OPEN, true);
            }
        }
        else
        {
            g_fb = FB_WRONG;    // digits stay: fix a typo, try again
        }
        g_fbTime = 0.0f;
    }
}

namespace Puzzle
{
    void Reset()
    {
        g_open = false;
        g_solved = false;
        g_digits[0] = g_digits[1] = g_digits[2] = '_';
        g_fb = FB_NONE;
        g_fbTime = 0.0f;
        g_justOpened = false;
    }

    void Open()  { g_open = true;  g_fb = FB_NONE; }
    void Close() { g_open = false; }
    bool IsOpen(){ return g_open;  }
    bool Solved(){ return g_solved; }
    bool ConsumeJustOpened()
    {
        bool v = g_justOpened;
        g_justOpened = false;
        return v;
    }

    bool HandleKey(unsigned char key)
    {
        if (!g_open) return false;

        if (key == 27) { Close(); return true; }            // ESC
        if (key == 'e' || key == 'E' || key == 13)          // confirm
        {
            if (g_fb != FB_OPENING && g_fb != FB_OPENED)
                Submit();
            return true;
        }
        if (key == 'c' || key == 'C' || key == 8)           // clear
        {
            g_digits[0] = g_digits[1] = g_digits[2] = '_';
            g_fb = FB_NONE;
            return true;
        }
        if (key >= '0' && key <= '9')
        {
            if (g_fb == FB_OPENING || g_fb == FB_OPENED) return true;
            for (int i = 0; i < 3; ++i)
            {
                if (g_digits[i] == '_')
                {
                    g_digits[i] = key;
                    if (i == 2) Submit();   // auto-submit on the third digit
                    return true;
                }
            }
            return true;    // full: ignore extra digits (C to clear)
        }
        return true;        // keypad swallows everything while open
    }

    // Per-frame tick (feedback timing). Call from the game update.
    void Update(float dt)
    {
        if (g_fb != FB_NONE)
        {
            g_fbTime += dt;
            if (g_fb == FB_OPENING && g_fbTime > 1.0f) g_fb = FB_OPENED;
        }
    }

    void Render(int windowWidth, int windowHeight)
    {
        if (!g_open) return;

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(0, windowWidth, 0, windowHeight);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);    // overlays draw untextured, whatever the 3D pass left

        const float pw = 340.0f, ph = 420.0f;
        const float px0 = (windowWidth - pw) / 2.0f;
        const float py0 = (windowHeight - ph) / 2.0f;

        // dim the world behind the keypad
        glColor4f(0, 0, 0, 0.45f);
        glBegin(GL_QUADS);
            glVertex2f(0, 0); glVertex2f((float)windowWidth, 0);
            glVertex2f((float)windowWidth, (float)windowHeight);
            glVertex2f(0, (float)windowHeight);
        glEnd();

        const float steel[4] = {0.16f, 0.17f, 0.19f, 0.97f};
        const float edgeD[4] = {0.55f, 0.45f, 0.20f, 1.0f};   // brass trim
        const float edgeI[4] = {0.35f, 0.30f, 0.18f, 1.0f};
        DrawBox(px0, py0, pw, ph, steel, edgeD, 3.0f);
        DrawBox(px0 + 7, py0 + 7, pw - 14, ph - 14, steel, edgeI, 1.0f);

        glColor3f(0.85f, 0.72f, 0.35f);
        DrawCentered(windowWidth / 2.0f, py0 + ph - 34.0f,
                     GLUT_BITMAP_HELVETICA_18, "LIBRARY SAFE");

        // digit slots
        const float dsW = 52.0f, dsH = 62.0f;
        const float dsX0 = windowWidth / 2.0f - (1.5f * dsW + 2 * 12.0f);
        const float dsY  = py0 + ph - 118.0f;
        for (int i = 0; i < 3; ++i)
        {
            float x0 = dsX0 + i * (dsW + 12.0f);
            const float dark[4] = {0.06f, 0.07f, 0.08f, 1.0f};
            const bool filled = g_digits[i] != '_';
            const float *dEdge = filled ? edgeD : edgeI;
            DrawBox(x0, dsY, dsW, dsH, dark, dEdge, 2.0f);

            char ch[2] = {g_digits[i], 0};
            glColor3f(filled ? 0.95f : 0.35f, filled ? 0.85f : 0.35f,
                      filled ? 0.40f : 0.35f);
            DrawCentered(x0 + dsW/2, dsY + 24.0f, GLUT_BITMAP_HELVETICA_18, ch);
        }

        // feedback line
        const float fy = dsY - 22.0f;
        if (g_fb == FB_WRONG)
        {
            glColor3f(0.95f, 0.55f, 0.35f);
            DrawCentered(windowWidth / 2.0f, fy, GLUT_BITMAP_HELVETICA_12,
                         "Nothing happened.");
        }
        else if (g_fb == FB_OPENING || g_fb == FB_OPENED)
        {
            glColor3f(0.6f, 0.95f, 0.5f);
            DrawCentered(windowWidth / 2.0f, fy, GLUT_BITMAP_HELVETICA_12,
                         g_fb == FB_OPENING ? "Click." : "The lock opened.");
        }

        // key pad 3x4
        const float bw = 62.0f, bh = 40.0f, gap = 10.0f;
        const char *rows[4] = {"123", "456", "789", "C0E"};
        const float kx0 = windowWidth / 2.0f - (3 * bw + 2 * gap) / 2.0f;
        const float ky0 = py0 + 26.0f;
        for (int r = 0; r < 4; ++r)
        {
            for (int c = 0; c < 3; ++c)
            {
                float x0 = kx0 + c * (bw + gap);
                float y0 = ky0 + (3 - r) * (bh + gap);
                bool isGo  = (r == 3 && c == 2);
                bool isClr = (r == 3 && c == 0);
                const float btn[4]  = {0.24f, 0.22f, 0.16f, 0.95f};
                const float bGo[4]  = {0.30f, 0.34f, 0.18f, 0.95f};
                const float bClr[4] = {0.34f, 0.22f, 0.14f, 0.95f};
                const float bEdge[4] = {0.5f, 0.42f, 0.22f, 1.0f};
                DrawBox(x0, y0, bw, bh, isGo ? bGo : (isClr ? bClr : btn),
                        bEdge, 1.5f);
                char label[2] = {rows[r][c], 0};
                glColor3f(0.9f, 0.85f, 0.7f);
                DrawCentered(x0 + bw/2, y0 + 14.0f, GLUT_BITMAP_HELVETICA_18,
                             label);
            }
        }

        glColor3f(0.55f, 0.50f, 0.38f);
        DrawCentered(windowWidth / 2.0f, py0 - 14.0f, GLUT_BITMAP_HELVETICA_12,
                     "digits enter | C clear | E confirm | ESC walk away");

        // restore the 3D state (this pass pushed its own ortho projection)
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);
        glEnable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }
}
