// ============================================================
//  THE LAST NIGHT - Camera.cpp
// ============================================================
#include "Camera.h"
#include <GL/freeglut.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Camera::Camera()
    : x(0), y(1.65f), z(8), yaw(0), pitch(0)
{
}

// Viewing coordinate transformation: eye point, look point, up vector.
// The eye stays at the player's eye height; the look point carries the
// pitch so looking up/down tilts the view around the player.
void Camera::setView() const
{
    float fx, fz;
    getForward(fx, fz);

    float lookDist = 4.0f; // arbitrary: look-at a point ahead of the eyes
    float lx = x + fx * lookDist;
    float ly = y + lookDist * sinf(pitch * M_PI / 180.0f);
    float lz = z + fz * lookDist;

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(x, y, z,          // eye
              lx, ly, lz,       // look-at point
              0.0f, 1.0f, 0.0f);// up vector
}

void Camera::getForward(float &fx, float &fz) const
{
    fx = sinf(yaw * M_PI / 180.0f);
    fz = -cosf(yaw * M_PI / 180.0f);
}

void Camera::getRight(float &rx, float &rz) const
{
    rx = cosf(yaw * M_PI / 180.0f);
    rz = sinf(yaw * M_PI / 180.0f);
}
