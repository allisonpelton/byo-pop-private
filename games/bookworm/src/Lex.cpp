#include "Lex.h"
#include "ModWords.h"
#include "WordBalloon.h"
#include "BookWormApp.h"
#include "UserManager.h"
#include "Board.h"
#include "Piece.h"
#include "StatKeeper.h"
#include "OverlayWidget.h"
#include "Res.h"
#include "GameText.h"
#include "Compat2004.h"
#include "Graphics.h"
#include "Image.h"
#include "Font.h"
#include "ImageFont.h"
#include <cmath>

using namespace Sexy;

// Animation table @0x4fd9f0 (.rdata, [28][50] steps of {image id, duration}). Image ids: 105 smile_idle,
// 106 smile_blink, 107/108 giggle_1/2, 109 slurp_1, 110 speak_ah, 111 speak_ee, 112 speak_oh,
// 113 speak_pb, 114 sweat, 115 sweat_facing, 116 burnt, 117 burnt_blink, 118 eating.
const LexAnimFrame Sexy::gLexAnims[NUM_LEX_ANIMS][LEX_ANIM_STEPS] =
{
	/*  0 */ {{105,1}, {110,8}, {106,6}, {105,3}, {106,3}, {105,2}, {-1,0}},
	/*  1 */ {{105,1}, {110,3}, {105,3}, {110,4}, {111,2}, {105,2}, {110,3}, {112,9}, {105,2}, {-1,0}},
	/*  2 */ {{105,1}, {110,8}, {105,6}, {110,8}, {113,8}, {114,8}, {113,8}, {105,2}, {-1,0}},
	/*  3 */ {{105,1}, {110,5}, {105,6}, {110,8}, {105,2}, {-1,0}},
	/*  4 */ {{105,1}, {110,4}, {105,5}, {112,6}, {110,4}, {105,2}, {-1,0}},
	/*  5 */ {{105,1}, {110,3}, {105,4}, {112,5}, {110,3}, {105,2}, {-1,0}},
	/*  6 */ {{110,3}, {105,3}, {110,4}, {105,4}, {110,4}, {105,4}, {110,4}, {105,3}, {105,5}, {-1,0}},
	/*  7 */ {{113,3}, {112,7}, {111,4}, {110,7}, {105,5}, {-1,0}},
	/*  8 */ {{110,3}, {105,3}, {113,3}, {110,3}, {105,2}, {110,2}, {105,2}, {110,3}, {105,3}, {106,3}, {105,3}, {-1,0}},
	/*  9 */ {{110,2}, {105,2}, {113,2}, {110,2}, {105,2}, {110,2}, {105,1}, {112,3}, {105,3}, {106,3}, {105,3}, {-1,0}},
	/* 10 */ {{105,1}, {110,3}, {105,3}, {112,2}, {105,3}, {110,3}, {105,2}, {110,2}, {105,3}, {-1,0}},
	/* 11 */ {{105,1}, {106,4}, {108,4}, {109,1}, {105,2}, {112,8}, {106,3}, {110,8}, {105,4}, {108,2}, {105,2}, {110,4}, {105,3}, {-1,0}},
	/* 12 */ {{105,1}, {106,2}, {107,2}, {106,2}, {105,1}, {110,4}, {105,3}, {110,4}, {105,3}, {112,6}, {105,2}, {110,3}, {105,3}, {-1,0}},
	/* 13 */ {{106,2}, {108,2}, {107,1}, {112,3}, {110,3}, {105,3}, {110,2}, {105,2}, {110,2}, {105,2}, {110,3}, {105,4}, {110,3}, {105,3}, {-1,0}},
	/* 14 */ {{110,4}, {105,4}, {110,4}, {105,4}, {110,3}, {112,2}, {105,3}, {105,3}, {-1,0}},
	/* 15 */ {{108,1}, {107,1}, {106,1}, {110,4}, {111,4}, {110,5}, {105,4}, {111,2}, {110,5}, {105,5}, {-1,0}},
	/* 16 */ {{105,1}, {111,3}, {110,19}, {113,1}, {112,10}, {110,5}, {105,5}, {-1,0}},
	/* 17 */ {{110,5}, {105,3}, {106,3}, {110,7}, {105,3}, {108,2}, {110,3}, {105,3}, {-1,0}},
	/* 18 */ {{110,4}, {105,5}, {110,5}, {105,4}, {110,4}, {105,5}, {-1,0}},
	/* 19 */ {{114,50}, {115,10}, {-1,0}},
	/* 20 */ {{112,9}, {110,3}, {-1,0}},
	/* 21 */ {{118,100000}, {-1,0}},
	/* 22 */ {{118,1}, {113,5}, {105,1}, {-1,0}},
	/* 23 */ {{118,100000}, {-1,0}},
	/* 24 */ {{116,1}, {-1,0}},
	/* 25 */ {{117,5}, {116,1}, {-1,0}},
	/* 26 */ {{105,1}, {-1,0}},
	/* 27 */ {{106,5}, {105,1}, {-1,0}},
};

