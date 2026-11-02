// Sexy::TextImage (vtable 0x7f188c, 0x74e5b8-0x74e701): a MemoryImage with a text drawn on it in white, and
// Sexy::GameOverShow (0x2b8 bytes, vtable 0x7e25ec, 0x6fe17a-0x6fe5d2): the "GAME OVER" that grows and fades in over
// the board at the end of a game (Board::EndGame), kept at board +0x26d0 until the board goes.
#pragma once

#include "ColorCycle.h"
#include "CurvedVal.h"
#include "MemoryImage.h"
#include "TwistWidget.h"

namespace Sexy {

class Board;
class EffectsManager;
class Font;

class TextImage : public MemoryImage {
public:
	int mTextWidth = 0;  // +0x8c: the widest line

	// 0x74e701: theText's lines (split at '\n') in theFont, with thePadX and thePadY round them, theLineSpacing apart
	// (0 or less: the font's own) and justified by theJustification; theRect, when given, sets the size instead
	TextImage(Font* theFont, const SexyString& theText, int thePadX, int thePadY, int theJustification,
		int theLineSpacing, const Rect* theRect);
	~TextImage() override {}  // 0x74e5b8
};

class GameOverShow : public TwistWidget {
public:
	TextImage* mText = nullptr;      // +0x148
	float mUnknown14C = 1.0f;        // +0x14c
	float mUnknown150 = 1.0f;        // +0x150
	float mUnknown154 = 0;           // +0x154: 4 pi
	float mUnknown158 = 0.75f;       // +0x158
	float mUnknown15C = 0;           // +0x15c
	float mUnknown160 = 0.75f;       // +0x160
	int mUnknown164 = 0;             // +0x164: from 2 on it is not drawn (never set)
	bool mUnknown168 = false;        // +0x168
	EffectsManager* mFXManager;      // +0x16c: added to the widget manager with the show (nothing uses it)
	ColorCycle mCycle;               // +0x170
	Board* mBoard;                   // +0x1a4
	float mUnknown1B0 = 0.4f;        // +0x1b0
	int mUnknown1B4 = 0;             // +0x1b4
	// +0x1b8: an empty vector of pointers
	float mUnknown1C8 = 0;           // +0x1c8
	float mUnknown1CC = 0;           // +0x1cc
	float mUnknown1D0 = 0;           // +0x1d0
	CurvedVal mScaleCurve;           // +0x1d8: the text's scale, and four times it less a half its alpha
	CurvedVal mHeightCurve;          // +0x248: its height on the screen, linked to mScaleCurve

	explicit GameOverShow(Board* theBoard);  // 0x6fe3d7
	~GameOverShow() override;                // 0x6fe35a
	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x6fe17a
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x6fe19a
	void Update() override;                  // 0x6fe1a8
	void Draw(Graphics* g) override;         // 0x6fe1ce
};

}  // namespace Sexy
