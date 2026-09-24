// ============================================================
//  THE LAST NIGHT - UI.cpp
//  Phase 0: 2D overlay using GLUT bitmap fonts.
//  The overlay is drawn in a separate 2D projection pass after
//  the 3D scene, a pattern reused later for HUD and menus.
// ============================================================
#include "UI.h"
#include <GL/freeglut.h>
#include <cmath>
#include <cstdio>

namespace
{
    GameMode g_mode = MODE_PLAYING;

    const int MAX_TEXT = 128;
    char g_statusText[MAX_TEXT] = "";
    char g_titleText[MAX_TEXT]  = "";

    // Phase 4: interaction prompt + message panel
    char g_prompt[64]  = "";
    char g_msgTitle[64] = "";
    char g_msgBody[512] = "";
    char g_objective[128] = "";
    char g_taskComplete[128] = "";
    float g_objectiveBearing = 0.0f;
    float g_objectiveDistance = 0.0f;
    float g_taskCompleteTime = 0.0f;

    // Pixel-measured width of `text` rendered in `font`.
    float TextWidth(void *font, const char *text)
    {
        float w = 0.0f;
        for (const char *c = text; *c; ++c)
            w += (float)glutBitmapWidth(font, *c);
        return w;
    }

    // Draw left-aligned text starting at (x, y).
    void DrawTextAt(float x, float y, void *font, const char *text)
    {
        glRasterPos2f(x, y);
        for (const char *c = text; *c; ++c)
            glutBitmapCharacter(font, *c);
    }

    void DrawCenteredText(float x, float y, void *font, const char *text)
    {
        glRasterPos2f(x - TextWidth(font, text) / 2.0f, y);
        for (const char *c = text; *c; ++c)
            glutBitmapCharacter(font, *c);
    }

    void DrawObjective(int windowWidth, int windowHeight)
    {
        if (!g_objective[0]) return;

        const float cx = windowWidth * 0.5f;
        const float top = windowHeight - 42.0f;
        const float panelW = 430.0f, panelH = 100.0f;
        const float x0 = cx - panelW * 0.5f, y0 = top - panelH;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.025f, 0.035f, 0.045f, 0.82f);
        glBegin(GL_QUADS);
            glVertex2f(x0, y0); glVertex2f(x0 + panelW, y0);
            glVertex2f(x0 + panelW, top); glVertex2f(x0, top);
        glEnd();
        glColor4f(0.67f, 0.51f, 0.23f, 0.9f);
        glLineWidth(1.5f);
        glBegin(GL_LINE_LOOP);
            glVertex2f(x0, y0); glVertex2f(x0 + panelW, y0);
            glVertex2f(x0 + panelW, top); glVertex2f(x0, top);
        glEnd();
        glLineWidth(1.0f);

        // Round compass plate. The bright needle turns toward the current
        // target; the task label sits directly beneath it.
        const float cy = top - 25.0f;
        glColor4f(0.10f, 0.12f, 0.13f, 0.96f);
        glBegin(GL_TRIANGLE_FAN);
            glVertex2f(cx, cy);
            for (int i = 0; i <= 32; ++i)
            {
                const float a = i * 6.2831853f / 32.0f;
                glVertex2f(cx + cosf(a) * 17.0f, cy + sinf(a) * 17.0f);
            }
        glEnd();
        glPushMatrix();
        glTranslatef(cx, cy, 0.0f);
        glRotatef(-g_objectiveBearing, 0, 0, 1);
        glColor3f(1.0f, 0.78f, 0.31f);
        glBegin(GL_TRIANGLES);
            glVertex2f(0, 13); glVertex2f(-6, -6); glVertex2f(0, -2);
            glVertex2f(0, 13); glVertex2f(0, -2); glVertex2f(6, -6);
        glEnd();
        glPopMatrix();

