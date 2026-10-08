// ============================================================
//  THE LAST NIGHT - Game.cpp
//  Application core: window, delta-timed loop, input routing,
//  player + camera, scene rendering, HUD and pause menu.
//  Phase 1/2: first-person player, mouse look, flashlight,
//  crosshair, pause, and the explorable mansion lobby.
// ============================================================
#include "Game.h"
#include "Camera.h"
#include "Player.h"
#include "Lighting.h"
#include "Texture.h"
#include "UI.h"
#include "Progress.h"
#include "Puzzle.h"
#include "Lobby.h"
#include "GrandHall.h"
#include "Library.h"
#include "Study.h"
#include "Garden.h"
#include "Basement.h"
#include "SecretRoom.h"
#include "Weather.h"
#include "Interaction.h"
#include "Objective.h"
#include "Audio.h"
#include <GL/freeglut.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace
{
    const char *WINDOW_TITLE = "The Last Night";
    const int   INIT_WIDTH   = 1280;
    const int   INIT_HEIGHT  = 720;

    int g_windowWidth  = INIT_WIDTH;
    int g_windowHeight = INIT_HEIGHT;

    unsigned int g_prevTimeMs = 0;
    float        g_elapsed    = 0.0f;

    Camera g_camera;
    Player g_player;

    bool g_pointerGrabbed = false;    // mouse captured for FPS look
    bool g_requestShot    = false;    // F4 dev-screenshot requested
    bool g_autoShot       = false;    // --shot: capture once, then exit

    // ending sequence state (Phase 10)
    float g_endingT   = 0.0f;
    float g_endingYaw = 0.0f;
    int   g_frontDial[3] = {0, 0, 0};   // front-door brass symbol positions

    // main-menu / pause navigation (Phase 11)
    int g_menuSel    = 0;               // 0=Play 1=Controls 2=Credits 4=Exit
    int g_menuScreen = 0;               // 0=menu 1=controls 2=credits
    int g_pauseSel   = 0;               // 0=Resume 1=Controls 2=Main Menu
    int g_pauseScreen = 0;              // 0=pause list 1=controls overlay
    bool g_devSpawn   = false;          // --pos/--yaw: skip menu, boot playing
    bool g_devFlash   = false;          // --flash: start with the flashlight on
    int g_devTurn     = -1;             // --turn NNN: preset front-dial state (dev)
    int g_devMsg      = 0;              // --msg N: show test message N on boot (dev)
    bool g_devPuzzle  = false;          // --puzzle: open the safe keypad on boot (dev)

    const char *MUSIC_LOOP = "assets/audio/music/mystery_loop.wav";
    const char *DOOR_OPEN = "assets/audio/sfx/door_wood_open.wav";
    const char *DOOR_CLOSE = "assets/audio/sfx/door_wood_close.wav";

    // ---- dev helper: dump the current frame to shot.bmp (F4) ----------
    void DumpScreenshot()
    {
        const int w = glutGet(GLUT_WINDOW_WIDTH);
        const int h = glutGet(GLUT_WINDOW_HEIGHT);
        const int rowBytes = w * 3;
        unsigned char *pix = new unsigned char[rowBytes * h];
        glReadBuffer(GL_BACK);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pix);
        // BMP scanlines are BGR: swap in place
        for (int i = 0; i < rowBytes * h; i += 3)
        {
            unsigned char t = pix[i]; pix[i] = pix[i+2]; pix[i+2] = t;
        }

        FILE *f = std::fopen("shot.bmp", "wb");
        if (f)
        {
            const int pad = (4 - (rowBytes % 4)) % 4;
            const int dataSize = (rowBytes + pad) * h;
            unsigned char header[54] = {0};
            header[0] = 'B'; header[1] = 'M';
            const int fileSize = 54 + dataSize;
            header[2] = fileSize & 255; header[3] = (fileSize>>8)&255;
            header[4] = (fileSize>>16)&255; header[5] = (fileSize>>24)&255;
            header[10] = 54;
            header[14] = 40;
            header[18] = w & 255; header[19] = (w>>8)&255;
            header[22] = h & 255; header[23] = (h>>8)&255;
            header[26] = 1; header[28] = 24;
            header[34] = dataSize & 255; header[35] = (dataSize>>8)&255;
            header[38] = 0x12; header[39] = 0x0B;
            std::fwrite(header, 1, 54, f);
            unsigned char padRow[4] = {0,0,0,0};
            for (int y = 0; y < h; ++y)
            {
                std::fwrite(pix + y*rowBytes, 1, rowBytes, f);
                if (pad) std::fwrite(padRow, 1, pad, f);
            }
            std::fclose(f);
        }
        delete[] pix;
    }
}

// ------------------------------------------------------------
//  Projection
// ------------------------------------------------------------
namespace
{
    void ApplyProjection()
    {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(60.0, (double)g_windowWidth / (double)g_windowHeight, 0.05, 200.0);
        glMatrixMode(GL_MODELVIEW);
    }

    void Reshape(int w, int h)
    {
        if (h == 0) h = 1;
        glViewport(0, 0, w, h);
        g_windowWidth  = w;
        g_windowHeight = h;
        ApplyProjection();
    }
}

// ------------------------------------------------------------
//  Pointer grab (FPS mouse look)
// ------------------------------------------------------------
namespace
{
    void GrabPointer()
    {
        int w = glutGet(GLUT_WINDOW_WIDTH);
        int h = glutGet(GLUT_WINDOW_HEIGHT);
        glutWarpPointer(w / 2, h / 2);
        glutSetCursor(GLUT_CURSOR_NONE);
        g_pointerGrabbed = true;
    }

    void ReleasePointer()
    {
        glutSetCursor(GLUT_CURSOR_INHERIT);
        g_pointerGrabbed = false;
    }
}

// ------------------------------------------------------------
//  Update
// ------------------------------------------------------------
namespace
{
    void Update(float dt)
    {
        Audio::Update();

        // ---- non-playing modes: the world still animates (doors finish
        // their swings, rain falls); only gameplay input is gated.
        if (UI::GetMode() == MODE_ENDING)
        {
            // cinematic: the camera slowly turns toward the mansion,
            // gliding for ~6 s (about 144 degrees) and then holding
            g_endingT += dt;
            const float turn = (g_endingT < 6.0f) ? g_endingT : 6.0f;
            g_player.SetYaw(g_endingYaw + turn * 24.0f);
            g_player.updateCamera(g_camera);
            Objective::Update(dt, g_player.x(), g_player.y(), g_player.z(), g_player.yaw());
            UI::Update(dt);
        }
        if (UI::GetMode() != MODE_PLAYING)
            return;

        g_player.update(dt);
        g_player.updateCamera(g_camera);
        Puzzle::Update(dt);
        // The safe begins its visual swing on the next render after this
        // one-shot is raised; play once for that real state transition.
        if (Puzzle::ConsumeJustOpened())
            Audio::PlayEffect(DOOR_OPEN);
        Objective::Update(dt, g_player.x(), g_player.y(), g_player.z(), g_player.yaw());
        UI::Update(dt);
        // The brass key is a world interaction as well as a rendered prop.
        // Arm it as soon as the keypad sets SAFE_OPEN; it begins disabled in
        // Library::Load so it cannot be taken through the closed shelf.
        if (Progress::Get(Progress::SAFE_OPEN))
            Interaction::SetEnabled(Interaction::ID_BRASSKEY, true);
        Library::UpdateStudyDoor(dt);   // study door swing + collision
        Lobby::UpdateGardenDoor(dt);    // garden entrance swing + collision
        Lobby::UpdateFrontDoor(dt);     // front double doors (Phase 10)
        Garden::UpdateCellarDoor(dt);   // cellar annex door
        Basement::UpdatePanel(dt);      // hidden stone panel slide
        Basement::UpdateSecretDoor(dt); // passage's secret door swing
        SecretRoom::UpdateDoor(dt);     // secret room door swing + collision
        SecretRoom::UpdateCabinet(dt);  // crest cabinet swing
        Weather::Update(dt, g_player.x(), g_player.z());   // rain + lightning

        // ---- the escape trigger: through the open front door, out of
        // the lobby, past the facade -> the ending begins
        if (!UI::IsPlaying()) return;
        if (Progress::Get(Progress::ESCAPED)) return;
        if (Progress::Get(Progress::FRONT_UNLOCKED) &&
            g_player.x() > -1.35f && g_player.x() < 1.35f &&
            g_player.z() < -7.8f && g_player.z() > -10.0f)
        {
            Progress::Set(Progress::ESCAPED, true);
            g_endingYaw  = g_player.yaw();
            g_endingT    = 0.0f;
            UI::SetMode(MODE_ENDING);
            UI::SetStatusText("");
            UI::SetTitleText("");
            ReleasePointer();
        }
    }
}

