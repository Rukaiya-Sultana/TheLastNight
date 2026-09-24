#ifndef INTERACTION_H
#define INTERACTION_H

// ============================================================
//  THE LAST NIGHT - Interaction.h
//  Focus-based interaction registry (Phase 4).
//  Scenes register interactable points (world position + radius);
//  each frame the active item = closest registered item that is
//  both within range and roughly in front of the camera.
//  Game.cpp polls the active item to show the prompt and to
//  route the E key.
// ============================================================

namespace Interaction
{
    // Well-known interactable ids (scenes may add more).
    const int ID_NONE      = -1;
    const int ID_LETTER    = 0;   // old letter on the hall desk
    const int ID_PORTRAIT  = 1;   // crooked portrait of the owner
    const int ID_CLOCK     = 2;   // stopped grandfather clock
    const int ID_HALLDOOR  = 3;   // west hall door (open action)
    const int ID_CLOSEDOOR = 4;   // west hall door (close action)
    const int ID_LOBBYDOOR = 5;   // east lobby door (locked)
    const int ID_LOBBYLETTER = 6; // faded letter on the lobby coffee table

    // --- Phase 5: the Grand Library ---
    const int ID_SECRETBOOK = 7;  // pulled-out red book (the journal)
    const int ID_STOPCLOCK  = 8;  // the library's stopped clock (clue 12)
    const int ID_FOUNDER    = 9;  // founder painting + brass plate (clue 7)
    const int ID_SAFE       = 10; // the 3-digit safe
    const int ID_BRASSKEY   = 11; // brass key on the shelf (post-safe)
    const int ID_STUDYDOOR  = 12; // study door (locked/unlock/open)
    const int ID_STUDYCLOSE = 13; // study door (close action)

    // --- Phase 6: the Study ---
    const int ID_PHOTO      = 14; // old photograph on the desk
    const int ID_JOURNAL    = 15; // the study journal
    const int ID_DRAWER     = 16; // locked desk drawer
    const int ID_DRAWEROPEN = 17; // opened drawer (take note)
    const int ID_NOTE       = 18; // hidden note inside the drawer
    const int ID_CLOSET     = 19; // map closet (opens with the mansion key)
    const int ID_MANSIONKEY = 20; // mansion key inside the closet
    const int ID_STUDYFIRE  = 21; // study fireplace (flavor)

    // --- Phase 7: the Garden ---
    const int ID_GARDENDOOR = 22; // garden door (unlock/open - close action)
    const int ID_GARDENCLOSE= 23;
    const int ID_FOUNTAIN   = 24; // the fountain (flavor + plaque direction)
    const int ID_FOUNTPLAQ  = 25; // fountain plaque ("The garden remembers")
    const int ID_STATUE     = 26; // garden statue (hidden plate below)
    const int ID_GPLATE     = 27; // hidden plate under the statue (cellar hint)
    const int ID_BENCH      = 28; // garden bench (flavor)
    const int ID_MAPTABLE   = 29; // map table in the study corner

    // --- Phase 8: the Basement ---
    const int ID_CELLARDOOR = 30; // cellar door in the garden annex
    const int ID_CELLARCLOSE= 31;
    const int ID_STAIRDOWN  = 32; // stairs down (flavor/description)
    const int ID_SYMBOL     = 33; // big carved symbol on the wall
    const int ID_DIAL1      = 34; // crest dial I (rotate)
    const int ID_DIAL2      = 35; // crest dial II
    const int ID_DIAL3      = 36; // crest dial III
    const int ID_CANDLES    = 37; // candle group (flavor)
    const int ID_BARRELS    = 38; // barrels (flavor)
    const int ID_WINE       = 39; // wine racks (flavor)
    const int ID_PASNOTE    = 40; // passage note ("It was waiting.")
    const int ID_PASPHOTO   = 41; // second photograph in the passage
    const int ID_SECRETDOOR = 42; // locked secret door (opens in phase 9)

    // --- Phase 9: the Secret Room ---
    const int ID_SRDOOR     = 43; // secret door (inside face: open/close)
    const int ID_SRCLOSE    = 44;
    const int ID_BLUEPRINT  = 45; // mansion blueprint on the table
    const int ID_SMAP       = 46; // old grounds map (second entrance hint)
    const int ID_SPHOTO     = 47; // the removed-person photograph
    const int ID_SJOURNAL   = 48; // the owner's journal (why it was hidden)
    const int ID_CREST      = 49; // the great wall crest (description)
    const int ID_RING1      = 50; // crest ring I (rotate)
    const int ID_RING2      = 51;
    const int ID_RING3      = 52;
    const int ID_CABINET    = 53; // cabinet (locked / open)
    const int ID_CABKEY     = 54; // master key inside the cabinet (pickup)
    const int ID_SLETTER    = 55; // the final letter
    const int ID_GLOBE      = 56; // the globe (flavor)

    // --- Phase 10: the ending ---
    const int ID_FRONTDOOR  = 57; // front door (lobby north exterior): locked + puzzle
    const int ID_SYMBOL1    = 58; // front-door brass symbol I (rotate)
    const int ID_SYMBOL2    = 59;
    const int ID_SYMBOL3    = 60;
    const int ID_FAMPORTRAIT= 61; // final family portrait on the route
    const int ID_MEDPLAQUE  = 62; // brass plaque beside the front-door medallions

    // Reliable cellar traversal points (Phase 8).
    const int ID_ENTERBASEMENT = 63;
    const int ID_RETURNGARDEN  = 64;
    struct Item
    {
        int   id;
        float x, y, z;      // world position of the interaction point
        float radius;       // activation distance
        char  label[40];    // prompt text, e.g. "Examine Letter"
        bool  enabled;
        bool  consumed;     // already used at least once
        bool  defEnabled;   // default state for RearmAll (new game)
    };

    const int MAX_ITEMS = 64;

    void Reset();                                   // scene (re)load
    void Add(int id, float x, float y, float z,
             float radius, const char *label);

    // New game: every interaction returns to its default state.
    // (Load happens once at boot; Reset would empty the registry.)
    void RearmAll();

    // Boot: snapshot each item's enabled state AFTER scene Loads have
    // applied their initial SetEnabled calls. RearmAll restores this.
    void SealDefaults();

    void SetEnabled(int id, bool enabled);
    bool IsConsumed(int id);
    void MarkConsumed(int id);

    // Pick the active item for this frame from the player camera.
    void Update(float px, float py, float pz, float fx, float fz);

    int  ActiveId();                                // ID_NONE when nothing
    const char *ActiveLabel();                      // "" when nothing

    // E was pressed: returns the active id (ID_NONE if none).
    // Does not mutate item state - the caller decides the response.
    int  Activate();
}

#endif // INTERACTION_H