        char line[160];
        std::snprintf(line, sizeof(line), "%s  |  %.0f m", g_objective, g_objectiveDistance);
        glColor3f(0.96f, 0.91f, 0.77f);
        DrawCenteredText(cx, y0 + 13.0f, GLUT_BITMAP_HELVETICA_12, line);
        glDisable(GL_BLEND);
    }

    void DrawTaskComplete(int windowWidth, int windowHeight)
    {
        if (g_taskCompleteTime <= 0.0f || !g_taskComplete[0]) return;
        const float cx = windowWidth * 0.5f;
        const float y = 78.0f;
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.025f, 0.035f, 0.025f, 0.85f);
        glBegin(GL_QUADS);
            glVertex2f(cx - 230, y - 12); glVertex2f(cx + 230, y - 12);
            glVertex2f(cx + 230, y + 32); glVertex2f(cx - 230, y + 32);
        glEnd();
        glColor3f(0.67f, 0.90f, 0.47f);
        DrawCenteredText(cx, y + 12, GLUT_BITMAP_HELVETICA_12, "TASK COMPLETE");
        glColor3f(0.95f, 0.91f, 0.78f);
        DrawCenteredText(cx, y - 4, GLUT_BITMAP_HELVETICA_12, g_taskComplete);
        glDisable(GL_BLEND);
    }
}

namespace UI
{
    void Init()
    {
        // Two-sided lighting-off surfaces etc. can be tuned here later.
    }

    void SetMode(GameMode mode)               { g_mode = mode; }
    GameMode GetMode()                        { return g_mode; }
    bool IsPlaying()                          { return g_mode == MODE_PLAYING; }

    void SetStatusText(const char *text)
    {
        int i = 0;
        for (; text[i] && i < MAX_TEXT - 1; ++i) g_statusText[i] = text[i];
        g_statusText[i] = '\0';
    }

    void SetTitleText(const char *text)
    {
        int i = 0;
        for (; text[i] && i < MAX_TEXT - 1; ++i) g_titleText[i] = text[i];
        g_titleText[i] = '\0';
    }

    void ClearTexts()
    {
        g_statusText[0] = '\0';
        g_titleText[0]  = '\0';
    }

    void SetObjective(const char *label, float bearingDegrees, float distance)
    {
        int i = 0;
        for (; label[i] && i < 127; ++i) g_objective[i] = label[i];
        g_objective[i] = '\0';
        g_objectiveBearing = bearingDegrees;
        g_objectiveDistance = distance;
    }

    void ClearObjective() { g_objective[0] = '\0'; }

    void ShowTaskComplete(const char *label)
    {
        int i = 0;
        for (; label[i] && i < 127; ++i) g_taskComplete[i] = label[i];
        g_taskComplete[i] = '\0';
        g_taskCompleteTime = 2.4f;
    }

    void ClearTaskComplete()
    {
        g_taskComplete[0] = '\0';
        g_taskCompleteTime = 0.0f;
    }

    void Update(float dt)
    {
        if (g_taskCompleteTime > 0.0f)
        {
            g_taskCompleteTime -= dt;
            if (g_taskCompleteTime <= 0.0f) ClearTaskComplete();
        }
    }

    void SetPrompt(const char *text)
    {
        int i = 0;
        for (; text[i] && i < 63; ++i) g_prompt[i] = text[i];
        g_prompt[i] = '\0';
    }

    void SetMessage(const char *title, const char *body)
    {
        int i = 0;
        for (; title[i] && i < 63; ++i) g_msgTitle[i] = title[i];
        g_msgTitle[i] = '\0';
        i = 0;
        for (; body[i] && i < 511; ++i) g_msgBody[i] = body[i];
        g_msgBody[i] = '\0';
    }

    void HideMessage()
    {
        g_msgTitle[0] = '\0';
        g_msgBody[0]  = '\0';
    }

    bool MessageVisible()       { return g_msgTitle[0] != '\0'; }
    const char *MessageTitle()  { return g_msgTitle; }
    const char *MessageBody()   { return g_msgBody; }

