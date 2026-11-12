// The tooltip: a box of text that shows over its parent while the parent is hot (the pointer is over it), fading in and out.

#ifndef VENICE_TOOLTIP_H
#define VENICE_TOOLTIP_H

#include "SceneObject.h"

// 0x390 bytes in the original
class Tooltip : public SceneObject
{
public:
	char mText[512] = {}; // 0x148: the data file's text, where '^' ends a line
	char mFontName[64] = {}; // 0x348
	int mLineWidth = 0; // 0x388: the most letters in a line
	int mJustification = 0; // 0x38c: a Justify value

	Tooltip(); // 0x437b20
	void Load(const char* theText, SceneTag* theTag) override; // 0x437b50
	void OnObjectClosed() override; // 0x437c70: breaks the text into lines, one text object each
	bool Update() override; // 0x437fc0
};

#endif
