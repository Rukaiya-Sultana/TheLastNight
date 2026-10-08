# THE LAST NIGHT

A 3D first-person mystery/adventure game built with **C++ / OpenGL / FreeGLUT**
for Code::Blocks on Windows (university Computer Graphics project).

Current state: **Complete escape-room route** - first-person movement and
flashlight, procedurally modeled mansion rooms, interactive clue cards and
puzzles, key/door progression, a sequential compass objective helper, and the
final front-door escape.

## Requirements

* Windows
* Code::Blocks with the bundled or standalone MinGW GCC compiler
* Nothing else - FreeGLUT 3.6.0 is **vendored inside this project as source**,
  so Code::Blocks compiles it with the same 64-bit or 32-bit MinGW target as
  the game; no system-wide installation is needed. All textures are generated
  procedurally in code - no image files, no asset folder, nothing to download.

## How to open and build

1. Double-click `TheLastNight.cbp` (opens in Code::Blocks).
2. Choose the **Debug** or **Release** target in the toolbar.
3. Press **F9** (Build and run).

The project is preconfigured with the right include paths, linker libraries and
the `FREEGLUT_STATIC` define - no project settings need to be touched.

Command-line build (same flags as the Release target):

```
cd TheLastNight
g++ -Wall -O2 -DFREEGLUT_STATIC -DFREEGLUT_REPLACE_GLUT -I. ^
  -Ivendor/freeglut/include *.cpp vendor/freeglut/lib/libfreeglut_static.a ^
  -o bin/Release/TheLastNight.exe -lopengl32 -lglu32 -lgdi32 -lwinmm
```

## Controls

| Key | Action |
|-----|--------|
| W / A / S / D | Walk (relative to where you look) |
| Mouse | Look around (yaw + pitch, pitch clamped to +/-89 deg) |
| SHIFT (hold) | Run |
| F | Toggle flashlight (beam pool + HUD indicator) |
| M | Mute / unmute audio |
| E | Interact (doors, letters, objects) |
| ESC | Pause / resume |
| F4 | Dev: save a screenshot to `bin/Release/shot.bmp` |

For the full puzzle order and combinations, see [WALKTHROUGH.md](WALKTHROUGH.md).

## The world

Four connected explorable rooms (one continuous space - you physically walk
through the doorways between them, no loading screens):

**Lobby** - wood plank floor, plaster walls with moldings, a brick fireplace
with an animated flame and flickering light, a slowly rotating chandelier, a
burgundy sofa with two green armchairs around a coffee table (an old letter
sits on it) on a patterned carpet, a grandfather clock with moving hands and a
swinging pendulum, a bookshelf of colorful books, three framed paintings, a
separate double front entrance opening onto the procedurally drawn night garden,
potted plants, a table lamp, and the east garden door (keyed from inside).

**Grand Hall** - six marble columns, a grand rotating chandelier, a large
patterned carpet, walnut floor, cream walls with wainscoting, portraits
(including a slightly crooked one of the mansion's owner and a mysterious dark
forest painting), a writing desk with a candle and the old letter (first clue),
a stopped clock, side tables with vases, bookshelf, potted plants, wall sconces,
two windows with the evening exterior, a hinged west door, and a wide framed
library entrance with its leaves held open. The hall clock and side table sit
clear of the doorway approach.

**Grand Library** - floor-to-ceiling walnut bookshelves (hundreds of colorful
books with the occasional leaning one), the dark-red book with the gold spine
pulled out from its row (the journal), the reading table under a green banker
lamp with an open book, papers, ink and a candle, a burgundy-gold rug, the
stopped-at-midnight grandfather clock, the founder painting with its brass
"No. 7" plate, a lit fireplace, a globe, two standing lamps, a big window with
the blue evening outside - and the 3-digit safe. Crack the code (clues:
journal, clock, plate) and the safe swings open to reveal the mansion key.
The brass key hidden where the book stood unlocks the study door.

**Study** - the master's private room: an antique desk with a working drawer
(the hidden note inside points to the garden), a burgundy leather chair, two
bookshelves, a lit fireplace under the carved mansion crest (circle, triangle,
moon - the same crest as in the old photograph), the photograph and journal on
the desk, a large window, potted plants, and the iron-locked map closet with
rolled charts marking the garden fountain.

