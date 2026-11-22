// Transition: the screen change (0x470a60..0x4721e0 in the original). Five door panels drop from the top and five rise
// from the bottom, staggered in one of five patterns that take turns, and meet in the middle; the game swaps screens
// behind them and they part again. A monkey's picture may grow over the middle as they close, and the "loading" text
// may spin in.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class Anim;

// Where the doors are (0x670228)
enum TransitionPhase
{
	TRANSITION_OPEN = 0,
	TRANSITION_CLOSING = 1,
	TRANSITION_CLOSED = 2,
	TRANSITION_OPENING = 3,
};

const int TRANSITION_DOORS = 5;

class Transition : public ObjBase
{
public:
	Anim* mMonkeyAnim;                       // +0x27c
	Anim* mDoorAnim;                         // +0x280
	Anim* mLoadingTextAnim;                  // +0x288
	ObjBase* mMonkey;                        // +0x28c
	ObjBase* mTopDoors[TRANSITION_DOORS];    // +0x290
	ObjBase* mBottomDoors[TRANSITION_DOORS]; // +0x2a4
	float mTopOpenY;                         // +0x2e0: the top doors' place while open, above the screen
	float mBottomOpenY;                      // +0x2e4
	int mPattern;                            // +0x2e8: which stagger the next move uses, 0 to 4 in turn
	int mDoorsArrived;                       // +0x2ec: the doors at the end of this move
	bool mMonkeyDone;                        // +0x2f0: the monkey finished growing or shrinking (or is not shown)
	bool mWithMonkey;                        // +0x2f1
	bool mShowingLoading;                    // +0x2f2
	ObjBase* mLoadingText;                   // +0x2f4
	float mMonkeyFullScale;                  // +0x2fc: the window's width / 100

	static int sPhase;                       // 0x670228: TransitionPhase
	static int sLoadingCel;                  // 0x5ffff4: the loading text's cel; 1 from the start

	Transition();
	~Transition();
	void Notify(ObjBase* theObj) override;

	void Close(bool withMonkey, bool showLoading);     // 0x471130
	void Open(bool withMonkey);                        // 0x471200
	static int GetPhase() { return sPhase; }           // 0x4721d0

private:
	void StartDoors(bool opening, int theDelay);       // 0x470cf0
	void StartDoorPair(int theIndex, bool opening, int theDelay);
	void BuildMonkey();                                // 0x4712a0
	void BuildDoors();                                 // 0x4715d0
	void BuildLoadingText();                           // 0x471d80
};

} // namespace Sexy
