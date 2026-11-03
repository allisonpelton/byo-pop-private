#include "ConversationPanel.h"

#include <SDL.h>

#include <cstdlib>

#include "Animator.h"
#include "BattleEngine.h"
#include "Board.h"
#include "Font.h"
#include "GameApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "LuaBind.h"
#include "LuaImage.h"
#include "LuaInterface.h"
#include "LuaWidgetTpl.h"
#include "Profile.h"
#include "Res.h"
#include "SoundPlayer.h"
#include "WidgetManager.h"
#include "lauxlib.h"
#include "lua.h"

using namespace Sexy;
using namespace LuaLib;

// ---- the panels ----

ConversationPanel::Panel::Panel() : mTextColor(Color::White) {}

ConversationPanel::Panel::~Panel()
{
	delete mAnim;
	for (TextPage* aPage : mPages)
		delete aPage;
}

ConversationPanel::TextPage* ConversationPanel::Panel::CurrentPage() const
{
	return mPageIndex < mPages.size() ? mPages[mPageIndex] : mPages.back();
}

// ---- the widget ----

ConversationPanel::ConversationPanel(GameApp* theApp) : mApp(theApp)
{
	mScreenColor = Color(0, 0, 0, 0xaf);
	mBeamColor = Color::White;
	mHasAlpha = true;
	mHasTransparencies = true;
	SetVisible(false);
	SetDisabled(true);
	mBeamX = mPanelX;
	mBeamTarget = mPanelW / 2 + mPanelX;
	AddNewPanel();
	mPriority = 9999;
	mOldPriority = 9999;
}

ConversationPanel::~ConversationPanel()
{
	for (Panel* aPanel : mPanels)
		delete aPanel;
}

void ConversationPanel::SetDrawPriority(int thePriority)
{
	mOldPriority = mPriority;
	mPriority = thePriority;
}

void ConversationPanel::SetDefaults(int theSide, const Color& theInner, Image* thePortrait, Font* theFont,
									int theAnimDelay)
{
	int aHeight = mApp->mHeight / 3 + 60;
	SetDimensions(0, mApp->mHeight / 2 - aHeight / 2, mApp->mWidth, aHeight);
	SetScreenColor(Color(0, 0, 0, 0x80));
	SetBeamColor(Color::White);
	SetOuterColor(Color(0, 0, 0, 0x80));
	SetMiddleColor(Color::Black);
	SetInnerColor(theInner);
	SetAnim(thePortrait, theAnimDelay);
	AlignAnim(theSide, true);
	SetLetterDelay(5);
	SetFont(theFont, Color::White);
	SlidePortrait(theSide);
	SetTextRectBeside(theSide);
}

// The side's x the portrait slides to: 20 from the screen's left (not the panel's), or 20 in from the panel's right
static int PortraitTarget(const ConversationPanel* thePanel, Image* theImage, int theSide)
{
	if (theSide < 0)
		return 20;
	return thePanel->mPanelW + thePanel->mPanelX - (theImage != nullptr ? theImage->mWidth : 0) - 20;
}

void ConversationPanel::SlidePortrait(int theSide)
{
	Panel* p = LastPanel();
	Image* anImage = p->mAnim->mImage;
	p->mTargetX = PortraitTarget(this, anImage, theSide);
	if (theSide < 0)
		p->mAnim->mX = (float)(mPanelX - (anImage != nullptr ? anImage->mWidth : 0));
	else
		p->mAnim->mX = (float)(mPanelW + mPanelX);
	p->mArrived = false;
}

void ConversationPanel::DontSlidePortrait(int theSide)
{
	Panel* p = LastPanel();
	int aTarget = PortraitTarget(this, p->mAnim->mImage, theSide);
	p->mAnim->mX = (float)aTarget;
	p->mArrived = true;
	p->mTargetX = aTarget;
}

void ConversationPanel::SetTextRect(int theX, int theY, int theW, int theH)
{
	LastPanel()->mTextRect = Rect(theX, theY, theW, theH);
}

void ConversationPanel::SetTextRectBeside(int theSide)
{
	Panel* p = LastPanel();
	int x, w;
	if (theSide < 0) {
		x = p->mTargetX + 20 + (p->mAnim->mImage != nullptr ? p->mAnim->mImage->GetCelWidth() : 0);
		w = mPanelW - x - 10;
	} else {
		w = p->mTargetX - mPanelX - 20;
		x = mPanelX + 20;
	}
	SetTextRect(x, mPanelY + 70, w, 400);
}

void ConversationPanel::SetInnerColor(const Color& theColor)
{
	LastPanel()->mInnerColor = theColor;
}

void ConversationPanel::SetContinueTextColor(const Color& theColor)
{
	LastPanel()->mContinueColor = theColor;
}

void ConversationPanel::SetAllContinueTextColors(const Color& theColor)
{
	for (Panel* aPanel : mPanels)
		aPanel->mContinueColor = theColor;
}

