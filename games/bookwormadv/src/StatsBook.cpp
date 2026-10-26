#include "StatsBook.h"

#include <SDL.h>

#include <climits>
#include <cstring>

#include <algorithm>

#include "FadeButton.h"
#include "Font.h"
#include "GameApp.h"
#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "LuaBind.h"
#include "LuaResourceManager.h"
#include "PopLua.h"
#include "Res.h"
#include "ResourceManager.h"
#include "SexyAppBase.h"
#include "Slider.h"
#include "SongPlayer.h"
#include "WidgetManager.h"
#include "XLayout.h"
#include "lua.h"

using namespace Sexy;
using namespace LuaLib;

namespace {

// _ftol (0x765380): truncates; what does not fit is the smallest int, as the x87 gives it
int ToInt(double theValue)
{
	if (theValue >= 2147483648.0 || theValue <= -2147483649.0 || theValue != theValue)
		return INT_MIN;
	return (int)theValue;
}

}  // namespace

// ---- CustomListWidget ----

// 0x495100
CustomListWidget::CustomListWidget(ListListener* theListener) : ListWidget(1, FONT_LARGE, theListener)
{
}

// 0x4952a0: ListWidget::Draw without its background and outline, the lines' colours the book's, and the bars of
// light, the second texts and the mark added
void CustomListWidget::Draw(Graphics* g)
{
	Graphics aClipG(*g);
	aClipG.ClipRect(4, 4, mWidth - 8, mHeight - 8);
	aClipG.SetFont(mFont);

	int aFirstLine = ToInt(mPosition);
	int aLastLine = std::min((int)mLines.size() - 1, aFirstLine + ToInt(mPageSize) + 1);
	int anItemHeight, anItemOffset;
	if (mItemHeight != -1) {
		anItemHeight = mItemHeight;
		anItemOffset = (anItemHeight - mFont->GetHeight()) / 2;
	} else {
		anItemHeight = mFont->GetHeight();
		anItemOffset = 0;
	}

	for (int i = aFirstLine; i <= aLastLine; i++) {
		if (i < 0)
			break;
		int aDrawY = 4 + ToInt((i - mPosition) * anItemHeight);
		if (i == mSelectIdx || i == mHiliteIdx) {
			// the bar of light, which reaches 5 pixels out of the list to the left; the line under the mouse (and not
			// the selected one) has it added on, as half as bright
			g->PushState();
			g->ClearClipRect();
			g->ClipRect(-5, 4, mWidth + 5, mHeight - 8);
			if (i == mHiliteIdx && i != mSelectIdx) {
				g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				g->SetColor(Color(255, 255, 255, 100));
			}
			g->DrawImage(IMAGE_SB_HILIGHT_BAR, -2, aDrawY + 2);
			g->PopState();
		}
		if (i == mIconIndex)
			g->DrawImage(IMAGE_MP_PLAY, 265, aDrawY + 1);

		std::string aLine = mLines[i];
		bool isRed = aLine == mRedString;
		Color aColor;
		if (i == mHiliteIdx)
			aColor = isRed ? Color(230, 150, 0) : Color(15, 150, 20);
		else if (i == mSelectIdx)
			aColor = isRed ? Color(255, 0, 0) : Color(0, 0, 0);
		else
			aColor = isRed ? Color(255, 0, 0) : Color(130, 110, 70);
		aClipG.SetColor(aColor);

		int aFontX;
		switch (mJustify) {
		case JUSTIFY_LEFT:
			aFontX = 4;
			break;
		case JUSTIFY_CENTER:
			aFontX = (mWidth - mFont->StringWidth(aLine)) / 2;
			break;
		default:
			aFontX = mWidth - mFont->StringWidth(aLine) - 4;
			break;
		}
		int aTextY = mFont->GetAscent() + aDrawY + anItemOffset;
		aClipG.DrawString(aLine, aFontX, aTextY);
		if (!mSecondary.empty()) {
			const std::string& aSecond = mSecondary[aLine];
			if (!aSecond.empty()) {
				// right-aligned, to the clip's height past the line's start (as the original wrote it)
				aClipG.DrawString(aSecond, aClipG.mClipRect.mHeight + aFontX - mFont->StringWidth(aSecond), aTextY);
			}
		}
	}
}

