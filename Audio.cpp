#include "Audio.h"
#include <windows.h>
#include <mmsystem.h>
#include <cstdio>
#include <cstring>

namespace
{
    const int EFFECT_SLOTS = 8;
    struct Stream { const char *alias; char path[MAX_PATH]; bool open; bool requested; };

    Stream g_music    = { "tln_music", "", false, false };
    Stream g_ambience = { "tln_ambience", "", false, false };
    Stream g_effects[EFFECT_SLOTS] = {
        { "tln_sfx_0", "", false, false }, { "tln_sfx_1", "", false, false },
        { "tln_sfx_2", "", false, false }, { "tln_sfx_3", "", false, false },
        { "tln_sfx_4", "", false, false }, { "tln_sfx_5", "", false, false },
        { "tln_sfx_6", "", false, false }, { "tln_sfx_7", "", false, false }
    };
    int g_nextEffect = 0;
    bool g_muted = false;

    bool Send(const char *command)
    {
        MCIERROR error = mciSendStringA(command, 0, 0, 0);
        if (error == 0) return true;

        char message[256] = "Unknown MCI error";
        mciGetErrorStringA(error, message, sizeof(message));
        char diagnostic[512];
        std::snprintf(diagnostic, sizeof(diagnostic),
                      "Audio MCI error: %s | command: %s\n", message, command);
        std::fputs(diagnostic, stderr);
        OutputDebugStringA(diagnostic);
        return false;
    }

    void Close(Stream &stream)
    {
        if (!stream.open) return;
        char command[96];
        std::snprintf(command, sizeof(command), "stop %s", stream.alias); Send(command);
        std::snprintf(command, sizeof(command), "close %s", stream.alias); Send(command);
        stream.open = false;
        stream.path[0] = '\0';
    }

    bool ResolvePath(const char *relativePath, char out[MAX_PATH])
    {
        if (!relativePath || !*relativePath) return false;
        const char *prefixes[] = { "", "..\\..\\", "..\\..\\..\\" };
        for (int i = 0; i < 3; ++i)
        {
            std::snprintf(out, MAX_PATH, "%s%s", prefixes[i], relativePath);
            if (GetFileAttributesA(out) != INVALID_FILE_ATTRIBUTES) return true;
        }

        char module[MAX_PATH];
        DWORD len = GetModuleFileNameA(0, module, MAX_PATH);
        if (len == 0 || len >= MAX_PATH) return false;
        char *slash = std::strrchr(module, '\\');
        if (!slash) return false;
        *(slash + 1) = '\0';
        for (int i = 0; i < 3; ++i)
        {
            std::snprintf(out, MAX_PATH, "%s%s%s", module, prefixes[i], relativePath);
            if (GetFileAttributesA(out) != INVALID_FILE_ATTRIBUTES) return true;
        }
        return false;
    }

    bool Open(Stream &stream, const char *relativePath)
    {
        char fullPath[MAX_PATH];
        if (!ResolvePath(relativePath, fullPath)) return false;
        Close(stream);
        char command[MAX_PATH + 96];
        std::snprintf(command, sizeof(command), "open \"%s\" type waveaudio alias %s", fullPath, stream.alias);
        if (!Send(command)) return false;
        stream.open = true;
        std::strncpy(stream.path, relativePath, sizeof(stream.path) - 1);
        stream.path[sizeof(stream.path) - 1] = '\0';
        return true;
    }

    Stream *CategoryStream(Audio::Category category)
    {
        if (category == Audio::MUSIC) return &g_music;
        if (category == Audio::AMBIENCE) return &g_ambience;
        return 0;
    }

    void StartLoop(Stream &stream)
    {
        if (!stream.open || g_muted) return;
        char command[96];
        // "repeat" is not implemented consistently by the legacy Windows
        // waveaudio MCI driver. Update() restarts a stopped WAV instead.
        std::snprintf(command, sizeof(command), "play %s from 0", stream.alias);
        Send(command);
    }

    bool IsStopped(const Stream &stream)
    {
        char command[96], mode[64] = "";
        std::snprintf(command, sizeof(command), "status %s mode", stream.alias);
        MCIERROR error = mciSendStringA(command, mode, sizeof(mode), 0);
        if (error == 0) return std::strcmp(mode, "stopped") == 0;

        char message[256] = "Unknown MCI error";
        mciGetErrorStringA(error, message, sizeof(message));
        char diagnostic[512];
        std::snprintf(diagnostic, sizeof(diagnostic),
                      "Audio MCI error: %s | command: %s\n", message, command);
        std::fputs(diagnostic, stderr);
        OutputDebugStringA(diagnostic);
        return false;
    }
}

namespace Audio
{
    void Init() { g_muted = false; g_nextEffect = 0; }
    void Shutdown()
    {
        Close(g_music); Close(g_ambience);
        for (int i = 0; i < EFFECT_SLOTS; ++i) Close(g_effects[i]);
    }

    void Update()
    {
        // MCI waveaudio has no portable repeat command. Polling each frame
        // avoids any gameplay blocking and restarts only after a loop ends.
        if (!g_muted && g_music.requested && g_music.open && IsStopped(g_music))
            StartLoop(g_music);
        if (!g_muted && g_ambience.requested && g_ambience.open && IsStopped(g_ambience))
            StartLoop(g_ambience);
    }

    void PlayLoop(Category category, const char *relativePath)
    {
        Stream *stream = CategoryStream(category);
        if (!stream || !relativePath) return;
        stream->requested = true;
        // Do not restart a matching loop if a future caller checks it often.
        if (stream->open && std::strcmp(stream->path, relativePath) == 0) return;
        if (Open(*stream, relativePath)) StartLoop(*stream);
    }

    void Stop(Category category)
    {
        Stream *stream = CategoryStream(category);
        if (!stream) return;
        stream->requested = false;
        if (stream->open)
        {
            char command[96];
            std::snprintf(command, sizeof(command), "stop %s", stream->alias);
            Send(command);
        }
    }

    void PlayEffect(const char *relativePath)
    {
        if (g_muted || !relativePath) return;
        Stream &stream = g_effects[g_nextEffect];
        g_nextEffect = (g_nextEffect + 1) % EFFECT_SLOTS;
        if (!Open(stream, relativePath)) return;
        char command[96];
        std::snprintf(command, sizeof(command), "play %s from 0", stream.alias);
        Send(command);
    }

    void SetMuted(bool muted)
    {
        if (g_muted == muted) return;
        g_muted = muted;
        if (g_muted)
        {
            // Preserve requested loop state so unmuting resumes it. Stop()
            // intentionally clears that state for a real category change.
            if (g_music.open)
            {
                char command[96];
                std::snprintf(command, sizeof(command), "stop %s", g_music.alias);
                Send(command);
            }
            if (g_ambience.open)
            {
                char command[96];
                std::snprintf(command, sizeof(command), "stop %s", g_ambience.alias);
                Send(command);
            }
            for (int i = 0; i < EFFECT_SLOTS; ++i)
                if (g_effects[i].open)
                {
                    char command[96];
                    std::snprintf(command, sizeof(command), "stop %s", g_effects[i].alias);
                    Send(command);
                }
        }
        else
        {
            if (g_music.requested) StartLoop(g_music);
            if (g_ambience.requested) StartLoop(g_ambience);
        }
    }

    void ToggleMuted() { SetMuted(!g_muted); }
    bool IsMuted() { return g_muted; }
}