// ------------------------------------------------------------
//  Render
// ------------------------------------------------------------
namespace
{
    // ---- the ending text overlay (Phase 11): drawn over the live 3D
    // view while the ending camera slowly turns toward the mansion.
    void RenderEndingText()
    {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(0, g_windowWidth, 0, g_windowHeight);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);

        // soft dark band so the text stays readable over the scene
        glColor4f(0.0f, 0.0f, 0.0f, 0.45f);
        glBegin(GL_QUADS);
            glVertex2f(0, 0); glVertex2f(g_windowWidth, 0);
            glVertex2f(g_windowWidth, g_windowHeight); glVertex2f(0, g_windowHeight);
        glEnd();

        auto Center = [&](float yFrac, float r, float g, float b, const char *text)
        {
            glColor3f(r, g, b);
            float w = 0;
            for (const char *c = text; *c; ++c)
                w += glutBitmapWidth(GLUT_BITMAP_HELVETICA_18, *c);
            glRasterPos2f((g_windowWidth - w) / 2.0f, g_windowHeight * yFrac);
            for (const char *c = text; *c; ++c)
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        };

        Center(0.80f, 1.00f, 0.92f, 0.72f, "THE LAST NIGHT");
        Center(0.70f, 0.90f, 0.86f, 0.72f, "The house kept its secrets for years.");
        Center(0.65f, 0.90f, 0.86f, 0.72f, "Tonight, they finally came to light.");
        Center(0.58f, 0.85f, 0.80f, 0.66f, "But as the last window went dark,");
        Center(0.545f, 0.85f, 0.80f, 0.66f, "one question remained...");
        Center(0.47f, 1.00f, 0.85f, 0.45f, "Who left the first clue?");
        Center(0.34f, 1.00f, 1.00f, 0.95f, "You escaped the mansion.");
        Center(0.16f, 0.75f, 0.72f, 0.62f, "Press ENTER to return to the menu");

