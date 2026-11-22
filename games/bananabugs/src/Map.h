// Map: the jungle map shown before a level (0x4449c0..0x4463c0 in the original). The fourteen level icons sit on it,
// those passed brighter and those ahead dimmer; an X stamps each level played in turn with a dotted trail between
// them, then a trail of footprints walks from the last level played to the next, and the play button takes the
// click that ends it.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class Anim;

const int MAP_LEVELS = 14;
const int MAP_MAX_DOTS = 128;

class Map : public ObjBase
{
public:
	ObjBase* mPlayButton;    // +0x27c
	bool mDone;              // +0x280: the map has closed after the click
	int mStampedLevel;       // +0x284: the last level the X has stamped
	int mFromLevel;          // +0x288: the last level played, or -1 for none
	int mToLevel;            // +0x28c: the level to play next
	float mWalkAngle;        // +0x290: the footprints' direction
	float mWalkStepX;        // +0x294
	float mWalkStepY;        // +0x298
	float mWalkSteps;        // +0x29c: steps left, each a pixel along the longer axis
	int mTrailCount;         // +0x2a0: the stamped trails' steps so far; a dot every 32
	int mWalkCount;          // +0x2a4: the footprints' steps so far; a print every 32
	float mWalkX;            // +0x2a8
	float mWalkY;            // +0x2ac
	float mWalkToX;          // +0x2b0
	float mWalkToY;          // +0x2b4
	int mDotCount;           // +0x2b8: dots used of mDots
	int mWalkDelay;          // +0x2bc: frames before the footprints start
	bool mClicked;           // +0x2c0
	ObjBase* mIcons[MAP_LEVELS]; // +0x2c4
	ObjBase* mStamps[MAP_LEVELS]; // +0x2fc: the X on each level played
	ObjBase* mDots[MAP_MAX_DOTS]; // +0x334: the trails' dots and the footprints

	static Anim* sPlayAnim;  // 0x60e64c
	static Anim* sMapAnim;   // 0x60e650
	static Anim* sDotAnim;   // 0x60e654: three cels, a trail dot and two footprints
	static Anim* sStampAnim; // 0x60e658
	static Anim* sIconAnims[MAP_LEVELS]; // 0x60e65c

	Map();
	~Map();
	void Notify(ObjBase* theObj) override;
	bool Update() override;
	bool MouseUp(ObjBase* theObj) override;

	void Start(int theFromLevel, int theToLevel);
	bool IsDone() { return mDone; } // 0x4452a0

private:
	void SetWalk(float theFromX, float theFromY, float theToX, float theToY);
	void AddTrail(int theFromLevel, int theToLevel);
	void AddDot(float theX, float theY, float theZ, float theScale, float theAlpha, float theRotation, float theCel);
	void BuildMap();
	void BuildIcons();
	void BuildDots();
	void BuildStamps();
	void BuildPlayButton();
};

void PreloadMap(); // 0x4463c0

} // namespace Sexy
