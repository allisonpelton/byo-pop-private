#include "PlayMenu.h"

#include "Bej3App.h"
#include "Color.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "MainMenu.h"
#include "Profile.h"
#include "Res.h"
#include "WidgetManager.h"
#include "X86Math.h"

#include <SDL.h>

#include <cmath>
#include <vector>

namespace Sexy {

namespace {

// The exe's measures are in 1200ths of the art size in use
int Scaled(int theMeasure)
{
	return gApp->mArtRes * theMeasure / 0x4b0;
}

// 0x475e30: the text of a numbered string in default.xml
std::string TextById(int theId, const std::string& theDefault)
{
	return gApp->GetString(std::to_string(theId), theDefault);
}

// 0x436200: a colour from an ARGB int (an alpha of 0 is taken as 255)
Color ColorFromInt(int theValue)
{
	int anAlpha = (theValue >> 24) & 0xff;
	return Color((theValue >> 16) & 0xff, (theValue >> 8) & 0xff, theValue & 0xff, anAlpha == 0 ? 255 : anAlpha);
}

// 0x6738c0: the ARGB int a third of the way (theBlend) from theFrom to theTo, the alpha of theFrom kept
int BlendColors(int theFrom, int theTo, float theBlend)
{
	float anInverse = 1.0f - theBlend;
	int aRed = (int)std::lrint((float)((theTo >> 16) & 0xff) * theBlend + anInverse * (float)((theFrom >> 16) & 0xff));
	int aGreen = (int)std::lrint((float)((theTo >> 8) & 0xff) * theBlend + (float)((theFrom >> 8) & 0xff) * anInverse);
	int aBlue = (int)std::lrint(anInverse * (float)(theFrom & 0xff) + (float)(theTo & 0xff) * theBlend);
	return (int)((unsigned)(((aRed << 8) | aGreen) << 8 | aBlue) | ((unsigned)theFrom & 0xff000000u));
}

int ColorToInt(const Color& theColor)
{
	return (theColor.mAlpha << 24) | (theColor.mRed << 16) | (theColor.mGreen << 8) | theColor.mBlue;
}

// 0x43b940: the text with its %d, %s and %S each given the next argument
std::string FormatText(const std::string& theFormat, const std::vector<std::string>& theArgs)
{
	std::string aResult;
	size_t anArg = 0;
	for (size_t i = 0; i < theFormat.size(); i++) {
		if (theFormat[i] == '%' && i + 1 < theFormat.size() &&
			(theFormat[i + 1] == 'd' || theFormat[i + 1] == 's' || theFormat[i + 1] == 'S')) {
			if (anArg < theArgs.size())
				aResult += theArgs[anArg++];
			i++;
		}
		else
			aResult += theFormat[i];
	}
	return aResult;
}

// the sound the mouse over the ball makes (the four SECRETMOUSEOVER sounds, one for each pair of balls' place)
int HoverSound(int theBall)
{
	switch (theBall % 4) {
	case 0: return SOUND_SECRETMOUSEOVER1;
	case 1: return SOUND_SECRETMOUSEOVER2;
	case 2: return SOUND_SECRETMOUSEOVER3;
	default: return SOUND_SECRETMOUSEOVER4;
	}
}

// 0x792bf0: the exe's float to int conversion (a truncation on a machine with SSE2)
int Truncated(double theValue)
{
	return Ftol(theValue);
}

}  // namespace

// 0x680760 (bejeweled3). port: see the header for what is left out
PlayMenu::PlayMenu(MainMenu* theMainMenu) : mMainMenu(theMainMenu), mBackButton(nullptr), mQuitButton(nullptr)
{
	mClip = false;   // +0x51
	mVisible = false;   // +0x64

	const char* aModeNames[BALL_COUNT] = {"CLASSIC", "ZEN", "LIGHTNING", "QUEST", "POKER", "BUTTERFLIES", "ICE STORM",
		"DIAMOND MINE"};
	std::string aModeTexts[BALL_COUNT];
	for (int i = 0; i < BALL_COUNT; i++)
		aModeTexts[i] = TextById(0x158 + i, aModeNames[i]);

	// the balls are made last to first, so the first is the one on top
	for (int i = BALL_COUNT - 1; i >= 0; i--) {
		mBalls[i] = new CrystalBall(i, this, aModeTexts[i]);
		mBalls[i]->mScaleCurve.SetConstant(i < FIRST_LOCKED_BALL ? (double)0.2f : (double)0.1f);
		if (i == 0)
			mFirstBallColor = ColorToInt(mBalls[0]->mTextColor);
		AddWidget(mBalls[i]);
	}
	mVisible = false;

	// the buttons are placed where the resources put their pictures (x= and y= of the image), a cel of the picture in
	// size, and show cel 1 at rest and cel 0 over and pressed
	mBackButton = new ButtonWidget(ID_BACK, this);
	mBackButton->Resize(Scaled(GetImageOffsetX(IMAGE_OVERWORLD_BACK_BUTTON_ID)),
		Scaled(GetImageOffsetY(IMAGE_OVERWORLD_BACK_BUTTON_ID)), IMAGE_OVERWORLD_BACK_BUTTON->GetCelWidth(),
		IMAGE_OVERWORLD_BACK_BUTTON->GetCelHeight());
	mBackButton->mButtonImage = IMAGE_OVERWORLD_BACK_BUTTON;
	mBackButton->mNormalRect = IMAGE_OVERWORLD_BACK_BUTTON->GetCelRect(1);
	mBackButton->mOverRect = IMAGE_OVERWORLD_BACK_BUTTON->GetCelRect(0);
	mBackButton->mDownRect = IMAGE_OVERWORLD_BACK_BUTTON->GetCelRect(0);
	mBackButton->mHasTransparencies = true;   // +0x94
	AddWidget(mBackButton);

	mQuitButton = new ButtonWidget(ID_QUIT, this);
	mQuitButton->Resize(Scaled(GetImageOffsetX(IMAGE_MAIN_MENU_QUIT_BUTTON_ID)),
		Scaled(GetImageOffsetY(IMAGE_MAIN_MENU_QUIT_BUTTON_ID)), IMAGE_MAIN_MENU_QUIT_BUTTON->GetCelWidth(),
		IMAGE_MAIN_MENU_QUIT_BUTTON->GetCelHeight());
	mQuitButton->mButtonImage = IMAGE_MAIN_MENU_QUIT_BUTTON;
	mQuitButton->mNormalRect = IMAGE_MAIN_MENU_QUIT_BUTTON->GetCelRect(1);
	mQuitButton->mOverRect = IMAGE_MAIN_MENU_QUIT_BUTTON->GetCelRect(0);
	mQuitButton->mDownRect = IMAGE_MAIN_MENU_QUIT_BUTTON->GetCelRect(0);
	mQuitButton->mHasTransparencies = true;
	AddWidget(mQuitButton);

	mFadeCurve.SetConstant(0.0);
	mAuxCurveA.SetCurve("b+0,1,0,1,####       8####  i~###");
	mAuxCurveB.SetConstant(0.0);
	for (int i = 0; i < 4; i++)
		mAuxCurves[i].SetConstant(0.0);
	mSelectedBall = 0;
	mChosenMode = -1;
	mAuxCurveC.SetConstant(0.0);
}

// 0x696ad0 (bejeweled3). port: see the header
void PlayMenu::Update()
{
	Widget::Update();
	if (!mMainMenu->mIntroStarted)
		return;

	double aFade = mFadeCurve.GetOutVal();
	mVisible = !(aFade <= 0.0 || mHideCurve.GetOutVal() == 1.0);

	// the mouse reaches the balls and buttons only once none of the balls is changing its size
	bool aAllIdle = true;
	for (int i = 0; i < BALL_COUNT; i++) {
		if (mBalls[i]->mScaleCurve.IsDoingCurve())
			aAllIdle = false;
	}
	mBackButton->mMouseVisible = aAllIdle;
	mQuitButton->mMouseVisible = aAllIdle;
	for (int i = 0; i < BALL_COUNT; i++)
		mBalls[i]->mMouseVisible = aAllIdle;

	if (mFadeCurve.GetOutVal() != 0.0) {
		if (mFadeCurve.GetOutVal(mFadeCurve.mInMax) == 1.0 && mFadeCurve.HasBeenTriggered())
			mWidgetManager->BringToFront(this);
		if (mFadeCurve.GetOutVal(mFadeCurve.mInMax) == 0.0)
			HideLabel();

		const int aBallSize = Scaled(0xb4);
		for (int i = 0; i < BALL_COUNT; i++) {
			double aBallX, aBallY;
			if (i < FIRST_LOCKED_BALL) {
				// the four modes that are open from the start: two to a side, the first pair above the second
				aBallY = Scaled(0xfa);
				aBallX = Scaled(0x1cc);
				if (i % 2 == 1)
					aBallX = mWidth - aBallX;
				if (i > 1)
					aBallY = Scaled(0x28a) + aBallY;
				double aFadeNow = mFadeCurve.GetOutVal();
				aBallX = aBallX + (aBallX - (double)(mWidth / 2)) * (1.0 - aFadeNow) * 1.25;
				aBallY = aBallY + 1.25 * (aBallY - (double)(mHeight / 2)) * (1.0 - aFadeNow);
			}
			else {
				// the four that start locked: smaller, in the middle, two rows of two
				CrystalBall* aBall = mBalls[i];
				aBall->mColor = ColorFromInt(0xff77ff);
				aBall->mTextColor = Color(0x22, 0x22, 0x22, 0xa0);
				aBallY = Scaled(0x1a4);
				aBallX = Scaled(0x2c7);
				int aLockedIndex = i - FIRST_LOCKED_BALL;
				if (aLockedIndex % 2 == 1)
					aBallX = (double)Scaled(4) + ((double)mWidth - aBallX);
				int aRowFactor;
				if (aLockedIndex < 2)
					aRowFactor = 3;
				else {
					aRowFactor = 4;
					aBallY = (double)Scaled(0x127) + aBallY;
				}
				double aFadeNow = mFadeCurve.GetOutVal();
				double aNewX = (1.0 - aFadeNow) * 3.0 * (aBallX - (double)(mWidth / 2)) * 1.25 + aBallX;
				aFadeNow = mFadeCurve.GetOutVal();
				double aNewY = (aBallY - (double)(mHeight / 2)) * aRowFactor * (1.0 - aFadeNow) * 1.25 + aBallY;
				aBallX = aNewX;
				aBallY = aNewY;
				// the flags at +0x180 of the profile say which of the four have been opened
				bool isLocked = gApp->mProfile->mTutorialFlags[aLockedIndex] == 0;
				aBall->mDrawPicture = isLocked;   // the question mark in place of the name
				aBall->mFlat = isLocked;
				aBall->mMoves = false;
				aBall->mColor = ColorFromInt(0xaaaaaa);
			}
			if (i == mSelectedBall) {
				// the ball that was chosen is drawn towards the middle as the +0x490 curve goes up
				double aHide = mHideCurve.GetOutVal();
				double aTowardsX = aBallX * (1.0 - aHide);
				double aTowardsY = (1.0 - aHide) * aBallY;
				aHide = mHideCurve.GetOutVal();
				aBallX = (double)(mWidth / 2) * aHide + aTowardsX;
				aBallY = aHide * (double)(mHeight / 2) + aTowardsY;
			}
			int aRoundedX = Truncated(aBallX);
			int aRoundedY = Truncated(aBallY);
			mBalls[i]->Resize(aRoundedX - aBallSize / 2, aRoundedY - aBallSize / 2, aBallSize, aBallSize);
			mBalls[i]->mSubpixelX = aBallX - (double)aRoundedX;
			mBalls[i]->mSubpixelY = aBallY - (double)aRoundedY;
		}

		if (mFadeCurve.GetOutVal() == 1.0 && mVisible) {
			if (mAuxCurveB.GetOutVal() == 0.0) {
				// nothing is being said of a mode, and the player has never played: the hint comes up
				if (gApp->mProfile->mStats[0] == 0 && !mAuxCurveC.IsDoingCurve() && mAuxCurveC.GetOutVal() == 0.0)
					mAuxCurveC.SetCurve("b;0,1,0.01,1,####         ~~###");
			}
			else if (!mAuxCurveC.IsDoingCurve())
				mAuxCurveC.SetCurveMult("b;0,1,0.05,1,~###         ~####");
		}
		else
			mAuxCurveC.SetConstant(0.0);
		MarkDirty();
	}
}

// 0x689d20 (bejeweled3): the balls' colours and the text under them. port: see the header
void PlayMenu::Draw(Graphics* g)
{
	if (mFadeCurve.GetOutVal() == 0.0)
		return;

	for (int i = 0; i < 4; i++) {
		float aRed = (float)mAuxCurves[i].GetOutVal();
		mBalls[i]->mTextColor = ColorFromInt(BlendColors(mFirstBallColor, 0xff0055, aRed));
		if (mAuxCurveC.GetOutVal() != 0.0 && i == 0) {
			// the hint makes the first ball's name pulse
			float aCos = std::cos((float)((double)mUpdateCnt * (double)0.075f));
			float aPulse = (float)(((double)0.5 + (double)aCos * 0.5) * mAuxCurveC.GetOutVal());
			mBalls[0]->mTextColor = ColorFromInt(BlendColors(ColorToInt(mBalls[0]->mTextColor), 0xff6090, aPulse));
		}
		mBalls[i]->mTextScaleAdd = (float)mAuxCurves[i].GetOutVal();
	}

	if (!(mAuxCurveB.GetOutVal() > 0.0) && mAuxCurveC.GetOutVal() == 0.0)
		return;

	g->SetColor(Color(255, 255, 255, Truncated(mAuxCurveB.GetOutVal() * 255.0)));
	g->SetColorizeImages(true);

	int aExtraLines = 0;
	if (mModeLocked) {
		// "SECRET GAME" over a locked mode's text
		g->SetFont(FONT_OVERWORLD_HOVER_LOCKED);
		Color anOldMain, anOldGlow, anOldOutline;
		SetLayerColor(FONT_OVERWORLD_HOVER_LOCKED, "MAIN", ColorFromInt(0xff6666), &anOldMain);
		SetLayerColor(FONT_OVERWORLD_HOVER_LOCKED, "GLOW", ColorFromInt(0), &anOldGlow);
		SetLayerColor(FONT_OVERWORLD_HOVER_LOCKED, "OUTLINE", ColorFromInt(0), &anOldOutline);
		g->WriteString(TextById(0x160, "SECRET GAME"), mWidth / 2, mHeight / 2 - Scaled(0x28), 0, 0, true, 0, -1, -1);
		RestoreLayerColor(FONT_OVERWORLD_HOVER_LOCKED, "MAIN", anOldMain);
		RestoreLayerColor(FONT_OVERWORLD_HOVER_LOCKED, "GLOW", anOldGlow);
		RestoreLayerColor(FONT_OVERWORLD_HOVER_LOCKED, "OUTLINE", anOldOutline);
		aExtraLines = 0xc;
	}

	// what the mode is, in the hover font with a black glow and outline
	g->SetFont(FONT_OVERWORLD_HOVER);
	Color anOldGlow, anOldOutline;
	SetLayerColor(FONT_OVERWORLD_HOVER, "GLOW", ColorFromInt(0), &anOldGlow);
	SetLayerColor(FONT_OVERWORLD_HOVER, "OUTLINE", ColorFromInt(0), &anOldOutline);
	const int aTextY = gApp->mArtRes * (aExtraLines + 10) / 0x4b0 + mHeight / 2;
	g->WriteString(mModeText, mWidth / 2, aTextY, 0, 0, true, 0, -1, -1);

	if (mAuxCurveC.GetOutVal() != 0.0) {
		g->SetColor(Color(255, 255, 255, Truncated(mAuxCurveC.GetOutVal() * 255.0)));
		g->WriteString(TextById(0x236, "New to Bejeweled? Try Classic mode"), mWidth / 2, aTextY, 0, 0, true, 0, -1, -1);
	}
	RestoreLayerColor(FONT_OVERWORLD_HOVER, "GLOW", anOldGlow);
	RestoreLayerColor(FONT_OVERWORLD_HOVER, "OUTLINE", anOldOutline);
	g->SetColorizeImages(false);
	g->SetColor(Color(255, 255, 255, 255));
}

// 0x69b360 (bejeweled3). port: the exe, with the fade at 0 ignoring the click, starts the mode (CLASSIC, ZEN, LIGHTNING,
// QUEST and the four that are open) or asks why the mode is locked; BACK takes the screen away again and QUIT asks the
// app whether to quit. Here every click only says which it was.
void PlayMenu::ButtonDepress(int theId)
{
	if (mFadeCurve.GetOutVal(mFadeCurve.mInMax) == 0.0)
		return;
	if (theId == ID_BACK)
		SDL_Log("PlayMenu: BACK");
	else if (theId == ID_QUIT)
		SDL_Log("PlayMenu: QUIT");
	else if (theId >= 0 && theId < BALL_COUNT) {
		bool isLocked = theId >= FIRST_LOCKED_BALL && gApp->mProfile->mTutorialFlags[theId - FIRST_LOCKED_BALL] == 0;
		SDL_Log("PlayMenu: mode %d (%s)%s", theId, mBalls[theId]->mText.c_str(), isLocked ? ", locked" : "");
		// 0x69b360: the field at +0x3cc and the app's +0xc31 are cleared, then the mode's start runs. port: only
		// Classic (0x7737d0), Zen (0x773100), Lightning (0x773440), Quest (0x773b70) and the secret modes (0x774060) are made; the others log
		gApp->mSavedGameChosen = false;
		if (theId == 0)
			gApp->StartClassic();
		else if (theId == 1)
			gApp->StartZen();
		else if (theId == 2)
			gApp->StartLightning();
		else if (theId == 3)
			gApp->ShowQuestMenu(false);   // 0x773b70: the quest map
		else if (theId >= FIRST_LOCKED_BALL && !isLocked)
			gApp->StartSecretMode(theId - 4);   // 0x774060: the balls 4 to 7 (Poker, Butterflies, Ice Storm, Diamond Mine) are the secret quests 0 to 3; a locked ball asks why instead (not made)
	}
}

// 0x68e0e0 (bejeweled3): the mouse is over a ball: its sound, and what the mode is said in the label (a locked mode
// says what opens it, and turns its ball red)
void PlayMenu::ButtonMouseEnter(int theId)
{
	std::string aText;
	bool isLocked = false;
	int aRedBall = -1;
	const std::string aColor = "ff93c0";

	if (theId == ID_BACK || theId == ID_QUIT)
		gApp->PlaySoundEx(SOUND_BUTTON_MOUSEOVER, 1.0, 0, 0.0);
	else if (theId >= 0 && theId < BALL_COUNT) {
		gApp->PlaySoundEx(HoverSound(theId), 0.5, 0, theId < FIRST_LOCKED_BALL ? -4.0 : 0.0);
		switch (theId) {
		case 0: aText = TextById(0x16d, "Play the original untimed game"); break;
		case 1: aText = TextById(0x16e, "Relax body and mind in this endless game"); break;
		case 2: aText = TextById(0x16f, "Collect Time Gems to charge up bonus rounds"); break;
		case 3: aText = TextById(0x170, "Solve 40 different puzzling minigames"); break;
		case 4:
			if (gApp->mProfile->mTutorialFlags[0] == 0) {
				aText = FormatText(TextById(0x172, "Reach level %d of ^%S^Classic Mode^oldclr^ to unlock"), {"5", aColor});
				isLocked = true;
				aRedBall = 0;
			}
			else
				aText = TextById(0x171, "Match gems and make your best hand");
			break;
		case 5:
			if (gApp->mProfile->mTutorialFlags[1] == 0) {
				aText = FormatText(TextById(0x174, "Reach level %d of ^%S^Zen Mode^oldclr^ to unlock"), {"5", aColor});
				isLocked = true;
				aRedBall = 1;
			}
			else
				aText = TextById(0x173, "Collect colorful jeweled butterflies");
			break;
		case 6:
			if (gApp->mProfile->mTutorialFlags[2] == 0) {
				// 0x43b2e0 (through 0x5a8f10 and 0x5a8fb0) writes 100000 (0x186a0) with its thousands grouped by the
				// locale's separator: CommaSeperate, as the English game shows it
				aText = FormatText(TextById(0x176, "Score %s points in ^%S^Lightning Mode^oldclr^ to unlock"),
					{CommaSeperate(100000), aColor});
				isLocked = true;
				aRedBall = 2;
			}
			else
				aText = TextById(0x175, "Hold back the rising ice columns");
			break;
		default:
			if (gApp->mProfile->mTutorialFlags[3] == 0) {
				aText = FormatText(TextById(0x178, "Reveal the first Relic in ^%S^Quest Mode^oldclr^ to unlock"), {aColor});
				isLocked = true;
				aRedBall = 3;
			}
			else
				aText = TextById(0x177, "Dig deep to unearth buried treasure");
			break;
		}
	}

	if (mFadeCurve.GetOutVal(mFadeCurve.mInMax) == 0.0 || aText.empty()) {
		HideLabel();
	}
	else {
		mModeText = aText;
		mModeLocked = isLocked;
		if (!mModeText.empty()) {
			double aLabelAlpha = mAuxCurveB.GetOutVal();
			if (aLabelAlpha == 1.0)
				mAuxCurveB.SetConstant(aLabelAlpha);
			else if (mAuxCurveB.GetOutVal(mAuxCurveB.mInMax) == 0.0)
				mAuxCurveB.SetCurve("b;0,1,0.01,0.25,####         ~~###");
			for (int i = 0; i < 4; i++) {
				double aFinal = mAuxCurves[i].GetOutVal(mAuxCurves[i].mInMax);
				if (i == aRedBall) {
					if (aFinal != 1.0)
						mAuxCurves[i].SetCurveSmooth("b;0,1,0.01,0.25,####         ~~###", 0.01);
				}
				else if (aFinal != 0.0)
					mAuxCurves[i].SetCurveSmooth("b;0,1,0.01,0.25,~###  v~###       +#G8)", 0.01);
			}
		}
	}
	mAuxCurveC.SetConstant(mAuxCurveC.GetOutVal());
}

// 0x6847b0 (bejeweled3)
void PlayMenu::ButtonMouseLeave(int theId)
{
	HideLabel();
}

// 0x684660 (bejeweled3)
void PlayMenu::HideLabel()
{
	for (int i = 0; i < 4; i++) {
		if (mAuxCurves[i].GetOutVal(mAuxCurves[i].mInMax) != 0.0)
			mAuxCurves[i].SetCurve("b;0,1,0.01,0.25,~###  v~###       +#G8)");
	}
	if (mAuxCurveB.GetOutVal(mAuxCurveB.mInMax) == 1.0)
		mAuxCurveB.SetCurve("b;0,1,0.01,0.55,~###  v~###       +#G8*");
}

bool PlayMenu::IsActive()
{
	return mFadeCurve.GetOutVal() != 0.0 || mFadeCurve.GetOutVal(mFadeCurve.mInMax) == 1.0;
}

}  // namespace Sexy
