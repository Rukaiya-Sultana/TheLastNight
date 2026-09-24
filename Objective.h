#ifndef OBJECTIVE_H
#define OBJECTIVE_H

namespace Objective
{
    void Reset();
    void NotifyInteraction(int id);
    void Update(float dt, float x, float y, float z, float yawDegrees);
}

#endif
