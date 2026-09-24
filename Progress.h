#ifndef PROGRESS_H
#define PROGRESS_H

// ============================================================
//  THE LAST NIGHT - Progress.h
//  Story/item progression flags (Phase 5+). A tiny, explicit
//  state table the whole game can query; later phases can move
//  this into a full inventory system without touching callers.
// ============================================================

namespace Progress
{
    enum Flag
    {
        SAFE_OPEN        = 0,   // library safe has been unlocked
        HAS_MANSION_KEY  = 1,   // mansion key taken from the safe
        HAS_BRASS_KEY    = 2,   // old brass key (hidden library compartment)
        STUDY_UNLOCKED   = 3,   // study door has been unlocked
        MAP_CLOSET_OPEN  = 4,   // study map closet opened
        NOTE_TAKEN       = 5,   // hidden drawer note taken
        // --- Phase 7: the garden ---
        GARDEN_DOOR_OPEN = 6,   // lobby east door (the garden entrance)
        PLAQUE_READ      = 7,   // fountain plaque examined
        STATUE_FOUND     = 8,   // statue's hidden plate discovered
        // --- Phase 8: the basement ---
        BASEMENT_FOUND   = 9,   // cellar door discovered / openable
        DIAL1_SOLVED     = 10,  // crest dial I aligned
        DIAL2_SOLVED     = 11,  // crest dial II aligned
        DIAL3_SOLVED     = 12,  // crest dial III aligned
        PANEL_OPEN       = 13,  // hidden stone panel slid open
        PASSAGE_NOTE     = 14,  // passage note read
        // --- Phase 9/10: the master (front-door) key ---
        HAS_FRONT_KEY    = 27,  // antique master key taken from the cabinet
        // --- Phase 9: the secret room ---
        SECRETDOOR_OPEN  = 15,  // the passage's secret door stands open
        SR_ENTERED       = 16,  // player has been inside the secret room
        CREST1_ALIGNED   = 17,  // wall crest ring I aligned
        CREST2_ALIGNED   = 18,  // wall crest ring II aligned
        CREST3_ALIGNED   = 19,  // wall crest ring III aligned
        CREST_SOLVED     = 20,  // mechanism fired: cabinet unlocked
        CABINET_OPEN     = 21,  // cabinet door standing open
        HAS_MASTER_KEY   = 22,  // antique master key taken from the cabinet
        BLUEPRINT_READ   = 23,  // mansion blueprint examined
        FINAL_LETTER     = 24,  // the owner's final letter read
        // --- Phase 10: the ending ---
        FRONT_UNLOCKED   = 25,  // brass symbol mechanism opened the front door
        ESCAPED          = 26,  // player crossed the threshold outside
        FLAG_COUNT
    };

    void Reset();                       // new game
    void Set(Flag f, bool value);
    bool Get(Flag f);
}

#endif // PROGRESS_H