        glEnable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);
        glEnable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }

    // ---- 2D helpers shared by the menu and pause screens ---------------
    void OverlayBasis()
    {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(0, g_windowWidth, 0, g_windowHeight);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);                      // the dim quads need alpha
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void OverlayRestore()
    {
        glDisable(GL_BLEND);                     // restore opaque 3D state
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);
        glEnable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }

    void OverlayCenter(const char *t, float y, void *font, float r, float g, float b)
    {
        float wdt = 0.0f;
        for (const char *c = t; *c; ++c) wdt += glutBitmapWidth(font, *c);
        glColor3f(r, g, b);
        glRasterPos2f(g_windowWidth / 2.0f - wdt / 2.0f, y);
        for (const char *c = t; *c; ++c) glutBitmapCharacter(font, *c);
    }

    void DrawControlsList(float yTop)
    {
        const char *lines[] =
        {
            "W A S D            Move",
            "SHIFT              Run",
            "MOUSE              Look Around",
            "E                  Interact / Close",
            "F                  Flashlight",
            "ESC                Pause / Menu",
        };
        float y = yTop;
        for (const char *l : lines)
        {
            OverlayCenter(l, y, GLUT_BITMAP_HELVETICA_12, 0.92f, 0.88f, 0.80f);
            y -= 26.0f;
        }
    }

    // ---- main menu (Phase 11): the dimmed mansion behind an overlay ----
    void RenderMenu()
    {
        OverlayBasis();

        // dim the rendered mansion - visible, never black
        glColor4f(0.02f, 0.02f, 0.04f, 0.55f);
        glBegin(GL_QUADS);
            glVertex2f(0, 0); glVertex2f(g_windowWidth, 0);
            glVertex2f(g_windowWidth, g_windowHeight); glVertex2f(0, g_windowHeight);
        glEnd();

        if (g_menuScreen == 1)          // controls
        {
            OverlayCenter("CONTROLS", g_windowHeight * 0.80f, GLUT_BITMAP_HELVETICA_18, 1.0f, 0.92f, 0.72f);
            DrawControlsList(g_windowHeight * 0.62f);
            OverlayCenter("[ESC] Back", g_windowHeight * 0.14f, GLUT_BITMAP_HELVETICA_12, 0.70f, 0.62f, 0.45f);
        }
        else if (g_menuScreen == 2)     // credits
        {
            OverlayCenter("CREDITS", g_windowHeight * 0.80f, GLUT_BITMAP_HELVETICA_18, 1.0f, 0.92f, 0.72f);
            const char *lines[] =
            {
                "THE LAST NIGHT - A Mansion Mystery",
                "",
                "A university Computer Graphics project",
                "built with C++, OpenGL and FreeGLUT.",
                "",
                "Every wall, candle, puzzle and clue",
                "was hand-built from OpenGL primitives.",
            };
            float y = g_windowHeight * 0.60f;
            for (const char *l : lines)
            {
                if (l[0]) OverlayCenter(l, y, GLUT_BITMAP_HELVETICA_12, 0.92f, 0.88f, 0.80f);
                y -= 22.0f;
            }
            OverlayCenter("[ESC] Back", g_windowHeight * 0.14f, GLUT_BITMAP_HELVETICA_12, 0.70f, 0.62f, 0.45f);
        }
        else                            // the menu proper
        {
            OverlayCenter("THE LAST NIGHT", g_windowHeight * 0.78f, GLUT_BITMAP_HELVETICA_18, 1.0f, 0.94f, 0.78f);
            OverlayCenter("A Mansion Mystery", g_windowHeight * 0.72f, GLUT_BITMAP_HELVETICA_12, 0.85f, 0.80f, 0.65f);
            const char *items[] = { "PLAY", "CONTROLS", "CREDITS", "EXIT" };
            float y = g_windowHeight * 0.55f;
            for (int i = 0; i < 4; ++i)
            {
                bool sel = (i == g_menuSel);
                char buf[24];
                std::snprintf(buf, sizeof(buf), "%s %s", sel ? ">" : " ", items[i]);
                OverlayCenter(buf, y, GLUT_BITMAP_HELVETICA_18,
                              sel ? 1.0f : 0.72f, sel ? 0.85f : 0.68f, sel ? 0.55f : 0.55f);
                y -= 34.0f;
            }
            OverlayCenter("W/S to move  -  ENTER to select", g_windowHeight * 0.12f,
                          GLUT_BITMAP_HELVETICA_12, 0.65f, 0.60f, 0.45f);
        }

        OverlayRestore();
    }

    // ---- pause overlay: dim + RESUME / CONTROLS / MAIN MENU ------------
    void RenderPause()
    {
        OverlayBasis();

        glColor4f(0.0f, 0.0f, 0.0f, 0.45f);
        glBegin(GL_QUADS);
            glVertex2f(0, 0); glVertex2f(g_windowWidth, 0);
            glVertex2f(g_windowWidth, g_windowHeight); glVertex2f(0, g_windowHeight);
        glEnd();

        if (g_pauseScreen == 1)         // controls
        {
            OverlayCenter("CONTROLS", g_windowHeight * 0.80f, GLUT_BITMAP_HELVETICA_18, 1.0f, 0.92f, 0.72f);
            DrawControlsList(g_windowHeight * 0.62f);
            OverlayCenter("[ESC] Back", g_windowHeight * 0.14f, GLUT_BITMAP_HELVETICA_12, 0.70f, 0.62f, 0.45f);
        }
        else
        {
            OverlayCenter("PAUSED", g_windowHeight * 0.78f, GLUT_BITMAP_HELVETICA_18, 1.0f, 0.94f, 0.78f);
            const char *items[] = { "RESUME", "CONTROLS", "MAIN MENU" };
            float y = g_windowHeight * 0.55f;
            for (int i = 0; i < 3; ++i)
            {
                bool sel = (i == g_pauseSel);
                char buf[24];
                std::snprintf(buf, sizeof(buf), "%s %s", sel ? ">" : " ", items[i]);
                OverlayCenter(buf, y, GLUT_BITMAP_HELVETICA_18,
                              sel ? 1.0f : 0.72f, sel ? 0.85f : 0.68f, sel ? 0.55f : 0.55f);
                y -= 34.0f;
            }
            OverlayCenter("W/S to move  -  ENTER to select", g_windowHeight * 0.12f,
                          GLUT_BITMAP_HELVETICA_12, 0.65f, 0.60f, 0.45f);
        }

        OverlayRestore();
    }


    // Fixed-function flashlight fallback: with per-vertex (Gouraud)
    // lighting and coarse wall geometry the GL_LIGHT7 cone alone reads as
    // barely visible. This screen-space "light pool" at the aim point
    // adds the bright circular spill players expect from a flashlight.
    // ADDS a warm gradient to the scene, so the beam also glows over
    // near-black surfaces. Values stay modest to keep lit rooms from
    // washing out.
    void DrawFlashlightOverlay()
    {
        if (!g_player.flashlightOn() || !UI::IsPlaying())
            return;

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(0, g_windowWidth, 0, g_windowHeight);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);   // additive: glows over dark areas too

        const float cx = 0.50f * g_windowWidth;
        const float cy = 0.56f * g_windowHeight;
        const float rx = 0.42f * g_windowWidth;
        const float ry = 0.46f * g_windowHeight;

        glBegin(GL_TRIANGLE_FAN);
        glColor4f(0.38f, 0.35f, 0.29f, 0.62f);   // hot warm-white center
        glVertex2f(cx, cy);
        glColor4f(0.29f, 0.26f, 0.21f, 0.46f);
        glVertex2f(cx + rx, cy);
        glColor4f(0.17f, 0.15f, 0.12f, 0.30f);
        glVertex2f(cx + rx * 0.71f, cy + ry * 0.71f);
        glColor4f(0.08f, 0.07f, 0.055f, 0.20f);   // soft rim
        glVertex2f(cx, cy + ry);
        glColor4f(0.17f, 0.15f, 0.12f, 0.30f);
        glVertex2f(cx - rx * 0.71f, cy + ry * 0.71f);
        glColor4f(0.29f, 0.26f, 0.21f, 0.46f);
        glVertex2f(cx - rx, cy);
        glColor4f(0.17f, 0.15f, 0.12f, 0.30f);
        glVertex2f(cx - rx * 0.71f, cy - ry * 0.71f);
        glColor4f(0.08f, 0.07f, 0.055f, 0.20f);
        glVertex2f(cx, cy - ry);
        glColor4f(0.17f, 0.15f, 0.12f, 0.30f);
        glVertex2f(cx + rx * 0.71f, cy - ry * 0.71f);
        glColor4f(0.29f, 0.26f, 0.21f, 0.46f);
        glVertex2f(cx + rx, cy);
        glEnd();

        glDisable(GL_BLEND);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);
        glEnable(GL_DEPTH_TEST);

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }

    void DrawCrosshair()
    {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(0, g_windowWidth, 0, g_windowHeight);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        const float cx = g_windowWidth / 2.0f;
        const float cy = g_windowHeight / 2.0f;
        glLineWidth(2.0f);
        glColor4f(1, 1, 1, 0.85f);
        glBegin(GL_LINES);
            glVertex2f(cx - 9, cy); glVertex2f(cx - 3, cy);
            glVertex2f(cx + 3, cy); glVertex2f(cx + 9, cy);
            glVertex2f(cx, cy - 9); glVertex2f(cx, cy - 3);
            glVertex2f(cx, cy + 3); glVertex2f(cx, cy + 9);
        glEnd();
        glEnable(GL_TEXTURE_2D);

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }

    void RenderScene()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // The 3D world always renders (it backs the menu and pause
        // overlays too); the overlays are chosen at the end.
        g_camera.setView();                       // viewing transformation

        // Lights: lobby keeps L0-L3, the hall keeps L4/L5 always (so
        // through-doorway views stay lit). L0/L1/L2 + L6 are REPROGRAMMED
        // to whichever north room the player is in - the fixed-function
        // pipeline only has 8 light slots, so rooms share them.
        const float ppx = g_player.x(), ppz = g_player.z();
        const float ppy = g_player.y();
        const bool inLibrary  = (ppx > 8.0f && ppz > 12.5f && ppz < 23.6f);
        const bool inStudy    = (ppx > 11.9f && ppz > 23.2f);
        const bool belowGround = (ppy < -1.0f);
        const bool inGarden   = (ppx > 8.0f && !belowGround && !inLibrary && !inStudy);
        const bool inSecret  = (ppx > 31.3f && belowGround);
        const bool inBasement = (ppx > 13.0f && belowGround && !inSecret);
        // Fresh light budget every frame: rooms claim slots each frame, so
        // lights from previously visited rooms must be reset first or they
        // keep shining into every other room (fixed-function has only 8).
        Lighting_DisableAll();
        // Lobby + hall keep their core lights always (connecting doorway
        // views stay lit); their fill/moon slots only run when nearby.
        Lobby::UpdateLights(g_elapsed, inLibrary || inStudy || inGarden ||
                                         inBasement || inSecret);
        GrandHall::UpdateLights(g_elapsed, inGarden || inBasement || inSecret);
        if (inLibrary)      Library::UpdateLights(g_elapsed);
        else if (inStudy)   Study::UpdateLights(g_elapsed);
        else if (inGarden)  Garden::UpdateLights(g_elapsed);
        else if (inBasement) Basement::UpdateLights(g_elapsed);
        else if (inSecret)  SecretRoom::UpdateLights(g_elapsed);
        if (inLibrary)      Library::UpdateFill();
        else if (inStudy)   Study::UpdateFill();
        else if (inGarden)  Garden::UpdateFill();
        else if (inBasement) Basement::UpdateFill();
        else if (inSecret)  SecretRoom::UpdateFill();
        Lighting_Apply();

        // per-room global-ambient boost, with the lightning flash on top.
        // (Basement: warm candle ambience; garden: cool night lift. The
        // mansion rooms keep the built-in warm ambient. Game owns the
        // global ambient; rooms own their individual light slots.)
        {
            float br = 0.0f, bg = 0.0f, bb = 0.0f;
            if (UI::GetMode() == MODE_MENU)                    { /* menu: untouched boot state */ }
            else if (inBasement)     { br = 0.105f; bg = 0.095f; bb = 0.075f; }
            else if (inSecret)  { br = 0.155f; bg = 0.135f; bb = 0.105f; }
            else if (inGarden)  { br = 0.045f; bg = 0.050f; bb = 0.070f; }
            // (Weather::FlashLevel() stays 0 - rain/lightning removed)
            Lighting_SetAmbientBoost(br, bg, bb);
        }

        // flashlight follows the view direction
        float fx, fz;
        g_camera.getForward(fx, fz);
        float fy = sinf(g_camera.pitch * 3.14159265f / 180.0f);
        Lighting_SetFlashlight(g_player.flashlightOn() && UI::IsPlaying(),
                               g_camera.x, g_camera.y, g_camera.z,
                               fx, fy, fz);

        // push the front-door medallion state so the drawn symbols match
        const bool frontSolved = Progress::Get(Progress::FRONT_UNLOCKED);
        Lobby::SetFrontDialState(g_frontDial, frontSolved);

        // all six areas draw every frame (one continuous world; shared
        // walls occlude whatever is out of view)
        Lobby::Draw(g_elapsed);
        GrandHall::Draw(g_elapsed);
        Library::Draw(g_elapsed);
        Study::Draw(g_elapsed);
        Garden::Draw(g_elapsed, ppx, ppz);
        Basement::Draw(g_elapsed, ppx, ppz);
        SecretRoom::Draw(g_elapsed);

        // interaction focus pick (playing only; no prompt behind overlays)
        if (UI::IsPlaying())
        {
            Interaction::Update(g_player.x(), g_player.eyeY(), g_player.z(),
                                fx, fz);
            char promptBuf[72];
            promptBuf[0] = '\0';
            if (!UI::MessageVisible() &&
                Interaction::ActiveId() != Interaction::ID_NONE)
                std::snprintf(promptBuf, sizeof(promptBuf), "[ E ] %s",
                              Interaction::ActiveLabel());
            UI::SetPrompt(promptBuf);
        }
        else
            UI::SetPrompt("");

        if (UI::GetMode() == MODE_MENU)
            RenderMenu();                // dimmed mansion behind the menu
        else if (UI::GetMode() == MODE_ENDING)
        {
            RenderEndingText();          // cinematic text over the live view
            UI::Render(g_windowWidth, g_windowHeight); // final task-complete toast
        }
        else if (UI::GetMode() == MODE_PAUSED)
            RenderPause();               // frozen world behind the menu
        else
        {
            DrawFlashlightOverlay();   // flashlight beam spill
            DrawCrosshair();
            // one-shot flashlight HUD ("FLASHLIGHT ON/OFF" after F)
            if (g_player.FlashHud()[0])
            {
                UI::SetTitleText(g_player.FlashHud());
                g_player.ClearFlashHud();
            }
            UI::Render(g_windowWidth, g_windowHeight);
            Puzzle::Render(g_windowWidth, g_windowHeight);   // on top of UI
        }

        if (g_requestShot)
        {
            DumpScreenshot();
            g_requestShot = false;
            if (g_autoShot)
                glutLeaveMainLoop();   // --shot: done, exit cleanly
        }
        glutSwapBuffers();
    }

    void Display() { RenderScene(); }
}

