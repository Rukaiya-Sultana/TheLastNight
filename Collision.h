#ifndef COLLISION_H
#define COLLISION_H

// ============================================================
//  THE LAST NIGHT - Collision.h
//  Simple, reliable AABB colliders in the XZ plane.
//  Animated blockers (swinging doors) are "dynamic slots":
//  ClaimDynamic() turns the last-added box into a named slot and
//  SetDynamicBox() re-implaces it every frame while enabled.
// ============================================================

namespace Collision
{
    struct Box
    {
        float minX, minZ;
        float maxX, maxZ;
    };

    const int MAX_BOXES = 192;

    void Reset();                       // called on scene load
    void AddBox(float minX, float minZ, float maxX, float maxZ);

    // Turn the LAST added box into an animated blocker and return its
    // slot index. Dynamic boxes start disabled (no collision) until the
    // scene sets them.
    int  ClaimDynamic();

    // Re-implace a claimed dynamic slot. A disabled slot blocks nothing
    // (open door); enabled boxes are solid for the frame.
    void SetDynamicBox(int slot, bool enabled,
                       float minX, float minZ, float maxX, float maxZ);

    // If moving the circle (x,z) radius r to the new position would overlap
    // any box, that axis component of the move is cancelled (wall sliding).
    void MoveCircle(float &x, float &z, float dx, float dz, float radius);
}

#endif // COLLISION_H
