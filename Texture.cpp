// ============================================================
//  THE LAST NIGHT - Texture.cpp
//  Procedural textures: every texture is generated in code with
//  simple noise and gradients, mipmapped, and registered by name.
//  (A file-based loader can be added in the texture phase without
//   changing any call sites - Bind("name") stays the same.)
// ============================================================
#include "Texture.h"
#include <GL/freeglut.h>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>

namespace
{
    const int MAX_TEXTURES = 24;
    const int TS           = 128;   // default texture size

    struct TexEntry
    {
        char         name[24];
        unsigned int id;
    };

    TexEntry g_textures[MAX_TEXTURES];
    int      g_count = 0;

    // ---- deterministic pseudo-random noise (same picture every run) ----
    unsigned int g_seed = 12345;
    float Rand01()
    {
        g_seed = g_seed * 1103515245u + 12345u;
        return ((g_seed >> 16) & 0x7fff) / 32768.0f;
    }

    void Put(unsigned char *img, int x, int y, int r, int g, int b)
    {
        int i = (y * TS + x) * 3;
        img[i] = (unsigned char)r; img[i+1] = (unsigned char)g; img[i+2] = (unsigned char)b;
    }

    void Get(const unsigned char *img, int x, int y, int &r, int &g, int &b)
    {
        x = (x + TS) % TS; y = (y + TS) % TS;
        int i = (y * TS + x) * 3;
        r = img[i]; g = img[i+1]; b = img[i+2];
    }

    void Upload(const char *name, unsigned char *img)
    {
        if (g_count >= MAX_TEXTURES) return;
        unsigned int id = 0;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, TS, TS, GL_RGB, GL_UNSIGNED_BYTE, img);

