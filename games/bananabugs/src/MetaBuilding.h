// MetaBuilding: one of the ten buildings of the base the player spends bananas on between levels (0x447060..0x448e10
// in the original; 0x3bc bytes there). Each has up to three stages (levels 1 to 3), a picture a stage, a price a stage,
// and sometimes small animated sprites beside it. A bought level drops the building in from above (SetLevel); the next
// level of it swaps the picture. While it drops, dust swells puff up around it.
// The range up to MetaGame (0x448e10) holds only this class.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class ObjString;
struct EmitterSet;

const int META_BUILDINGS = 10;
const int META_STAGES = 3;

// The ten buildings, in the order the player's upgrades and the base keep them (the names are the pictures' names)
enum MetaBuildingKind
{
	BUILDING_LEISURE = 0,
	BUILDING_HOUSE = 1,
	BUILDING_MILITARY = 2,
	BUILDING_SHOT_SPEED = 3,
	BUILDING_SPEAR_CRAFT = 4,
	BUILDING_HELPER = 5,
	BUILDING_FLIGHT_SCHOOL = 6,
	BUILDING_GREENHOUSE = 7,
	BUILDING_WEATHER = 8,
	BUILDING_IDOL = 9,
};

// A picture of "images/meta" and how it is cut and animated (0x20 bytes in the original). The stages' pictures use
// everything but the object; the sprites beside a building use the object too.
struct MetaPicture
{
	const char* mName;       // the picture's name after "images/meta/"
	int mUnused4;            // never written or read
	int mUnused8;            // never written or read
	int mCelWidth;
	int mCelHeight;
	int mCels;
	int mFrameDelay;         // frames each cel shows
	ObjBase* mObj;           // the sprite beside the building (the stages have none)
};

class MetaBuilding : public ObjBase
{
public:
	int mKind;                       // +0x27c: MetaBuildingKind
	int mFrameDelayLeft;             // +0x280: frames until the picture's next cel
	MetaPicture mSprites[META_STAGES]; // +0x284: the sprites beside the building, one for each level it has reached
	bool mDropping;                  // +0x2e4: the building is dropping in, or leaving: dust puffs up
	bool mDrawShadow;                // +0x2e5: Draw draws it again, dimmed (nothing ever sets this)
	bool mLanded;                    // +0x2e6: the drop has landed, or the level was set
	int mPrices[META_STAGES + 1];    // +0x2e8: the bananas each level costs; the fourth (+0x2f4) is 0, and what a level 3 building reads
	MetaPicture mStages[META_STAGES]; // +0x2f8: the picture of level 1, 2 and 3
	MetaPicture* mStagePicture;      // +0x358: the picture shown now
	int mStage;                      // +0x35c: 1 until the building has been shown, then 2
	int mLevel;                      // +0x360: 0 not built, 1 to 3
	float mHomeX;                    // +0x364: where it stands, in design pixels from the centre
	float mHomeY;                    // +0x368
	int mDropTrack;                  // +0x37c: the track that drops it in
	int mLeaveTrack;                 // +0x380: the track that sends it away up
	int mIdleTrack;                  // +0x384: a track of one key, which holds it still
	AnimKey* mDropKey;               // +0x38c: the keys' values change with the picture's height (SetupDrop)
	AnimKey* mDropRestKey;           // +0x390
	AnimKey* mDropLandKey;           // +0x394
	AnimKey* mLeaveKey;              // +0x398
	AnimKey* mLeaveEndKey;           // +0x39c
	AnimKey* mIdleKey;              // +0x3a0
	ObjString* mPriceText;           // +0x3a8: a number that floats up from a bought building
	bool mAffordable;                // +0x3a4: the player has the bananas for the next level (CanAfford)
	EmitterSet* mDust[4];            // +0x3ac: swells of dust, from the one furthest back to the nearest

	static ObjBase* sMetaGame;       // 0x60e698: the screen the buildings stand on

	MetaBuilding(int theKind, ObjBase* theMetaGame); // 0x447060
	~MetaBuilding();                                  // 0x447260
	void Init() override;                             // 0x447340 (slot 1)
	void Notify(ObjBase* theObj) override;            // 0x447390
	bool Update() override;                           // 0x447430
	void Draw(Graphics* g) override;                  // 0x447610

	void SetDepthFromY(float theDepth);               // 0x447680
	int GetPrice(int theLevel);                       // 0x4476a0: of the level (below 0: the building's own); EAX the level
	bool CanAfford();                                 // 0x4476c0
	void LoadStagePicture();                          // 0x447710
	void SetLevel(int theLevel, bool theAnimate);     // 0x4480e0 (EAX this, then the level and a flag)
	void StartDrop();                                 // 0x4481f0
	void StartLeave();                                // 0x4482a0

private:
	void AddSprite(const char* theName, int theCelWidth, int theIndex, int theX, int theY, int theCelHeight,
		int theDelay);                                // 0x4477e0
	void SetupKind();                                 // 0x447980
	void BuildTracks();                               // 0x448330
	void BuildPriceText();                            // 0x448980
};

} // namespace Sexy
