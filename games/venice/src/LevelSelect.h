// The level select screen: a strip of thumbnails of the game's levels in a layer that slides along, with the shooter's marker on
// the level the player is up to, and a scroller that moves the strip when the pointer leaves the screen's edge.

#ifndef VENICE_LEVELSELECT_H
#define VENICE_LEVELSELECT_H

#include "SceneObject.h"

// 0x150 bytes in the original
class LevelSelect : public SceneObject
{
public:
	int mPositioned = 0; // 0x148: the strip has been put in place for the level the player is up to

	LevelSelect(); // 0x409f70
	void Load(const char* theText, SceneTag* theTag) override; // 0x411170: the thumbnails and the marker, from the level files
	void Draw() override; // 0x411790

private:
	void Position(); // 0x411820: at the first draw, the layers slide from the start of the strip to the marker's level
};

// 0x148 bytes in the original: the "go on" of the screen's custom event moves the strip a half screen in the direction of the
// scroller's side, when a thumbnail is past the edge that way
class LevelScroller : public SceneObject
{
public:
	void OnCustomEvent() override; // 0x411a50
	void Draw() override; // 0x411a30
};

#endif