// ------------------------------------------------------------
//  Input
// ------------------------------------------------------------
namespace
{
    // Show the readable panel for an interactable id.
    void ShowMessageFor(int id)
    {
        switch (id)
        {
            case Interaction::ID_LETTER:
                UI::SetMessage("OLD LETTER",
                    "\"If you are reading this, the house has chosen you.\n"
                    "The night everything changed, I hid the truth where\n"
                    "the house remembers. Find the room where the clock\n"
                    "stopped at midnight.\"");
                Interaction::MarkConsumed(Interaction::ID_LETTER);
                break;
            case Interaction::ID_LOBBYLETTER:
                UI::SetMessage("FADED LETTER",
                    "\"The guests left in such a hurry. Only the master\n"
                    "stayed behind, walking the halls after midnight.\n"
                    "They say the portrait in the great hall watches\n"
                    "whoever reads these words.\"");
                break;
            case Interaction::ID_FAMPORTRAIT:
                UI::SetMessage("PORTRAIT OF THE OWNER",
                    "The frame hangs crooked, as if someone searched\n"
                    "behind it. The eyes follow you across the room.\n"
                    "A small brass plate reads: E. VANDERMEER.");
                break;
            case Interaction::ID_CLOCK:
                UI::SetMessage("THE STOPPED CLOCK",
                    "The pendulum is still. The hands point to midnight\n"
                    "and refuse to move. The case is covered in a thin\n"
                    "layer of dust - except for the glass, which someone\n"
                    "polished recently.");
                break;
            case Interaction::ID_LOBBYDOOR:
                if (Progress::Get(Progress::HAS_MANSION_KEY))
                {
                    UI::SetMessage("LOCKED - BUT...",
                        "The mansion key slides in and turns - the lock\n"
                        "opens. But the door will not move: something is\n"
                        "barred from the other side. Not yet, then.");
                }
                else
                {
                    UI::SetMessage("LOCKED",
                        "The east door is locked. The keyhole is brass and\n"
                        "the mechanism turns - but something engages deep\n"
                        "inside the frame. It will need more than a push.");
                }
                break;

            // ---------------- Phase 5: the library ----------------
            case Interaction::ID_SECRETBOOK:
                UI::SetMessage("OLD JOURNAL",
                    "The book comes free in your hand. Bound in dark red\n"
                    "leather, a gold spine. Inside, in faded ink:\n\n"
                    "\"The truth was never kept in the rooms people\n"
                    "searched. Look for the numbers hidden where time\n"
                    "no longer moves.\"");
                Interaction::MarkConsumed(Interaction::ID_SECRETBOOK);
                break;
            case Interaction::ID_STOPCLOCK:
                UI::SetMessage("THE STOPPED CLOCK",
                    "A grandfather clock, frozen at midnight. The pendulum\n"
                    "is still, the hands frozen straight up: 12:00.\n"
                    "Unlike the clock in the great hall, this one was\n"
                    "stopped on purpose. The first number: 12.");
                Interaction::MarkConsumed(Interaction::ID_STOPCLOCK);
                break;
            case Interaction::ID_FOUNDER:
                UI::SetMessage("THE FOUNDER",
                    "A stern man in a dark coat. The brass plate below\n"
                    "reads: EDWARD VANDERMEER - FOUNDER - No. 7.\n"
                    "A number, carved in plain sight. The second piece.");
                Interaction::MarkConsumed(Interaction::ID_FOUNDER);
                break;
            case Interaction::ID_BRASSKEY:
                UI::SetMessage("OLD BRASS KEY",
                    "A small brass key, tucked where the red book stood.\n"
                    "It is warm, as if someone held it recently.");
                break;

            // ---------------- Phase 6: the study ----------------
            case Interaction::ID_PHOTO:
                UI::SetMessage("OLD PHOTOGRAPH",
                    "A family stands before the mansion, many years ago.\n"
                    "One figure has been carefully scratched out.\n\n"
                    "Behind them, carved into the wall: a circle, a\n"
                    "triangle, a moon. The same crest hangs above the\n"
                    "study fireplace.");
                Interaction::MarkConsumed(Interaction::ID_PHOTO);
                break;
            case Interaction::ID_JOURNAL:
                UI::SetMessage("THE STUDY JOURNAL",
                    "\"Father never trusted the clock. He said the house\n"
                    "remembers everything we try to forget.\n"
                    "He kept his maps where the light lands last.\"");
                Interaction::MarkConsumed(Interaction::ID_JOURNAL);
                break;
            case Interaction::ID_NOTE:
                UI::SetMessage("HIDDEN NOTE",
                    "\"The garden remembers what the house has forgotten.\n\n"
                    "If you are reading this, you have seen further\n"
                    "than the guests ever did. Take the mansion key.\n"
                    "The maps in the closet will show you where the\n"
                    "ground still lies open.\"");
                Interaction::MarkConsumed(Interaction::ID_NOTE);
                break;
            case Interaction::ID_STUDYFIRE:
                UI::SetMessage("THE FIRE",
                    "The fire has been kept burning every night since the\n"
                    "family left - as if someone still expects the master\n"
                    "to walk in and warm his hands.");
                break;

            // ---------------- Phase 7: the garden ----------------
            case Interaction::ID_FOUNTAIN:
                UI::SetMessage("THE FOUNTAIN",
                    "The basin still holds water, and the jets still leap -\n"
                    "but the pump house is silent. Something keeps the\n"
                    "water moving. The stone rim is worn smooth where\n"
                    "generations leaned against it.");
                break;
            case Interaction::ID_FOUNTPLAQ:
                UI::SetMessage("FOUNTAIN PLAQUE",
                    "Weather-worn letters around the old crest:\n\n"
                    "\"The garden remembers what the house has forgotten.\"\n\n"
                    "The same circle, triangle and moon that hangs above\n"
                    "the study fireplace.");
                Progress::Set(Progress::PLAQUE_READ, true);
                Interaction::MarkConsumed(Interaction::ID_FOUNTPLAQ);
                break;
            case Interaction::ID_STATUE:
                if (!Progress::Get(Progress::STATUE_FOUND))
                {
                    UI::SetMessage("THE GARDEN STATUE",
                        "A marble figure with an outstretched arm, holding\n"
                        "a basin toward the fountain. Its pedestal is wet\n"
                        "on one side - as if something behind it catches\n"
                        "the spray.\n\n(Something is set into the base.)");
                    Progress::Set(Progress::STATUE_FOUND, true);
                    // The plate occupies the same front face; once the
                    // statue has been inspected, leave its prompt to the
                    // newly revealed plate instead of competing for focus.
                    Interaction::SetEnabled(Interaction::ID_STATUE, false);
                    Interaction::SetEnabled(Interaction::ID_GPLATE, true);
                }
                else
                {
                    UI::SetMessage("THE GARDEN STATUE",
                        "The figure's basin points at the fountain. The\n"
                        "crest is carved again on the pedestal's rim.");
                }
                break;
            case Interaction::ID_GPLATE:
                UI::SetMessage("HIDDEN PLATE",
                    "A small brass plate, kept clean while everything\n"
                    "around it weathers:\n\n"
                    "\"Below the oldest wall the ground remembers its own.\n"
                    "The cellar door keeps the family crest.\"\n\n"
                    "The annex at the garden's north-east corner - that\n"
                    "wall is older than the rest.");
                Interaction::MarkConsumed(Interaction::ID_GPLATE);
                break;
            case Interaction::ID_BENCH:
                UI::SetMessage("THE BENCH",
                    "Two seats worn smooth by decades of use. A seated\n"
                    "view of the fountain - and of the mansion windows,\n"
                    "still warm against the evening.");
                break;

            // ---------------- Phase 8: the basement ----------------
            case Interaction::ID_STAIRDOWN:
                UI::SetMessage("THE STAIRS",
                    "Stone steps, hollowed in the middle by a century of\n"
                    "feet, descend beneath the lawn. Candlelight breathes\n"
                    "at the bottom.");
                break;
            case Interaction::ID_SYMBOL:
                UI::SetMessage("THE CARVED SYMBOL",
                    "The crest again - circle, triangle, moon - but this\n"
                    "one is different. The rings beneath it move. Three\n"
                    "stone dials sit in a row below the carving, each\n"
                    "grooved for turning.");
                break;
            case Interaction::ID_CANDLES:
                UI::SetMessage("THE CANDLES",
                    "Someone trims these wicks. The wax pools are fresh,\n"
                    "the brass polished. This cellar is not abandoned -\n"
                    "it is tended.");
                break;
            case Interaction::ID_BARRELS:
                UI::SetMessage("THE BARRELS",
                    "Wine casks, staved and banded, stacked by a hand\n"
                    "that knew its work. One is newer than the rest -\n"
                    "and empty.");
                break;
            case Interaction::ID_WINE:
                UI::SetMessage("THE WINE STORE",
                    "Bottles in green, brown and burgundy glass sleep in\n"
                    "the diamond racks. The vintages on the labels stop\n"
                    "abruptly - the year the family left.");
                break;
            case Interaction::ID_PASNOTE:
                UI::SetMessage("THE WAITING NOTE",
                    "The handwriting is steady, almost warm:\n\n"
                    "\"Everything you have found was left for you.\n"
                    "The house was never abandoned.\n"
                    "It was waiting.\"");
                Progress::Set(Progress::PASSAGE_NOTE, true);
                Interaction::MarkConsumed(Interaction::ID_PASNOTE);
                break;
            case Interaction::ID_PASPHOTO:
                UI::SetMessage("THE SECOND PHOTOGRAPH",
                    "The same family, the same mansion - but younger,\n"
                    "and standing on the lawn by the fountain. The\n"
                    "scratched-out figure stands a little apart, by the\n"
                    "annex. Behind them, on the garden wall: the crest.");
                Interaction::MarkConsumed(Interaction::ID_PASPHOTO);
                break;
            case Interaction::ID_SECRETDOOR:
                // The master key is inside the secret room, so it cannot be
                // required to enter. The opened basement panel is the gate.
                if (Progress::Get(Progress::PANEL_OPEN))
                {
                    UI::SetMessage("THE SECRET DOOR",
                        "The opened panel exposes the door's release.\n"
                        "The mechanism wakes after its long sleep, and\n"
                        "the door swings inward.");
                    Progress::Set(Progress::SECRETDOOR_OPEN, true);
                    Basement::SetSecretDoorOpen(true);
                    SecretRoom::SetDoorOpen(true);
                    Audio::PlayEffect(DOOR_OPEN);
                    Interaction::SetEnabled(Interaction::ID_SECRETDOOR, false);
                    Interaction::SetEnabled(Interaction::ID_SRDOOR, true);
                }
                else
                {
                    UI::SetMessage("THE SECRET DOOR",
                        "A door of dark oak and iron, sealed from this side.\n"
                        "The crest in gold at its center matches the one\n"
                        "above the dials. First open the hidden stone panel\n"
                        "with the three basement dials.");
                }
                break;

            // ---------------- Phase 9: the secret room ----------------
            case Interaction::ID_SRDOOR:
                // Keep the room-side control available in both states. If
                // the player closes the door after entering, they must still
                // be able to reopen it and return through the passage.
                if (SecretRoom::IsDoorOpen())
                {
                    SecretRoom::SetDoorOpen(false);
                    Audio::PlayEffect(DOOR_CLOSE);
                }
                else if (!SecretRoom::IsDoorOpening())
                {
                    SecretRoom::SetDoorOpen(true);
                    Audio::PlayEffect(DOOR_OPEN);
                }
                break;
            case Interaction::ID_BLUEPRINT:
                UI::SetMessage("THE MANSION BLUEPRINT",
                    "Ink on pale blue paper: every room you have walked -\n"
                    "lobby, hall, library, study, garden, cellar. And in the\n"
                    "basement's east wall, a chamber drawn in a heavier hand,\n"
                    "labelled simply: PRIVATE.\n\nThe mansion was designed with hidden spaces.");
                Progress::Set(Progress::BLUEPRINT_READ, true);
                Interaction::MarkConsumed(Interaction::ID_BLUEPRINT);
                break;
            case Interaction::ID_SMAP:
                UI::SetMessage("THE OLD MAP",
                    "The grounds, drawn before the garden walls were built.\n"
                    "A pencil line leaves the lawn through the north hedge\n"
                    "and does not stop at the wall. A note beneath:\n\n"
                    "\"The mansion has another entrance - one that does not\n"
                    "appear on the original plans.\"");
                Interaction::MarkConsumed(Interaction::ID_SMAP);
                break;
            case Interaction::ID_SPHOTO:
                UI::SetMessage("THE FAMILY PHOTOGRAPH",
                    "The same family from the study - younger here, posed\n"
                    "on the lawn. One figure has been scratched away again,\n"
                    "but this time the cut is rough, hurried.\n\n"
                    "Someone was deliberately removed from the family's history.");
                Interaction::MarkConsumed(Interaction::ID_SPHOTO);
                break;
            case Interaction::ID_SJOURNAL:
                UI::SetMessage("THE OWNER'S JOURNAL",
                    "The final entries, in a tired but steady hand:\n\n"
                    "\"They believe the house is empty. Let them.\n"
                    "Everything they came to take is already hidden.\n"
                    "The rooms remember what we cannot say aloud.\n"
                    "If you are reading this, you walked the same path\n"
                    "we did - backwards.\"");
                Interaction::MarkConsumed(Interaction::ID_SJOURNAL);
                break;
            case Interaction::ID_CREST:
                if (Progress::Get(Progress::CREST_SOLVED))
                    UI::SetMessage("THE GREAT CREST",
                        "All three rings rest in alignment. The bronze has\n"
                        "gone warm, as if the metal itself approves.");
                else
                    UI::SetMessage("THE GREAT CREST",
                        "Three bronze rings, each carrying the family crest,\n"
                        "each grooved for turning. Above them the same motto\n"
                        "line that runs around the fountain.\n\n"
                        "Every ring turns. The cabinet beside you is locked\n"
                        "until they agree with the carving above.");
                break;
            case Interaction::ID_GLOBE:
                UI::SetMessage("THE GLOBE",
                    "A traveller's globe, brass meridian worn bright.\n"
                    "Someone circled one coastline twice - as if checking\n"
                    "a promise the sea could not keep.");
                break;
            case Interaction::ID_SLETTER:
                UI::SetMessage("THE SEALED LETTER",
                    "The wax is unbroken. The handwriting matches the\n"
                    "journal:\n\n"
                    "\"You followed the house's trail exactly as I hoped.\n"
                    "If you are reading this, you have found what they\n"
                    "tried to bury.\n\n"
                    "But understand this: the front door was never\n"
                    "locked from the outside.\"");
                Progress::Set(Progress::FINAL_LETTER, true);
                Interaction::MarkConsumed(Interaction::ID_SLETTER);
                break;
            default:
                break;
        }
    }