// Widget::SetColors tables (r, g, b, a): 0 balloon tint, 1 header, 2 text, 3 footer
static int gLexSayColors[4][4] = {{255,255,255,255}, {0,0,0,255}, {0,0,0,255}, {0,0,0,255}};      // 0x52f428 (Say, SayDefinition)
static int gLexThinkColors[4][4] = {{255,255,255,255}, {0,0,0,255}, {0,0,0,255}, {0,0,0,255}};    // 0x52f468 (Think)

// 0x42d560
Lex::Lex(BookWormApp* theApp)
{
	mApp = theApp;
	mWobble = 0.0;
	mWobbleVel = 0.004;
	mTargetX = 0;
	mTargetY = 0;
	mCurX = 0.0;
	mCurY = 0.0;
	mVelX = 0.0;
	mVelY = 0.0;
	mGlow = 0.0;
	mAnim = LEXANIM_NONE;
	mAnimStep = 0;
	mAnimTick = 0;
	mFrame = IMAGE_LEX_SMILE_IDLE;
	mAnimLoop = false;
	mHasAlpha = true;
	mDoFinger = true;
	mHasTransparencies = true;
	mIdle = true;
	mShowShadow = true;
	mHintPending = false;
	mHintShowing = false;
	mHintLevel = 0.0;
	mHintDelay = 0;     // port: not set by the original; MouseEnter always writes it before Update reads it
	mSweating = false;  // port: the original's SetSweating below compares against an unset byte (no effect)
	SetSweating(false);
	mSweatFrame = 0;
	mMouseInsets.mTop = 28;
	mMouseInsets.mBottom = 18;
}

// 0x42d050
Lex::~Lex()
{
}

// 0x42d060 slot 47
// The 88-byte record in the board's saved game (lex.md 8.1). The hint state and the glow are not saved.
void Lex::Save(FILE* theFile)
{
	fwrite(&mX, 1, 4, theFile);
	fwrite(&mY, 1, 4, theFile);
	fwrite(&mWobble, 1, 8, theFile);
	fwrite(&mWobbleVel, 1, 8, theFile);
	fwrite(&mTargetX, 1, 4, theFile);
	fwrite(&mTargetY, 1, 4, theFile);
	fwrite(&mCurX, 1, 8, theFile);
	fwrite(&mCurY, 1, 8, theFile);
	fwrite(&mVelX, 1, 8, theFile);
	fwrite(&mVelY, 1, 8, theFile);
	fwrite(&mShowShadow, 1, 1, theFile);
	fwrite(&mIdle, 1, 1, theFile);
	fwrite(&mAnim, 1, 4, theFile);
	fwrite(&mAnimStep, 1, 4, theFile);
	fwrite(&mAnimTick, 1, 4, theFile);
	fwrite(&mFrame, 1, 4, theFile);
	fwrite(&mAnimLoop, 1, 1, theFile);
	fwrite(&mSweating, 1, 1, theFile);
	fwrite(&mSweatFrame, 1, 4, theFile);
}

