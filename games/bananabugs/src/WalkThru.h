// WalkThru: the tutorial's hints (0x474f00..0x475f50 in the original, less the unrelated code that follows 0x475e91;
// 0x2fc bytes there). A hint is a picture of text at a place on the level; the first three levels of a player who has
// not done the tutorial start one, a few pages long. A page waits for what the player does (a shot, a lost life, a bug
// zapped, an area filled, the rope turned, or just time) and the one it is told to wait for sends the hint on to the
// next page: the picture swells out, the next swells in. A page with no next page ends the hint.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class Anim;

// What a page can wait for (the index of its transition)
enum WalkTrigger
{
	WALK_TRIGGER_NONE = 0,           // nothing raises it
	WALK_TRIGGER_SHOOT = 1,          // the monkey shot the rope (Player::Shoot)
	WALK_TRIGGER_LOSE_LIFE = 2,      // a life was lost
	WALK_TRIGGER_BUG_ZAPPED = 3,     // a bug was zapped
	WALK_TRIGGER_AREA_FILLED = 4,    // the cut's last cell has gone
	WALK_TRIGGER_TURN = 5,           // the player turned the rope
	WALK_TRIGGER_DISMISS = 6,        // the level is over (Dismiss); no page waits for it
	WALK_TRIGGER_UNUSED = 7,         // nothing raises it, no page waits for it
	WALK_TRIGGER_TIMEOUT = 8,        // 200 frames went by on the page
	WALK_TRIGGER_COUNT = 9
};

const int WALK_PAGE_MAX = 16;        // pages one hint can hold (+0x2b8..+0x2f8)

// One page of a hint (0x5c bytes in the original; made by 0x475db0)
struct WalkPage
{
	Anim* mPicture;                  // +0x00: the text, or none
	bool mEnded;                     // +0x04: the last trigger that was looked up here led nowhere (nothing reads it)
	bool mFinal;                     // +0x05: true until a transition to a page is added (nothing reads it)
	float mX;                        // +0x08: where it stands, in the level's design space
	float mY;                        // +0x0c
	int mTimer;                      // +0x10: frames left before a timeout transition fires
	bool mHasTransition[WALK_TRIGGER_COUNT]; // +0x14
	WalkPage* mNext[WALK_TRIGGER_COUNT];     // +0x38: the page each trigger leads to (0 for the end)

	WalkPage();                      // 0x475db0
	void Tick();                     // 0x475e00: the timeout counts down
	void SetPlace(float theX, float theY); // 0x475e20
	void AddTransition(WalkPage* theNext, int theTrigger); // 0x475e40
	WalkPage* GetNext(int theTrigger); // 0x475e70: the page a trigger leads to (0 for none, or for the end)
};

class WalkThru : public ObjBase
{
public:
	bool mDismissed;                 // +0x27c: Dismiss ran: the hint ends at its next turn
	int mTrigger;                    // +0x280: the trigger the page is leaving by; -1 while it waits for one
	int mShowTrack;                  // +0x284: the picture swells in, then raises event 98
	int mHideTrack;                  // +0x288: swells out, then raises event 99
	bool mTriggered[WALK_TRIGGER_COUNT]; // +0x28c: what has happened since the page began waiting
	WalkPage* mFirstPage;            // +0x2b0
	WalkPage* mPage;                 // +0x2b4: the page shown
	WalkPage* mPages[WALK_PAGE_MAX]; // +0x2b8: every page, to be freed
	int mPageCount;                  // +0x2f8

	static WalkThru* sInstance;      // 0x670254: the hint being shown (the hints' callers reach it through this)
	static Anim* sTextAnims[9];      // 0x67022c: images/walkthru/text01..text09

	WalkThru();                      // 0x474f00
	~WalkThru() override;            // 0x474fc0 (the body 0x474fe0)
	void Notify(ObjBase* theObj) override; // 0x475000
	bool Update() override;          // 0x475100

	static void OnShoot();           // 0x475610
	static void OnLoseLife();        // 0x475630
	static void OnAreaFilled();      // 0x475650
	static void OnTurn();            // 0x475670
	static void OnBugZapped();       // 0x475690
	static void OnTimeout();         // 0x4755f0 (a page's timer ran out; a guess at the name)
	static void Dismiss();           // 0x4756b0
	static void ShowFirstStep();     // 0x4757d0: the hint of the first level (the names of the three are guesses)
	static void ShowSecondStep();    // 0x4759e0: the second level's
	static void ShowThirdStep();     // 0x475c40: the third's

private:
	void BuildTracks();              // 0x4752b0
	void ClearTriggers();            // 0x475180
	void SetFirstPage(WalkPage* thePage); // 0x4751c0
	void AddPage(WalkPage* thePage); // 0x475210: remembered to be freed (once)
	void FreePages();                // 0x475250
	static void Raise(int theTrigger);
};

void PreloadWalkThru();              // 0x4756d0: the nine pictures

} // namespace Sexy
