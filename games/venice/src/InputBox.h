// The name box: the text field of the screens that ask for a player's name. It types what the keys give into a string and
// draws it in a framed box, with a blinking bar after it.

#ifndef VENICE_INPUTBOX_H
#define VENICE_INPUTBOX_H

#include "SceneObject.h"

// 0x478 bytes in the original (the level editor's use of the box, which sets a value of the object it edits, is not made)
class InputBox : public SceneObject
{
public:
	char mText[256] = {}; // 0x170: what has been typed (the start is the text the data file gives)
	char mFontName[256] = {}; // 0x370
	int mWidth = 100; // 0x470: half the width of the box
	int mHeight = 16; // 0x474
	int mNoLines = 0; // 0x168: the box has no frame
	int mBoxOffset = 0; // 0x16c: how far the frame lies below the text
	bool mTyped = false; // 0x15c: a letter has been typed (the first one of an empty box is a capital)

	InputBox(); // 0x42cdc0
	void Load(const char* theText, SceneTag* theTag) override; // 0x42cec0
	bool Update() override; // 0x42d1b0
	void Draw() override; // 0x42d520
};

#endif