// 0x42d1c0 slot 48
// mX/mY are written directly (no Resize) and mSweating without SetSweating (mDoFinger keeps its value).
void Lex::Load(FILE* theFile)
{
	fread(&mX, 1, 4, theFile);
	fread(&mY, 1, 4, theFile);
	fread(&mWobble, 1, 8, theFile);
	fread(&mWobbleVel, 1, 8, theFile);
	fread(&mTargetX, 1, 4, theFile);
	fread(&mTargetY, 1, 4, theFile);
	fread(&mCurX, 1, 8, theFile);
	fread(&mCurY, 1, 8, theFile);
	fread(&mVelX, 1, 8, theFile);
	fread(&mVelY, 1, 8, theFile);
	fread(&mShowShadow, 1, 1, theFile);
	fread(&mIdle, 1, 1, theFile);
	fread(&mAnim, 1, 4, theFile);
	fread(&mAnimStep, 1, 4, theFile);
	fread(&mAnimTick, 1, 4, theFile);
	fread(&mFrame, 1, 4, theFile);
	fread(&mAnimLoop, 1, 1, theFile);
	fread(&mSweating, 1, 1, theFile);
	fread(&mSweatFrame, 1, 4, theFile);
}

// 0x42d320
// Restarts the row even when it is already playing.
void Lex::PlayAnim(int theAnim, bool theLoop)
{
	mAnim = theAnim;
	mAnimStep = 0;
	mAnimTick = 0;
	mFrame = gLexAnims[theAnim][0].mImage;
	mAnimLoop = theLoop;
	MarkDirty();
}

// 0x42d370 slot 14
// Teleport: the spring target and position jump to the new place (the velocities are kept).
void Lex::Move(int theNewX, int theNewY)
{
	Widget::Move(theNewX, theNewY);
	mTargetX = mX;
	mTargetY = mY;
	mCurX = (double)mX;
	mCurY = (double)mY;
}

// 0x42d3c0
// The sweat drops restart on a change; no hand cursor while sweating.
void Lex::SetSweating(bool isSweating)
{
	if (isSweating != mSweating)
		mSweatFrame = 0;
	mSweating = isSweating;
	mDoFinger = !isSweating;
}

// 0x42d3f0 slot 24
// Skipped while the board is paused (port: the WidgetManager clears mIsOver itself, see MouseEnter).
void Lex::MouseLeave()
{
	if ((mApp->mBoard != NULL) && (mApp->mBoard->mPauseCount > 0))
		return;

	Widget::MouseLeave();
	WordBalloon* aBalloon = mApp->mBoard->mOverlay->mBalloon;
	mHintPending = false;
	if (mHintShowing)
	{
		mHintShowing = false;
		aBalloon->Show(0);
	}
	mHintDelay = (mHintLevel > 1.0) ? 75 : 0;
}

// 0x42d460 slot 27
// Any button: click sound, the glow drops and the hint balloon fades out.
void Lex::MouseDown(int x, int y, int theClickCount)
{
	if ((mApp->mBoard != NULL) && (mApp->mBoard->mPauseCount > 0))
		return;

	Widget::MouseDown(x, y, theClickCount);
	mGlow = 0.0;
	mApp->PlaySample(SOUND_BUTTONCLICK);
	WordBalloon* aBalloon = mApp->mBoard->mOverlay->mBalloon;
	if (mHintShowing)
	{
		mHintShowing = false;
		aBalloon->Show(0);
	}
}

// 0x42d4e0 slot 29
// Released over Lex (any button): scramble (Board::Scramble may refuse with its can't-scramble dialog).
void Lex::MouseUp(int x, int y, int theClickCount)
{
	if ((mApp->mBoard != NULL) && (mApp->mBoard->mPauseCount > 0))
		return;

	Widget::MouseUp(x, y, theClickCount);
	WordBalloon* aBalloon = mApp->mBoard->mOverlay->mBalloon;
	if (mIsOver)
	{
		mApp->mBoard->Scramble();
		if (mHintPending)
			mHintPending = false;
		if (mHintShowing)
		{
			mHintShowing = false;
			aBalloon->Show(0);
		}
	}
}