        TexEntry &e = g_textures[g_count++];
        std::strncpy(e.name, name, sizeof(e.name) - 1);
        e.name[sizeof(e.name) - 1] = '\0';
        e.id = id;
    }

    // ---------------- generators ----------------

    // Wood planks: vertical boards with grain lines + darker gaps.
    void GenWood(unsigned char *img,
                 unsigned char r1, unsigned char g1, unsigned char b1,
                 unsigned char r2, unsigned char g2, unsigned char b2)
    {
        const int BOARD = 32;   // 4 boards across a 128px texture
        for (int y = 0; y < TS; ++y)
            for (int x = 0; x < TS; ++x)
            {
                float t = (x % BOARD) / (float)BOARD;
                float edge = (t < 0.04f || t > 0.96f) ? 0.55f : 1.0f;
                // wavy grain
                float grain = 0.88f + 0.12f * sinf((y + 8*sinf(x*0.31f)) * 0.55f);
                float n = 0.94f + 0.06f * Rand01();
                float k = edge * grain * n;
                Put(img, x, y, (int)(r1*k), (int)(g1*k), (int)(b1*k));
            }
        // subtle horizontal tone variation between boards
        for (int i = 0; i < TS; ++i)
        {
            int y0 = (i * BOARD + BOARD/2) % TS;
            for (int x = 0; x < TS; ++x)
            {
                int r,g,b; Get(img,x,y0,r,g,b);
                Put(img,x,y0,(int)(r*0.96f),(int)(g*0.96f),(int)(b*0.96f));
            }
        }
    }

    // Plaster: soft warm base with blotchy noise (walls, ceiling).
    void GenPlaster(unsigned char *img,
                    unsigned char r, unsigned char g, unsigned char b)
    {
        for (int y = 0; y < TS; ++y)
            for (int x = 0; x < TS; ++x)
            {
                float n = 0.93f + 0.07f * Rand01();
                // low-frequency blotches via two sines
                float blotch = 0.97f + 0.03f * sinf(x*0.11f)*cosf(y*0.09f);
                float k = n * blotch;
                Put(img, x, y, (int)(r*k), (int)(g*k), (int)(b*k));
            }
    }

    // Carpet: rich base with a diamond lattice pattern.
    void GenCarpet(unsigned char *img,
                   unsigned char r, unsigned char g, unsigned char b,
                   unsigned char r2, unsigned char g2, unsigned char b2)
    {
        for (int y = 0; y < TS; ++y)
            for (int x = 0; x < TS; ++x)
            {
                float d = fabsf(fmodf((x + y) * 0.125f, 2.0f) - 1.0f);
                float d2 = fabsf(fmodf((x - y) * 0.125f, 2.0f) - 1.0f);
                bool  line = (d < 0.16f) || (d2 < 0.16f);
                float n = 0.9f + 0.1f * Rand01();
                if (line) Put(img, x, y, (int)(r2*n), (int)(g2*n), (int)(b2*n));
                else      Put(img, x, y, (int)(r*n),  (int)(g*n),  (int)(b*n));
            }
    }

    // Stone bricks with mortar gaps (fireplace surround).
    void GenBrick(unsigned char *img,
                  unsigned char r, unsigned char g, unsigned char b)
    {
        const int BW = 42, BH = 21;
        for (int y = 0; y < TS; ++y)
            for (int x = 0; x < TS; ++x)
            {
                int row = y / BH;
                int ox  = (row % 2) ? BW/2 : 0;
                int bx  = (x + ox) % BW;
                int by  = y % BH;
                float mortar = (bx < 3 || by < 3) ? 0.55f : 1.0f;
                float n = 0.9f + 0.1f * Rand01();
                // rounded stone shading
                float shade = 0.92f + 0.08f * sinf(bx*0.15f)*sinf(by*0.2f);
                float k = mortar * n * shade;
                Put(img, x, y, (int)(r*k), (int)(g*k), (int)(b*k));
            }
    }

    // Marble: light veins wandering over a pale base.
    void GenMarble(unsigned char *img,
                   unsigned char r, unsigned char g, unsigned char b)
    {
        for (int y = 0; y < TS; ++y)
            for (int x = 0; x < TS; ++x)
            {
                float v = sinf((x + 14*sinf(y*0.09f)) * 0.22f);
                float vein = (v > 0.86f) ? 0.72f : 1.0f;
                float n = 0.96f + 0.04f * Rand01();
                float k = vein * n;
                Put(img, x, y, (int)(r*k), (int)(g*k), (int)(b*k));
            }
    }

    // Simple canvas painting: gradient sky + hills + sun/moon disc.
    void GenLandscape(unsigned char *img,
                      unsigned char sr, unsigned char sg, unsigned char sb,
                      unsigned char hr, unsigned char hg, unsigned char hb,
                      bool night)
    {
        for (int y = 0; y < TS; ++y)
            for (int x = 0; x < TS; ++x)
            {
                float ty = y / (float)TS;
                int r,g,b;
                // sky -> horizon blend
                r = (int)(sr + (hr-sr) * ty*1.6f);
                g = (int)(sg + (hg-sg) * ty*1.6f);
                b = (int)(sb + (hb-sb) * ty*1.6f);
                // rolling hills on the lower third
                float hill = 0.62f + 0.09f * sinf(x*0.09f) + 0.05f * sinf(x*0.23f+2.0f);
                if (ty > hill)
                {
                    float d = (ty-hill)*3.0f; if (d>1) d=1;
                    r = (int)(hr*(1.0f-0.25f*d)); g = (int)(hg*(1.0f-0.25f*d)); b = (int)(hb*(1.0f-0.25f*d));
                }
                // moon or sun
                int cx = TS*3/4, cy = TS/4;
                int dx = x-cx, dy = y-cy;
                if (dx*dx+dy*dy < 90) { r = night?238:255; g = night?232:240; b = night?190:170; }
                Put(img, x, y, r, g, b);
            }
        // frame vignette
        for (int i = 0; i < TS; ++i)
            for (int e = 0; e < 2; ++e)
            {
                Put(img, i, e, 40, 30, 20); Put(img, i, TS-1-e, 40, 30, 20);
                Put(img, e, i, 40, 30, 20); Put(img, TS-1-e, i, 40, 30, 20);
            }
    }

    // Abstract painting: overlapping translucent circles.
    void GenAbstract(unsigned char *img)
    {
        for (int y = 0; y < TS; ++y)
            for (int x = 0; x < TS; ++x)
                Put(img, x, y, 236, 226, 205);
        const int N = 9;
        const unsigned char cols[N][3] = {
            {178,52,54},{54,96,150},{70,130,80},{214,164,60},
            {120,70,140},{200,110,60},{90,160,170},{160,60,110},{100,110,60}};
        for (int i = 0; i < 26; ++i)
        {
            int c = rand() % N;
            int cx = rand() % TS, cy = rand() % TS, rad = 8 + rand() % 22;
            for (int y = 0; y < TS; ++y)
                for (int x = 0; x < TS; ++x)
                {
                    int dx=x-cx, dy=y-cy;
                    if (dx*dx+dy*dy < rad*rad)
                    {
                        int r,g,b; Get(img,x,y,r,g,b);
                        Put(img,x,y,(r+cols[c][0])/2,(g+cols[c][1])/2,(b+cols[c][2])/2);
                    }
                }
        }
    }

    // Book spines: vertical strips of varied warm colors.
    void GenBooks(unsigned char *img)
    {
        unsigned char palette[][3] = {
            {148,44,48},{58,92,148},{66,118,74},{168,124,52},
            {108,66,138},{150,92,50},{92,118,120},{170,66,90},
            {70,96,110},{140,110,60}};
        const int PAL = sizeof(palette)/sizeof(palette[0]);
        int x = 0;
        while (x < TS)
        {
            int w = 5 + rand() % 7;
            unsigned char *c = palette[rand() % PAL];
            float shade = 0.85f + 0.15f*Rand01();
            for (int sx = x; sx < x+w && sx < TS; ++sx)
                for (int y = 0; y < TS; ++y)
                {
                    float band = (y%22 < 2) ? 0.7f : 1.0f;   // spine bands
                    Put(img, sx, y, (int)(c[0]*shade*band), (int)(c[1]*shade*band), (int)(c[2]*shade*band));
                }
            x += w;
        }
    }

    // Night sky gradient with a scatter of stars (visible evening blue).
    void GenNightSky(unsigned char *img)
    {
        for (int y = 0; y < TS; ++y)
            for (int x = 0; x < TS; ++x)
            {
                float ty = y/(float)TS;
                Put(img, x, y,
                    (int)(36 + 26*(1-ty)),
                    (int)(52 + 40*(1-ty)),
                    (int)(96 + 58*(1-ty)));
            }
        for (int i = 0; i < 46; ++i)
        {
            int x = rand()%TS, y = rand()%TS/2, v = 170 + rand()%85;
            Put(img, x, y, v, v, (int)(v*0.92f));
        }
    }

    // Grass: green base with blade flecks.
    void GenGrass(unsigned char *img)
    {
        for (int y = 0; y < TS; ++y)
            for (int x = 0; x < TS; ++x)
            {
                float n = 0.85f + 0.15f*Rand01();
                Put(img, x, y, (int)(52*n), (int)(96*n), (int)(44*n));
            }
        for (int i = 0; i < 260; ++i)
        {
            int x = rand()%TS, y = rand()%TS;
            Put(img, x, y, 70, 128, 58);
        }
    }

    // ---- Phase 7/8: stone + flower palettes -----------------------------
    // Cut-stone blocks with warm gray/beige variation (courtyard, cellar).
    void GenStone(unsigned char *img)
    {
        for (int y = 0; y < TS; ++y)
            for (int x = 0; x < TS; ++x)
            {
                float n = 0.80f + 0.20f*Rand01();
                Put(img, x, y, (int)(168*n), (int)(158*n), (int)(140*n));
            }
        // block joints: horizontal courses + staggered verticals
        for (int y = 0; y < TS; y += 32)
            for (int x = 0; x < TS; ++x)
                Put(img, x, y, 118, 112, 100);
        for (int y = 0; y < TS; ++y)
        {
            int course = (y / 32) % 2;
            int jx = course ? 32 : 64;
            int x0 = (y / 32) * 0 + jx;
            for (int off = 0; off <= TS; off += 64)
            {
                int x = x0 + off;
                if (x >= 0 && x < TS) Put(img, x, y, 118, 112, 100);
            }
            if (course) { if (32 < TS) Put(img, 32, y, 118, 112, 100);
                          if (96 < TS) Put(img, 96, y, 118, 112, 100); }
        }
        for (int i = 0; i < 140; ++i)          // weather speckle
        {
            int x = rand()%TS, y = rand()%TS;
            Put(img, x, y, 140, 132, 116);
        }
    }

    // Forward decl (defined below): filled ellipse used by the generators.
    void FillEllipse(unsigned char *img, float cx, float cy, float rx, float ry,
                     int r, int g, int b);

    // Flower-petal palette for the garden beds (colorful blooms).
    void GenFlowers(unsigned char *img)
    {
        for (int y = 0; y < TS; ++y)
            for (int x = 0; x < TS; ++x)
                Put(img, x, y, 255, 255, 255);
        const int cols[6][3] = {
            {214, 52, 58}, {242, 196, 52}, {232, 122, 178},
            {156, 96, 214}, {255, 255, 255}, {240, 128, 46}};
        for (int i = 0; i < 26; ++i)
        {
            const int *c = cols[i % 6];
            float cx = 10 + (float)(rand() % (TS - 20));
            float cy = 10 + (float)(rand() % (TS - 20));
            float r  = 4 + rand() % 5;
            FillEllipse(img, cx, cy, r, r, c[0], c[1], c[2]);
            FillEllipse(img, cx + r*0.35f, cy - r*0.2f, r*0.45f, r*0.45f,
                        c[0]*9/10, c[1]*9/10, c[2]*9/10);
        }
        for (int i = 0; i < 40; ++i)           // leaf flecks
        {
            int x = rand()%TS, y = rand()%TS;
            Put(img, x, y, 52, 96, 40);
        }
    }

    // Filled axis-aligned ellipse helper for painted textures.
    void FillEllipse(unsigned char *img, float cx, float cy, float rx, float ry,
                     int r, int g, int b)
    {
        int x0 = (int)(cx - rx) - 1, x1 = (int)(cx + rx) + 1;
        int y0 = (int)(cy - ry) - 1, y1 = (int)(cy + ry) + 1;
        for (int y = y0; y <= y1; ++y)
            for (int x = x0; x <= x1; ++x)
            {
                if (x < 0 || y < 0 || x >= TS || y >= TS) continue;
                float nx = (x - cx) / rx, ny = (y - cy) / ry;
                if (nx*nx + ny*ny <= 1.0f)
                    Put(img, x, y, r, g, b);
            }
    }

    // Rich walnut hall floor: wider darker planks with stronger grain.
    void GenHallWood(unsigned char *img)
    {
        const int BOARD = 32;
        for (int y = 0; y < TS; ++y)
            for (int x = 0; x < TS; ++x)
            {
                float t = (x % BOARD) / (float)BOARD;
                float edge = (t < 0.05f || t > 0.95f) ? 0.45f : 1.0f;
                float grain = 0.84f + 0.16f * sinf((y + 12*sinf(x*0.27f)) * 0.45f);
                float n = 0.92f + 0.08f * Rand01();
                float k = edge * grain * n;
                Put(img, x, y, (int)(96*k), (int)(62*k), (int)(36*k));
            }
        // staggered plank end-joints
        for (int row = 0; row < TS / BOARD; ++row)
        {
            int gapY = row * BOARD + BOARD/2;
            int gapX = (row * 37) % TS;
            for (int dy = 0; dy < 2; ++dy)
                for (int dx = 0; dx < 3; ++dx)
                {
                    int x = (gapX + dx) % TS, y = (gapY + dy) % TS;
                    Put(img, x, y, 52, 32, 18);
                }
        }
    }

    // Formal portrait of the mansion owner (future puzzle object).
    void GenPortrait(unsigned char *img)
    {
        // dark sepia backdrop with vignette
        for (int y = 0; y < TS; ++y)
            for (int x = 0; x < TS; ++x)
            {
                float dx = (x - TS/2) / (float)TS, dy = (y - TS/2) / (float)TS;
                float vig = 1.0f - 1.6f * (dx*dx + dy*dy);
                float n = 0.9f + 0.1f*Rand01();
                float k = vig * n;
                Put(img, x, y, (int)(74*k), (int)(58*k), (int)(40*k));
            }
        // shoulders / dark suit, white collar, head, hair
        FillEllipse(img, TS/2, TS-14, 44, 34, 26, 22, 24);
        FillEllipse(img, TS/2, TS-26, 12, 9, 205, 198, 178);
        FillEllipse(img, TS/2, 52, 20, 26, 198, 164, 128);
        FillEllipse(img, TS/2, 34, 21, 12, 46, 34, 26);
        // pale eyes (they should read at distance)
        FillEllipse(img, TS/2-8, 52, 4, 2, 226, 222, 200);
        FillEllipse(img, TS/2+8, 52, 4, 2, 226, 222, 200);
        FillEllipse(img, TS/2-8, 52, 1, 2, 30, 26, 22);
        FillEllipse(img, TS/2+8, 52, 1, 2, 30, 26, 22);
        // mouth
        FillEllipse(img, TS/2, 66, 7, 2, 120, 74, 60);
        // brass nameplate
        for (int y = 108; y < 118; ++y)
            for (int x = TS/2-24; x < TS/2+24; ++x)
                Put(img, x, y, 168, 130, 48);
    }

    // Mysterious moonlit forest (hall painting).
    void GenForest(unsigned char *img)
    {
        // deep green-blue night gradient
        for (int y = 0; y < TS; ++y)
            for (int x = 0; x < TS; ++x)
            {
                float t = y / (float)TS;
                int r = (int)(16 + 10*t);
                int g = (int)(30 + 18*t);
                int b = (int)(40 + 20*t);
                float n = 0.92f + 0.08f*Rand01();
                Put(img, x, y, (int)(r*n), (int)(g*n), (int)(b*n));
            }
        // moon glow upper left
        for (int y = 0; y < TS; ++y)
            for (int x = 0; x < TS; ++x)
            {
                float dx = (x - 34)/22.0f, dy = (y - 30)/22.0f;
                float d = dx*dx + dy*dy;
                if (d < 1.0f)
                {
                    int r,g,b; Get(img,x,y,r,g,b);
                    float k = (1.0f - d) * 0.85f;
                    Put(img, x, y, (int)(r + (215-r)*k), (int)(g + (225-g)*k), (int)(b + (205-b)*k));
                }
            }
        // dark tree trunks
        for (int i = 0; i < 7; ++i)
        {
            int x0 = 8 + i*18 + (int)(Rand01()*6);
            int w  = 4 + (int)(Rand01()*4);
            for (int y = 0; y < TS; ++y)
                for (int x = x0; x < x0 + w; ++x)
                {
                    float n = 0.7f + 0.3f*Rand01();
                    Put(img, x % TS, y, (int)(18*n), (int)(22*n), (int)(16*n));
                }
        }
        // ground mist band
        for (int y = 100; y < TS; ++y)
            for (int x = 0; x < TS; ++x)
            {
                int r,g,b; Get(img,x,y,r,g,b);
                float k = 0.25f;
                Put(img, x, y, (int)(r + (120-r)*k), (int)(g + (130-g)*k), (int)(b + (125-b)*k));
            }
    }
}