// 0x495760: the book hears the wheel first
void CustomListWidget::MouseWheel(int theDelta)
{
	if (mOwner != nullptr)
		mOwner->MouseWheel(theDelta);
	ListWidget::MouseWheel(theDelta);
}

// 0x495790: only inside its rectangle
void CustomListWidget::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	if (x >= mHitRect.mX && x < mHitRect.mX + mHitRect.mWidth && y >= mHitRect.mY &&
	    y < mHitRect.mY + mHitRect.mHeight)
		ListWidget::MouseDown(x, y, theBtnNum, theClickCount);
}

// 0x4936f0
bool CustomListWidget::IsPointVisible(int x, int y)
{
	return x >= mHitRect.mX && x < mHitRect.mX + mHitRect.mWidth && y >= mHitRect.mY &&
	       y < mHitRect.mY + mHitRect.mHeight;
}

// ---- StatsBook ----

// 0x495b30...: the class and its 15 methods, registered by the constructor each time in the original
void StatsBook::RegisterLua()
{
	static bool sRegistered = false;
	if (sRegistered)
		return;
	sRegistered = true;
	Class<StatsBook>("StatsBook", false)
		.Base<LuaWidget>()
		.Method<&StatsBook::SetLayoutFile>("SetLayoutFile")
		.Method<&StatsBook::AddLine>("AddLine")
		.Method<&StatsBook::ClearList>("ClearList")
		.Method<&StatsBook::SelectIndex>("SelectIndex")
		.Method<&StatsBook::AddSecondaryLine>("AddSecondaryLine")
		.Method<&StatsBook::Bounce>("Bounce")
		.Method<&StatsBook::SetIconIndex>("SetIconIndex")
		.Method<&StatsBook::IsBouncing>("IsBouncing")
		.Method<&StatsBook::ResetSlider>("ResetSlider")
		.Method<&StatsBook::ListClicked>("ListClicked")
		.Method<&StatsBook::GetNumLines>("GetNumLines")
		.Method<&StatsBook::PlaySWF>("PlaySWF")
		.Method<&StatsBook::PlayMusic>("PlayMusic")
		.Method<&StatsBook::HideArrowButtons>("HideArrowButtons")
		.Method<&StatsBook::SetRedString>("SetRedString");
}