// 0x42d680
// Called by Board::SaveGame before it writes the record: no animation, idle frame, not sweating.
// Position, glow and hint state are kept.
void Lex::Reset()
{
	mAnim = LEXANIM_NONE;
	mAnimStep = 0;
	mAnimTick = 0;
	mFrame = IMAGE_LEX_SMILE_IDLE;
	mAnimLoop = false;
	SetSweating(false);
	mSweatFrame = 0;
}

// 0x42d6c0
// Speech box as wide as the text (106..213; the width measures the whole string, newlines included),
// centred text. Nothing happens while the balloon is visible: a message never replaces another one.
void Lex::Say(const SexyString& theText, Font* theFont, int theDuration)
{
	OverlayWidget* anOverlay = mApp->mBoard->mOverlay;
	if (anOverlay == NULL)
		return;
	WordBalloon* aBalloon = anOverlay->mBalloon;
	if (aBalloon->mVisible)
		return;

	// port: measured once (the original calls StringWidth up to four times for the clamp)
	int aTextWidth = theFont->StringWidth(theText);
	int aWidth;
	if ((aTextWidth < 106) || (aTextWidth < 214))
		aWidth = (aTextWidth < 106) ? 106 : aTextWidth;
	else
		aWidth = 213;

	aBalloon->SetBoxImages(mApp->mImages[IMAGE_BACKDROP_WB_TEXT], mApp->mImages[IMAGE_WB_TEXT_TALK_TAIL],
		Point((int)(aWidth * 0.75), -2));
	aBalloon->SetText(theText, theFont, 0);
	aBalloon->SetColors(gLexSayColors, 4);
	aBalloon->ClearHeader();
	aBalloon->ClearFooter();
	aBalloon->mTextInsets = Insets(8, 0, 8, 0);
	aBalloon->Resize(0, 0, aWidth, aBalloon->GetPreferredHeight(aWidth));
	aBalloon->Show(theDuration);
}

// 0x42d800
// Thought balloon: the whole backdrop_wb_thought image (188x137 at the balloon's position), centred text.
void Lex::Think(const SexyString& theText, Font* theFont, int theDuration)
{
	OverlayWidget* anOverlay = mApp->mBoard->mOverlay;
	if (anOverlay == NULL)
		return;
	WordBalloon* aBalloon = anOverlay->mBalloon;
	if (aBalloon->mVisible)
		return;

	aBalloon->SetPlainImage(mApp->mImages[IMAGE_BACKDROP_WB_THOUGHT], Insets(20, 25, 20, 40), true);
	aBalloon->SetText(theText, theFont, 0);
	aBalloon->SetColors(gLexThinkColors, 4);
	aBalloon->mTextInsets = Insets(0, 0, 0, 0);
	aBalloon->ClearHeader();
	aBalloon->ClearFooter();
	aBalloon->Show(theDuration);
}

// 0x42d8d0
// A played word's definition: 213-wide speech box, the word as the header, both left-aligned.
void Lex::SayDefinition(const SexyString& theWord, Font* theWordFont, const SexyString& theDefinition,
	Font* theDefFont, int theDuration, const SexyString& theNote)
{
	OverlayWidget* anOverlay = mApp->mBoard->mOverlay;
	if (anOverlay == NULL)
		return;
	WordBalloon* aBalloon = anOverlay->mBalloon;
	if (aBalloon->mVisible)
		return;

	aBalloon->SetBoxImages(mApp->mImages[IMAGE_BACKDROP_WB_TEXT], mApp->mImages[IMAGE_WB_TEXT_TALK_TAIL],
		Point((int)(0.75 * 213.0), -2));
	aBalloon->SetHeader(theWord, theWordFont, -1);
	aBalloon->SetText(theDefinition, theDefFont, -1);
	aBalloon->mLookupURL = ModWordLookupURL(theWord);  // port(mod): a link to the word's definition
	aBalloon->mDoFinger = !aBalloon->mLookupURL.empty();
	aBalloon->SetColors(gLexSayColors, 4);
	aBalloon->ClearFooter();
	if (theNote.size() != 0)
		aBalloon->SetFooter(theNote, theDefFont, -1);   // port(mod): the word review's note (WordReview.h)
	aBalloon->mTextInsets = Insets(12, 0, 12, 0);
	aBalloon->Resize(0, 0, 213, aBalloon->GetPreferredHeight(213));
	aBalloon->Show(theDuration);
}

