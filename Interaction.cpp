// ============================================================
//  THE LAST NIGHT - Interaction.cpp
//  Distance + view-direction focus picking (Phase 4).
//  No raycasting: the active item is the closest interactable
//  that is within its activation radius and within the player's
//  forward cone. Simple, reliable, frame-rate independent.
// ============================================================
#include "Interaction.h"
#include <cmath>
#include <cstring>

namespace
{
    Interaction::Item g_items[Interaction::MAX_ITEMS];
    int  g_count   = 0;
    int  g_active  = Interaction::ID_NONE;
    char g_label[64] = "";

    const float VIEW_DOT = 0.55f;   // ~56 degrees half-angle cone
}

namespace Interaction
{
    void Reset()
    {
        g_count = 0;
        g_active = ID_NONE;
        g_label[0] = '\0';
    }

    void Add(int id, float x, float y, float z, float radius, const char *label)
    {
        if (g_count >= MAX_ITEMS) return;
        Item &it = g_items[g_count++];
        it.id = id;
        it.x = x; it.y = y; it.z = z;
        it.radius = radius;
        it.enabled = true;
        it.defEnabled = true;   // everything starts interactable
        it.consumed = false;
        std::strncpy(it.label, label, sizeof(it.label) - 1);
        it.label[sizeof(it.label) - 1] = '\0';
    }

    void SetEnabled(int id, bool enabled)
    {
        for (int i = 0; i < g_count; ++i)
            if (g_items[i].id == id) g_items[i].enabled = enabled;
    }    void RearmAll()
    {
        for (int i = 0; i < g_count; ++i)
        {
            g_items[i].consumed = false;
            g_items[i].enabled = g_items[i].defEnabled;
        }
    }

    void SealDefaults()
    { 
        for (int i = 0; i < g_count; ++i)
            g_items[i].defEnabled = g_items[i].enabled;
    }

    bool IsConsumed(int id)
    {
        for (int i = 0; i < g_count; ++i)
            if (g_items[i].id == id) return g_items[i].consumed;
        return false;
    }

    void MarkConsumed(int id)
    {
        for (int i = 0; i < g_count; ++i)
            if (g_items[i].id == id) g_items[i].consumed = true;
    }

    void Update(float px, float py, float pz, float fx, float fz)
    {
        (void)py; // height unused for now; items carry their own y
        g_active = ID_NONE;
        g_label[0] = '\0';

        float best = 1e9f;
        for (int i = 0; i < g_count; ++i)
        {
            const Item &it = g_items[i];
            if (!it.enabled) continue;

            float dx = it.x - px;
            float dz = it.z - pz;
            float dist = sqrtf(dx*dx + dz*dz);
            if (dist > it.radius) continue;

            // in front of the camera? (XZ cone test)
            if (dist > 0.25f) // very close items are always focusable
            {
                float len = sqrtf(fx*fx + fz*fz);
                if (len < 0.001f) continue;
                float dot = (dx*fx + dz*fz) / (dist * len);
                if (dot < VIEW_DOT) continue;
            }

            if (dist < best)
            {
                best = dist;
                g_active = it.id;
                std::strncpy(g_label, it.label, sizeof(g_label) - 1);
                g_label[sizeof(g_label) - 1] = '\0';
            }
        }
    }

    int ActiveId() { return g_active; }

    const char *ActiveLabel() { return g_label; }

    int Activate()
    {
        return g_active;
    }
}
