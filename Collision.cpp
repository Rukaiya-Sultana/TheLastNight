// ============================================================
//  THE LAST NIGHT - Collision.cpp
//  XZ-plane circle-vs-AABB overlap. The player slides along walls
//  because movement is resolved per axis (see Player::applyMovement
//  and Collision::MoveCircle).
// ============================================================
#include "Collision.h"

namespace Collision
{
    Box   g_boxes[MAX_BOXES];
    int   g_count = 0;

    // Animated blockers (swinging doors). Each claimed slot aliases the
    // last box added by a room's Load(); disabled slots block nothing.
    struct DynSlot { int index; bool valid; };
    // The mansion currently contains dynamic blockers for the lobby front
    // doors, garden entrance, hall door, study door, cellar door, hidden
    // panel and secret-room door.  Keeping this at four silently made every
    // slot claimed after the Grand Hall fail, leaving several visible doors
    // without collision.  Leave headroom for future rooms as well.
    const int MAX_DYN = 16;
    DynSlot g_dyn[MAX_DYN];
    int     g_dynCount = 0;

    void Reset()
    {
        g_count = 0;
        g_dynCount = 0;
    }

    void AddBox(float minX, float minZ, float maxX, float maxZ)
    {
        if (g_count >= MAX_BOXES) return;
        g_boxes[g_count].minX = minX;
        g_boxes[g_count].minZ = minZ;
        g_boxes[g_count].maxX = maxX;
        g_boxes[g_count].maxZ = maxZ;
        ++g_count;
    }

    int ClaimDynamic()
    {
        if (g_count == 0 || g_dynCount >= MAX_DYN) return -1;
        g_dyn[g_dynCount].index = g_count - 1;
        g_dyn[g_dynCount].valid = false;
        return g_dynCount++;
    }

    void SetDynamicBox(int slot, bool enabled,
                       float minX, float minZ, float maxX, float maxZ)
    {
        if (slot < 0 || slot >= g_dynCount) return;
        int bi = g_dyn[slot].index;
        if (bi >= g_count) return;

        g_dyn[slot].valid = enabled;
        if (enabled)
        {
            g_boxes[bi].minX = minX;
            g_boxes[bi].minZ = minZ;
            g_boxes[bi].maxX = maxX;
            g_boxes[bi].maxZ = maxZ;
        }
    }

    namespace
    {
        bool DynBlocked(int i)
        {
            for (int s = 0; s < g_dynCount; ++s)
                if (g_dyn[s].index == i && !g_dyn[s].valid)
                    return true;
            return false;
        }

        // True if the circle (cx,cz) radius r overlaps the box (expanded test).
        bool Overlaps(const Box &b, float cx, float cz, float r)
        {
            return cx + r > b.minX && cx - r < b.maxX &&
                   cz + r > b.minZ && cz - r < b.maxZ;
        }
    }

    void MoveCircle(float &x, float &z, float dx, float dz, float radius)
    {
        // Axis-separated resolution gives natural sliding along walls:
        // if the X move is blocked, the Z move can still succeed.
        float nx = x + dx;
        bool  xBlocked = false;
        for (int i = 0; i < g_count; ++i)
        {
            if (DynBlocked(i)) continue;
            if (Overlaps(g_boxes[i], nx, z, radius)) { xBlocked = true; break; }
        }
        if (!xBlocked) x = nx;

        float nz = z + dz;
        bool  zBlocked = false;
        for (int i = 0; i < g_count; ++i)
        {
            if (DynBlocked(i)) continue;
            if (Overlaps(g_boxes[i], x, nz, radius)) { zBlocked = true; break; }
        }
        if (!zBlocked) z = nz;
    }
}