    void ActivateInteraction()
    {
        int id = Interaction::Activate();
        if (id == Interaction::ID_NONE)
            return;
        Objective::NotifyInteraction(id);

        switch (id)
        {
            // ---------------- Phase 5: library actions ----------------
            case Interaction::ID_SAFE:
                if (!Progress::Get(Progress::SAFE_OPEN))
                {
                    Puzzle::Open();   // the keypad takes over the input
                }
                else if (!Progress::Get(Progress::HAS_MANSION_KEY))
                {
                    Progress::Set(Progress::HAS_MANSION_KEY, true);
                    UI::SetMessage("INSIDE THE SAFE",
                        "Obtained: Mansion Key.\n\n"
                        "The heavy iron key hangs on a hook, exactly where\n"
                        "the owner left it. Beside it, a folded photograph\n"
                        "of the family - one face scratched away - and a\n"
                        "note: \"The library shelf remembers me.\"");
                }
                else
                {
                    UI::SetMessage("THE SAFE",
                        "The door hangs open. Empty now, except for the\n"
                        "shape the key left in the dust.");
                }
                break;
            case Interaction::ID_BRASSKEY:
                if (Progress::Get(Progress::SAFE_OPEN) &&
                    !Progress::Get(Progress::HAS_BRASS_KEY))
                {
                    Progress::Set(Progress::HAS_BRASS_KEY, true);
                    Interaction::SetEnabled(Interaction::ID_BRASSKEY, false);
                    Progress::Set(Progress::STUDY_UNLOCKED, true);
                    Library::SetStudyDoorOpen(true);
                    Audio::PlayEffect(DOOR_OPEN);
                    Interaction::SetEnabled(Interaction::ID_STUDYDOOR, false);
                    Interaction::SetEnabled(Interaction::ID_STUDYCLOSE, true);
                    UI::SetMessage("OLD BRASS KEY",
                        "Obtained: Old Brass Key.\n\n"
                        "It is small, old, and heavier than it looks.\n"
                        "Somewhere in the library there is a lock it fits.");
                }
                else if (!Progress::Get(Progress::SAFE_OPEN))
                {
                    UI::SetMessage("THE SHELF",
                        "A gap in the row of books - something was kept\n"
                        "here, and taken away. Or is waiting to be found.");
                }
                break;
            case Interaction::ID_STUDYDOOR:
                if (Progress::Get(Progress::HAS_BRASS_KEY))
                {
                    Progress::Set(Progress::STUDY_UNLOCKED, true);
                    Library::SetStudyDoorOpen(true);
                    Audio::PlayEffect(DOOR_OPEN);
                    Interaction::SetEnabled(Interaction::ID_STUDYDOOR, false);
                    Interaction::SetEnabled(Interaction::ID_STUDYCLOSE, true);
                    UI::SetMessage("THE STUDY DOOR",
                        "The brass key fits. The lock gives with a soft\n"
                        "click, and the door swings open on quiet hinges.");
                }
                else
                {
                    UI::SetMessage("LOCKED",
                        "The door is locked. The keyhole is small and old -\n"
                        "it wants an old brass key.");
                }
                break;
            case Interaction::ID_STUDYCLOSE:
                Library::SetStudyDoorOpen(false);
                Audio::PlayEffect(DOOR_CLOSE);
                Interaction::SetEnabled(Interaction::ID_STUDYDOOR, true);
                Interaction::SetEnabled(Interaction::ID_STUDYCLOSE, false);
                break;

            // ---------------- Phase 6: study actions ----------------
            case Interaction::ID_DRAWER:
                Study::SetDrawerOpen(true);
                Interaction::SetEnabled(Interaction::ID_DRAWER, false);
                Interaction::SetEnabled(Interaction::ID_DRAWEROPEN, true);
                break;
            case Interaction::ID_DRAWEROPEN:
                if (!Progress::Get(Progress::NOTE_TAKEN))
                {
                    Progress::Set(Progress::NOTE_TAKEN, true);
                    Interaction::SetEnabled(Interaction::ID_DRAWEROPEN, false);
                    Interaction::SetEnabled(Interaction::ID_NOTE, true);
                    UI::SetMessage("THE HIDDEN NOTE",
                        "A folded note was hidden under the tray.\n"
                        "(See it with [ E ] Read Note.)");
                }
                break;
            case Interaction::ID_NOTE:
                ShowMessageFor(Interaction::ID_NOTE);
                break;
            case Interaction::ID_CLOSET:
                if (Progress::Get(Progress::MAP_CLOSET_OPEN))
                {
                    UI::SetMessage("THE MAP CLOSET",
                        "Rolled maps, charts of the grounds - and a fresh\n"
                        "pencil circle around the garden fountain. The last\n"
                        "thing anyone marked here.\n\n"
                        "\"The garden remembers what the house has forgotten.\"");
                }
                else if (Progress::Get(Progress::HAS_MANSION_KEY))
                {
                    Progress::Set(Progress::MAP_CLOSET_OPEN, true);
                    Audio::PlayEffect(DOOR_OPEN);
                    UI::SetMessage("THE MAP CLOSET",
                        "The mansion key turns. The closet opens on rolled\n"
                        "maps and brass fittings.");
                }
                else
                {
                    UI::SetMessage("THE MAP CLOSET",
                        "A sturdy closet with a heavy iron lock. It wants\n"
                        "the mansion key - a bigger key than the brass one.");
                }
                break;

            case Interaction::ID_HALLDOOR:
                GrandHall::SetWestDoorOpen(true);
                Interaction::SetEnabled(Interaction::ID_HALLDOOR, false);
                Interaction::SetEnabled(Interaction::ID_CLOSEDOOR, true);
                Audio::PlayEffect(DOOR_OPEN);
                break;
            case Interaction::ID_CLOSEDOOR:
                GrandHall::SetWestDoorOpen(false);
                Interaction::SetEnabled(Interaction::ID_HALLDOOR, true);
                Interaction::SetEnabled(Interaction::ID_CLOSEDOOR, false);
                Audio::PlayEffect(DOOR_CLOSE);
                break;

            // ---------------- Phase 7: garden actions ----------------
            case Interaction::ID_GARDENDOOR:
                if (Progress::Get(Progress::HAS_MANSION_KEY))
                {
                    // Progress can already record this door as opened while
                    // the runtime animation has been reset/closed. Always
                    // reapply the open state on its player-facing control.
                    const bool wasOpen = Progress::Get(Progress::GARDEN_DOOR_OPEN);
                    Progress::Set(Progress::GARDEN_DOOR_OPEN, true);
                    Lobby::SetGardenDoorOpen(true);
                    Audio::PlayEffect(DOOR_OPEN);
                    Interaction::SetEnabled(Interaction::ID_GARDENDOOR, false);
                    Interaction::SetEnabled(Interaction::ID_GARDENCLOSE, true);
                    if (!wasOpen)
                    {
                        UI::SetMessage("THE GARDEN DOOR",
                            "The mansion key turns. Beyond the door: rain-washed\n"
                            "stone, lantern light, and the sound of water.\n"
                            "The night air smells of wet roses.");
                    }
                }
                else
                {
                    UI::SetMessage("LOCKED",
                        "The garden door is locked from this side as well.\n"
                        "The keyhole is large and old - it wants the\n"
                        "mansion key.");
                }
                break;
            case Interaction::ID_GARDENCLOSE:
                Lobby::SetGardenDoorOpen(false);
                Audio::PlayEffect(DOOR_CLOSE);
                Interaction::SetEnabled(Interaction::ID_GARDENDOOR, true);
                Interaction::SetEnabled(Interaction::ID_GARDENCLOSE, false);
                break;
            case Interaction::ID_CELLARDOOR:
            {
                if (!Progress::Get(Progress::PLAQUE_READ) ||
                    !Progress::Get(Progress::STATUE_FOUND) ||
                    !Interaction::IsConsumed(Interaction::ID_GPLATE))
                {
                    UI::SetMessage("THE CELLAR DOOR",
                        "The crest-marked door stays latched. Read the\n"
                        "fountain plaque, inspect the statue, and examine\n"
                        "the hidden plate beneath its pedestal first.");
                    break;
                }
                Garden::SetCellarDoorOpen(true);
                Audio::PlayEffect(DOOR_OPEN);
                Interaction::SetEnabled(Interaction::ID_CELLARDOOR, false);
                Interaction::SetEnabled(Interaction::ID_CELLARCLOSE, true);
                Interaction::SetEnabled(Interaction::ID_ENTERBASEMENT, true);
                UI::SetStatusText("Cellar door unlocked - walk through and descend the stairs.");
                break;
            }
            case Interaction::ID_CELLARCLOSE:
                Garden::SetCellarDoorOpen(false);
                Audio::PlayEffect(DOOR_CLOSE);
                Interaction::SetEnabled(Interaction::ID_CELLARDOOR, true);
                Interaction::SetEnabled(Interaction::ID_CELLARCLOSE, false);
                Interaction::SetEnabled(Interaction::ID_ENTERBASEMENT, false);
                break;

            case Interaction::ID_ENTERBASEMENT:
                if (Garden::IsCellarDoorOpen())
                {
                    // Clear basement landing: inside the hall, south of the
                    // stair-wall opening and west of the east wall.
                    g_player.SetPos(26.4f, 0.2f);
                    g_player.SetYaw(0.0f);       // faces into the basement
                    g_player.updateCamera(g_camera);
                    UI::SetStatusText("Entered the Basement.");
                }
                break;
            case Interaction::ID_RETURNGARDEN:
                g_player.SetPos(26.4f, 4.75f);
                g_player.SetYaw(0.0f);           // faces back into the garden
                g_player.updateCamera(g_camera);
                UI::SetStatusText("Returned to the Garden.");
                break;

            // ---------------- Phase 8: mechanism actions ----------------
            case Interaction::ID_DIAL1:
                Basement::TurnDial(0);
                if (Basement::MechanismJustSolved())
                {
                    Progress::Set(Progress::PANEL_OPEN, true);
                    Basement::SetPanelOpen(true);
                    Audio::PlayEffect(DOOR_OPEN);
                    UI::SetMessage("THE MECHANISM",
                        "Deep in the wall, something heavy disengages.\n"
                        "The stone panel grinds aside - the crest now\n"
                        "glows softly on all three dials.");
                }
                break;
            case Interaction::ID_DIAL2:
                Basement::TurnDial(1);
                if (Basement::MechanismJustSolved())
                {
                    Progress::Set(Progress::PANEL_OPEN, true);
                    Basement::SetPanelOpen(true);
                    Audio::PlayEffect(DOOR_OPEN);
                    UI::SetMessage("THE MECHANISM",
                        "Deep in the wall, something heavy disengages.\n"
                        "The stone panel grinds aside - the crest now\n"
                        "glows softly on all three dials.");
                }
                break;
            case Interaction::ID_DIAL3:
                Basement::TurnDial(2);
                if (Basement::MechanismJustSolved())
                {
                    Progress::Set(Progress::PANEL_OPEN, true);
                    Basement::SetPanelOpen(true);
                    Audio::PlayEffect(DOOR_OPEN);
                    UI::SetMessage("THE MECHANISM",
                        "Deep in the wall, something heavy disengages.\n"
                        "The stone panel grinds aside - the crest now\n"
                        "glows softly on all three dials.");
                }
                break;
            // ---------------- Phase 9: crest puzzle ----------------
            case Interaction::ID_RING1:
            case Interaction::ID_RING2:
            case Interaction::ID_RING3:
            {
                const int ring = id - Interaction::ID_RING1;
                SecretRoom::TurnRing(ring);
                if (SecretRoom::CrestJustSolved())
                {
                    Progress::Set(Progress::CREST_SOLVED, true);
                    UI::SetMessage("THE MECHANISM",
                        "All three rings align with the carving above.\n"
                        "Inside the cabinet something releases - a soft\n"
                        "metallic click, then silence.");
                }
                break;
            }
            case Interaction::ID_CABINET:
                if (Progress::Get(Progress::CREST_SOLVED))
                {
                    if (!Progress::Get(Progress::CABINET_OPEN))
                    {
                        Progress::Set(Progress::CABINET_OPEN, true);
                        SecretRoom::SetCabinetOpen(true);
                        Audio::PlayEffect(DOOR_OPEN);
                        Interaction::SetEnabled(Interaction::ID_CABINET, false);
                        Interaction::SetEnabled(Interaction::ID_CABKEY, true);
                        UI::SetMessage("THE CABINET",
                            "The door swings open without a sound. Inside,\n"
                            "on green velvet: documents, a sealed envelope -\n"
                            "and an antique brass key, kept polished.");
                    }
                }
                else
                {
                    UI::SetMessage("LOCKED",
                        "The cabinet is sealed. The keyhole plate is shaped\n"
                        "like the family crest - it answers to the rings,\n"
                        "not to a key.");
                }
                break;
            case Interaction::ID_CABKEY:
                Progress::Set(Progress::HAS_FRONT_KEY, true);
                Progress::Set(Progress::HAS_MASTER_KEY, true);
                Interaction::MarkConsumed(Interaction::ID_CABKEY);
                UI::SetMessage("ITEM",
                    "Obtained: Antique Master Key\n\n"
                    "Heavy in the hand. The crest is stamped into its bow.");
                break;

            // ---------------- Phase 10: the way out ----------------
            case Interaction::ID_FRONTDOOR:
                if (Progress::Get(Progress::FRONT_UNLOCKED))
                {
                    if (!Progress::Get(Progress::ESCAPED))
                    {
                        if (!Lobby::IsFrontOpen())
                        {
                            Lobby::SetFrontOpen(true);
                            Audio::PlayEffect(DOOR_OPEN);
                        }
                        UI::SetMessage("THE FRONT DOOR",
                            "The doors swing outward. The night air carries\n"
                            "rain and roses. The path is open.");
                    }
                }
                else if (Progress::Get(Progress::HAS_FRONT_KEY))
                {
                    UI::SetMessage("THE FRONT DOOR",
                        "The master key turns - but three brass medallions\n"
                        "beside the lock must first agree with each other.\n\n"
                        "Moon. Star. Sun. The moon settles on the second\n"
                        "mark, the star on the first, the sun on the last.");
                }
                else
                {
                    UI::SetMessage("THE FRONT DOOR",
                        "The great doors do not open. The lock is strong,\n"
                        "and three brass medallions beside it glint in\n"
                        "the lamplight. A small plaque sits beside them.");
                }
                break;
            case Interaction::ID_SYMBOL1:
            case Interaction::ID_SYMBOL2:
            case Interaction::ID_SYMBOL3:
            {
                if (!Progress::Get(Progress::HAS_FRONT_KEY))
                {
                    UI::SetMessage("THE MECHANISM",
                        "The medallions are held fast. The lock needs its\n"
                        "master key before the symbols can be turned.");
                    break;
                }
                if (Progress::Get(Progress::FRONT_UNLOCKED))
                {
                    UI::SetMessage("THE MECHANISM",
                        "The medallions are already in agreement. The way\n"
                        "out stands open.");
                    break;
                }
                static const char *kName[3] = { "moon", "star", "sun" };
                const int idx = id - Interaction::ID_SYMBOL1;
                g_frontDial[idx] = (g_frontDial[idx] + 1) % 4;   // 0..3 quarters
                const bool solved =
                    g_frontDial[0] == 2 && g_frontDial[1] == 1 &&
                    g_frontDial[2] == 3;
                if (solved)
                {
                    Progress::Set(Progress::FRONT_UNLOCKED, true);
                    UI::SetMessage("THE MECHANISM",
                        "The three medallions click into agreement. Behind\n"
                        "the door, something heavy withdraws. The lock is\n"
                        "open.");
                }
                else
                {
                    // Live feedback: show this medallion's current quarter
                    // so turning never feels like guessing in the dark.
                    char buf[128];
                    static const char *kMark[4] =
                        { "first", "second", "third", "fourth" };
                    std::snprintf(buf, sizeof(buf),
                        "The %s medallion turns, stopping at the %s mark.\n\n"
                        "The correct order is written on the brass plaque\n"
                        "beside the door.",
                        kName[idx], kMark[g_frontDial[idx]]);
                    UI::SetMessage("THE MECHANISM", buf);
                }
                break;
            }
            case Interaction::ID_MEDPLAQUE:
                // Never consumes - the combination stays readable.
                UI::SetMessage("ENGRAVED PLAQUE",
                    "Freshly polished, unlike everything else in the hall:\n\n"
                    "\"The MOON rests on the second mark.\n"
                    "  The STAR keeps the first.\n"
                    "  The SUN holds the last.\"\n\n"
                    "Turn each medallion until its golden pip reaches\n"
                    "the mark that agrees with this verse.");
                break;
            case Interaction::ID_FAMPORTRAIT:
                UI::SetMessage("THE FINAL PORTRAIT",
                    "The whole family on the front steps, the mansion\n"
                    "warm behind them. The scratched-out figure stands\n"
                    "closest to the door - nearest to leaving first.");
                Interaction::MarkConsumed(Interaction::ID_FAMPORTRAIT);
                break;
            default:
                ShowMessageFor(id);
                break;
        }
    }