void ConversationPanel::SetDimensions(int theX, int theY, int theW, int theH)
{
	mPanelY = theY;
	mPanelW = theW;
	mPanelH = theH;
	mPanelX = theX;
	mBeamX = theX;
	mBeamTarget = theW / 2 + theX;
}

void ConversationPanel::SetAnim(Image* theImage, int theDelay)
{
	Panel* p = LastPanel();
	delete p->mAnim;
	p->mAnim = new JeffLib::Animator();
	p->mAnim->SetImage(theImage);
	p->mAnim->SetDelay(theDelay);
	p->mAnim->SetStopped(false);
	p->mAnim->mOnce = false;
}

void ConversationPanel::SetAnimXY(int theX, int theY)
{
	JeffLib::Animator* anAnim = LastPanel()->mAnim;
	anAnim->mX = (float)theX;
	anAnim->mY = (float)theY;
}

void ConversationPanel::AlignAnim(int theSide, bool theAlignX)
{
	JeffLib::Animator* anAnim = LastPanel()->mAnim;
	if (anAnim->mImage == nullptr)
		return;
	int y = mPanelH / 2 + mPanelY - anAnim->mImage->GetCelHeight() / 2;
	float x;
	if (theSide < 0)
		x = theAlignX ? (float)(mPanelX + 20) : anAnim->mX;
	else
		x = theAlignX ? (float)(mPanelW + mPanelX - 20) : anAnim->mX;
	SetAnimXY((int)x, y);
}

static std::string ReplaceAll(const std::string& theText, const std::string& theFrom, const std::string& theTo)
{
	std::string aResult;
	size_t aPos = 0;
	while (aPos < theText.size()) {
		size_t aFound = theText.find(theFrom, aPos);
		if (aFound == std::string::npos) {
			aResult += theText.substr(aPos);
			break;
		}
		aResult += theText.substr(aPos, aFound - aPos) + theTo;
		aPos = aFound + theFrom.size();
	}
	return aResult;
}

// "%name" in a page becomes the player's name (V1:0x6a7610 replaces every one)
static std::string WithPlayerName(GameApp* theApp, const std::string& theText)
{
	std::string aName = theApp->mProfile != nullptr ? theApp->mProfile->mName : "";
	if (aName.empty())
		return theText;
	return ReplaceAll(theText, "%name", aName);
}

// The page's top (the text centred in the panel's inner band) and its widest line, then the text wrapped to the
// text rect
void ConversationPanel::LayOutPage(Panel* thePanel, TextPage* thePage, const std::string& theText)
{
	int aMaxWidth = 0;
	if (thePanel->mFont == nullptr) {
		thePage->mY = mPanelY + 70;
	} else {
		Graphics g;
		g.SetFont(thePanel->mFont);
		int aHeight = g.GetWordWrappedHeight(thePanel->mTextRect.mWidth, theText, thePanel->mFont->GetAscent(),
											 &aMaxWidth);
		thePage->mY = (mPanelH - 120) / 2 + mPanelY - aHeight / 2 + 60;
	}
	thePage->mMaxWidth = aMaxWidth;
	thePage->mText = WordWrap(thePanel->mFont, thePage->mText, thePanel->mTextRect.mWidth, false);
}

void ConversationPanel::AddTextPage(const std::string& theText)
{
	std::string aText = WithPlayerName(mApp, theText);
	// port: the original also split the text into a list it never used (V1:0x6a7810)
	TextPage* aPage = new TextPage();
	aPage->mText = aText;
	Panel* p = LastPanel();
	LayOutPage(p, aPage, aText);
	p->mPages.push_back(aPage);
}

void ConversationPanel::ReplaceCurrentText(const std::string& theText)
{
	Panel* p = CurPanel();
	TextPage* aPage = p->CurrentPage();
	p->mTyping = true;
	aPage->mLetters = 0;
	std::string aText = WithPlayerName(mApp, theText);
	aPage->mText = aText;
	LayOutPage(p, aPage, aText);
	mState = STATE_TEXT;
	mSkipLetterCallback = true;
}

void ConversationPanel::SetLetterDelay(int theDelay)
{
	mLetterDelay = theDelay;
	for (Panel* aPanel : mPanels)
		aPanel->mTyping = theDelay != 0;
}

void ConversationPanel::SetFont(Font* theFont, const Color& theColor)
{
	LastPanel()->mFont = theFont;
	LastPanel()->mTextColor = theColor;
}

void ConversationPanel::AddNewPanel()
{
	Panel* aPanel = new Panel();
	if (!mPanels.empty()) {
		Panel* aLast = LastPanel();
		aPanel->mFont = aLast->mFont;
		aPanel->mTextColor = aLast->mTextColor;
		aPanel->mInnerColor = aLast->mInnerColor;
		aPanel->mTextRect = aLast->mTextRect;
		aPanel->mTyping = aLast->mTyping;
	}
	mPanels.push_back(aPanel);
}

void ConversationPanel::OffsetPanelY(int theOffset)
{
	mPanelY += theOffset;
	mBeamY += theOffset;
	for (Panel* aPanel : mPanels) {
		aPanel->mTextRect.mY += theOffset;
		for (TextPage* aPage : aPanel->mPages)
			aPage->mY += theOffset;
	}
}

