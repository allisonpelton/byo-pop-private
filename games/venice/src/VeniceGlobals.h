// The engine's global state: the screen size, the scene stack and the pointers the game's code keeps to the objects of the level
// being played.

#ifndef VENICE_GLOBALS_H
#define VENICE_GLOBALS_H

#include "CursorList.h"

class Level;
class SceneObject;

namespace Sexy
{
class Graphics;
}

extern float gScreenWidth; // 0x5b5160: 800
extern float gScreenHeight; // 0x5b5164: 600

// the Graphics of the frame being drawn (0xa2a180, set by the game widget's Draw)
extern Sexy::Graphics* gGraphics;

// The scenes, bottom first: the screens and levels that are on the stack. Only the top one (the tail) takes the mouse; each is
// drawn over the ones below it.
extern CursorList<SceneObject> gScenes; // 0x5ba2b8

// The level being updated (0x5ba2a4, each level sets it as it updates, so after an update it is the top scene) and the objects
// the game finds in it with each update
extern Level* gActiveLevel; // 0x5ba2a4
extern SceneObject* gShooter; // 0x5ba2a8: the child of the level that throws the plates
extern SceneObject* gSparkleLayer; // 0x5ba29c: the layer that holds the sparkles and particles
extern SceneObject* gOuterLayer; // 0x5ba2ac: the level's first layer
extern SceneObject* gMiddleLayer; // 0x5ba2b0: the layer in the first, which also names the state of the play by its stage
extern SceneObject* gPlateLayer; // 0x5ba2b4: the layer in the middle one that holds the plates
extern SceneObject* gTutorial; // 0x5ba2a0: the tutorial's child of the level, when there is one

// The loading state of the game (0x5b51b8): -2 before the partner pictures, -1 while they show, 0 and 1 around the loading
// screen's first draw, 2 to 99 while loading, 100 when the data is loaded and 101 once the game runs.
extern int gLoadState;
const int LoadStateLoadingDone = 100;
const int LoadStateRunning = 101;
// Whether the window has the focus (0x5b51b4)
extern bool gHasFocus;
// 2 until the first update after the focus returns (0x5b51bc): frames drawn while it counts down are not updated
extern int gFocusFrames;
// whether the pointer is over the window (0x5b51c0)
extern bool gMouseInside;
// the level number the next level to play has (0x5b516c)
extern int gLevelNumber;

#endif
