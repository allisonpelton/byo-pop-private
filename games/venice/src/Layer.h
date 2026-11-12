// The layer: the scene object that holds the moving parts of a level (the tiles, the plates, the background). The level's tiles
// layer is the one that scrolls, and the Survival mode makes its plates, wall decorations and minute markers from it.

#ifndef VENICE_LAYER_H
#define VENICE_LAYER_H

#include "SceneObject.h"

// 0x16c bytes in the original
class Layer : public SceneObject
{
public:
	float mContentHeight = 0; // 0x148: how far the children's pictures reach, from the lowest edge to the highest
	float mThumbnailsWidth = 0; // 0x14c: the width of the level select screen's thumbnails, which that screen adds up (0 anywhere else)
	float mLowestEdge = 0; // 0x150: the greatest bottom edge of a child's picture
	float mHighestEdge = 0; // 0x154: the least top edge
	int mMerged = 0; // 0x160: the children are to be drawn into a few big pictures (see MergeChildren)
	float mScrollBonus = 0; // 0x168: the Survival mode's scroll speed grows by a little every 4000 updates

	Layer(); // 0x40c0f0
	void Load(const char* theText, SceneTag* theTag) override; // 0x40e5d0
	void OnObjectClosed() override; // 0x40c1b0
	bool Update() override; // 0x40c580

	// 0x40e660: the plates shake sideways and up and down (a plate that fell past the water's top rattles the layer)
	void Shake(float theHorizontalAmplitude, float theVerticalAmplitude);

private:
	void UpdateContentHeight(); // 0x40c200
	void MergeChildren(); // 0x40de30
	void AdjustScroll(); // 0x40c780: the scroll speed of the Flood, Journey and Trick Shot modes follows the scroll point's height
	void EndlessScroll(); // 0x40cb60: the same for the Survival mode, which also makes the plates, walls and markers
	void RemoveFallenChildren(); // 0x40d750
	void SpawnSurvivalPlates(); // 0x40d820
	void SpawnWallDecoration(); // 0x40d310
	void MakeMinuteMarker(); // 0x40ce00
	const char* PickPlateFile(); // 0x40da70
};

// 0x408710: the child of this type with the greatest world y (the lowest on the screen) in the layer, which is the plate layer
// when none is given; null when there is none
SceneObject* FindLowestOfType(int theType, SceneObject* theLayer);
// 0x408790: the child of this type with the least world y (the highest on the screen)
SceneObject* FindHighestOfType(int theType, SceneObject* theLayer);

#endif
