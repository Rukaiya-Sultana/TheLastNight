#ifndef AUDIO_H
#define AUDIO_H

// Small WinMM/MCI audio service. Categories remain independent so later
// phases can add ambience and effects without coupling them to rendering.
namespace Audio
{
    enum Category { MUSIC, AMBIENCE, EFFECT };

    void Init();
    void Shutdown();
    void Update();                       // keeps MCI wave loops alive

    // Example: "assets/audio/music/mystery_loop.wav". Missing files are
    // ignored safely so audio assets never affect gameplay.
    void PlayLoop(Category category, const char *relativePath);
    void Stop(Category category);
    void PlayEffect(const char *relativePath);

    void SetMuted(bool muted);
    void ToggleMuted();
    bool IsMuted();
}

#endif // AUDIO_H