// the battle's board holds while the panel shows
static void PauseBoard(GameApp* theApp, bool thePause)
{
	if (theApp->mBoard != nullptr)
		theApp->mBoard->Pause(thePause, false);
}

void ConversationPanel::Activate(bool thePauseBoard)
{
	mState = STATE_BEAM_IN;
	mSkipLetterCallback = false;
	mApp->mWidgetManager->BringToFront(this);
	SetVisible(true);
	SetDisabled(false);
	if (thePauseBoard)
		PauseBoard(mApp, true);
	mRestoreFocus = mApp->mWidgetManager->mFocusWidget;
	mPausesBoard = thePauseBoard;
	mApp->Unmute(true);
	MarkDirty();
}

void ConversationPanel::TriggersNewUserDialog()
{
	if (mApp->mProfile == nullptr)
		LastPanel()->mTriggersNewUserDialog = true;
}

void ConversationPanel::ForgetCallbacks()
{
	mDrawCallback.clear();
	mUpdateCallback.clear();
	mLetterCallback.clear();
	mFinishedCallback.clear();
	mDisappearingCallback.clear();
	mInactiveCallback.clear();
	mGenericCompletionCallback.clear();
}

void ConversationPanel::Deactivate()
{
	mState = STATE_OFF;
	SetVisible(false);
	SetDisabled(true);
	mApp->mWidgetManager->BringToBack(this);
	if (mPausesBoard)
		PauseBoard(mApp, false);
	if (mRestoreFocus != nullptr) {
		mApp->mWidgetManager->SetFocus(mRestoreFocus);
		mRestoreFocus = nullptr;
	}
	mOnInactive = nullptr;
	ForgetCallbacks();
}

bool ConversationPanel::SlideToTarget(Panel* thePanel)
{
	JeffLib::Animator* anAnim = thePanel->mAnim;
	float aTarget = (float)thePanel->mTargetX;
	if (aTarget > anAnim->mX) {
		anAnim->mX += 10;
		if (aTarget > anAnim->mX)
			return false;
	} else {
		// port: as the original, a portrait that stands on its target without having arrived never does
		if (!(anAnim->mX > aTarget))
			return false;
		anAnim->mX -= 10;
		if (aTarget < anAnim->mX)
			return false;
	}
	anAnim->mX = aTarget;
	thePanel->mArrived = true;
	return true;
}

// The scripts' function called with no arguments for one result, true or not (nil is false)
static bool CallForBool(const std::string& theFunc)
{
	lua_State* L = gLuaSupport->L;
	if (!gLuaSupport->Call(theFunc, 0, 1))
		return false;
	bool aResult = lua_toboolean(L, -1) != 0;
	lua_pop(L, 1);
	return aResult;
}

void ConversationPanel::NextPanel()
{
	if (mCurPanel + 1 < mPanels.size()) {
		mCurPanel++;
		Panel* p = CurPanel();
		RewrapPage();
		mState = STATE_NEXT_PANEL;
		Panel* aPrev = mPanels[mCurPanel - 1];
		if (p->mTargetX == aPrev->mTargetX) {
			aPrev->mArrived = true;
			p->mArrived = true;
			p->mAnim->mX = (float)p->mTargetX;
		} else {
			// the one who spoke slides out on the far side from where the new speaker comes in
			int aWidth = aPrev->mAnim->mImage != nullptr ? aPrev->mAnim->mImage->GetCelWidth() : 0;
			if (p->mAnim->mX <= (float)p->mTargetX)
				aPrev->mTargetX = mPanelW + mPanelX + aWidth;
			else
				aPrev->mTargetX = mPanelX - aWidth;
			aPrev->mArrived = false;
		}
		mCurrentInner = aPrev->mInnerColor;
		if (!mStartedCallback.empty())
			CallForBool(mStartedCallback);
	} else {
		mState++;  // STATE_CLOSING
		mAlpha = mScreenColor.mAlpha;
		if (!mDisappearingCallback.empty())
			gLuaSupport->Call(mDisappearingCallback, 0, 0);
	}
}

void ConversationPanel::RewrapPage()
{
	if (mApp->mProfile == nullptr || mApp->mProfile->mName.empty())
		return;
	Panel* p = CurPanel();
	TextPage* aPage = p->CurrentPage();
	aPage->mText = WithPlayerName(mApp, aPage->mText);
	aPage->mText = WordWrap(p->mFont, aPage->mText, p->mTextRect.mWidth, true);
	Graphics g;
	g.SetFont(p->mFont);
	g.GetWordWrappedHeight(p->mTextRect.mWidth, aPage->mText, p->mFont->GetAscent(), &aPage->mMaxWidth);
}

