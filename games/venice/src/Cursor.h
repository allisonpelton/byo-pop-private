// The cursor object: the game hides the system's cursor and draws its own picture at the pointer.

#ifndef VENICE_CURSOR_H
#define VENICE_CURSOR_H

#include "SceneObject.h"

// 0x14c bytes in the original
class Cursor : public SceneObject
{
public:
	int mTopLeft = 0; // 0x148: the picture's top left is at the pointer (the picture is centred on it otherwise)

	Cursor(); // 0x4219d0
	void Load(const char* theText, SceneTag* theTag) override; // 0x421a50
	bool Update() override; // 0x421a90
	void DrawSelf() override; // 0x421b20
};

#endif
