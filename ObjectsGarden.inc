    // ==================================================================
    //  Phase 7: GARDEN
    // ==================================================================
    void Tree(float scale, float timeSec)
    {
        (void)scale; // caller applies glScalef for size variation
        // trunk (tapered: two stacked cylinders)
        SetSurface(Materials::POT, "darkwood");
        glPushMatrix(); glTranslatef(0, 0.9f, 0); glRotatef(3.0f, 0, 0, 1);
        glPushMatrix(); glRotatef(-90, 1, 0, 0); glutSolidCylinder(0.22f, 1.8f, 10, 4); glPopMatrix();
        glPopMatrix();
        glPushMatrix(); glTranslatef(0.10f, 1.7f, 0.02f); glRotatef(9, 0, 0, 1);
        glPushMatrix(); glRotatef(-90, 1, 0, 0); glutSolidCylinder(0.14f, 1.2f, 10, 4); glPopMatrix();
        glPopMatrix();

        // foliage clusters (sway gently with time - subtle animation)
        const float sway = sinf(timeSec * 0.8f) * 2.2f;
        SetSurface(Materials::LEAF, "grass");
        struct L { float x, y, z, s; };
        const L cl[6] = { {0.00f, 3.6f, 0.00f, 1.25f}, { 0.75f, 3.1f,  0.25f, 0.85f},
                          {-0.65f, 3.2f, -0.30f, 0.90f}, { 0.15f, 3.0f,  0.75f, 0.75f},
                          {-0.10f, 4.5f, -0.10f, 0.95f}, { 0.85f, 4.1f,  0.30f, 0.60f} };
        for (int i = 0; i < 6; ++i)
        {
            glPushMatrix();
            glTranslatef(cl[i].x, cl[i].y, cl[i].z);
            glRotatef(sway, 0.3f, 1.0f, 0.2f);
            glutSolidSphere(cl[i].s, 12, 9);
            glPopMatrix();
        }
    }

    void Bush(float scale, int seed)
    {
        SetSurface(Materials::LEAF, "grass");
        for (int i = 0; i < 4; ++i)
        {
            float ox = ((seed * 7 + i * 13) % 10 - 5) * 0.06f * scale;
            float oz = ((seed * 5 + i * 17) % 10 - 5) * 0.06f * scale;
            float s  = (0.28f + 0.10f * ((seed + i * 3) % 4)) * scale;
            glPushMatrix();
            glTranslatef(ox, s * 0.7f, oz);
            glutSolidSphere(s, 10, 8);
            glPopMatrix();
        }
    }

    void Flower(float h, int kind)
    {
        static const float petalCol[6][3] = {
            {0.86f, 0.20f, 0.22f}, {0.95f, 0.78f, 0.20f}, {0.91f, 0.48f, 0.70f},
            {0.61f, 0.38f, 0.84f}, {0.96f, 0.96f, 0.92f}, {0.94f, 0.50f, 0.18f}};
        const float *pc = petalCol[kind % 6];

        // stem + leaves
        SetSurface(Materials::LEAF, 0);
        glPushMatrix(); glTranslatef(0, h / 2, 0); Box(0.02f, h, 0.02f); glPopMatrix();
        glPushMatrix(); glTranslatef(0.05f, h * 0.45f, 0); glRotatef(25, 0, 1, 0);
        Box(0.09f, 0.015f, 0.045f); glPopMatrix();

        // petals: 5 flat boxes rotated around the center
        Material pm = Materials::Tinted(Materials::BOOK, pc[0], pc[1], pc[2]);
        SetSurface(pm, 0);
        for (int i = 0; i < 5; ++i)
        {
            glPushMatrix();
            glTranslatef(0, h + 0.015f, 0);
            glRotatef(i * 72.0f, 0, 1, 0);
            glTranslatef(0.045f, 0, 0);
            Box(0.05f, 0.012f, 0.028f);
            glPopMatrix();
        }
        // center
        SetSurface(Materials::Tinted(Materials::BOOK, 1.0f, 0.85f, 0.2f), 0);
        glPushMatrix(); glTranslatef(0, h + 0.02f, 0);
        glutSolidSphere(0.018f, 8, 6); glPopMatrix();
    }

    void FlowerBed(float w, float d, int seed)
    {
        // stone border (4 slabs)
        SetSurface(Materials::STONE, "stone");
        glPushMatrix(); glTranslatef(0, 0.05f,  d / 2 + 0.05f); Box(w + 0.2f, 0.10f, 0.12f); glPopMatrix();
        glPushMatrix(); glTranslatef(0, 0.05f, -d / 2 - 0.05f); Box(w + 0.2f, 0.10f, 0.12f); glPopMatrix();
        glPushMatrix(); glTranslatef( w / 2 + 0.05f, 0.05f, 0); Box(0.12f, 0.10f, d); glPopMatrix();
        glPushMatrix(); glTranslatef(-w / 2 - 0.05f, 0.05f, 0); Box(0.12f, 0.10f, d); glPopMatrix();
        // soil
        SetSurface(Materials::Tinted(Materials::WOOD, 0.25f, 0.18f, 0.12f), 0);
        glPushMatrix(); glTranslatef(0, 0.02f, 0); Box(w, 0.05f, d); glPopMatrix();
        // blooms (deterministic per-seed)
        int n = 10 + seed % 5;
        for (int i = 0; i < n; ++i)
        {
            float fx = ((seed * 3 + i * 29) % 100 / 100.0f - 0.5f) * (w - 0.25f);
            float fz = ((seed * 7 + i * 53) % 100 / 100.0f - 0.5f) * (d - 0.25f);
            glPushMatrix();
            glTranslatef(fx, 0.04f, fz);
            Flower(0.16f + 0.05f * ((seed + i) % 3), i);
            glPopMatrix();
        }
    }

    void Hedge(float length, float height)
    {
        SetSurface(Materials::LEAF, "grass");
        const int SEG = (int)(length / 0.7f);
        for (int i = 0; i < SEG; ++i)
        {
            float x = -length / 2 + 0.35f + i * 0.7f;
            float s = 0.52f + 0.07f * ((i * 5) % 3);
            glPushMatrix();
            glTranslatef(x, height * 0.5f, ((i % 2) ? 0.03f : -0.03f));
            glScalef(1.15f, height / s, 0.85f);
            glutSolidSphere(s, 9, 7);
            glPopMatrix();
        }
    }

    void Bench()
    {
        // legs + frame
        SetSurface(Materials::DARKWOOD, "darkwood");
        for (int i = 0; i < 2; ++i)
        {
            float sx = (i ? 0.75f : -0.75f);
            glPushMatrix(); glTranslatef(sx, 0.21f, 0); Box(0.08f, 0.42f, 0.52f); glPopMatrix();
        }
        // seat slats
        for (int i = 0; i < 3; ++i)
        {
            glPushMatrix(); glTranslatef(0, 0.43f, -0.17f + i * 0.17f);
            Box(1.7f, 0.045f, 0.13f); glPopMatrix();
        }
        // back slats (slightly reclined)
        for (int i = 0; i < 3; ++i)
        {
            glPushMatrix();
            glTranslatef(0, 0.58f + i * 0.20f, 0.26f);
            glRotatef(-12, 1, 0, 0);
            Box(1.7f, 0.16f, 0.04f);
            glPopMatrix();
        }
        // arm rests
        glPushMatrix(); glTranslatef(-0.82f, 0.62f, 0.05f); Box(0.06f, 0.05f, 0.5f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.82f, 0.62f, 0.05f); Box(0.06f, 0.05f, 0.5f); glPopMatrix();
    }

    void GardenLamp(float timeSec)
    {
        (void)timeSec;
        // stone base
        SetSurface(Materials::STONE, "stone");
        glPushMatrix(); glTranslatef(0, 0.09f, 0); Box(0.34f, 0.18f, 0.34f); glPopMatrix();
        // dark post
        SetSurface(Materials::Tinted(Materials::METAL, 0.35f, 0.28f, 0.20f), 0);
        glPushMatrix(); glTranslatef(0, 0.95f, 0); Box(0.055f, 1.7f, 0.055f); glPopMatrix();
        // lantern cage + glowing bulb
        glPushMatrix(); glTranslatef(0, 1.86f, 0);
        SetSurface(Materials::Tinted(Materials::METAL, 0.35f, 0.28f, 0.20f), 0);
        Box(0.22f, 0.035f, 0.22f);                                    // top cap
        glPushMatrix(); glTranslatef(0, -0.30f, 0); Box(0.24f, 0.035f, 0.24f); glPopMatrix();
        SetSurface(Materials::BULB, 0);
        glutSolidSphere(0.085f, 10, 8);
        glPopMatrix();
        // cage corner posts
        SetSurface(Materials::Tinted(Materials::METAL, 0.35f, 0.28f, 0.20f), 0);
        for (int i = 0; i < 4; ++i)
        {
            float a = i * 3.14159265f / 2 + 0.785f;
            glPushMatrix(); glTranslatef(cosf(a) * 0.10f, 1.70f, sinf(a) * 0.10f);
            Box(0.02f, 0.30f, 0.02f); glPopMatrix();
        }
    }

    void Statue(float scale)
    {
        (void)scale; // caller applies glScalef
        SetSurface(Materials::MARBLE, "marble");
        // pedestal
        glPushMatrix(); glTranslatef(0, 0.20f, 0); Box(0.66f, 0.40f, 0.66f); glPopMatrix();
        glPushMatrix(); glTranslatef(0, 0.46f, 0); Box(0.50f, 0.12f, 0.50f); glPopMatrix();
        // figure: robe (cylinder), torso, head, extended arm with a basin
        glPushMatrix();
        glTranslatef(0, 0.52f, 0);
        glPushMatrix(); glTranslatef(0, 0.55f, 0); glRotatef(-90, 1, 0, 0);
        glutSolidCylinder(0.16f, 1.10f, 12, 6); glPopMatrix();         // robe
        glPushMatrix(); glTranslatef(0, 1.18f, 0); glutSolidSphere(0.15f, 12, 9); glPopMatrix();
        glPushMatrix(); glTranslatef(0, 1.42f, 0.01f); glutSolidSphere(0.10f, 12, 9); glPopMatrix();
        glPushMatrix(); glTranslatef(0.14f, 1.22f, 0.10f); glRotatef(65, 1, 0, 0);
        Box(0.05f, 0.42f, 0.05f); glPopMatrix();                       // arm
        glPushMatrix(); glTranslatef(0.14f, 1.30f, 0.32f);
        glutSolidTorus(0.035f, 0.10f, 8, 14); glPopMatrix();           // basin
        // wing (flattened sphere)
        glPushMatrix(); glTranslatef(-0.17f, 1.15f, -0.08f); glRotatef(20, 0, 0, 1);
        glScalef(0.5f, 1.0f, 0.25f); glutSolidSphere(0.22f, 10, 8); glPopMatrix();
        glPopMatrix();
    }

    void WaterSurface(float r, float timeSec)
    {
        // translucent rippling disc: concentric rings pulsing outward
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glDisable(GL_LIGHTING);
        const int RINGS = 3;
        for (int k = 0; k < RINGS; ++k)
        {
            float phase = fmodf(timeSec * 0.5f + k * 0.33f, 1.0f);
            float rr = r * (0.35f + 0.6f * phase);
            glColor4f(0.55f, 0.85f, 1.0f, 0.38f * (1.0f - phase));
            glBegin(GL_TRIANGLE_FAN);
            glNormal3f(0, 1, 0);
            glVertex3f(0, 0.015f, 0);
            for (int i = 0; i <= 24; ++i)
            {
                float ang = i * 2 * 3.14159265f / 24;
                glVertex3f(cosf(ang) * rr, 0.015f + 0.012f * sinf(ang * 3 + timeSec * 2),
                           sinf(ang) * rr);
            }
            glEnd();
        }
        // still water body beneath
        glColor4f(0.30f, 0.55f, 0.80f, 0.55f);
        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0, 1, 0);
        glVertex3f(0, 0.008f, 0);
        for (int i = 0; i <= 24; ++i)
        {
            float ang = i * 2 * 3.14159265f / 24;
            glVertex3f(cosf(ang) * r, 0.008f, sinf(ang) * r);
        }
        glEnd();
        glEnable(GL_LIGHTING);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    void Fountain(float timeSec)
    {
        // bottom basin: torus rim + stone wall + water
        SetSurface(Materials::STONE, "stone");
        glPushMatrix(); glTranslatef(0, 0.25f, 0); glRotatef(90, 0, 1, 0);
        glutSolidTorus(0.14f, 1.05f, 10, 26); glPopMatrix();
        glPushMatrix(); glTranslatef(0, 0.12f, 0); glRotatef(-90, 1, 0, 0);
        glutSolidCylinder(1.02f, 0.26f, 24, 3); glPopMatrix();
        glPushMatrix(); glTranslatef(0, 0.27f, 0);
        WaterSurface(0.92f, timeSec);
        glPopMatrix();
        // central pedestal
        glPushMatrix(); glTranslatef(0, 0.55f, 0); glRotatef(-90, 1, 0, 0);
        glutSolidCylinder(0.20f, 0.70f, 14, 4); glPopMatrix();
        // middle bowl
        glPushMatrix(); glTranslatef(0, 0.95f, 0); glRotatef(90, 0, 1, 0);
        glutSolidTorus(0.07f, 0.55f, 8, 20); glPopMatrix();
        glPushMatrix(); glTranslatef(0, 0.98f, 0);
        WaterSurface(0.50f, timeSec);
        glPopMatrix();
        // upper pedestal + finial sphere
        glPushMatrix(); glTranslatef(0, 1.15f, 0); glRotatef(-90, 1, 0, 0);
        glutSolidCylinder(0.10f, 0.35f, 10, 3); glPopMatrix();
        SetSurface(Materials::MARBLE, "marble");
        glPushMatrix(); glTranslatef(0, 1.62f, 0); glutSolidSphere(0.14f, 12, 10); glPopMatrix();

        // three arcing water jets (translucent, animated)
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        for (int i = 0; i < 3; ++i)
        {
            float a = i * 2 * 3.14159265f / 3 + timeSec * 0.9f;
            float bob = 0.05f * sinf(timeSec * 6.0f + i * 2.1f);
            glColor4f(0.62f, 0.85f, 1.0f, 0.50f);
            glPushMatrix();
            glTranslatef(0, 1.72f + bob, 0);
            glRotatef(38.0f, cosf(a), 0, sinf(a));
            glTranslatef(0, 0.32f, 0);
            glScalef(1.0f, 2.6f, 1.0f);
            glutSolidSphere(0.055f, 8, 6);
            glPopMatrix();
        }
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }

    // ==================================================================
    //  Phase 8: BASEMENT
    // ==================================================================
    // ==================================================================
    //  Phase 9: SECRET ROOM
    // ==================================================================
    void Globe(float scale)
    {
        // stand
        SetSurface(Materials::DARKWOOD, "darkwood");
        glPushMatrix(); glTranslatef(0, 0.03f * scale, 0); Box(0.34f * scale, 0.06f * scale, 0.34f * scale); glPopMatrix();
        glPushMatrix(); glTranslatef(0, 0.24f * scale, 0); glRotatef(-90, 1, 0, 0);
        glutSolidCylinder(0.035f * scale, 0.42f * scale, 8, 3); glPopMatrix();
        // sphere: deep teal with parchment continents suggestion
        SetSurface(Materials::Tinted(Materials::LEAF, 0.42f, 0.62f, 0.55f), 0);
        glPushMatrix(); glTranslatef(0, 0.52f * scale, 0); glRotatef(23, 0, 0, 1);
        glutSolidSphere(0.30f * scale, 14, 10); glPopMatrix();
        // brass meridian ring
        SetSurface(Materials::GOLD, 0);
        glPushMatrix(); glTranslatef(0, 0.52f * scale, 0); glRotatef(23, 0, 0, 1);
        glutSolidTorus(0.012f * scale, 0.315f * scale, 6, 20); glPopMatrix();
    }

    void DrawingTable()
    {
        // heavy antique drawing table: top + edge trim + 4 carved legs
        SetSurface(Materials::DARKWOOD, "darkwood");
        glPushMatrix(); glTranslatef(0, 0.79f, 0); Box(2.6f, 0.08f, 1.4f); glPopMatrix();
        glPushMatrix(); glTranslatef(0, 0.735f, 0); Box(2.68f, 0.045f, 1.48f); glPopMatrix();
        for (int i = 0; i < 4; ++i)
        {
            float lx = (i % 2) ? 1.12f : -1.12f;
            float lz = (i / 2) ? 0.52f : -0.52f;
            // tapered leg (two boxes) + top bracket
            glPushMatrix(); glTranslatef(lx, 0.36f, lz); Box(0.13f, 0.72f, 0.13f); glPopMatrix();
            glPushMatrix(); glTranslatef(lx, 0.06f, lz); Box(0.18f, 0.12f, 0.18f); glPopMatrix();
            glPushMatrix(); glTranslatef(lx * 0.92f, 0.70f, lz * 0.92f); Box(0.09f, 0.10f, 0.09f); glPopMatrix();
        }
        // drawer band under the top with brass knobs
        glPushMatrix(); glTranslatef(0, 0.66f, 0); Box(2.3f, 0.14f, 1.25f); glPopMatrix();
        SetSurface(Materials::GOLD, 0);
        glPushMatrix(); glTranslatef(0, 0.66f, 0.64f); Box(0.16f, 0.035f, 0.035f); glPopMatrix();
    }

    void SmallCabinet()
    {
        // antique cabinet: plinth, body, two doors (drawn by the caller), top
        SetSurface(Materials::DARKWOOD, "darkwood");
        glPushMatrix(); glTranslatef(0, 0.05f, 0); Box(1.5f, 0.10f, 0.5f); glPopMatrix();
        glPushMatrix(); glTranslatef(0, 0.60f, 0); Box(1.42f, 1.0f, 0.44f); glPopMatrix();
        glPushMatrix(); glTranslatef(0, 1.13f, 0); Box(1.5f, 0.06f, 0.5f); glPopMatrix();
        // cornice
        glPushMatrix(); glTranslatef(0, 1.18f, 0); Box(1.58f, 0.05f, 0.54f); glPopMatrix();
    }

    void Barrel(float r, float h)
    {
        SetSurface(Materials::Tinted(Materials::WOOD, 0.85f, 0.65f, 0.45f), "wood");
        glPushMatrix(); glTranslatef(0, h / 2, 0);
        glPushMatrix(); glRotatef(90, 0, 1, 0);
        glutSolidCylinder(r, h, 14, 4);
        glPopMatrix();
        // iron hoops
        SetSurface(Materials::Tinted(Materials::METAL, 0.45f, 0.38f, 0.28f), 0);
        glPushMatrix(); glTranslatef(0, h * 0.25f, 0); glRotatef(90, 0, 1, 0);
        glutSolidTorus(0.022f, r + 0.012f, 7, 18); glPopMatrix();
        glPushMatrix(); glTranslatef(0, h * 0.75f, 0); glRotatef(90, 0, 1, 0);
        glutSolidTorus(0.022f, r + 0.012f, 7, 18); glPopMatrix();
        glPopMatrix();
    }

    void Bottle(int kind)
    {
        static const float glassCol[4][3] = {
            {0.16f, 0.42f, 0.20f},   // dark green
            {0.42f, 0.26f, 0.12f},   // brown
            {0.45f, 0.14f, 0.16f},   // burgundy
            {0.14f, 0.38f, 0.40f}};  // blue-green
        const float *c = glassCol[kind % 4];
        Material gm = Materials::Tinted(Materials::GLASS, c[0], c[1], c[2]);
        SetSurface(gm, 0);
        glPushMatrix();
        glTranslatef(0, 0.055f, 0); glutSolidCylinder(0.045f, 0.11f, 10, 3);   // base
        glTranslatef(0, 0.11f, 0);  glutSolidCylinder(0.032f, 0.14f, 8, 3);    // body
        glTranslatef(0, 0.14f, 0);  glutSolidCylinder(0.014f, 0.06f, 8, 2);    // neck
        glTranslatef(0, 0.055f, 0); glutSolidSphere(0.016f, 8, 6);             // cork
        // cream paper label
        SetSurface(Materials::Tinted(Materials::BOOK, 1.05f, 1.02f, 0.95f), 0);
        glTranslatef(0, -0.11f, 0);
        Box(0.066f, 0.05f, 0.066f);
        glPopMatrix();
    }

    void StorageShelf(float w, float h)
    {
        const float T = 0.05f, D = 0.42f;
        SetSurface(Materials::DARKWOOD, "darkwood");
        // sides + top + bottom
        Box(T, h, D); glPushMatrix(); glTranslatef(w, 0, 0); Box(T, h, D); glPopMatrix();
        glPushMatrix(); glTranslatef(w / 2, h - T / 2, 0); Box(w, T, D); glPopMatrix();
        glPushMatrix(); glTranslatef(w / 2, T / 2, 0); Box(w, T, D); glPopMatrix();
        // shelves + contents
        const int ROWS = 3;
        float rowH = (h - T * 2) / ROWS;
        for (int s = 0; s < ROWS; ++s)
        {
            float sy = T + rowH * s + T / 2;
            glPushMatrix(); glTranslatef(w / 2, sy, 0); Box(w, T, D); glPopMatrix();
            float bx = 0.06f;
            int i = 0;
            while (bx < w - 0.14f)
            {
                int pick = (s * 7 + i * 5 + (int)(w * 10.0f)) % 10;
                float bh = rowH - T - 0.06f;
                if (pick < 5)          // bottle
                {
                    glPushMatrix();
                    glTranslatef(bx, sy + T / 2, 0.02f);
                    glScalef(1, bh / 0.38f, 1);
                    Bottle(i + s);
                    glPopMatrix();
                    bx += 0.10f;
                }
                else if (pick < 8)     // crate / box
                {
                    float cw = 0.16f + 0.05f * (pick % 3);
                    glPushMatrix();
                    glTranslatef(bx + cw / 2, sy + T / 2 + cw / 2, 0);
                    SetSurface(Materials::Tinted(Materials::WOOD, 0.8f, 0.7f, 0.55f), "wood");
                    Box(cw, cw, D * 0.8f);
                    glPopMatrix();
                    bx += cw + 0.03f;
                }
                else                   // jar
                {
                    glPushMatrix();
                    glTranslatef(bx, sy + T / 2 + 0.06f, 0);
                    SetSurface(Materials::Tinted(Materials::GLASS, 0.5f, 0.55f, 0.45f), 0);
                    glutSolidSphere(0.055f, 9, 7);
                    glPopMatrix();
                    bx += 0.12f;
                }
                ++i;
            }
        }
    }

    void SupportBeam(float length)
    {
        SetSurface(Materials::DARKWOOD, "darkwood");
        Box(length, 0.22f, 0.22f);
    }

    void CandleGroup(float timeSec)
    {
        // brass tray + three candles of different heights
        SetSurface(Materials::GOLD, 0);
        glPushMatrix(); glTranslatef(0, 0.012f, 0); glRotatef(90, 0, 1, 0);
        glutSolidCylinder(0.13f, 0.025f, 12, 2); glPopMatrix();
        glPushMatrix(); glTranslatef(-0.055f, 0.025f,  0.03f);  Candle(0.14f, true, timeSec); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.045f, 0.025f, -0.045f); Candle(0.18f, true, timeSec + 1.3f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.01f,  0.025f,  0.06f);  Candle(0.10f, true, timeSec + 2.6f); glPopMatrix();
    }

    // ==================================================================
    //  Recurring mansion crest (story symbol)
    // ==================================================================
    void Crest(float size, float r, float g, float b, float alpha)
    {
        const float s = size;
        glDisable(GL_LIGHTING);
        glColor4f(r, g, b, alpha);
        // outer circle (flat fan)
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(0, 0, 0);
        for (int i = 0; i <= 20; ++i)
        {
            float a = i * 2 * 3.14159265f / 20;
            glVertex3f(cosf(a) * s * 0.55f, sinf(a) * s * 0.55f, 0);
        }
        glEnd();
        // triangle inside
        glBegin(GL_TRIANGLES);
        glVertex3f(0, s * 0.30f, 0.002f);
        glVertex3f(-s * 0.26f, -s * 0.18f, 0.002f);
        glVertex3f( s * 0.26f, -s * 0.18f, 0.002f);
        glEnd();
        // moon crescent at the top
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(1.0f - r, 1.0f - g, 1.0f - b, alpha);
        for (int i = 0; i <= 12; ++i)
        {
            float a = 3.14159265f * 0.5f + i * 3.14159265f / 12;
            glVertex3f(cosf(a) * s * 0.13f, s * 0.40f + sinf(a) * s * 0.13f, 0.004f);
        }
        glEnd();
        glEnable(GL_LIGHTING);
    }

    void CrestDial(float size, float angleDeg, bool lit)
    {
        // recessed stone disc + rotating engraved ring + fixed metal rim
        glPushMatrix();
        glTranslatef(0, 0, 0.012f);
        SetSurface(Materials::STONE, "stone");
        glRotatef(90, 0, 1, 0);
        glutSolidCylinder(size * 0.62f, 0.024f, 20, 2);
        glPopMatrix();
        glPushMatrix();
        glRotatef(angleDeg, 0, 0, 1);
        glTranslatef(0, 0, 0.032f);
        if (lit) Crest(size, 1.0f, 0.85f, 0.35f, 0.95f);
        else     Crest(size, 0.16f, 0.14f, 0.12f, 1.0f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(0, 0, 0.034f);
        SetSurface(Materials::Tinted(Materials::METAL, 0.5f, 0.4f, 0.25f), 0);
        glRotatef(90, 0, 1, 0);
        glutSolidTorus(0.018f, size * 0.60f, 8, 22);
        glPopMatrix();
    }