// The page's lines: whole words while they fit in theWidth (a word wider than the line starts a line of its own,
// after an empty one if it is the first). theJoinLines: the lines of an earlier wrap become spaces first
std::string ConversationPanel::WordWrap(Font* theFont, std::string theText, int theWidth, bool theJoinLines)
{
	if (theJoinLines) {
		for (char& c : theText)
			if (c == '\n')
				c = ' ';
	}
	std::string aResult;
	int aSpaceWidth = theFont->StringWidth(" ");
	int aLineWidth = 0;
	size_t aStart = 0;
	while (aStart < theText.size()) {
		size_t aSpace = theText.find(' ', aStart);
		std::string aWord;
		if (aSpace == std::string::npos) {
			aWord = theText.substr(aStart);
			aSpace = theText.size();
		} else {
			aWord = theText.substr(aStart, aSpace - aStart);
		}
		int aWidth = theFont->StringWidth(aWord);
		if (aWidth + aSpaceWidth + aLineWidth < theWidth) {
			aWidth += aLineWidth;
			if (!aResult.empty()) {
				aResult += " ";
				aWidth += aSpaceWidth;
			}
			aResult += aWord;
		} else {
			aResult = aResult + "\n";
			aResult = aResult + aWord;
		}
		aLineWidth = aWidth;
		aStart = aSpace + 1;
	}
	return aResult;
}

// One more letter, or a "%n%" code's n more at once (the code itself is taken out of the text); the scripts hear
// of the letter, or of the page's end
bool ConversationPanel::NextLetter()
{
	Panel* p = CurPanel();
	TextPage* aPage = p->CurrentPage();
	std::string& aText = aPage->mText;
	int aShown = aPage->mLetters;
	int aSkip = 0;
	if ((size_t)(aShown + 1) < aText.size() && aText[aShown + 1] == '%') {
		int aPercent = (int)aText.find('%', aShown + 2);
		int aSpace = (int)aText.find(' ', aShown + 1);
		if ((aPercent < aSpace && aPercent != -1) || (aSpace == -1 && aPercent != -1)) {
			aSkip = std::atol(aText.substr(aShown + 2, aPercent - aShown - 2).c_str());
			aText = aText.substr(0, aShown + 1) + aText.substr(aPercent + 1);
		}
	}
	aPage->mLetters += aSkip + 1;
	bool aDone = false;
	if ((size_t)aPage->mLetters < aText.size()) {
		if (mLetterCallback.empty()) {
			MarkDirty();
			return false;
		}
		if (!mSkipLetterCallback) {
			lua_pushstring(gLuaSupport->L, StrFormat("%c", aText[aPage->mLetters]).c_str());
			gLuaSupport->Call(mLetterCallback, 1, 0);
		}
	} else {
		p->mTyping = false;
		aDone = true;
		if (!mFinishedCallback.empty() && !mSkipLetterCallback)
			gLuaSupport->Call(mFinishedCallback, 0, 0);
	}
	mSkipLetterCallback = false;
	MarkDirty();
	return aDone;
}

// 0x49fad0: each part 4 nearer the target's; true when they are equal
static bool StepColor(Color& theColor, const Color& theTarget)
{
	int* aParts[4] = {&theColor.mRed, &theColor.mGreen, &theColor.mBlue, &theColor.mAlpha};
	const int aTargets[4] = {theTarget.mRed, theTarget.mGreen, theTarget.mBlue, theTarget.mAlpha};
	for (int i = 0; i < 4; i++) {
		int& v = *aParts[i];
		if (v < aTargets[i]) {
			v += 4;
			if (v > aTargets[i])
				v = aTargets[i];
		} else if (v > aTargets[i]) {
			v -= 4;
			if (v < aTargets[i])
				v = aTargets[i];
		}
	}
	return theColor == theTarget;
}

