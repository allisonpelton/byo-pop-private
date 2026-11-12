// The scene files the config names: each level, screen and menu is a file of tags that the scene loader makes a scene of. The
// config loader keeps the text of every file (and reads the name and title from its first tag); the game's levels are in
// one list, the relaxed (Journey) levels in a second and the trick shots in a third.

#ifndef VENICE_SCENEFILES_H
#define VENICE_SCENEFILES_H

#include "CursorList.h"

#include <string>

// 0x98 bytes in the original
class SceneFile
{
public:
	SceneFile* mNext = nullptr;
	SceneFile* mPrev = nullptr;
	std::string mText; // the file's text (0xc)
	char mName[64] = {}; // 0x10: the name the first tag gives, which the game pushes the scene by
	char mTitle[64] = {}; // 0x50: the first tag's title, empty when there is none
	unsigned int mNumber = 0; // 0x90: the level number in the list (1 for the first level; 0 for a screen)
};

extern CursorList<SceneFile> gGameScenes; // 0xa295a4: the levels of the Flood mode and the screens
extern CursorList<SceneFile> gRelaxedScenes; // 0xa295b0: the Journey levels
extern CursorList<SceneFile> gTrickScenes; // 0xa29598: the trick shots

#endif