namespace Textures
{
    void Init()
    {
        glEnable(GL_TEXTURE_2D);
        static unsigned char img[TS * TS * 3];

        GenPlaster(img, 255, 255, 255);        Upload("white",   img);
        GenWood(img, 148, 96, 52, 0,0,0);      Upload("wood",    img);
        GenWood(img, 108, 70, 40, 0,0,0);      Upload("darkwood",img);
        GenPlaster(img, 226, 214, 188);        Upload("plaster", img);
        GenPlaster(img, 208, 196, 170);        Upload("wall",    img);
        GenCarpet(img, 122, 30, 38, 196,160,84); Upload("carpet", img);
        GenBrick(img, 148, 106, 84);           Upload("brick",   img);
        GenMarble(img, 226, 222, 210);         Upload("marble",  img);
        GenLandscape(img, 110,150,190, 74,110,66, false); Upload("paint_land",  img);
        GenLandscape(img, 30,42,72,   40,64,44, true);    Upload("paint_night", img);
        GenAbstract(img);                      Upload("paint_abs", img);
        GenBooks(img);                         Upload("books",   img);
        GenNightSky(img);                      Upload("nightsky",img);
        GenGrass(img);                         Upload("grass",   img);
        GenStone(img);                         Upload("stone",   img);
        GenFlowers(img);                       Upload("flowers", img);
        GenHallWood(img);                      Upload("hallwood",img);
        GenPortrait(img);                      Upload("paint_owner", img);
        GenForest(img);                        Upload("paint_forest", img);

        glBindTexture(GL_TEXTURE_2D, 0);
        std::srand(7); // reset for any later procedural use
    }

    unsigned int Get(const char *name)
    {
        for (int i = 0; i < g_count; ++i)
            if (std::strcmp(g_textures[i].name, name) == 0)
                return g_textures[i].id;
        return 0;
    }

    void Bind(const char *name)
    {
        unsigned int id = Get(name);
        if (id != 0)
            glBindTexture(GL_TEXTURE_2D, id);
    }
}