// 0x493830: the widgets, then the group "StatsBook" of resources
StatsBook::StatsBook(GameApp* theApp, bool theFromGame, bool theBestiary, bool theMediaPlayer)
	: mApp(theApp), mLayout(nullptr), mContinueButton(nullptr), mList(nullptr), mSlider(nullptr), mScrollUp(nullptr),
	  mScrollDown(nullptr), mFlash(nullptr), mScreenImage(nullptr), mFromGame(theFromGame),
	  mBestiary(theBestiary), mMediaPlayer(theMediaPlayer)
{
	RegisterLua();
	// the movie's place on the screen (a MyFlashWidget, which the book draws itself)
	mFlash = new FlashWidget(0, static_cast<FlashListener*>(this));
	mFlash->Resize(0, 0, 310, 233);
	mHasAlpha = true;
	mHasTransparencies = true;
	mLayout = new XLayout();
	Resize(0, 0, mApp->mWidth, mApp->mHeight);

	mList = new CustomListWidget(static_cast<ListListener*>(this));
	mList->mDrawOutline = false;
	mList->mItemHeight = 20;
	mList->mOwner = this;
	AddWidget(mList);

	mSlider = new Slider(IMAGE_SLIDER_TRACK_VERT, IMAGE_SLIDER_THUMB, 1, static_cast<SliderListener*>(this));
	mSlider->mHorizontal = false;
	AddWidget(mSlider);

	// no clicks for the first five updates (the click that opened the book is still on its way)
	mWidgetFlagsMod.mRemoveFlags |= WIDGETFLAGS_ALLOW_MOUSE;
	// the 320 x 240 picture of the music's spectrum (Update draws it), clear. port: a MemoryImage (a DDImage)
	mScreenImage = new MemoryImage(mApp);
	mScreenImage->Create(320, 240);
	mScreenImage->Clear();
	mScreenImage->SetVolatile(true);
	memset(mScreenImage->GetBits(), 0, mScreenImage->mWidth * mScreenImage->mHeight * 4);
	mScreenImage->BitsChanged();
	mPriority = 10001;

	if (!mApp->mResourceManager->LoadResources("StatsBook"))
		mApp->ShowResourceError(false);
	if (!ExtractStatsBookResources(mApp->mResourceManager))
		mApp->ShowResourceError(false);
	static_cast<LuaResourceManager*>(mApp->mResourceManager)->ExportToLua();
	mApp->mWidgetManager->SetFocus(mList);

	mContinueButton = new FadeButton(1, static_cast<ButtonListener*>(this));
	mContinueButton->SetupImage(mBestiary || mMediaPlayer ? IMAGE_SB_BUTTON_MENU : IMAGE_UI_BOOK_TS_CONTINUE_BUTTON);
	mContinueButton->mWidgetFlagsMod.mRemoveFlags |= WIDGETFLAGS_ALLOW_MOUSE;
	AddWidget(mContinueButton);

	mScrollUp = new FadeButton(10, static_cast<ButtonListener*>(this));
	mScrollUp->SetupImage(IMAGE_SB_SCROLL_UP);
	mScrollDown = new FadeButton(11, static_cast<ButtonListener*>(this));
	mScrollDown->SetupImage(IMAGE_SB_SCROLL_DOWN);
	AddWidget(mScrollUp);
	AddWidget(mScrollDown);
	if (mFromGame && !mBestiary && !mMediaPlayer)
		mList->mUnused138 = true;
}

// 0x493fa0: the movie goes, then the scripts' Destructor (unless the app is shutting down), the widgets, the
// layout and the resources
StatsBook::~StatsBook()
{
	delete mFlash;
	if (!mApp->mShutdown)
		CallLinkedMethod("Destructor", 0);
	RemoveAllWidgets(true, true);
	delete mLayout;
	mApp->mResourceManager->DeleteResources("StatsBook");
	delete mScreenImage;
}

// 0x4942f0
void StatsBook::SetLayoutFile(const std::string& theFile)
{
	mLayout->Clear();
	mLayout->Load(theFile);
	PositionWidgets();
}

// 0x494540: the continue button where the layout's node of that name is, the list in its node (the whole list takes
// the mouse), the slider beside the list between the two arrows
void StatsBook::PositionWidgets()
{
	XLayout* aNode = mLayout->FindNode("Continue Button");
	if (aNode == nullptr || mLayout->FindNode("List") == nullptr) {
		SDL_Log("StatsBook: the layout has no \"Continue Button\" or \"List\"");
		return;
	}
	mContinueButton->Move(aNode->GetX(), aNode->GetY());
	aNode = mLayout->FindNode("List");
	mList->Resize(aNode->GetX(), aNode->GetY(), aNode->GetWidth(), aNode->GetHeight());
	mList->mHitRect = Rect(0, 0, mList->mWidth, mList->mHeight - 5);
	int anArrowHeight = IMAGE_SB_SCROLL_UP->GetCelHeight();
	mSlider->Resize(mList->mX + mList->mWidth + 5, anArrowHeight + mList->mY, IMAGE_SLIDER_TRACK_VERT->GetWidth(),
	                mList->mHeight - anArrowHeight * 2);
	mSlider->SetValue(0.0);
	mScrollUp->Move(mSlider->mX, mSlider->mY - anArrowHeight);
	mScrollDown->Move(mSlider->mX, mSlider->mHeight + mSlider->mY);
}

// the slider and the arrow buttons: there, and used, or hidden and not
void StatsBook::ShowArrows(bool theShow)
{
	mSlider->SetVisible(theShow);
	mScrollUp->SetVisible(theShow);
	mScrollUp->SetDisabled(!theShow);
	mScrollDown->SetVisible(theShow);
	mScrollDown->SetDisabled(!theShow);
}

