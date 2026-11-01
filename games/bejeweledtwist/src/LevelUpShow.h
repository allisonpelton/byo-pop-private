// Sexy::LevelUpShow (0x2c8 bytes, vtable 0x7e64cc, 0x71c667-0x71d003): the "LEVEL n COMPLETE" that grows and fades
// in over the board at the end of a level (Board 0x6a66d5), kept at board +0x26ac until it is clicked away. A twin of
// GameOverShow with two pictures and the level's number between them.
#pragma once

#include "ColorCycle.h"
#include "CurvedVal.h"
#include "TwistWidget.h"

namespace Sexy {

class Board;
class EffectsManager;
class Image;
class MemoryImage;

class LevelUpShow : public TwistWidget {
public:
	Image* mLevelImage;              // +0x148: IMAGE_LEVELCOMPLETE_LEVEL
	Image* mCompleteImage;           // +0x14c: IMAGE_LEVELCOMPLETE_COMPLETE
	float mUnknown150 = 1.0f;        // +0x150
	float mUnknown154 = 1.0f;        // +0x154
	float mUnknown158 = 0;           // +0x158: 4 pi
	float mUnknown15C = 0.75f;       // +0x15c
	float mUnknown160 = 0;           // +0x160
	float mUnknown164 = 0.75f;       // +0x164
	int mState = 0;                  // +0x168: from 2 on it is not drawn, and the number shrinks away (never set)
	bool mUnknown16C = false;        // +0x16c
	EffectsManager* mFXManager;      // +0x170: added to the widget manager with the show (nothing uses it)
	ColorCycle mCycle;               // +0x174
	Board* mBoard;                   // +0x1a8
	float mUnknown1B4 = 0.4f;        // +0x1b4
	int mUnknown1B8 = 0;             // +0x1b8
	// +0x1bc: an empty vector of pointers
	float mUnknown1CC = 0;           // +0x1cc
	float mUnknown1D0 = 0;           // +0x1d0
	float mUnknown1D4 = 0;           // +0x1d4
	CurvedVal mScaleCurve;           // +0x1e0: the pictures' scale, and four times it less a half their alpha
	CurvedVal mHeightCurve;          // +0x250: their height on the screen, linked to mScaleCurve
	float mNumberScale = 0.8f;       // +0x2c0: the number's scale against the pictures'
	MemoryImage* mNumber = nullptr;  // +0x2c4: the level's number in FONT_LEVEL_NUMBER

	explicit LevelUpShow(Board* theBoard);  // 0x71cc30
	~LevelUpShow() override;                // 0x71cbb1
	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x71c667
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x71c687
	void Update() override;                  // 0x71c695
	void Draw(Graphics* g) override;         // 0x71c744
};

}  // namespace Sexy