// port(mod): Lex's comment on a mod's word (rude-words, ModWordReaction) in SayDefinition's box: the word as
// the header, the comment, and the word's definition (if any) under it. Unlike a definition it replaces
// whatever the balloon shows, a hint included.
void Lex::SayReaction(const SexyString& theWord, const SexyString& theReaction, const SexyString& theDefinition,
	int theDuration)
{
	OverlayWidget* anOverlay = mApp->mBoard->mOverlay;
	if (anOverlay == NULL)
		return;
	WordBalloon* aBalloon = anOverlay->mBalloon;
	mHintShowing = false;

	aBalloon->SetBoxImages(mApp->mImages[IMAGE_BACKDROP_WB_TEXT], mApp->mImages[IMAGE_WB_TEXT_TALK_TAIL],
		Point((int)(0.75 * 213.0), -2));
	aBalloon->SetHeader(theWord, mApp->mFonts[FONT_MYRIADWEB12BOLD], -1);
	aBalloon->SetText(theReaction, mApp->mFonts[FONT_MYRIADWEB10], -1);
	aBalloon->ClearFooter();
	if (theDefinition.size() != 0)
		aBalloon->SetFooter(theDefinition, mApp->mFonts[FONT_MYRIADWEB10], -1);
	aBalloon->mLookupURL = ModWordLookupURL(theWord);
	aBalloon->mDoFinger = !aBalloon->mLookupURL.empty();
	aBalloon->SetColors(gLexSayColors, 4);
	aBalloon->mTextInsets = Insets(12, 0, 12, 0);
	aBalloon->Resize(0, 0, 213, aBalloon->GetPreferredHeight(213));
	aBalloon->Show(theDuration);
}

// 0x42d9e0 slot 23
// Arms the scramble hint when one of the 7 top tiles is not a fire tile, the board has fewer than 10 fire
// tiles (Board::Scramble refuses beyond 9), Lex is not sweating and the balloon is free (or shows the
// hint already). Skipped while the board is paused.
// port: the original's skipped Widget::MouseEnter is what set mIsOver (and the finger cursor) in 2004;
// the port's WidgetManager sets them before calling this, paused or not. Not restored here: the port's
// dialog code pauses and unpauses the board in another order around the mouse calls than the 2004 app,
// so undoing it would leave mIsOver stale where the original's was right.
void Lex::MouseEnter()
{
	if ((mApp->mBoard != NULL) && (mApp->mBoard->mPauseCount > 0))
		return;

	Widget::MouseEnter();
	Board* aBoard = mApp->mBoard;
	if ((aBoard->mFireTileCount < 10) && ((!aBoard->mOverlay->mBalloon->mVisible) || (!mHintShowing)))
	{
		// the top tile of each column, columns 0..6 (even columns start one row lower)
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			Piece* aPiece = aBoard->mBoard[(aCol % 2 == 0) ? 1 : 0][aCol];
			if ((aPiece != NULL) && (aPiece->mType != TILE_FIRE) && (!mSweating))
				mHintPending = true;
		}
	}
	mHintDelay = (mHintLevel > 1.0) ? 75 : 0;
}

