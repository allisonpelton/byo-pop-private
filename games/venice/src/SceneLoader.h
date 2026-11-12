// The scene loader: the game's screens, levels and objects are files of tags (<object>, <stage>, <attribute>, <filter>, <event>)
// that this code turns into objects, and the config file names every picture, font, sound and scene file the game has.

#ifndef VENICE_SCENELOADER_H
#define VENICE_SCENELOADER_H

#include <string>

class SceneObject;

// 0x442f60: the text of a game file (a path under the game's folder), false when there is none
bool ReadGameFile(const char* theName, std::string* theText);

// 0x4381a0: makes the objects the text describes, as children of theParent (a text with a <level> object makes a scene of its
// own instead). Returns the first object made, null for a level.
SceneObject* LoadSceneObjects(const char* theText, SceneObject* theParent);

// 0x438160: LoadSceneObjects of a game file
SceneObject* LoadSceneFile(const char* theName, SceneObject* theParent);

// 0x438bf0: makes a scene (put on the top of the scene stack) of the level or screen file the config gave this name
void PushSceneByName(const char* theName);

// 0x438c90: the same by the number the config gave the file. The game's list (the Journey list in that mode) holds the numbers
// below 1000; the trick shots are numbered 1000 and up.
void PushSceneByNumber(int theNumber);

// The text of an insert-level event: a number between 1 and 1999 is a level's number, any other text a scene's name.
void PushSceneByText(const char* theText);

// Whether the config gave a screen or level file this name (the test commands ask before they push a screen)
bool SceneFileExists(const char* theName);

// 0x439720: reads config.xml, which makes every picture record, font, sound and scene file of the game, then shows the main
// menu. The original does it on a thread while the loading screen draws its bar; the port does it in slices of a few
// milliseconds between the screen's draws: BeginLoadConfig starts, and each ContinueLoadConfig runs the config's tags for
// up to the given time and returns true once the last is done.
void BeginLoadConfig();
bool ContinueLoadConfig(unsigned theMilliseconds);

#endif
