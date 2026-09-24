#ifndef UI_H
#define UI_H

// ============================================================
//  THE LAST NIGHT - UI.h
//  Phase 0: overlay state + centered text helper.
//  Later phases add HUD, inventory panel, menus.
// ============================================================

enum GameMode
{
    MODE_MENU,
    MODE_PLAYING,
    MODE_PAUSED,
    MODE_ENDING
};

namespace UI
{
    // Must be called once after the OpenGL context exists.
    void Init();

    void SetMode(GameMode mode);
    GameMode GetMode();
    bool IsPlaying();

    void SetStatusText(const char *text);   // small line at bottom (Phase 0 demo)
    void SetTitleText(const char *text);    // large centered banner

    // --- Phase 4: interaction prompt + readable message panel ---
    void SetPrompt(const char *text);       // "[ E ] Examine Letter"; "" hides
    void SetMessage(const char *title, const char *body); // parchment panel
    void HideMessage();
    bool MessageVisible();
    const char *MessageTitle();
    const char *MessageBody();

    void ClearTexts();

    // Sequential objective pointer and short task-complete notification.
    void SetObjective(const char *label, float bearingDegrees, float distance);
    void ClearObjective();
    void ShowTaskComplete(const char *label);
    void ClearTaskComplete();
    void Update(float dt);

    // Draws 2D overlay (call last, after 3D scene).
    void Render(int windowWidth, int windowHeight);
}

#endif // UI_H