## The mystery chain

Lobby letter -> west hall door -> hall letter + stopped clock -> library
journal, clock and founder plate -> safe code 127 -> mansion key -> garden
fountain plaque + statue plate -> basement dials -> secret room crest -> Master Key ->
front door: read the brass plaque beside the three medallions, then turn the
moon / star / sun dials (with E) until their golden pips match the verse
(moon 2, star 1, sun 3). The doors swing open - walk out to end the night.

The private study is an optional side route: the brass key on the library shelf
opens its door, and the desk drawer note/map closet add story and garden clues.
The on-screen compass points to one main objective at a time; a completion toast
appears each time the current task is satisfied.

## Project structure

```
TheLastNight/
|-- main.cpp            Program entry point
|-- Game.h/.cpp         Window, game loop, input, frame timing, pause menu
|-- Audio.h/.cpp        WinMM/MCI music and sound-effect service
|-- Player.h/.cpp       First-person player: movement, run, flashlight state
|-- Camera.h/.cpp       Camera attached to the player (gluLookAt view transform)
|-- Collision.h/.cpp    Circle-vs-AABB colliders + dynamic door slots
|-- Interaction.h/.cpp  Focus picking, interaction prompts, message panel
|-- Progress.h/.cpp     Story flags: keys found, locks opened, clues read
|-- Puzzle.h/.cpp       The library safe's 3-digit keypad (overlay + logic)
|-- Lobby.h/.cpp        The mansion lobby scene (walls, furniture, lights)
|-- GrandHall.h/.cpp    The Grand Hall scene, doors, hall lights
|-- Library.h/.cpp      The Grand Library scene, safe door, study door
|-- Study.h/.cpp        The Study scene, desk drawer, map closet
|-- Garden.h/.cpp       The courtyard: fountain, cellar annex, stairs, front yard
|-- Basement.h/.cpp     The cellar hall: crest-dial mechanism, hidden panel, passage
|-- SecretRoom.h/.cpp   The hidden chamber: crest puzzle, cabinet, master key, letter
|-- Weather.h/.cpp      Garden ambience (rain/lightning removed by request)
|-- Objects.h/.cpp      Reusable furniture/decoration builders
|-- Lighting.h/.cpp     OpenGL light registry + 16 material presets + flashlight
|-- Texture.h/.cpp      Named procedural texture registry (17 textures)
|-- UI.h/.cpp           2D overlay text, crosshair, hints, game modes
`-- vendor/freeglut/    Vendored FreeGLUT 3.6.0 (static, MinGW build)
```

## Development phases

The game is developed in phases; each phase compiles and runs on its own.

* Phase 0 - foundation: game loop, delta time, render pipeline (done)
* Phase 1 - player: WASD, mouse-look, running, flashlight, crosshair (done)
* Phase 2 - the mansion lobby: detailed, textured, lit, explorable (done)
* Phase 3 - advanced textures, materials, lighting (done)
* Phase 4 - Grand Hall, doors, collision, interaction, first clue (done)
* Phase 5 - Grand Library, secret book, 3-digit safe puzzle (done)
* Phase 6 - Study, key/lock progression, drawer, map closet (done)
* Phase 7 - Garden courtyard: fountain, statue clue (done; rain removed by request)
* Phase 8 - Basement: candlelit cellar, crest-dial mechanism, hidden passage (done)
* Phase 9 - Secret Room: crest puzzle, cabinet, master key, the big reveal (done)
* Phase 10 - Final escape: brass symbols, double doors, ending sequence (done)
* Phase 11 - Final polish: main menu, pause menu, controls & credits screens,
  progression reset on new game, ending camera fix, interaction-reach consistency

The full quest chain: lobby key -> library code 127 -> study key -> garden
statue plate -> basement dials -> secret-room crest -> master key -> the three
front-door symbols -> escape into the night.

## Dev flags (used for automated verification)

`--shot` (capture one frame and exit), `--pos X Z`, `--yaw DEG`.
Passing `--pos`/`--yaw` boots straight into gameplay (the menu is skipped);
without them the game starts at the main menu.