// 0x42db60 slot 16
// Poses are 101x202 images drawn 6 pixels down in the 107x208 widget (the eating pose fills it): the body
// (lex_headless, or lex_burnt on a burnt board), the idle face under a blink, the mouse-over glow, then the
// frame. A burnt idle Lex draws lex_burnt twice (body and frame), as the original does.
void Lex::Draw(Graphics* g)
{
	int aYOffset;
	if (mFrame == IMAGE_LEX_EATING)
		aYOffset = 0;
	else
		aYOffset = mApp->mImages[IMAGE_LEX_EATING]->GetHeight() - mApp->mImages[IMAGE_LEX_SMILE_IDLE]->GetHeight();

	if (mApp->mBoard->mBurnedDown)
		g->DrawImage(mApp->mImages[IMAGE_LEX_BURNT], 0, aYOffset);
	else if ((mFrame != IMAGE_LEX_SMILE_IDLE) && (mFrame != IMAGE_LEX_EATING))
		g->DrawImage(mApp->mImages[IMAGE_LEX_HEADLESS], 0, aYOffset);

	if (mFrame == IMAGE_LEX_SMILE_BLINK)
		g->DrawImage(mApp->mImages[IMAGE_LEX_SMILE_IDLE], 0, aYOffset);

	if ((mGlow > 0.0) && (!mApp->mBoard->mBurnedDown) && (!IsBusy()))
	{
		g->SetColorizeImages(true);
		double anAlpha = mGlow * 255.0;
		if (anAlpha < 0.0)
			anAlpha = 0.0;
		else if (anAlpha > 255.0)
			anAlpha = 255.0;
		g->SetColor(Color(255, 255, 255, (int)anAlpha));
		g->DrawImage(mApp->mImages[IMAGE_LEX_GLOW_IDLE], 0, aYOffset - 6);
		g->SetColorizeImages(false); // the colour is left as it is
	}

	g->DrawImage(mApp->mImages[mFrame], 0, aYOffset);
}

