#include "Objective.h"
#include "Interaction.h"
#include "Progress.h"
#include "UI.h"
#include <cmath>

namespace
{
    enum GoalType { GOAL_INTERACTION, GOAL_FLAG, GOAL_REACH };
    enum ReachType { REACH_LIBRARY, REACH_GARDEN, REACH_BASEMENT,
                     REACH_SECRET, REACH_PASSAGE, REACH_STAIRS, REACH_LOBBY };

    struct Goal
    {
        const char *label;
        float x, z;
        GoalType type;
        int value;
    };

    // Coordinates use the game's shared X/Z world map. The first marker is
    // the west door in the lobby's north wall (left of the room center).
    const Goal GOALS[] = {
        {"Read the faded letter",             -3.0f,  0.0f, GOAL_INTERACTION, Interaction::ID_LOBBYLETTER},
        {"Open the WEST HALL DOOR",            -4.6f,  7.25f, GOAL_INTERACTION, Interaction::ID_HALLDOOR},
        {"Read the letter on the hall desk",   -6.4f, 13.0f, GOAL_INTERACTION, Interaction::ID_LETTER},
        {"Examine the hall's stopped clock",    5.8f, 18.6f, GOAL_INTERACTION, Interaction::ID_CLOCK},
        {"Enter the Grand Library",            9.0f, 19.5f, GOAL_REACH, REACH_LIBRARY},
        {"Find the pulled red journal",        17.8f, 21.4f, GOAL_INTERACTION, Interaction::ID_SECRETBOOK},
        {"Read the library clock (midnight)",   9.3f, 22.6f, GOAL_INTERACTION, Interaction::ID_STOPCLOCK},
        {"Read the founder's brass plate",     10.0f, 13.5f, GOAL_INTERACTION, Interaction::ID_FOUNDER},
        {"Unlock the safe with code 127",      19.0f, 22.5f, GOAL_FLAG, Progress::SAFE_OPEN},
        {"Take the mansion key from the safe", 19.0f, 22.5f, GOAL_FLAG, Progress::HAS_MANSION_KEY},
        {"Enter the garden",                   10.0f,  0.0f, GOAL_REACH, REACH_GARDEN},
        {"Read the fountain plaque",           16.5f,  0.0f, GOAL_INTERACTION, Interaction::ID_FOUNTPLAQ},
        {"Inspect the garden statue",          22.6f,  4.6f, GOAL_FLAG, Progress::STATUE_FOUND},
        {"Read the plate under the statue",    22.6f,  5.2f, GOAL_INTERACTION, Interaction::ID_GPLATE},
        {"Open the cellar door",               26.6f,  5.8f, GOAL_INTERACTION, Interaction::ID_CELLARDOOR},
        {"Descend the stairs to the basement", 26.6f,  2.2f, GOAL_REACH, REACH_BASEMENT},
        {"Align the three basement dials",     26.2f, -1.8f, GOAL_FLAG, Progress::PANEL_OPEN},
        {"Release the secret-room door",       30.4f, -0.5f, GOAL_FLAG, Progress::SECRETDOOR_OPEN},
        {"Enter the secret room",              32.0f, -0.3f, GOAL_REACH, REACH_SECRET},
        {"Align the three crest rings",        38.8f, -1.1f, GOAL_FLAG, Progress::CREST_SOLVED},
        {"Open the crest cabinet",              38.3f,  1.15f, GOAL_FLAG, Progress::CABINET_OPEN},
        {"Take the antique master key",        38.3f,  1.15f, GOAL_FLAG, Progress::HAS_FRONT_KEY},
        {"Return through the secret doorway",  31.1f, -0.3f, GOAL_REACH, REACH_PASSAGE},
        {"Climb the garden stairs",            26.6f,  2.2f, GOAL_REACH, REACH_STAIRS},
        {"Return to the mansion lobby",         8.0f,  0.0f, GOAL_REACH, REACH_LOBBY},
        {"Read the plaque beside the front door",-5.25f, -6.8f, GOAL_INTERACTION, Interaction::ID_MEDPLAQUE},
        {"Set moon 2, star 1, sun 3",          -3.65f, -6.8f, GOAL_FLAG, Progress::FRONT_UNLOCKED},
        {"Open the front door",                 0.0f, -6.7f, GOAL_INTERACTION, Interaction::ID_FRONTDOOR},
        {"Walk outside to finish the escape",   0.0f, -9.2f, GOAL_FLAG, Progress::ESCAPED}
    };
    const int GOAL_COUNT = sizeof(GOALS) / sizeof(GOALS[0]);
    int g_current = 0;
    int g_pendingInteraction = Interaction::ID_NONE;
    float g_noticeClock = 0.0f;

    bool Reached(int which, float x, float y, float z)
    {
        switch (which)
        {
            case REACH_LIBRARY:  return x > 8.35f && x < 21.0f && z > 13.0f && z < 23.5f;
            case REACH_GARDEN:   return x > 8.5f && x < 29.0f && z > -9.2f && z < 9.2f && y > -0.6f;
            case REACH_BASEMENT: return y < -2.7f;
            case REACH_SECRET:   return x > 32.0f && y < -2.7f;
            case REACH_PASSAGE:  return x < 31.0f && x > 27.2f && z > -1.6f && z < 0.6f;
            // Stair run: x=26.0..27.2, z=0.8..5.6. Requiring both axes
            // prevents a false completion elsewhere along the garden edge.
            case REACH_STAIRS:   return y > -0.1f && x > 25.7f && x < 27.5f &&
                                         z > 0.5f && z < 5.9f;
            case REACH_LOBBY:    return x < 7.9f && x > -8.2f && z > -7.2f && z < 7.2f;
            default:             return false;
        }
    }

    bool CompleteCurrent(float x, float y, float z)
    {
        if (g_current >= GOAL_COUNT) return false;
        const Goal &g = GOALS[g_current];
        if (g.type == GOAL_INTERACTION && g_pendingInteraction == g.value)
            return true;
        if (g.type == GOAL_FLAG && Progress::Get((Progress::Flag)g.value))
            return true;
        if (g.type == GOAL_REACH && Reached(g.value, x, y, z))
            return true;
        return false;
    }
}

namespace Objective
{
    void Reset()
    {
        g_current = 0;
        g_pendingInteraction = Interaction::ID_NONE;
        g_noticeClock = 0.0f;
        UI::ClearObjective();
    }

    void NotifyInteraction(int id)
    {
        g_pendingInteraction = id;
    }

    void Update(float dt, float x, float y, float z, float yawDegrees)
    {
        if (g_noticeClock > 0.0f)
        {
            g_noticeClock -= dt;
            if (g_noticeClock <= 0.0f) UI::ClearTaskComplete();
        }

        // At most one transition per tick keeps each completion visible,
        // including stages completed together by a single puzzle action.
        if (CompleteCurrent(x, y, z))
        {
            UI::ShowTaskComplete(GOALS[g_current].label);
            g_noticeClock = 2.4f;
            ++g_current;
        }
        g_pendingInteraction = Interaction::ID_NONE;

        if (g_current >= GOAL_COUNT)
        {
            UI::ClearObjective();
            return;
        }

        const Goal &g = GOALS[g_current];
        const float dx = g.x - x;
        const float dz = g.z - z;
        const float distance = sqrtf(dx*dx + dz*dz);
        float targetYaw = atan2f(dx, -dz) * 57.2957795f;
        float bearing = targetYaw - yawDegrees;
        while (bearing > 180.0f) bearing -= 360.0f;
        while (bearing < -180.0f) bearing += 360.0f;
        UI::SetObjective(g.label, bearing, distance);
    }
}