void ConversationPanel::Update()
{
	if (mApp->GetDialog(GameApp::DIALOG_NEW_USER) != nullptr || mState == STATE_OFF)
		return;
	mUpdateCnt++;
	Panel* p = CurPanel();
	switch (mState) {
	case STATE_BEAM_IN:
		if (mAlpha < mScreenColor.mAlpha) {
			mAlpha += 6;
			if (mAlpha > mScreenColor.mAlpha)
				mAlpha = mScreenColor.mAlpha;
		}
		mBeamWidth += 30;
		if (mBeamWidth + mBeamX >= mBeamTarget)
			mBeamWidth = mBeamTarget - mBeamX;
		if (mBeamWidth + mBeamX == mBeamTarget && mAlpha == mScreenColor.mAlpha)
			mState = STATE_OPENING;
		break;
	case STATE_OPENING:
		mOpenHalf += 6;
		if (mOpenHalf * 2 >= mPanelH) {
			mOpenHalf = mPanelH / 2;
			mState = STATE_TEXT;
			mAlpha = 255;
		}
		break;
	case STATE_TEXT:
		if (p->mAnim->Update(true))
			MarkDirty();
		if (mAlpha > 0) {
			mAlpha -= 6;
			MarkDirty();
		}
		if (!p->mArrived && !SlideToTarget(p))
			MarkDirty();
		if (p->mTyping && mLetterDelay > 0 && mAlpha < 1 && p->mArrived && mUpdateCnt % mLetterDelay == 0) {
			if (mUpdateCnt % (mLetterDelay * 2) == 0)
				mApp->mSoundPlayer->PlaySampleMinTime(SOUND_TEXT_BLIP, 0);
			NextLetter();
		}
		goto callbacks;
	case STATE_CLOSING:
		mOpenHalf -= 6;
		if (mOpenHalf < 1) {
			mOpenHalf = 0;
			mState = STATE_BEAM_OUT;
		}
		break;
	case STATE_BEAM_OUT:
		if (mAlpha > 0) {
			mAlpha -= 3;
			if (mAlpha < 1)
				mAlpha = 0;
		}
		mBeamWidth -= 20;
		if (mBeamX + mBeamWidth < 1)
			mBeamWidth = 0;
		if (mBeamWidth == 0 && mAlpha == 0) {
			mState = STATE_OFF;
			SetVisible(false);
			SetDisabled(true);
			mApp->mWidgetManager->BringToBack(this);
			if (mPausesBoard)
				PauseBoard(mApp, false);
			if (mRestoreFocus != nullptr) {
				mApp->mWidgetManager->SetFocus(mRestoreFocus);
				mRestoreFocus = nullptr;
			}
			if (!mInactiveCallback.empty())
				gLuaSupport->Call(mInactiveCallback, 0, 0);
			if (!mGenericCompletionCallback.empty())
				gLuaSupport->Call(mGenericCompletionCallback, 0, 0);
			if (mOnInactive)
				mOnInactive();
			ForgetCallbacks();
			mOnInactive = nullptr;
			mPriority = mOldPriority;
			if (mApp->mBoard != nullptr)
				mApp->mBoard->RehupMouse();
		}
		break;
	case STATE_NEXT_PANEL: {
		Panel* aPrev = mPanels[mCurPanel - 1];
		if (!aPrev->mArrived)
			SlideToTarget(aPrev);
		bool aSlid = p->mArrived || SlideToTarget(p);
		if (StepColor(mCurrentInner, p->mInnerColor) && aSlid)
			mState = STATE_TEXT;
		p->mAnim->Update(true);
		aPrev->mAnim->Update(true);
		mAlpha = 0;
		break;
	}
	default:
		goto callbacks;
	}
	MarkDirty();
callbacks:
	if (mState != STATE_OFF && !mUpdateCallback.empty() && CallForBool(mUpdateCallback))
		MarkDirty();
	// then the battle engine's (0x825930)
	if (gBattleEngine != nullptr)
		gBattleEngine->CallLinkedMethod("GenericConvPanelUpdateCallback", 0);
}

void ConversationPanel::CallDrawCallback(Graphics* g)
{
	PushGraphics(g);
	gLuaSupport->Call(mDrawCallback, 1, 0);
	gLuaSupport->ObjectDeleted(g);
}

