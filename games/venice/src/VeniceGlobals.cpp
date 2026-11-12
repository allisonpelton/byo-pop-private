// The engine's global state (see VeniceGlobals.h); each starts at the value the original's data section gives it.

#include "VeniceGlobals.h"

#include "SceneObject.h"
#include "VeniceMath.h"

float gScreenWidth = 800.0f; // 0x5b5160
float gScreenHeight = 600.0f; // 0x5b5164

Sexy::Graphics* gGraphics = nullptr;

CursorList<SceneObject> gScenes;

Level* gActiveLevel = nullptr;
Shooter* gShooter = nullptr;
SceneObject* gSparkleLayer = nullptr;
SceneObject* gOuterLayer = nullptr;
SceneObject* gMiddleLayer = nullptr;
SceneObject* gPlateLayer = nullptr;
SceneObject* gTutorial = nullptr;

int gLoadState = -2;
bool gHasFocus = true;
int gFocusFrames = 2;
bool gMouseInside = true;
int gLevelNumber = 1;

uint32_t gRandomSeed = RandomSeedInitial;
