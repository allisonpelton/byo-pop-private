// JeffLib::BonusText (0x58 bytes, code 0x6c46a0-0x6c4b23): a line of text that floats up the screen, in a
// rainbow of colours until its life runs out, and fades away; the mini-games' points and bonuses (the app keeps
// them: GameApp's AddBonusText, UpdateBonusText, DrawBonusText and RemoveAllBonusText).
#pragma once

#include <string>

namespace Sexy {
class Font;
class Graphics;
}

namespace JeffLib {

class BonusText {
public:
	float mX;                  // +0x00
	float mY;                  // +0x04: the top of the text
	int mUpdateCnt = 0;        // +0x08
	int mWidth;                // +0x0c: the text's
	int mHue = 0;              // +0x10: cycles while it lives
	int mLife = 250;           // +0x14: updates of colour left
	int mFadeDelay;            // +0x18: updates before it fades (without a target)
	bool mDead = false;        // +0x1c
	bool mWhite = false;       // +0x1d: white instead of the rainbow (nothing sets it)
	bool mHasTarget = false;   // +0x1e: rises to mTargetY, waits there for mHoldUpdates and goes on
	std::string mText;         // +0x20
	Sexy::Font* mFont;         // +0x3c
	float mTargetX = -1;       // +0x40
	float mTargetY = -1;       // +0x44
	int mHoldUpdates = 0x7fffffff;  // +0x48: with a target; 0x7fffffff (none) lets it fade
	int mHoldStart = 0;        // +0x4c: the update it reached its target
	int mAlpha = 255;          // +0x50
	float mSpeed;              // +0x54: pixels up an update

	// 0x6c46a0: at (x, y), where a negative x centres it across the screen and a negative y down it (a y of
	// its own is moved down by the font's ascent, and Draw moves it down by that once more)
	BonusText(const std::string& theText, Sexy::Font* theFont, float x, float y, float theSpeed, int theFadeDelay);
	// 0x6c4820: rises from the bottom of the screen (y 480) at x to theTargetY, where it stays theHoldUpdates
	// (port: nothing in the game uses it)
	BonusText(const std::string& theText, Sexy::Font* theFont, float x, float theTargetY, int theHoldUpdates,
	          float theSpeed);

	void Update();                // 0x6c4910
	void Draw(Sexy::Graphics* g);  // 0x6c4a10
};

}  // namespace JeffLib
