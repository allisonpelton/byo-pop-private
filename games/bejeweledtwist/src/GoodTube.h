// Sexy::GoodTube (0x770 bytes, vtable 0x7e3d64 with 71 slots; 0x702736-0x70a9bb): the tube beside the board that fills
// with the level's progress, with the level's number and the menu button. Being ported (docs/NOTES.md, "The board"):
// so far its constructor, which draws from Rand, and the setters the board calls; its fields are named by their offset
// until their use is known.
#pragma once

#include "CurvedVal.h"
#include "TwistWidget.h"

#include <vector>

namespace Sexy {

class Board;
class Image;

class GoodTube : public TwistWidget {
public:
	// +0x1a0: 16 of them, each a random phase (0 to 0.099)
	struct Bubble {
		float mUnknown0 = 0;
		float mPhase = 0;  // +0x4
	};

	bool mShown = false;                 // +0x14c: passed on to +0x32c's +0x54 and +0x55 (0x702ad3)
	bool mUnknown14D = false;            // +0x14d: the level's number changes with a sound (0x702ac3, 0x703431)
	Board* mBoard = nullptr;             // +0x154
	int mUnknown158 = 0;                 // +0x158: 4 in zen (0x702b87)
	int mUnknown15C = 0;                 // +0x15c: 1 in blitz (0x702bae)
	int mUnknown160 = 0;                 // +0x160: 1, 2 in zen (0x702bbe)
	int mUnknown16C = 0;                 // +0x16c: the glass's height over the bottom (0x7030ec)
	Image* mTopImage = nullptr;          // +0x178: the challenge's top in a challenge
	Image* mBottomImage = nullptr;       // +0x17c
	Image* mGlassImage = nullptr;        // +0x180
	Image* mFillImage = nullptr;         // +0x184
	Image* mFillImage2 = nullptr;        // +0x188
	float mUnknown194 = 0;               // +0x194
	float mUnknown198 = 0;               // +0x198
	std::vector<Bubble*> mBubbles;       // +0x1a0
	int mLevel = 0;                      // +0x288 (0x702aa6)
	float mUnknown2D0 = 0;               // +0x2d0
	float mUnknown2E0 = 0;               // +0x2e0: a random 0 to 1
	float mUnknown308 = 1.0f;            // +0x308
	CurvedVal mUnknown338;               // +0x338
	bool mUnknown4A8 = false;            // +0x4a8 (0x702bce)
	int mUnknown4BC = 0;                 // +0x4bc (0x702b6a): blitz's 30000
	bool mChallenge = false;             // +0x4c4 (0x70371c)

	explicit GoodTube(Board* theBoard);  // 0x707b7f
	~GoodTube() override;

	void SetShown(bool theShown);                  // 0x702ad3
	void SetUnknown14D(bool on) { mUnknown14D = on; }        // 0x702ac3
	void SetUnknown158(int theValue);              // 0x702b87
	void SetUnknown15C(int theValue) { mUnknown15C = theValue; }  // 0x702bae
	void SetUnknown160(int theValue) { mUnknown160 = theValue; }  // 0x702bbe
	void SetChallenge(bool on);                    // 0x70371c
	void SetLevel(int theLevel) { mLevel = theLevel; }        // 0x702aa6
	void SetUnknown4BC(int theValue) { mUnknown4BC = theValue; }  // 0x702b6a
	void SetUnknown4A8(bool on) { mUnknown4A8 = on; }         // 0x702bce
	void UpdateImages();                           // 0x7030ec
	int GetGlassTop();                             // 0x702921
	int GetBottomTop();                            // 0x70295d
};

}  // namespace Sexy