    void StartNewGame()
    {
        g_player.SetPos(0.0f, 2.5f);          // the lobby spawn
        g_player.SetYaw(180.0f);              // facing the fireplace (into the room)
        Progress::Reset();
        Objective::Reset();
        Puzzle::Reset();
        Weather::Reset();
        Interaction::RearmAll(); // re-arm consumed notes/keys (Reset would wipe the registry)
        SecretRoom::Reset();
        Basement::Reset();      // dial angles, panel, secret door
        GrandHall::Reset();     // door angle is stateful, not just a progress flag
        g_frontDial[0] = g_frontDial[1] = g_frontDial[2] = 0;
        Lobby::SetGardenDoorOpen(false);
        Lobby::SetFrontOpen(false);
        Library::SetStudyDoorOpen(false);
        Study::SetDrawerOpen(false);
        Garden::SetCellarDoorOpen(false);
        SecretRoom::SetDoorOpen(false);
        SecretRoom::SetCabinetOpen(false);
        g_menuScreen = 0; g_pauseScreen = 0; g_menuSel = 0; g_pauseSel = 0;
        UI::SetMode(MODE_PLAYING);
        UI::ClearTexts();
        UI::SetStatusText("Explore the mansion - the house keeps its secrets");
        Audio::PlayLoop(Audio::MUSIC, MUSIC_LOOP);
        GrabPointer();
    }