void ConversationPanel::Draw(Graphics* g)
{
	Panel* p = CurPanel();
	if (mState != STATE_OFF && !mDrawCallback.empty() && mDrawCallbackUnder)
		CallDrawCallback(g);
	int aCenterY = mPanelH / 2 + mPanelY;
	switch (mState) {
	case STATE_BEAM_IN:
	case STATE_BEAM_OUT:
		g->SetColor(Color(mScreenColor.mRed, mScreenColor.mGreen, mScreenColor.mBlue, mAlpha));
		g->FillRect(0, 0, mApp->mWidth, mApp->mHeight);
		g->SetColor(mBeamColor);
		g->DrawLine(mBeamX, aCenterY, mBeamX + mBeamWidth, aCenterY);
		g->DrawLine(mBeamX + mPanelW, aCenterY, mBeamX + mPanelW - mBeamWidth, aCenterY);
		break;
	case STATE_OPENING:
	case STATE_CLOSING: {
		g->SetColor(mScreenColor);
		g->FillRect(0, 0, mApp->mWidth, mApp->mHeight);
		if (mOpenHalf < 1)
			break;
		int aMiddleH = mPanelH - 80;
		int anInnerH = aMiddleH - 40;
		if (mOpenHalf * 2 > aMiddleH) {
			g->SetColor(mOuterColor);
			g->FillRect(Rect(mPanelX, aCenterY - mOpenHalf, mPanelW, mOpenHalf * 2));
		}
		if (mOpenHalf * 2 > anInnerH) {
			g->SetColor(mMiddleColor);
			int aHalf = mOpenHalf * 2 > aMiddleH ? aMiddleH / 2 : mOpenHalf;
			g->FillRect(Rect(mPanelX, aCenterY - aHalf, mPanelW, aHalf * 2));
		}
		int aHalf = mOpenHalf * 2 > anInnerH ? anInnerH / 2 : mOpenHalf;
		g->SetColor(p->mInnerColor);
		g->FillRect(Rect(mPanelX, aCenterY - aHalf, mPanelW, aHalf * 2));
		if (mState == STATE_OPENING) {
			g->SetColor(Color(mBeamColor.mRed, mBeamColor.mGreen, mBeamColor.mBlue, mAlpha));
			int aTop = aCenterY - mOpenHalf;
			int aBottom = aCenterY + mOpenHalf;
			g->DrawLine(mBeamX, aTop, mBeamX + mBeamWidth * 2, aTop);
			g->DrawLine(mBeamX, aBottom, mBeamX + mBeamWidth * 2, aBottom);
		}
		break;
	}
	case STATE_TEXT: {
		g->SetColor(mScreenColor);
		g->FillRect(0, 0, mApp->mWidth, mApp->mHeight);
		g->SetColor(mOuterColor);
		g->FillRect(mPanelX, mPanelY, mPanelW, mPanelH);
		g->SetColor(mMiddleColor);
		g->FillRect(mPanelX, mPanelY + 40, mPanelW, mPanelH - 80);
		g->SetColor(p->mInnerColor);
		g->FillRect(mPanelX, mPanelY + 60, mPanelW, mPanelH - 120);
		p->mAnim->Draw(g, 0, 0);
		g->SetFont(p->mFont);
		g->SetColor(p->mTextColor);
		if (mAlpha < 1 && p->mArrived) {
			TextPage* aPage = p->CurrentPage();
			Rect aRect(p->mTextRect.mX, aPage->mY, p->mTextRect.mWidth, p->mTextRect.mHeight);
			if (aPage->mMaxWidth < aRect.mWidth) {
				int aMargin = (aRect.mWidth - aPage->mMaxWidth) / 2;
				aRect.mX += aMargin;
				aRect.mWidth -= aMargin * 2;
			}
			if (p->mTyping) {
				std::string aShown = aPage->mText.substr(0, aPage->mLetters);
				g->WriteWordWrapped(aRect, aShown, p->mFont->GetAscent(), -1, nullptr, -1, nullptr);
			} else {
				g->WriteWordWrapped(aRect, aPage->mText, p->mFont->GetAscent(), -1, nullptr, -1, nullptr);
				g->SetColor(p->mContinueColor);
				g->SetFont(FONT_SMALL);
				// "(Click anywhere to continue.)"
				g->WriteString(GT(0x6f9be4), aRect.mX, FONT_SMALL->GetAscent() + mPanelH + mPanelY - 62, aRect.mWidth, 0,
							   true, 0, -1, -1);
			}
		}
		if (mAlpha > 0) {
			g->SetColor(Color(mBeamColor.mRed, mBeamColor.mGreen, mBeamColor.mBlue, mAlpha));
			int aTop = aCenterY - mOpenHalf;
			int aBottom = aCenterY + mOpenHalf;
			g->DrawLine(mBeamX, aTop, mBeamX + mBeamWidth * 2, aTop);
			g->DrawLine(mBeamX, aBottom, mBeamX + mBeamWidth * 2, aBottom);
		}
		break;
	}
	case STATE_NEXT_PANEL:
		g->SetColor(mScreenColor);
		g->FillRect(0, 0, mApp->mWidth, mApp->mHeight);
		g->SetColor(mOuterColor);
		g->FillRect(mPanelX, mPanelY, mPanelW, mPanelH);
		g->SetColor(mMiddleColor);
		g->FillRect(mPanelX, mPanelY + 40, mPanelW, mPanelH - 80);
		g->SetColor(mCurrentInner);
		g->FillRect(mPanelX, mPanelY + 60, mPanelW, mPanelH - 120);
		mPanels[mCurPanel - 1]->mAnim->Draw(g, 0, 0);
		p->mAnim->Draw(g, 0, 0);
		break;
	}
	if (mState != STATE_OFF && !mDrawCallback.empty() && !mDrawCallbackUnder)
		CallDrawCallback(g);
}

void ConversationPanel::KeyDown(KeyCode theKey)
{
	// the original looks for the menu dialog and drops the key either way
}

void ConversationPanel::MouseMove(int x, int y)
{
	mApp->UpdateTooltipHover(x, y);
}

void ConversationPanel::MouseDown(int x, int y, int theClickCount)
{
	Widget::MouseDown(x, y, theClickCount);
	if (mApp->GetDialog(GameApp::DIALOG_NEW_USER) != nullptr)
		return;
	Panel* p = CurPanel();
	int aLast = mLastClick;
	mLastClick = mUpdateCnt;
	if (mUpdateCnt - aLast <= 1 || theClickCount != 1 || mState != STATE_TEXT)
		return;
	MarkDirty();
	if (p->mTyping) {
		while (!NextLetter())
			;
		return;
	}
	p->mPageIndex++;
	if (p->mPageIndex >= p->mPages.size()) {
		p->mTyping = false;
		if (p->mTriggersNewUserDialog) {
			// the panels wait under the new player's name (NewUserDialogDone goes on to the next)
			mApp->ShowNewUserDialog(Dialog::BUTTONS_FOOTER);
			return;
		}
		NextPanel();
		return;
	}
	p->mTyping = mLetterDelay != 0;
	RewrapPage();
	if (!mStartedCallback.empty())
		CallForBool(mStartedCallback);
}

// ---- convpanel.*: the C functions (V1:0x4c0b60-0x4a2e80), all on the app's panel ----