// 0x42dd20 slot 17
// Nothing at all while the board is paused. Otherwise: the scramble hint, the idle latch, the spring
// that slides Lex to his target, the sweat drops, the mouse-over glow (which holds the animations), the
// animation step and the random idle animations.
void Lex::Update()
{
	if ((mApp->mBoard != NULL) && (mApp->mBoard->mPauseCount > 0))
		return;

	Widget::Update();

	// The scramble hint, once the delay armed by MouseEnter has run out and the balloon is free: the long
	// text greeting the player by name while mHintLevel <= 1.0 (each hint adds the game's scramble count,
	// the long one 0.2 more), else the short thought (also when there is no player name).
	if (mHintPending && (!mApp->mBoard->mOverlay->mBalloon->mVisible))
	{
		if (mHintDelay > 0)
			mHintDelay--;
		else
		{
			mHintPending = false;
			SexyString aName = mApp->mUserManager->GetCurrentUserName();
			mHintLevel = (double)mApp->mBoard->mGameStats->mScrambles + mHintLevel;
			if ((mHintLevel > 1.0) || (aName.length() == 0))
			{
				Think(GameText(GT_LEX_SCRAMBLE_SHORT), mApp->mFonts[FONT_VERDANA14BOLD], -1);
				mHintDelay = 150;
			}
			else
			{
				Say(StrFormat(GameTextC(GT_LEX_SCRAMBLE_LONG_FORMAT), aName.c_str()), mApp->mFonts[FONT_VERDANA12BOLD], -1);
				mApp->mBoard->mOverlay->mBalloon->SetJustification(0, 0, 0);
				mHintLevel = mHintLevel + 0.2;
			}
			mHintShowing = true;
		}
	}

	bool isBurnt = (mApp->mBoard != NULL) && mApp->mBoard->mBurnedDown;

	// Idle latch (12 s without a mouse press anywhere): nothing reads mIdle; kept for the saved record.
	// port: GameTickCount() (the framework clock, repeatable under -fast); a press tick of 0 means no press
	// yet, which the original's GetTickCount() (milliseconds since boot) always measured as idle
	DWORD aLastPress = GetLastMouseDownTick(mWidgetManager);
	if ((aLastPress != 0) && ((DWORD)(GameTickCount() - aLastPress) <= 12000))
		mIdle = false;
	else if ((!mIdle) && (mApp->mBoard != NULL) && (!mApp->mBoard->mBurnedDown) && (!IsBusy()))
		mIdle = true;

	// Springs (all double). The wobble is integrated but never drawn.
	double aWobble = mWobble + mWobbleVel;
	mWobble = aWobble;
	mWobbleVel = mWobbleVel - aWobble * 0.0004;
	double aWobbleDamped = aWobble * 0.99;
	mWobble = aWobbleDamped;

	double anAccelX = ((double)mTargetX - mCurX) * 0.0515;
	mVelX = anAccelX + mVelX;
	if (fabs(mVelX) <= 0.1)
	{
		mCurX = (double)mTargetX;
		mVelX = 0.0;
	}
	else
		mVelX = mVelX * 0.54;
	mWobble = anAccelX * 0.01 + aWobbleDamped;

	double aVelY = ((double)mTargetY - mCurY) * 0.0515 + mVelY;
	mVelY = aVelY;
	if (fabs(aVelY) <= 0.1)
	{
		mCurY = (double)mTargetY;
		mVelY = 0.0;
	}
	else
		mVelY = aVelY * 0.54;

	mCurX = mCurX + mVelX;
	mCurY = mCurY + mVelY;
	if ((fabs(mVelX) > 1.0) || (fabs(mVelY) > 1.0))
		MarkDirtyFull();
	Resize((int)mCurX, (int)mCurY, mWidth, mHeight);

	// Sweat drops: the next of 8 rows every 6 updates
	if (mSweating && (mUpdateCnt % 6 == 0))
	{
		mSweatFrame = (mSweatFrame + 1) % 8;
		MarkDirty();
	}

	// Mouse-over glow, 0.075 per update, idle face. While it rises or fades, the animation does not
	// advance and no random animation starts (a playing one resumes afterwards).
	if (!mIsOver)
	{
		if (mGlow > 0.0)
		{
			double aGlow = mGlow - 0.075;
			if (aGlow < 0.0)
				aGlow = 0.0;
			else if (aGlow > 1.0)
				aGlow = 1.0;
			mGlow = aGlow;
			mFrame = IMAGE_LEX_SMILE_IDLE;
			MarkDirty();
			return;
		}
	}
	else if ((!isBurnt) && (!IsBusy()))
	{
		double aGlow = mGlow + 0.075;
		if (aGlow < 0.0)
			aGlow = 0.0;
		else if (aGlow > 1.0)
			aGlow = 1.0;
		mGlow = aGlow;
		mFrame = IMAGE_LEX_SMILE_IDLE;
		MarkDirty();
		return;
	}

	// Animation: a step of duration d shows for 2d updates
	if (mAnim == LEXANIM_NONE)
		mFrame = isBurnt ? IMAGE_LEX_BURNT : IMAGE_LEX_SMILE_IDLE;  // no MarkDirty
	else
	{
		mAnimTick++;
		if (mAnimTick >= gLexAnims[mAnim][mAnimStep].mDuration * 2)
		{
			mAnimStep++;
			mAnimTick = 0;
			mFrame = gLexAnims[mAnim][mAnimStep].mImage;
			if ((mFrame == -1) && mAnimLoop)
			{
				mAnimStep = 0;
				mFrame = gLexAnims[mAnim][0].mImage;
			}
			if (mFrame == -1)
			{
				mAnim = LEXANIM_NONE;
				mFrame = mApp->mBoard->mBurnedDown ? IMAGE_LEX_BURNT : IMAGE_LEX_SMILE_IDLE;
			}
			MarkDirty();
		}
	}

	// Random idle animations from the C library's rand(): 2 draws per update, a third one when the
	// second hits and Lex is free on an unburnt board.
	if ((CrtRand() % 500 == 0) && (!IsBusy()))
		PlayAnim(((mApp->mBoard != NULL) && mApp->mBoard->mBurnedDown) ? LEXANIM_BURNT_BLINK : LEXANIM_BLINK, false);

	if ((CrtRand() % 4000 == 0) && (!IsBusy()) && ((mApp->mBoard == NULL) || (!mApp->mBoard->mBurnedDown)))
		PlayAnim(LEXANIM_IDLE + CrtRand() % 2, false);
}
