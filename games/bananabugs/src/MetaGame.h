// MetaGame: the base the player spends bananas on between levels, "monkeytown" (0x448e10..0x44f450 in the original; 0x918
// bytes there). The player buys one of up to three offers (the next level of a building), the lives they have earned
// float in from the left, and the done button moves on: to the map when the level has changed, and then to the level.
// The range holds only this class.
#pragma once

#include "MenuObj.h"
#include "MetaBuilding.h"
#include "ObjBase.h"

namespace Sexy
{

class Anim;
class InGameMenu;
class Map;
class MenuButton;
class ObjString;
class PathTrace;
class Slots;

const int META_OFFERS = 3;          // the offers on the panel
const int META_ENTRIES = 27;        // the upgrade levels there are to buy
const int META_LIFE_ICONS = 4;      // the life icons that fly in (the first is never used)
const int META_ENTRY_NONE = META_ENTRIES; // the entry whose level is never missing: no prerequisite
const int META_ENTRY_LINES = 4;

// What the offers are in. A kind of upgrade is the number of its building (MetaBuildingKind).
enum MetaOfferState
{
	OFFER_AVAILABLE = 0,    // can be bought
	OFFER_BOUGHT = 1,       // sold out
	OFFER_TOO_DEAR = 2,     // costs more than the player has
};

// One level of one building, what it costs and says (0x28 bytes in the original). Its level number is what the
// player's upgrade level must reach for it to count as bought.
struct MetaEntry
{
	int mKind;               // +0: the building (MetaBuildingKind); -1 for the one after the last
	int mLevelIndex;         // +4: the picture's cel of the offer, 0 to 2
	int mLevelNumber;        // +8: the level of the building this gives, 1 to 3
	int mPrerequisite;       // +0xc: the entry that must have been bought before this one is offered
	bool mBought;            // +0x10
	const char* mName;       // +0x14
	const char* mLines[META_ENTRY_LINES]; // +0x18: what the offer says, one line each (the last may be null)
};

// An offer on the panel (0x20 bytes in the original)
struct MetaOffer
{
	int mEntryIndex;         // +0: which entry (the table's number)
	int mBuilding;           // +4: the building it upgrades
	int mLevel;              // +8: the level it builds (what GetPrice(level - 1) reads)
	int mState;              // +0xc: MetaOfferState
	ObjBase* mButton;        // +0x10: the picture of the building; its id is the offer's number from 1
	ObjBase* mSoldOut;       // +0x14: shown over the picture once bought
	ObjString* mPrice;       // +0x18: what it costs
	MetaEntry* mEntry;       // +0x1c: its entry (set only when the player had offers from before)
};

class MetaGame : public ObjBase
{
	friend class Slots;    // asks for the bananas to be shown again (UpdateBananasText)

public:
	bool mWantsMenu;                       // +0x27c: the player quit from the game menu
	bool mLeaving;                         // +0x27d: the player is done: TakeLevelChosen takes it
	MenuButton* mMenuButton;               // +0x280
	int mLivesToGive;                      // +0x284: lives still to float in (the flight school's level)
	Map* mMap;                             // +0x288
	InGameMenu* mGameMenu;                 // +0x28c
	Anim* mCoronaAnim;                     // +0x290: the glow behind a hovered offer
	Anim* mLifeAnim;                       // +0x294: the monkey of a life
	Anim* mArrowAnim;                      // +0x298
	Anim* mSoldOutAnim;                    // +0x29c
	ObjBase* mCoronas[2];                  // +0x2a0
	ObjBase* mLifeIcons[META_LIFE_ICONS];  // +0x2a8
	ObjBase* mArrowHint;                   // +0x2b8: points at the first offer, until the mouse is on one
	MenuObj* mSoldOutPanel;                // +0x2bc: "everything is sold" over the panel
	PathTrace* mPathTrace;                 // +0x2c0: the way a life icon flies
	bool mShowIntro;                       // +0x2c4: the first picture, shown until the player has built five levels
	int mUnused2c8;                        // +0x2c8: set to 0, never read
	bool mBroke;                           // +0x2cc: no building can be afforded
	int mBuiltCount;                       // +0x2d0: buildings at least at level 1 (counted, never read)
	int mState;                            // +0x2d4: MetaState
	bool mFirstTrackDone;                  // +0x2d8: the first event of the start track came
	bool mUnused2d9;                       // +0x2d9: set to 0, never read
	int mWaitFrames;                       // +0x2dc: frames until the next life flies in
	int mUnused2e0;                        // +0x2e0: set to 0x140, never read
	Slots* mSlots;                         // +0x2e4: the casino's game while it runs
	MenuObj* mPanel;                       // +0x2e8: the panel the offers are on
	MenuObj* mBuildTip;                    // +0x2ec: the text of the offer the mouse is on
	ObjString* mLivesText;                 // +0x2f0
	ObjString* mBananasText;               // +0x2f4
	ObjBase* mBananaPanel;                 // +0x2f8
	ObjBase* mDoneButton;                  // +0x2fc
	ObjBase* mCasinoButton;                // +0x300
	ObjBase* mBuyTag;                      // +0x304: the tag on the offer the mouse is on
	ObjString* mBuyTagText;                // +0x308
	int mLeaveTrack;                       // +0x30c: the track of this object that ends the screen
	int mStartTrack;                       // +0x310: the track of this object that times its first events
	ObjBase* mIntro;                       // +0x31c
	MetaBuilding* mBuildings[META_BUILDINGS]; // +0x320
	Anim* mIconAnims[META_BUILDINGS];      // +0x348: the picture of an offer, by building
	MetaOffer mOffers[META_OFFERS];        // +0x448
	int mUnused4a8;                        // +0x4a8: set to 0, never read
	MetaEntry mEntries[META_ENTRIES + 1];  // +0x4ac: the last is the one after
	bool mNoMap;                           // +0x90c: never set; the done button shows the map unless it is
	int mLivesBefore;                      // +0x910: the player's lives before the flight school's, and the count shown
	bool mNothingLeft;                     // +0x914: no offer was left to show
	bool mCasinoUnlocked;                  // +0x915: the first event of the casino button came

