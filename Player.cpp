// ============================================================
//  THE LAST NIGHT - Player.cpp
//  Input mapping, mouse-look, delta-timed movement, flashlight.
// ============================================================
#include "Player.h"
#include "Collision.h"
#include <GL/freeglut.h>
#include <cmath>
#include <cstdio>

// --- configuration -------------------------------------------------------
const float Player::WALKING_SPEED     = 3.0f;
const float Player::RUNNING_SPEED     = 5.2f;
const float Player::PLAYER_HEIGHT     = 1.75f;
const float Player::CAMERA_HEIGHT     = 1.65f;
const float Player::COLLISION_RADIUS  = 0.35f;
const float Player::GROUND_HEIGHT     = 0.0f;

namespace
{
    const float MOUSE_SENSITIVITY = 0.14f;  // degrees per pixel
    const float PITCH_LIMIT       = 89.0f;  // cannot flip upside down

    Player::GroundFn g_groundFn = 0;        // world floor sampler (stairs)
    char g_flashHud[24] = "";               // one-shot flashlight HUD line
}

Player::Player()
    : m_x(0), m_y(GROUND_HEIGHT), m_z(6),
      m_yaw(0), m_pitch(0),   // facing the window/fireplace from the entrance
      m_forward(false), m_back(false), m_left(false), m_right(false),    m_running(false), m_turnLeft(false), m_turnRight(false),
    m_flashlightOn(false)
{
    g_flashHud[0] = '\0';
}

void Player::handleKeyboard(unsigned char key, bool down)
{
    switch (key)
    {
        case 'w': case 'W': m_forward = down; break;
        case 's': case 'S': m_back    = down; break;
        case 'a': case 'A': m_left    = down; break;
        case 'd': case 'D': m_right   = down; break;
        case 'f': case 'F': if (down) toggleFlashlight(); break;
        case 'r': case 'R': m_running = down; break;
        default: break;
    }
}

void Player::handleSpecial(int key, bool down)
{
    // Arrow keys mirror mouse-look as a fallback (some laptop touchpads
    // make precise mouse control awkward during development).
    if (key == GLUT_KEY_LEFT)  { m_turnLeft  = down; }
    if (key == GLUT_KEY_RIGHT) { m_turnRight = down; }
#ifdef GLUT_KEY_SHIFT_L
    if (key == GLUT_KEY_SHIFT_L || key == GLUT_KEY_SHIFT_R)
        m_running = down;
#endif
}

void Player::toggleFlashlight()
{
    m_flashlightOn = !m_flashlightOn;
    g_flashHud[0] = '\0';
    if (m_flashlightOn)
        std::snprintf(g_flashHud, sizeof(g_flashHud), "FLASHLIGHT ON");
    else
        std::snprintf(g_flashHud, sizeof(g_flashHud), "FLASHLIGHT OFF");
}

const char *Player::FlashHud()              { return g_flashHud; }
void Player::ClearFlashHud()                { g_flashHud[0] = '\0'; }

void Player::SetYaw(float yaw)              { m_yaw = yaw; }

void Player::applyMouseDelta(int dx, int dy)
{
    m_yaw   += dx * MOUSE_SENSITIVITY;
    m_pitch -= dy * MOUSE_SENSITIVITY;
    if (m_pitch >  PITCH_LIMIT) m_pitch =  PITCH_LIMIT;
    if (m_pitch < -PITCH_LIMIT) m_pitch = -PITCH_LIMIT;
}

void Player::SetGroundSampler(GroundFn fn) { g_groundFn = fn; }

void Player::SetPos(float x, float z)
{
    m_x = x;
    m_z = z;
    // Re-sample immediately so scripted/spawned positions land on the
    // correct floor (stairs, basement, secret room) instead of y=0.
    m_y = g_groundFn ? g_groundFn(m_x, m_z) : GROUND_HEIGHT;
}

void Player::applyMovement(float dx, float dz)
{
    // Collision-ready: route every move through the Collision system so
    // walls/furniture can block the player (wall sliding on both axes).
    Collision::MoveCircle(m_x, m_z, dx, dz, COLLISION_RADIUS);

    // Ground height: the registered sampler returns the floor y for the
    // resolved position (flat 0 unless the mansion has stairs/slopes).
    m_y = g_groundFn ? g_groundFn(m_x, m_z) : GROUND_HEIGHT;
}

void Player::update(float dt)
{
    float speed = m_running ? RUNNING_SPEED : WALKING_SPEED;

    // View direction on the ground plane (yaw only).
    float fx =  sinf(m_yaw * 3.14159265f / 180.0f);
    float fz = -cosf(m_yaw * 3.14159265f / 180.0f);
    float rx =  cosf(m_yaw * 3.14159265f / 180.0f);
    float rz =  sinf(m_yaw * 3.14159265f / 180.0f);

    float dx = 0.0f, dz = 0.0f;
    if (m_forward) { dx += fx; dz += fz; }
    if (m_back)    { dx -= fx; dz -= fz; }
    if (m_left)    { dx -= rx; dz -= rz; }
    if (m_right)   { dx += rx; dz += rz; }        // Arrow-key look fallback at mouse-free sensitivity
        if (m_turnLeft)  m_yaw += 90.0f * dt;
        if (m_turnRight) m_yaw -= 90.0f * dt;

        float len = sqrtf(dx * dx + dz * dz);
        if (len > 0.0001f)
            applyMovement(dx / len * speed * dt, dz / len * speed * dt);
}

void Player::updateCamera(Camera &cam) const
{
    cam.x     = m_x;
    cam.y     = eyeY();
    cam.z     = m_z;
    cam.yaw   = m_yaw;
    cam.pitch = m_pitch;
}
