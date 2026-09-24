// ============================================================
//  THE LAST NIGHT - Progress.cpp
//  Story/item progression flags. Kept trivially simple so later
//  phases can serialize or migrate it without churn.
// ============================================================
#include "Progress.h"

namespace Progress
{
    bool g_flags[FLAG_COUNT];

    void Reset()
    {
        for (int i = 0; i < FLAG_COUNT; ++i)
            g_flags[i] = false;
    }

    void Set(Flag f, bool value)
    {
        if (f >= 0 && f < FLAG_COUNT) g_flags[f] = value;
    }

    bool Get(Flag f)
    {
        return (f >= 0 && f < FLAG_COUNT) ? g_flags[f] : false;
    }
}
