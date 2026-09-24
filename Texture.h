#ifndef TEXTURE_H
#define TEXTURE_H

// ============================================================
//  THE LAST NIGHT - Texture.h
//  Reusable texture system. All textures are generated
//  procedurally in code (no external image files needed).
// ============================================================

namespace Textures
{
    // Must be called once after an OpenGL context exists.
    void Init();

    // Bind a texture by name. Unknown names bind nothing (leave last state).
    void Bind(const char *name);

    // Optional: returns OpenGL texture id or 0.
    unsigned int Get(const char *name);

    // --- palette shared by object code -------------------------------
    // (kept here so every module tints textures identically)
    struct RGB { unsigned char r, g, b; };
}

#endif // TEXTURE_H