    // Wrap g_msgBody for the panel. Returns the number of wrapped lines and
    // fills up to maxLines entries of `out`. Honors the author's explicit
    // \n breaks, re-wraps any run longer than the panel, and never
    // mutates the message buffer. (Lines beyond maxLines are still counted
    // so the card can size itself truthfully.)
    int WrapMessage(void *font, float maxLineW, char out[][128], int maxLines)
    {
        const int CAP = 512;                       // g_msgBody capacity + 1
        char word[CAP], cur[CAP];
        int wl = 0, cl = 0, lineCount = 0;

        // The buffer is static at the call site. Clear it before every
        // layout pass so explicit blank lines never show stale clue text.
        for (int i = 0; i < maxLines; ++i) out[i][0] = '\0';

        auto Flush = [&]()
        {
            if (cl == 0) return;
            if (lineCount < maxLines)
            {
                int k = 0;
                for (; k < cl && k < 127; ++k) out[lineCount][k] = cur[k];
                out[lineCount][k] = '\0';
            }
            ++lineCount;
            cl = 0;
        };

        for (const char *p = g_msgBody; ; ++p)
        {
            char c = *p;
            if (c == '\n' || c == '\0' || c == ' ')
            {
                word[wl] = '\0';
                if (wl > 0)
                {
                    const float wordW  = TextWidth(font, word);
                    const float spaceW = (cl > 0) ? (float)glutBitmapWidth(font, ' ')
                                                  : 0.0f;

                    if (wordW > maxLineW)
                    {
                        // A single word longer than the whole line: start it
                        // on a fresh line, then chunk it character by char.
                        Flush();
                        for (const char *q = word; *q; ++q)
                        {
                            if (cl > 0 &&
                                TextWidth(font, cur) + (float)glutBitmapWidth(font, *q)
                                    > maxLineW)
                                Flush();
                            if (cl < CAP - 1) cur[cl++] = *q;
                            cur[cl] = '\0';
                        }
                    }
                    else
                    {
                        if (cl > 0 &&
                            TextWidth(font, cur) + spaceW + wordW > maxLineW)
                            Flush();                   // wrap before the word
                        if (cl > 0) cur[cl++] = ' ';
                        for (const char *q = word; *q && cl < CAP - 1; ++q)
                            cur[cl++] = *q;
                        cur[cl] = '\0';
                    }
                    wl = 0;
                }
                if (c == '\n')
                {
                    Flush();
                    if (p[1] == '\n')                 // keep paragraph breaks
                    {
                        if (lineCount < maxLines) out[lineCount][0] = '\0';
                        ++lineCount;
                    }
                }
                if (c == '\0') break;
                continue;
            }
            if (wl < CAP - 1) word[wl++] = c;
        }
        Flush();
        return lineCount;
    }

