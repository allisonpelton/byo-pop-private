// The small objects that go with the shooter: the marker a level leaves where the shooter's path runs, the point the scroll
// follows, and the pointer that shows the jewel that is ready to throw.

#ifndef VENICE_SHOOTEROBJECTS_H
#define VENICE_SHOOTEROBJECTS_H

#include "Cursor.h"
#include "SceneObject.h"

// 0x148 bytes in the original. It is a marker of the level data: invisible, and gone once it has scrolled off the bottom of the
// screen (when the level scrolls down).
class ShooterPath : public SceneObject
{
public:
	ShooterPath() { mType = ObjectTypeShooterPath; } // 0x409ca0
	bool Update() override; // 0x41bb70
	void Draw() override {} // 0x41bb20: the original draws it only in its level editor
};

// 0x14c bytes in the original. The scroll points are the places where the level's scroll slows down or stops; the lowest one
// that is on the screen is the one that counts.
class ScrollPoint : public ShooterPath
{
public:
	ScrollPoint() { mType = ObjectTypeScrollPoint; } // 0x409cd0
	bool Update() override; // 0x41bc00
};

// The pointer of the playing field: the picture of the jewel that is ready to throw is shown next to it.
class ShooterCursor : public Cursor
{
public:
	ShooterCursor() { mType = ObjectTypeShooterCursor; } // 0x409d40
	bool Update() override; // 0x41be80: only the top scene moves it
	void Draw() override; // 0x41beb0
};

#endif