namespace {

ConversationPanel* Panel()
{
	return gApp->mConversationPanel;
}

int IntArg(lua_State* L, int theIndex)
{
	return (int)luaL_checknumber(L, theIndex);
}

// {r, g, b, a} at theIndex (V1:0x4c5590 reads each part; a missing one is 0)
Color ColorArg(lua_State* L, int theIndex)
{
	luaL_checktype(L, theIndex, LUA_TTABLE);
	static const char* const kParts[4] = {"r", "g", "b", "a"};
	int aParts[4];
	for (int i = 0; i < 4; i++) {
		lua_pushstring(L, kParts[i]);
		lua_gettable(L, theIndex);
		aParts[i] = (int)lua_tonumber(L, -1);
		lua_pop(L, 1);
	}
	return Color(aParts[0], aParts[1], aParts[2], aParts[3]);
}

Image* ImageArg(lua_State* L, int theIndex)
{
	return ImageOf((LuaImage*)gLuaSupport->CheckObject(theIndex, ClassOf<LuaImage>::info));
}

Font* FontArg(lua_State* L, int theIndex)
{
	return FontOf((LuaFont*)gLuaSupport->CheckObject(theIndex, ClassOf<LuaFont>::info));
}

std::string StringArg(lua_State* L, int theIndex)
{
	const char* s = lua_tostring(L, theIndex);
	return s != nullptr ? s : "";
}

int AddNewPanel(lua_State* L)  // V1:0x4c2070
{
	Panel()->AddNewPanel();
	return 0;
}

// convpanel.Activate([pauseBoard = true]), V1:0x4c0be0: then the curtain stays on top (V1:0x450d20) and the cursor is
// the arrow
int Activate(lua_State* L)
{
	bool aPause = lua_gettop(L) == 1 ? lua_toboolean(L, 1) != 0 : true;
	Panel()->Activate(aPause);
	gApp->KeepCurtainOnTop();
	gApp->SetCursor(CURSOR_POINTER);
	return 0;
}

int SetAnim(lua_State* L)  // V1:0x4c0c60: image[, delay]
{
	Image* anImage = ImageArg(L, 1);
	int aDelay = lua_gettop(L) == 2 ? (int)lua_tonumber(L, 2) : 0;
	Panel()->SetAnim(anImage, aDelay);
	return 0;
}

int SetAnimXY(lua_State* L)  // V1:0x4c0db0
{
	int y = IntArg(L, 2);
	Panel()->SetAnimXY(IntArg(L, 1), y);
	return 0;
}

int AlignAnim(lua_State* L)  // V1:0x4c0ed0: side, alignX
{
	Panel()->AlignAnim((int)lua_tonumber(L, 1), lua_toboolean(L, 2) != 0);
	return 0;
}

int SetDimensions(lua_State* L)  // V1:0x4c0f40
{
	int h = IntArg(L, 4), w = IntArg(L, 3), y = IntArg(L, 2);
	Panel()->SetDimensions(IntArg(L, 1), y, w, h);
	return 0;
}

int SetDefaults(lua_State* L)  // V1:0x4c1160: side, {inner colour}, portrait, font[, portrait delay]
{
	int aSide = (int)lua_tonumber(L, 1);
	Color anInner = ColorArg(L, 2);
	Image* aPortrait = ImageArg(L, 3);
	Font* aFont = FontArg(L, 4);
	int aDelay = lua_gettop(L) == 5 ? (int)lua_tonumber(L, 5) : 0;
	Panel()->SetDefaults(aSide, anInner, aPortrait, aFont, aDelay);
	return 0;
}

int SetScreenColor(lua_State* L)  // V1:0x4c1490
{
	Panel()->SetScreenColor(ColorArg(L, 1));
	return 0;
}

int SetBeamColor(lua_State* L)  // V1:0x4c15e0
{
	Panel()->SetBeamColor(ColorArg(L, 1));
	return 0;
}

int SetOuterColor(lua_State* L)  // V1:0x4c1730
{
	Panel()->SetOuterColor(ColorArg(L, 1));
	return 0;
}

int SetMiddleColor(lua_State* L)  // V1:0x4c1880
{
	Panel()->SetMiddleColor(ColorArg(L, 1));
	return 0;
}

int SetInnerColor(lua_State* L)  // V1:0x4c19d0
{
	Panel()->SetInnerColor(ColorArg(L, 1));
	return 0;
}

int SetContinueTextColor(lua_State* L)  // V1:0x4c1b20
{
	Panel()->SetContinueTextColor(ColorArg(L, 1));
	return 0;
}

int AddTextPage(lua_State* L)  // V1:0x4c1c70
{
	Panel()->AddTextPage(StringArg(L, 1));
	return 0;
}

// convpanel.SetTextRect(x, y, w, h) or (side), V1:0x4c1d20: beside the portrait on that side
int SetTextRect(lua_State* L)
{
	if (lua_gettop(L) > 1)
		Panel()->SetTextRect((int)lua_tonumber(L, 1), (int)lua_tonumber(L, 2), (int)lua_tonumber(L, 3),
							 (int)lua_tonumber(L, 4));
	else
		Panel()->SetTextRectBeside((int)lua_tonumber(L, 1));
	return 0;
}

int New(lua_State* L)  // V1:0x4c1e00
{
	gApp->NewConversationPanel();
	return 0;
}

int SetFont(lua_State* L)  // V1:0x4c1e20: font, {colour}
{
	Font* aFont = FontArg(L, 1);
	if (aFont == nullptr)
		luaL_typerror(L, 1, "LuaFont");
	Panel()->SetFont(aFont, ColorArg(L, 2));
	return 0;
}

int SlidePortrait(lua_State* L)  // V1:0x4c1ff0
{
	Panel()->SlidePortrait((int)lua_tonumber(L, 1));
	return 0;
}

int DontSlidePortrait(lua_State* L)  // V1:0x4c2030
{
	Panel()->DontSlidePortrait((int)lua_tonumber(L, 1));
	return 0;
}

int TriggersNewUserDialog(lua_State* L)  // V1:0x4c2090
{
	Panel()->TriggersNewUserDialog();
	return 0;
}

int Active(lua_State* L)  // V1:0x4c2a00
{
	lua_pushboolean(L, Panel() != nullptr && Panel()->Active());
	return 1;
}

// the callback setters (V1:0x4c20b0-V1:0x4c28e0): the function's path
int SetUpdateCallback(lua_State* L)
{
	Panel()->SetUpdateCallback(StringArg(L, 1));
	return 0;
}

int SetDrawCallback(lua_State* L)  // (function[, under the panel])
{
	bool anUnder = lua_gettop(L) == 2 ? lua_toboolean(L, 2) != 0 : false;
	Panel()->SetDrawCallback(StringArg(L, 1), anUnder);
	return 0;
}

int SetLetterCallback(lua_State* L)
{
	Panel()->mLetterCallback = StringArg(L, 1);
	return 0;
}

int SetFinishedCallback(lua_State* L)
{
	Panel()->mFinishedCallback = StringArg(L, 1);
	return 0;
}

int SetDisappearingCallback(lua_State* L)
{
	Panel()->mDisappearingCallback = StringArg(L, 1);
	return 0;
}

int SetInactiveCallback(lua_State* L)
{
	Panel()->mInactiveCallback = StringArg(L, 1);
	return 0;
}

int SetStartedCallback(lua_State* L)
{
	Panel()->mStartedCallback = StringArg(L, 1);
	return 0;
}

int SetGenericCompletionCallback(lua_State* L)
{
	Panel()->mGenericCompletionCallback = StringArg(L, 1);
	return 0;
}

int SetDrawPriority(lua_State* L)  // V1:0x4c0b60
{
	Panel()->SetDrawPriority((int)lua_tonumber(L, 1));
	return 0;
}

int OffsetPanelY(lua_State* L)  // V1:0x4c0ba0
{
	Panel()->OffsetPanelY((int)lua_tonumber(L, 1));
	return 0;
}

int DebugReplaceCurrentText(lua_State* L)  // V1:0x4c2a80
{
	Panel()->ReplaceCurrentText(StringArg(L, 1));
	return 0;
}

}  // namespace

