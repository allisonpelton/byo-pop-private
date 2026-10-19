#ifndef __BOOKWORM_LEX_H__
#define __BOOKWORM_LEX_H__

// Lex.cpp: Lex the bookworm beside the board. One instance, Board::mLex, a top-level widget (screen
// coordinates) made by Board::AddedToManager (Resize(0, 0, 107, 208), AddWidget, Move(-200, 0), then
// mTargetX/mTargetY = 60/210: he slides in from the left) or by Board::LoadGame (Load). Board,
// FlyingPieceList and LevelUpWidget start his animations (each caller plays the matching voice sample);
// he idles with random blinks, glows under the mouse, scrambles the board when clicked and talks through
// the OverlayWidget's shared WordBalloon (Say, Think, SayDefinition).
// Original: class Sexy::Lex : public Widget, sizeof 0xd0 (new(0xd0) in Board::AddedToManager 0x403730 and
// in Board::LoadGame 0x408ab0), vftable 0x5005b4 (49 slots: Widget's 47, overrides 0 dtor, 14 Move,
// 16 Draw, 17 Update, 23 MouseEnter, 24 MouseLeave, 27 MouseDown, 29 MouseUp; new 47 Save, 48 Load).
// Offsets are absolute (the 2004 Widget is 0x58 bytes).
// Behaviour spec: re/bookworm/analysis/lex.md section 2.

#include "Widget.h"
#include <cstdio>

namespace Sexy
{

class BookWormApp;
class Graphics;
class Font;

// Lex::mAnim values: the rows of gLexAnims. Named after the voice sample the caller plays with the
// animation (lip-sync); lex.md 2.2 lists every caller.
enum LexAnim
{
	LEXANIM_NONE = -1,
	LEXANIM_GO = 0,                 // Board classic greeting + SOUND_LEX_GO
	LEXANIM_LETSGO = 1,             // Board classic greeting + SOUND_LEX_LETSGO
	LEXANIM_LOOKOUT = 2,            // Board danger talk + SOUND_LEX_LOOKOUT
	LEXANIM_WATCHOUT = 3,           // + SOUND_LEX_WATCHOUT
	LEXANIM_CAREFUL = 4,            // + SOUND_LEX_CAREFUL
	LEXANIM_DANGER = 5,             // + SOUND_LEX_DANGER
	LEXANIM_UNUSED_6 = 6,           // no caller
	LEXANIM_BONUS = 7,              // Board bonus word (SOUND_BONUSWORD), time bonus (SOUND_WORDCOMPLETE)
	LEXANIM_ASTONISHING = 8,        // Board word praise + SOUND_LEX_ASTONISHING
	LEXANIM_AWESOME = 9,            // + SOUND_LEX_AWESOME
	LEXANIM_EXCELLENT = 10,         // + SOUND_LEX_EXCELLENT
	LEXANIM_FANTASTIC = 11,         // + SOUND_LEX_FANTASTIC
	LEXANIM_INCREDIBLE = 12,        // + SOUND_LEX_INCREDIBLE
	LEXANIM_UNBELIEVABLE = 13,      // + SOUND_LEX_UNBELIEVABLE
	LEXANIM_VERYGOOD = 14,          // + SOUND_LEX_VERYGOOD
	LEXANIM_LEVELUP = 15,           // Board level up (GT_LEVEL_UP) + SOUND_LEX_LEVELUP
	LEXANIM_UNUSED_16 = 16,         // no caller
	LEXANIM_GETREADY = 17,          // Board action greeting + SOUND_LEX_GETREADY
	LEXANIM_SCRAMBLE = 18,          // Board::Scramble + SOUND_FLAMETILE, SOUND_LEX_SCRAMBLE
	LEXANIM_SWEAT = 19,             // looped while in danger (FlyingPieceList and OverlayWidget test it)
	LEXANIM_OH = 20,                // Board::Refill green tile + SOUND_APPEAR2
	LEXANIM_EAT = 21,               // FlyingPieceList: mouth open (held 200000 updates, until replaced)
	LEXANIM_EAT_END = 22,           // FlyingPieceList (all eaten), Board (burp fade)
	LEXANIM_BURP = 23,              // Board::Update burp + SOUND_LEX_BURP (held like LEXANIM_EAT)
	LEXANIM_BURNT = 24,             // no caller
	LEXANIM_BURNT_BLINK = 25,       // Lex random idle on a burnt board; Board when the board burns
	LEXANIM_IDLE = 26,              // Lex random idle
	LEXANIM_BLINK = 27,             // Lex random idle
	NUM_LEX_ANIMS = 28
};

// One step of an animation row: image id mImage (BookWormApp::mImages index) shown for mDuration * 2
// updates. mImage -1 ends the row; the steps after the terminator are zero.
struct LexAnimFrame
{
	int				mImage;
	int				mDuration;
};

enum
{
	LEX_ANIM_STEPS = 50             // steps per row in the tables (0x190 bytes)
};

extern const LexAnimFrame gLexAnims[NUM_LEX_ANIMS][LEX_ANIM_STEPS];   // 0x4fd9f0, defined in Lex.cpp

class Lex : public Widget
{
public:
	double			mHintLevel;             // +0x58 0.0; += mGameStats->mScrambles at each hint, +0.2 after the long one; > 1.0: short hint, 75-update delay
	bool			mHintPending;           // +0x60 false; armed by MouseEnter, Update shows the scramble hint
	bool			mHintShowing;           // +0x61 false; the shared WordBalloon shows Lex's hint
	// +0x62..+0x63 pad
	int				mHintDelay;             // +0x64 updates before the hint appears (MouseEnter/Leave 0 or 75; 150 after the short hint)
	BookWormApp*	mApp;                   // +0x68 ctor arg
	// +0x6c pad
	double			mWobble;                // +0x70 0.0; spring driven by the x motion; never read (saved only)
	double			mWobbleVel;             // +0x78 0.004; read only by Update (saved)
	int				mTargetX;               // +0x80 0; where Lex slides to; Move sets it, Board / LevelUpWidget write it
	int				mTargetY;               // +0x84 0
	double			mCurX;                  // +0x88 0.0; spring position, the widget is at (int)mCurX; BoardDraw reads it (shadow)
	double			mCurY;                  // +0x90 0.0
	double			mVelX;                  // +0x98 0.0
	double			mVelY;                  // +0xa0 0.0
	double			mGlow;                  // +0xa8 0.0; mouse-over glow 0..1, +-0.075 per update
	bool			mShowShadow;            // +0xb0 true, never cleared; BoardDraw draws IMAGE_LEX_SHADOW when set
	bool			mIdle;                  // +0xb1 true; no mouse press for over 12 s; never read (saved only)
	// +0xb2..+0xb3 pad
	int				mAnim;                  // +0xb4 LEXANIM_NONE; LexAnim row playing
	int				mAnimStep;              // +0xb8 0; step in the row
	int				mAnimTick;              // +0xbc 0; updates spent on the step
	int				mFrame;                 // +0xc0 IMAGE_LEX_SMILE_IDLE; image id drawn as the pose / head
	bool			mAnimLoop;              // +0xc4 false
	bool			mSweating;              // +0xc5 false; danger state (sweat drops, no hand cursor, no hint)
	// +0xc6..+0xc7 pad
	int				mSweatFrame;            // +0xc8 0; 0..7, +1 every 6 updates while sweating; OverlayWidget draws that row of IMAGE_FX_LEX_SWEAT_SEQ
	// +0xcc..+0xcf pad; sizeof 0xd0

public:
	Lex(BookWormApp* theApp);                                       // 0x42d560
	virtual ~Lex();                                                 // 0x42d050 (deleting dtor 0x42d650) slot 0