// 0x4941f0: a long list has the slider and arrows
void StatsBook::AddLine(const std::string& theLine, bool theAlphabetical)
{
	mList->AddLine(theLine, theAlphabetical);
	ShowArrows(mList->mLines.size() > 14);
}

// 0x494420
void StatsBook::ClearList()
{
	mList->RemoveAll();
	mSlider->SetVisible(false);
	mScrollUp->SetVisible(false);
	mScrollUp->SetDisabled(true);
	mScrollDown->SetVisible(false);
	mScrollDown->SetDisabled(true);
}

// 0x494100
void StatsBook::HideArrowButtons()
{
	mScrollUp->SetVisible(false);
	mScrollUp->SetDisabled(true);
	mScrollDown->SetVisible(false);
	mScrollDown->SetDisabled(true);
}

// 0x4937c0
void StatsBook::AddSecondaryLine(const std::string& theLine, const std::string& theText)
{
	mList->mSecondary[theLine] = theText;
}

// 0x494170
void StatsBook::ResetSlider()
{
	mSlider->SetValue(0.0);
	mList->ScrollPosition(mSlider->mId, 0.0);
}

// 0x4949b0: as a click on the line
void StatsBook::SelectIndex(int theIdx)
{
	ListClicked(mList->mId, theIdx, 1);
}

// 0x4941c0: back above the screen, falling at 20 pixels an update
void StatsBook::Bounce()
{
	mBounceSpeed = 20.0f;
	mBouncing = true;
	Move(mX, -mHeight);
}

// 0x494490: not while it drops in or the curtain moves; the line is selected, and the scripts told unless it was
// the selected one already (the media player plays a song again)
void StatsBook::ListClicked(int theId, int theIdx, int theClickCount)
{
	if (mBouncing || mApp->IsCurtainMoving())
		return;
	int anOld = mList->mSelectIdx;
	mList->SetSelect(theIdx);
	if (anOld == theIdx && !mMediaPlayer)
		return;
	lua_pushint(LuaLib::gLuaSupport->L, theIdx);
	CallLinkedMethod("ListClicked", 1);
}

// 0x495820: a movie (a .swf) on the screen
void StatsBook::PlaySWF(const std::string& theFile)
{
	delete mFlash;
	mFlash = new FlashWidget(0, static_cast<FlashListener*>(this));
	mFlash->Resize(0, 0, 310, 233);
	mFlash->mRedraw = true;
	mFlash->StartAnimation(theFile);
	mPlayingMusic = false;
	mPlayingMovie = true;
}

// 0x495910: a song, brought in softly
void StatsBook::PlayMusic(int theSong)
{
	if (!mPlayingMusic)
		mCountdown = 100;
	if (mApp->mSongPlayer != nullptr)
		mApp->mSongPlayer->PlaySong(theSong, 0.005f, true);
	mPlayingMusic = true;
	mPlayingMovie = false;
}

// 0x495090: the scripts are told how the book is opened
void StatsBook::AddedToManager(WidgetManager* theWidgetManager)
{
	LuaWidget::AddedToManager(theWidgetManager);
	theWidgetManager->SetFocus(mList);
	lua_State* L = LuaLib::gLuaSupport->L;
	lua_pushboolean(L, mFromGame);
	lua_pushboolean(L, mBestiary);
	lua_pushboolean(L, mMediaPlayer);
	CallLinkedMethod("AddedToManager", 3);
	mApp->ClearUpdateBacklog(false);
}