void Sexy::RegisterConvPanelFunctions()
{
	static const struct {
		const char* mName;
		lua_CFunction mFunc;
	} kFunctions[] = {
		{"AddNewPanel", AddNewPanel},
		{"Activate", Activate},
		{"SetAnim", SetAnim},
		{"SetAnimXY", SetAnimXY},
		{"AlignAnim", AlignAnim},
		{"SetDimensions", SetDimensions},
		{"SetDefaults", SetDefaults},
		{"SetScreenColor", SetScreenColor},
		{"SetBeamColor", SetBeamColor},
		{"SetOuterColor", SetOuterColor},
		{"SetInnerColor", SetInnerColor},
		{"SetMiddleColor", SetMiddleColor},
		{"SetContinueTextColor", SetContinueTextColor},
		{"AddTextPage", AddTextPage},
		{"SetTextRect", SetTextRect},
		{"New", New},
		{"SetFont", SetFont},
		{"SlidePortrait", SlidePortrait},
		{"DontSlidePortrait", DontSlidePortrait},
		{"TriggersNewUserDialog", TriggersNewUserDialog},
		{"Active", Active},
		{"SetUpdateCallback", SetUpdateCallback},
		{"SetDrawCallback", SetDrawCallback},
		{"SetLetterCallback", SetLetterCallback},
		{"SetFinishedCallback", SetFinishedCallback},
		{"SetDisappearingCallback", SetDisappearingCallback},
		{"SetInactiveCallback", SetInactiveCallback},
		{"SetStartedCallback", SetStartedCallback},
		{"SetGenericCompletionCallback", SetGenericCompletionCallback},
		{"SetDrawPriority", SetDrawPriority},
		{"OffsetPanelY", OffsetPanelY},
		{"DebugReplaceCurrentText", DebugReplaceCurrentText},
	};
	for (const auto& f : kFunctions)
		gLuaInterface->RegisterFunction(std::string("convpanel.") + f.mName, f.mFunc);
}