    void KeyboardDown(unsigned char key, int, int)
    {
        // Global audio control remains available over menus, clue cards,
        // keypad overlays, pause, and the ending.
        if (key == 'm' || key == 'M')
        {
            Audio::ToggleMuted();
            if (UI::IsPlaying())
                UI::SetStatusText(Audio::IsMuted() ? "Audio muted." : "Audio enabled.");
            return;
        }

        // the safe keypad swallows all input while it is open
        if (Puzzle::IsOpen())
        {
            Puzzle::HandleKey(key);
            return;
        }

        // ---- main menu (start / restart) ----
        if (UI::GetMode() == MODE_MENU)
        {
            if (g_menuScreen == 0)
            {
                if (key == 'w' || key == 'W') { g_menuSel = (g_menuSel + 3) % 4; return; }
                if (key == 's' || key == 'S') { g_menuSel = (g_menuSel + 1) % 4; return; }
                if (key == 13 || key == ' ')
                {
                    if (g_menuSel == 0)      StartNewGame();
                    else if (g_menuSel == 1) g_menuScreen = 1;   // CONTROLS
                    else if (g_menuSel == 2) g_menuScreen = 2;   // CREDITS
                    else                     glutLeaveMainLoop(); // EXIT
                }
            }
            else if (key == 27) g_menuScreen = 0;   // back from a sub-screen
            return;
        }
        // ---- ending: ENTER returns to the menu ----
        if (UI::GetMode() == MODE_ENDING)
        {
            if (key == 13)
                UI::SetMode(MODE_MENU);
            return;
        }

        if (key == 27) // ESC: pause / resume
        {
            if (UI::IsPlaying())
            {
                UI::SetMode(MODE_PAUSED);
                g_pauseSel = 0; g_pauseScreen = 0;
                ReleasePointer();
            }
            else if (UI::GetMode() == MODE_PAUSED)
            {
                if (g_pauseScreen == 1) { g_pauseScreen = 0; return; } // back from controls
                UI::SetMode(MODE_PLAYING);
                UI::ClearTexts();
                UI::SetStatusText("Explore the mansion - the house keeps its secrets");
                GrabPointer();
            }
            return;
        }

        // ---- pause menu navigation ----
        if (UI::GetMode() == MODE_PAUSED)
        {
            if (g_pauseScreen == 0)
            {
                if (key == 'w' || key == 'W') { g_pauseSel = (g_pauseSel + 2) % 3; return; }
                if (key == 's' || key == 'S') { g_pauseSel = (g_pauseSel + 1) % 3; return; }
                if (key == 13 || key == ' ')
                {
                    if (g_pauseSel == 0)        // RESUME
                    {
                        UI::SetMode(MODE_PLAYING);
                        UI::ClearTexts();
                        UI::SetStatusText("Explore the mansion - the house keeps its secrets");
                        GrabPointer();
                    }
                    else if (g_pauseSel == 1)   // CONTROLS
                        g_pauseScreen = 1;
                    else                        // MAIN MENU
                    {
                        UI::SetMode(MODE_MENU);
                        g_menuSel = 0; g_menuScreen = 0;
                    }
                }
            }
            return;
        }

        // close an open message first
        if (UI::MessageVisible())
        {
            if (key == 'e' || key == 'E' || key == 27)
                UI::HideMessage();
            return;
        }

        if (key == 'e' || key == 'E')
        {
            if (UI::IsPlaying())
                ActivateInteraction();
            return;
        }

        if (UI::IsPlaying())
            g_player.handleKeyboard(key, true);
    }

