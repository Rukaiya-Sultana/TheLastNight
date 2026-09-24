#ifndef CAMERA_H
#define CAMERA_H

// ============================================================
//  THE LAST NIGHT - Camera.h
//  First-person camera: state + view transformation helpers.
//  The camera is attached to the Player (position = player's eyes).
// ============================================================

class Camera
{
public:
    Camera();

    float x, y, z;       // eye position (world space)
    float yaw, pitch;    // degrees: yaw = turn left/right, pitch = look up/down

    void setView() const;          // applies viewing coordinate transformation
    void getForward(float &fx, float &fz) const;
    void getRight(float &rx, float &rz) const;
};

#endif // CAMERA_H
