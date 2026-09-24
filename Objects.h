#ifndef OBJECTS_H
#define OBJECTS_H

// ============================================================
//  THE LAST NIGHT - Objects.h
//  Reusable object builders. Every function draws at the CURRENT
//  origin in a LOCAL coordinate system - callers position/rotate/
//  scale objects with glTranslate/glRotate/glScale + push/pop.
//  Ground plane is y = 0; objects extend upward from it.
// ============================================================

#include "Lighting.h"

namespace Objects
{
    // --- low-level helpers -------------------------------------------------
    void QuadXY();                                  // unit XY quad, +z normal
    void Box(float sx, float sy, float sz);         // centered box, scaled
    void TexturedBox(const Material &m, const char *tex,
                     float sx, float sy, float sz);

    // --- furniture ---------------------------------------------------------
    void Sofa(float w, const Material &fabric);     // seat/back/arms/legs/cushions
    void Armchair(const Material &fabric);
    void CoffeeTable();                             // + decorative items on top
    void SideTable();
    void GrandfatherClock(float timeSec);           // hands + pendulum animate
    void StoppedClock();                            // frozen at midnight (library clue)
    void Bookshelf(float w, float h);               // colorful procedural books
    void Door(float w, float h);                    // hinged at local x=0 edge
    void SideChair(const Material &fabric);         // dining/reading chair
    void DeskLamp(float timeSec);                   // banker lamp: base+stem+shade
    void BrassKey();                                // small key model (pickup)

    // --- fireplace ---------------------------------------------------------
    // Draws surround + logs + animated flame; fire intensity 0..1.
    void Fireplace(bool lit, float timeSec);
    float FireFlicker(float timeSec);               // 0.75..1.15 for light animation

    // --- lighting fixtures -------------------------------------------------
    void Chandelier(float timeSec);                 // slow continuous rotation
    void TableLamp();
    void WallSconce();
    void Column(float height, float radius);        // marble column: base/shaft/capital

    // --- decorations -------------------------------------------------------
    void Painting(float w, float h, const char *texName); // frame + canvas
    void PottedPlant(float scale);
    void VaseWithFlowers(float scale);
    void Candle(float h, bool lit, float timeSec);
    void Curtains(float w, float h);                // drawn centered on x
    void Rug(float w, float d);                     // lies on the floor
    void PictureFrame(float w, float h);            // frame only (photographs)

    // --- garden (Phase 7) -----------------------------------------------
    void Tree(float scale, float timeSec);          // trunk + swaying foliage
    void Bush(float scale, int seed);               // leafy sphere cluster
    void Flower(float h, int kind);                 // stem + petals + center
    void FlowerBed(float w, float d, int seed);     // stone-edged blooms
    void Hedge(float length, float height);         // trimmed garden wall
    void Bench();                                   // slatted mansion bench
    void GardenLamp(float timeSec);                 // post + glowing lantern
    void Statue(float scale);                       // classical marble figure
    void Fountain(float timeSec);                   // 3-tier animated fountain
    void WaterSurface(float r, float timeSec);      // translucent ripple disc

    // --- basement (Phase 8) ----------------------------------------------
    void Globe(float scale);                        // desk globe on stand
    void DrawingTable();                            // the secret room's map table
    void SmallCabinet();                            // antique crest cabinet
    void Barrel(float r, float h);                  // staved wine barrel
    void Bottle(int kind);                          // colored glass bottle
    void StorageShelf(float w, float h);            // crates, jars, bottles
    void SupportBeam(float length);                 // heavy walnut ceiling beam
    void CandleGroup(float timeSec);                // three flickering candles

    // --- mansion crest (recurring story symbol) ---------------------------
    void Crest(float size, float r, float g, float b, float alpha);
    void CrestDial(float size, float angleDeg, bool lit);
}

#endif // OBJECTS_H