    void KeyboardUp(unsigned char key, int, int)
    {
        g_player.handleKeyboard(key, false);
    }

    void SpecialDown(int key, int, int)
    {
        if (key == GLUT_KEY_F4)
        {
            g_requestShot = true;      // dev: capture the current frame
            return;
        }
        if (UI::IsPlaying())
            g_player.handleSpecial(key, true);
    }

    void SpecialUp(int key, int, int)
    {
        g_player.handleSpecial(key, false);
    }

    void PassiveMotion(int mx, int my)
    {
        if (!UI::IsPlaying() || !g_pointerGrabbed)
            return;

        // The cursor is warped back to the center every frame; the delta
        // between event and center IS the look delta.
        int w = glutGet(GLUT_WINDOW_WIDTH);
        int h = glutGet(GLUT_WINDOW_HEIGHT);
        int dx = mx - w / 2;
        int dy = my - h / 2;

        if (dx == 0 && dy == 0)
            return; // our own warp-confirmation event

        g_player.applyMouseDelta(dx, dy);
        glutWarpPointer(w / 2, h / 2);
    }
}

// ------------------------------------------------------------
//  Loop
// ------------------------------------------------------------
namespace
{
    void TimerCallback(int)
    {
        unsigned int now = glutGet(GLUT_ELAPSED_TIME);
        float dt = (now - g_prevTimeMs) / 1000.0f;
        if (dt > 0.1f) dt = 0.1f;
        g_prevTimeMs = now;

        // the world keeps animating in every mode (candles, rain, doors
        // finish their swings); only gameplay input is mode-gated.
        g_elapsed += dt;
        Update(dt);

        if (g_autoShot && g_elapsed > 1.5f)
            g_requestShot = true;      // dev: unattended capture

        glutPostRedisplay();
        glutTimerFunc(16, TimerCallback, 0);
    }
}

// ------------------------------------------------------------
//  Init / Run
// ------------------------------------------------------------
bool Game::Init(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(INIT_WIDTH, INIT_HEIGHT);
    glutInitContextVersion(2, 1);
    glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
    glutCreateWindow(WINDOW_TITLE);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);

    Lighting_Init();
    Audio::Init();
    Textures::Init();
    UI::Init();
    UI::SetMode(MODE_MENU);    // boot into the main menu (Phase 11)
    UI::SetPrompt("");

    Lobby::Load();          // registers lobby collision + interactables
    GrandHall::Load();      // appends hall collision + interactables
    Library::Load();        // appends library collision + interactables
    Study::Load();          // appends study collision + interactables
    Garden::Load();         // appends garden collision + interactables
    Basement::Load();       // appends basement collision + interactables
    SecretRoom::Load();     // appends secret room collision + interactables
    Interaction::SealDefaults();   // snapshot boot state for RearmAll
    Progress::Reset();
    Puzzle::Reset();
    Objective::Reset();
    Weather::Reset();
    Player::SetGroundSampler(&Garden::GroundHeight);   // stairs + floors
    g_player.updateCamera(g_camera);
    ApplyProjection();

    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--shot") == 0)
            g_autoShot = true;
        if (std::strcmp(argv[i], "--yaw") == 0 && i + 1 < argc)
        {
            g_player.SetYaw((float)std::atof(argv[++i]));
            g_devSpawn = true;
        }
        if (std::strcmp(argv[i], "--pos") == 0 && i + 2 < argc)
        {
            float px = (float)std::atof(argv[i + 1]);
            float pz = (float)std::atof(argv[i + 2]);
            g_player.SetPos(px, pz);
            g_devSpawn = true;
            i += 2;
        }
        if (std::strcmp(argv[i], "--flash") == 0)
            g_devFlash = true;   // dev captures: flashlight pre-lit
        if (std::strcmp(argv[i], "--turn") == 0 && i + 1 < argc)
            g_devTurn = std::atoi(argv[++i]);   // dev captures: preset dials
        if (std::strcmp(argv[i], "--msg") == 0 && i + 1 < argc)
            g_devMsg = std::atoi(argv[++i]);    // dev captures: force a message card
        if (std::strcmp(argv[i], "--puzzle") == 0)
            g_devPuzzle = true;                 // dev captures: open the keypad
    }

    // dev captures boot straight into the world (menu would block them)
    if (g_devSpawn)
    {
        UI::SetMode(MODE_PLAYING);
        UI::SetStatusText("Explore the mansion - the house keeps its secrets");
    }
    if (g_devTurn >= 0)
    {
        g_frontDial[0] = g_devTurn / 100;
        g_frontDial[1] = (g_devTurn / 10) % 10;
        g_frontDial[2] = g_devTurn % 10;
        if (g_frontDial[0]==2 && g_frontDial[1]==1 && g_frontDial[2]==3)
            Progress::Set(Progress::FRONT_UNLOCKED, true);
    }
    if (g_devPuzzle)
        Puzzle::Open();
    if (g_devMsg == 1)   // real 9-line plaque text: regression test for wrapping
    {
        UI::SetMessage("ENGRAVED PLAQUE",
            "Freshly polished, unlike everything else in the hall:\n\n"
            "\"The MOON rests on the second mark.\n"
            "  The STAR keeps the first.\n"
            "  The SUN holds the last.\"\n\n"
            "Turn each medallion until its golden pip reaches\n"
            "the mark that agrees with this verse.");
    }
    if (g_devMsg == 2)   // long single paragraph: card must grow + wrap
    {
        UI::SetMessage("THE LONG LETTER",
            "The corridor stretches on and on while the wallpaper repeats "
            "the same dying roses, and every few steps the floorboards "
            "announce your weight to the empty rooms, and still the house "
            "says nothing back, and the candle-lamp in your free hand "
            "trembles just enough to make the shadows lean, and somewhere "
            "far above a door closes with the patience of something that "
            "knows you will eventually come upstairs.");
    }
    if (g_devMsg == 3)   // pathological single word: hard-break path
    {
        UI::SetMessage("THE TAPED LABEL",
            "Scratched into the brass in one unbroken stroke: "
            "VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV"
            "VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV");
    }
    if (g_devFlash)
    {
        g_player.toggleFlashlight();
        g_player.ClearFlashHud();
    }

    g_prevTimeMs = glutGet(GLUT_ELAPSED_TIME);
    return true;
}

bool Game::Run()
{
    glutKeyboardFunc(KeyboardDown);
    glutKeyboardUpFunc(KeyboardUp);
    glutSpecialFunc(SpecialDown);
    glutSpecialUpFunc(SpecialUp);
    glutPassiveMotionFunc(PassiveMotion);
    glutReshapeFunc(Reshape);
    glutDisplayFunc(Display);
    glutTimerFunc(16, TimerCallback, 0);

    // Menus need a visible, usable pointer.  The pointer is captured by
    // StartNewGame() (or by a developer world spawn in Init) once gameplay
    // actually begins.
    if (UI::IsPlaying()) GrabPointer();
    else                 ReleasePointer();

    glutMainLoop();
    Audio::Shutdown();
    return true;
}