// 0x494a10: the music's spectrum, the drop, the movie, and the scripts' Update
void StatsBook::Update()
{
	MarkDirty();
	mUpdateCnt++;
	// while a song plays in the media player: its spectrum, and after the first 100 updates the picture of it, each
	// lit cell green at the bottom to red 120 rows up, the others fading by 10 an update
	JeffLib::SongPlayer* aSongs = mApp->mSongPlayer;
	if (mMediaPlayer && mPlayingMusic && mApp->mHasFocus && !mApp->IsCurtainMoving() && aSongs != nullptr) {
		aSongs->UpdateSpectrum(mUpdateCnt, mScreenMode);
		if (mCountdown > 0) {
			mCountdown--;
		} else {
			unsigned char* aSpectrum = aSongs->mSpectrum;
			ulong* aBits = mScreenImage->GetBits();
			int aRowFromBottom = -1;
			for (int y = 0; y < aSongs->mSpectrumHeight; y++) {
				int aRed = y * 2;
				if (y > 120)
					aRed = 240;
				for (int x = 0; x < aSongs->mSpectrumWidth; x++) {
					ulong& aPixel = aBits[(mScreenImage->mHeight + aRowFromBottom) * mScreenImage->mWidth + x];
					if (aSpectrum[aSongs->mSpectrumWidth * y + x] != 0) {
						aPixel = 0xFF000000 | (aRed << 16) | ((240 - aRed) << 8);
					} else {
						int anAlpha = (int)(aPixel >> 24) - 10;
						if (anAlpha < 0)
							anAlpha = 0;
						aPixel = ((ulong)anAlpha << 24) | (aPixel & 0xFFFFFF);
					}
				}
				aRowFromBottom--;
			}
		}
		mScreenImage->BitsChanged();
	}
	// the clicks count after five updates
	if (mUpdateCnt == 5) {
		mWidgetFlagsMod.mRemoveFlags &= ~WIDGETFLAGS_ALLOW_MOUSE;
		mContinueButton->mWidgetFlagsMod.mRemoveFlags &= ~WIDGETFLAGS_ALLOW_MOUSE;
	}
	if (mFlash->mState == FlashWidget::STATE_PLAYING && mApp->mHasFocus && mMediaPlayer && mPlayingMovie)
		mFlash->Update();
	if (mBouncing) {
		// 20 pixels an update down, at 0.3 more an update; at the top of the screen it turns back at half the speed,
		// and is still once that is less than a quarter pixel
		mBounceSpeed = (float)(mBounceSpeed + 0.30000001192092896);
		int aY = ToInt(mY + mBounceSpeed);
		if (mBounceSpeed >= 0 && aY >= 0) {
			mBounceSpeed = (float)(mBounceSpeed * -0.5);
			aY = 0;
			if (!(mBounceSpeed <= -0.25f))
				mBouncing = false;
		}
		Move(mX, aY);
	}
	CallLinkedMethod("Update", 0);
}

// 0x494cc0: the layout, the media player's screen (the movie, or what the song shows), then the scripts' Draw (with
// the graphics and the selected line)
void StatsBook::Draw(Graphics* g)
{
	mLayout->Draw(g);
	if (mMediaPlayer) {
		g->SetColor(Color::Black);
		g->FillRect(410, 104, mFlash->mWidth, mFlash->mHeight);
	}
	if ((mFlash->mState == FlashWidget::STATE_PLAYING || mFlash->mState == FlashWidget::STATE_IDLE) && mPlayingMovie) {
		if (mMediaPlayer) {
			g->PushState();
			g->Translate(417, 104);
			mFlash->Draw(g);
			g->PopState();
		}
	} else if (mMediaPlayer && mPlayingMusic && mScreenImage != nullptr) {
		g->DrawImage(mScreenImage, 417, 104);
	}
	mGraphics = g;
	PushGraphics(g);
	lua_pushint(LuaLib::gLuaSupport->L, mList->mSelectIdx);
	CallLinkedMethod("Draw", 2);
	mGraphics = nullptr;
}

// 0x4943e0
void StatsBook::MouseMove(int x, int y)
{
	if (mBouncing)
		return;
	lua_State* L = LuaLib::gLuaSupport->L;
	lua_pushint(L, x);
	lua_pushint(L, y);
	CallLinkedMethod("MouseMove", 2);
}

// 0x494340: the scripts hear it; a click on the media player's screen changes what the song shows on it
void StatsBook::MouseDown(int x, int y, int theClickCount)
{
	if (mBouncing)
		return;
	lua_State* L = LuaLib::gLuaSupport->L;
	lua_pushint(L, x);
	lua_pushint(L, y);
	lua_pushint(L, theClickCount);
	CallLinkedMethod("MouseDown", 3);
	if (mMediaPlayer && x >= 410 && y >= 104 && x <= 730 && y <= 344)
		mScreenMode = mScreenMode == 0 ? 1 : 0;
}