    // Parchment message card. Sizes itself to the wrapped body text so the
    // full message always fits, whatever its length.
    void DrawMessagePanel(int windowWidth, int windowHeight)
    {
        const float marginX = 46.0f;
        const float headerH = 76.0f;    // title, divider, then clear body gap
        const float footerH = 44.0f;
        const float lineH   = 20.0f;
        float maxLineW = (float)windowWidth - 2.0f * marginX - 64.0f;
        if (maxLineW > 640.0f) maxLineW = 640.0f;
        if (maxLineW < 240.0f) maxLineW = 240.0f;
        void *bodyFont = GLUT_BITMAP_HELVETICA_12;

        static char lines[24][128];
        const int n = WrapMessage(bodyFont, maxLineW, lines, 24);

        // --- size the card to its content ---
        float bodyW = 0.0f;
        for (int i = 0; i < n && i < 24; ++i)
            bodyW = (bodyW > TextWidth(bodyFont, lines[i]))
                        ? bodyW : TextWidth(bodyFont, lines[i]);
        float titleW = TextWidth(GLUT_BITMAP_HELVETICA_18, g_msgTitle);
        if (titleW > bodyW) bodyW = titleW;

        float pw = bodyW + 2.0f * marginX;
        if (pw < 420.0f) pw = 420.0f;                     // readable minimum
        float pwMax = (float)windowWidth - 40.0f;
        if (pw > pwMax) pw = pwMax;

        float ph = headerH + (float)n * lineH + footerH;
        if (ph < 170.0f) ph = 170.0f;                     // pleasing minimum
        float phMax = (float)windowHeight - 60.0f;
        if (ph > phMax) ph = phMax;                       // never leave the screen

        const float px0 = ((float)windowWidth - pw) / 2.0f;
        const float py0 = ((float)windowHeight - ph) / 2.0f - 20.0f;

        // parchment with a soft double border
        glColor4f(0.87f, 0.80f, 0.62f, 0.96f);
        glBegin(GL_QUADS);
            glVertex2f(px0, py0); glVertex2f(px0+pw, py0);
            glVertex2f(px0+pw, py0+ph); glVertex2f(px0, py0+ph);
        glEnd();
        glLineWidth(3.0f);
        glColor4f(0.42f, 0.30f, 0.16f, 1.0f);
        glBegin(GL_LINE_LOOP);
            glVertex2f(px0+5, py0+5); glVertex2f(px0+pw-5, py0+5);
            glVertex2f(px0+pw-5, py0+ph-5); glVertex2f(px0+5, py0+ph-5);
        glEnd();
        glLineWidth(1.0f);
        glColor4f(0.55f, 0.42f, 0.24f, 1.0f);
        glBegin(GL_LINE_LOOP);
            glVertex2f(px0+11, py0+11); glVertex2f(px0+pw-11, py0+11);
            glVertex2f(px0+pw-11, py0+ph-11); glVertex2f(px0+11, py0+ph-11);
        glEnd();

        // title
        glColor3f(0.30f, 0.20f, 0.10f);
        DrawCenteredText(windowWidth / 2.0f, py0 + ph - 38.0f,
                         GLUT_BITMAP_HELVETICA_18, g_msgTitle);

        glColor3f(0.54f, 0.40f, 0.22f);
        glBegin(GL_LINES);
            glVertex2f(px0 + marginX, py0 + ph - 58.0f);
            glVertex2f(px0 + pw - marginX, py0 + ph - 58.0f);
        glEnd();

        // body: left-aligned wrapped lines, top-down
        float y = py0 + ph - headerH - 8.0f;
        glColor3f(0.16f, 0.11f, 0.05f);
        for (int i = 0; i < n && i < 24; ++i)
        {
            if (y >= py0 + footerH - 4.0f)
                DrawTextAt(px0 + marginX, y, bodyFont, lines[i]);
            y -= lineH;
        }

        // close hint
        glColor3f(0.45f, 0.32f, 0.16f);
        DrawCenteredText(windowWidth / 2.0f, py0 + 22.0f,
                         GLUT_BITMAP_HELVETICA_12, "[ E ] Close");
    }

    void Render(int windowWidth, int windowHeight)
    {
        // ---- switch to 2D overlay projection ----
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

        // ---- centered title banner ----
        if (g_titleText[0])
        {
            glColor3f(1.0f, 0.92f, 0.75f);
            DrawCenteredText(windowWidth / 2.0f, windowHeight * 0.72f,
                             GLUT_BITMAP_HELVETICA_18, g_titleText);
        }

        // ---- bottom status line ----
        if (g_statusText[0])
        {
            glColor3f(0.9f, 0.9f, 0.85f);
            DrawCenteredText(windowWidth / 2.0f, 30.0f,
                             GLUT_BITMAP_HELVETICA_12, g_statusText);
        }

        // ---- interaction prompt (below the crosshair) ----
        if (g_prompt[0])
        {
            glColor3f(1.0f, 0.88f, 0.55f);
            DrawCenteredText(windowWidth / 2.0f, windowHeight * 0.44f,
                             GLUT_BITMAP_HELVETICA_18, g_prompt);
        }

        // ---- readable message panel (parchment, self-sizing) ----
        if (g_msgTitle[0])
            DrawMessagePanel(windowWidth, windowHeight);

        if (g_mode == MODE_PLAYING && !g_msgTitle[0])
            DrawObjective(windowWidth, windowHeight);
        DrawTaskComplete(windowWidth, windowHeight);

        // ---- controls hint ----
        glColor3f(0.65f, 0.65f, 0.6f);
        if (g_mode != MODE_ENDING)
            DrawCenteredText(windowWidth / 2.0f, windowHeight - 24.0f,
                             GLUT_BITMAP_HELVETICA_12,
                             "WASD move | Mouse look | SHIFT run | E interact | F flashlight | ESC pause");

        // ---- restore 3D state (many room builders expect textures on) ----
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);
        glEnable(GL_DEPTH_TEST);

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }
}