	virtual void	Move(int theNewX, int theNewY) override;        // 0x42d370 slot 14
	virtual void	Draw(Graphics* g) override;                     // 0x42db60 slot 16
	virtual void	Update() override;                              // 0x42dd20 slot 17
	virtual void	MouseEnter() override;                          // 0x42d9e0 slot 23
	virtual void	MouseLeave() override;                          // 0x42d3f0 slot 24
	virtual void	MouseDown(int x, int y, int theClickCount) override;     // 0x42d460 slot 27
	virtual void	MouseUp(int x, int y, int theClickCount) override;       // 0x42d4e0 slot 29
	virtual void	Save(FILE* theFile);                            // 0x42d060 slot 47 (88-byte record, lex.md 8.1)
	virtual void	Load(FILE* theFile);                            // 0x42d1c0 slot 48

	using Widget::MouseDown;
	using Widget::MouseUp;

	void			PlayAnim(int theAnim, bool theLoop);            // 0x42d320
	void			SetSweating(bool isSweating);                   // 0x42d3c0
	void			Reset();                                        // 0x42d680
	void			Say(const SexyString& theText, Font* theFont, int theDuration);     // 0x42d6c0 speech box, 106..213 wide
	void			Think(const SexyString& theText, Font* theFont, int theDuration);   // 0x42d800 thought image
	void			SayDefinition(const SexyString& theWord, Font* theWordFont, const SexyString& theDefinition,
						Font* theDefFont, int theDuration);         // 0x42d8d0 213 wide, word as header

	// 0x404340 (inline, emitted in Board.cpp): an animation plays or Lex is not at his target
	bool			IsBusy()
	{
		return !((mAnim == LEXANIM_NONE) && ((double)mTargetX == mCurX) && ((double)mTargetY == mCurY));
	}

	// 0x403ab0 (inline, emitted in Board.cpp): Lex is not at his target
	bool			IsMoving()
	{
		return !(((double)mTargetX == mCurX) && ((double)mTargetY == mCurY));
	}
};

}

#endif