// 0x494f30: on the list (and its slider) a long list scrolls a line a notch
void StatsBook::MouseWheel(int theDelta)
{
	int aMouseX = mWidgetManager->mLastMouseX;
	int aMouseY = mWidgetManager->mLastMouseY;
	if (mList->mLines.size() > 14 && aMouseX >= mList->mX && aMouseX <= mSlider->mWidth + mSlider->mX &&
	    aMouseY >= mList->mY && aMouseY <= mList->mHeight + mList->mY &&
	    (int)mList->mLines.size() - mList->mHeight / mList->mItemHeight > 0) {
		int aSteps = (int)mList->mLines.size() - 14;
		double aSize = (double)aSteps;
		if (aSteps < 0)
			aSize += 4294967296.0;
		double aStep = 1.0 / aSize;
		if (theDelta > 0)
			mSlider->SetValue(mSlider->mVal - aStep);
		else if (theDelta < 0)
			mSlider->SetValue(mSlider->mVal + aStep);
		SliderVal(mSlider->mId, mSlider->mVal);
	}
}

// 0x4947c0
void StatsBook::ButtonPress(int theId, int theClickCount)
{
	mApp->PlaySample(SOUND_CLICK1);
}

// 0x4947e0: "Continue" (the whole curtain closes, then the arena's hall of fame after a timed game; for the tome and
// the media player the book goes and the main menu's song comes back), the arrows move the slider a step
void StatsBook::ButtonDepress(int theId)
{
	if (mBouncing)
		return;
	if (theId == mContinueButton->mId) {
		mContinueButton->SetDisabled(true);
		if (!mBestiary && !mMediaPlayer) {
			if (mApp->IsCurtainMoving())
				return;
			mApp->DoFullCurtainTransition("");
			mApp->mCurtainCallback = [](void* theApp) { static_cast<GameApp*>(theApp)->ShowArenaHallOfFame(); };
			mApp->mCurtainCallbackObject = mApp;
			return;
		}
		mApp->RemoveStatsBook();
		if (mApp->mMainMenu != nullptr)
			mApp->mWidgetManager->SetFocus(reinterpret_cast<Widget*>(mApp->mMainMenu));
		if (mMediaPlayer) {
			mFlash->Pause();
			if (mApp->mSongPlayer != nullptr)
				mApp->mSongPlayer->PlaySong(0x8c, 0.005f, true);  // the main menu's song
		}
		return;
	}
	if (theId == mScrollUp->mId)
		mSlider->SetValue(mSlider->mVal - 0.05);
	else if (theId == mScrollDown->mId)
		mSlider->SetValue(mSlider->mVal + 0.05);
	else
		return;
	SliderVal(mSlider->mId, mSlider->mVal);
}

// 0x494df0: the slider's place (0 to 1) is the list's first line, of those that do not fit
void StatsBook::SliderVal(int theId, double theVal)
{
	int aLines = (int)mList->mLines.size() - 14;
	double aSize = (double)aLines;
	if (aLines < 0)
		aSize += 4294967296.0;
	ScrollList(ToInt(theVal * aSize));
}

// 0x494e60: the first line shown, kept to what the list can show; the line under the mouse stays under it
void StatsBook::ScrollList(int thePosition)
{
	int aLast = (int)mList->mLines.size() - mList->mHeight / mList->mItemHeight;
	if (aLast < 0)
		aLast = 0;
	if (thePosition > aLast)
		thePosition = aLast;
	if (thePosition < 0)
		thePosition = 0;
	int anOld = ToInt(mList->mPosition);
	mList->ScrollPosition(mSlider->mId, (double)thePosition);
	if (mList->mHiliteIdx != -1)
		mList->mHiliteIdx = ToInt((mList->mPosition - anOld) + mList->mHiliteIdx);
}