	static bool sFirstVisit;               // 0x5ff082: the first time here since a level; gives the lives
	static bool sOverOffer;                // 0x6a558c: the mouse was on an offer last frame

	MetaGame();                            // 0x448e10
	~MetaGame();                           // 0x4490a0
	void Notify(ObjBase* theObj) override;   // 0x449160
	bool Update() override;                  // 0x4492c0
	bool MouseUp(ObjBase* theObj) override;  // 0x4497f0

	bool TakeLevelChosen();                // 0x449b90
	static void ClearFirstVisit() { sFirstVisit = false; } // 0x44f440

private:
	void LoadEntryStatus();                // 0x449b10: which entries the player's levels have bought
	void GiveLives();                      // 0x449b40
	void UpdateBananasText(bool theSound); // 0x449bc0
	void SetLivesText(int theLives);       // 0x449c90
	void RefreshOffers();                  // 0x449d00
	bool BuildBuildingsAndOffers();        // 0x449e40
	void StartLeaveTrack();                // 0x44a170
	void StartStartTrack();                // 0x44a180
	void ShowHintArrow();                  // 0x44a190
	void HideHintArrow();                  // 0x44a200
	void ShowTagOnFirstOffer();            // 0x44a230
	void ShowBuyTag(ObjBase* theButton);   // 0x44a2b0
	void ShowCoronas(ObjBase* theButton, float theScale); // 0x44a330
	void HideCoronas();                    // 0x44a440
	void ShowBuildTip(int theEntry);       // 0x44a470
	void HideBuildTip();                   // 0x44a590
	void BuildNumberPulse(ObjString* theText); // 0x44c1d0 (twice)
	void BuildLeaveTrack();                // 0x44a5c0
	void BuildStartTrack();                // 0x44a810
	void BuildPanel();                     // 0x44ab40
	void BuildSoldOutPanel();              // 0x44aeb0
	void BuildDoneButton();                // 0x44b180
	void BuildCasinoButton();              // 0x44b890
	void BuildMenuButton();                // 0x44c140
	void BuildBananaPanel();               // 0x44c1d0
	void BuildIntro();                     // 0x44c8f0
	void BuildOffers();                    // 0x44cbc0
	void BuildEntries();                   // 0x44d500
	void BuildLifeIcons();                 // 0x44df40
	void BuildCoronas();                   // 0x44e4c0
	void BuildBuildTip();                  // 0x44e640
	void BuildBuyTag();                    // 0x44ebf0
	void BuildArrowHint();                 // 0x44f080
};

} // namespace Sexy
