// Sexy::Menu (Menu.h).
#include "Menu.h"

#include "Dialog.h"
#include "EffectsManager.h"
#include "GameText.h"
#include "Gates.h"
#include "Graphics.h"
#include "Hyperspace.h"
#include "Image.h"
#include "LoaderScreen.h"
#include "Logo.h"
#include "MenuButtonWidget.h"
#include "MenuPanels.h"
#include "PlayButton.h"
#include "Profile.h"
#include "TwistDialog.h"
#include "TwistEditWidget.h"
#include "Res.h"
#include "SoundInstance.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <SDL.h>

using namespace Sexy;

namespace {

// 0x84e63c: set elsewhere (not yet found); SetState(2) then starts mStart at its end
bool gMenuStartAtEnd = false;

}  // namespace

Menu::Menu(WinTwistApp* theApp)
{
	mApp = theApp;
	mUnknown408 = -1;  // the constructor's order: +0x4a4, +0x4b4, +0x3d0, +0x3d8 and the z order come first
	mZOrder = -5;
	SetState(0);
	mUnknown408 = -1;
	mUnknown1DC = 0;
	mGameMode = -1;
	mStartDelay = 0;
	mUnknown2D8 = 0.01f;
	mMouseVisible = false;
	for (int i = 0; i < 4; i++) {
		ColorCycle& aCycle = mColorCycles[i];
		aCycle.mColors.clear();
		aCycle.mColors.push_back(Color(255, 0, 0));
		aCycle.mColors.push_back(Color(255, 255, 0));
		aCycle.mColors.push_back(Color(0, 255, 0));
		aCycle.mColors.push_back(Color(0, 255, 255));
		aCycle.SetPosition((float)i * 0.25f);
		aCycle.mSpeed = 0.15f;
	}
	mGatesShut = 1.0f;
	mNoBoard = 1.0f;
	mFXManager = new EffectsManager();
	mFXManager->mZOrder = mZOrder - 1;
	mChallengeMenu = new ChallengeMenu();
	mAutoPlayGames = 0;
	mNewUserPanel = nullptr;
	mEndStats = nullptr;

	// The loader screen's spiral button stays: it becomes the menu's
	if (mApp->mLoaderScreen == nullptr) {
		mPlayButton = new PlayButton(0, IMAGE_MAINMENU_PLAYBUTTON, this);
	} else {
		mPlayButton = mApp->mLoaderScreen->mPlayButton;
		mPlayButton->mImage = IMAGE_MAINMENU_PLAYBUTTON;
		mPlayButton->mButtonListener = this;
		mPlayButton->mMouseVisible = true;
		mApp->mLoaderScreen->RemoveWidget(mPlayButton);
		mApp->mLoaderScreen->mPlayButton = nullptr;
	}
	mProfilePanel = new ProfilePanel(this);
	mHighScoresPanel = new HighScoresPanel(this);
	mHelpPanel = new HelpPanel(mApp);
	mOptionsPanel = new OptionsPanel(mApp);
	mFirstShow = true;
}

Menu::~Menu()
{
	RemoveAllWidgets(false, false);
	delete mChallengeMenu;
	delete mProfilePanel;
	delete mPlayButton;
	delete mHighScoresPanel;
	delete mHelpPanel;
	delete mOptionsPanel;
	delete mNewUserPanel;
	if (mEndStats != nullptr) {
		mApp->mWidgetManager->RemoveWidget(mEndStats);
		delete mEndStats;
	}
	mApp->mWidgetManager->RemoveWidget(mFXManager);
	delete mFXManager;
}

void Menu::AddedToManager(WidgetManager* theWidgetManager)
{
	Profile* aProfile = mApp->mProfile;
	mNoProfile = aProfile->mName.empty() || aProfile->mName == GT(0x7da070);
	if (mNoProfile && mApp->mAutoPlayMode >= 0) {
		// -autoplay plays as its own player
		aProfile->Create(GT(0x7da118));
		aProfile->mFlags1E8 = 0xffffffff;
		mNoProfile = false;
	}
	mState = 0;
	Resize(0, 0, 0, 0);
	mGameMode = -1;
	mUnknown1E8 = 0;
	int aHeight = mApp->mHeight;
	float aScale = (float)mApp->mArtRes / 1200.0f;
	int aWidth = mApp->mWidth;
	if (mApp->mAutoPlayMode >= 0) {
		// -autoplay (not ported): app +0x1048 = 1, and 0x7344c1 unless the profile's flag 14 is set
	}
	theWidgetManager->AddWidget(mFXManager);

	int aHalfHeight = aHeight / 2;
	int aHalfWidth = aWidth / 2;
	int aCelHeight = IMAGE_MAINMENU_OUTERRING->GetCelHeight();
	int aCelWidth = IMAGE_MAINMENU_OUTERRING->GetCelWidth();
	mPlayButton->Resize(aHalfWidth - IMAGE_MAINMENU_OUTERRING->GetCelWidth() / 2,
		aHalfHeight - IMAGE_MAINMENU_OUTERRING->GetCelHeight() / 2, aCelWidth, aCelHeight);
	mPlayButton->mZOrder = 0;
	mPlayButton->mX += aWidth;
	mPlayButton->mY += aHeight;
	mPlayButton->mAlpha.SetConstant(1.0);
	if (!mNoProfile) {
		if (mPlayButton->mAppear.GetOutVal() == 1.0) {
			mPlayButton->mImageAppear.SetCurve(GTS(0x7da090));
		} else {
			mPlayButton->mAppear.SetCurve(GTS(0x7da0f8));
			mPlayButton->mImageAppear.SetConstant(1.0);
		}
	} else {
		mPlayButton->mImageAppear.SetConstant(0);
		mProfilePanel->Show();
	}
	mPlayButton->mMouseVisible = true;
	AddWidget(mPlayButton);

	mProfilePanel->mX = (aHalfWidth - mProfilePanel->mWidth / 2) + aWidth;
	int anOffsetY = GetImageOffsetY(GetIdByImage(mProfilePanel->mDialogImage));
	mProfilePanel->mY = Ftol((double)aScale * 0.0 + (double)anOffsetY * (double)aScale + (double)mApp->mHeight);
	mProfilePanel->mWidgetFlagsMod.mAddFlags |= WIDGETFLAGS_ALLOW_MOUSE;
	AddWidget(mProfilePanel);

	mHighScoresPanel->mX = aHalfWidth - mHighScoresPanel->mWidth / 2;
	float aScoresDrop = mApp->mScale * 55.0f;
	mHighScoresPanel->mY = Ftol((double)((aHalfHeight - mHighScoresPanel->mHeight / 2) + aHeight) + aScoresDrop);
	mHelpPanel->mX = (aWidth * 2 - mHelpPanel->mWidth / 2) + aHalfWidth;
	float aHelpDrop = mApp->mScale * 135.0f;
	mHelpPanel->mY = Ftol((double)((aHalfHeight - mHelpPanel->mHeight / 2) + aHeight) + aHelpDrop);
	mOptionsPanel->mX = (aHalfWidth - mOptionsPanel->mWidth / 2) + aWidth;
	mOptionsPanel->mY = Ftol((double)aScale * 64.0 + (double)(aHalfHeight - mOptionsPanel->mHeight / 2));
	AddWidget(mOptionsPanel);
	mChallengeMenu->mZOrder = -98;
	SetPan(-aWidth, -aHeight, true);
	mGatesShut = 0;
	mMouseVisible = false;
	if (mApp->mLoaderScreen == nullptr)
		mApp->mGates->mLogo->Reset(mFirstShow);
	mFirstShow = false;
}

void Menu::RemovedFromManager(WidgetManager* theWidgetManager)
{
	if (mEndStats != nullptr) {
		mApp->mWidgetManager->RemoveWidget(mEndStats);
		delete mEndStats;
		mEndStats = nullptr;
	}
	theWidgetManager->RemoveWidget(mChallengeMenu);
	RemoveWidget(mProfilePanel);
	RemoveAllWidgets(false, true);
}

void Menu::Update()
{
	Profile* aProfile = mApp->mProfile;
	mNoProfile = aProfile->mName.empty() || aProfile->mName == GT(0x7da070);
	if (mPlayButton == nullptr || !mVisible || mApp->mLoaderScreen != nullptr)
		return;
	TwistWidget::Update();
	if (mApp->mAutoPlayMode >= 0 && mAutoPlayGames == 0 && mApp->mAutoPlayDelay > 0.0f) {
		mApp->mAutoPlayDelay = (float)(mApp->mAutoPlayDelay - 0.009999999776482582);
		if (mApp->mAutoPlayDelay <= 0.0f) {
			mAutoPlayGames++;
			static_cast<ButtonListener*>(this)->ButtonDepress(0);
		}
	}

	float aFade;
	if (mApp->mBoard == nullptr) {
		aFade = (float)(mNoBoard + 0.019999999552965164);
		if (aFade > 1.0f)
			aFade = 1.0f;
	} else {
		aFade = (float)(mNoBoard - 0.019999999552965164);
		if (aFade < 0.0f)
			aFade = 0.0f;
	}
	mNoBoard = aFade;

	if (mEndStats != nullptr && mEndStats->mClicked) {
		mApp->mWidgetManager->RemoveWidget(mEndStats);
		mApp->SafeDeleteWidget(mEndStats);
		mEndStats = nullptr;
		Gates* aGates = mApp->mGates;
		if (aGates->mParent == nullptr)
			mApp->mWidgetManager->AddWidget(aGates);
		ShowScores(true);
		mHighScoresPanel->SetCategory(mApp->mLastGameMode, true);
		mHighScoresPanel->ShowBar();
		mHighScoresPanel->mAlpha = 0;
		if (mUnknown408 < 0)
			mHighScoresPanel->ShowAll();
		else
			SDL_Log("Menu 0x69aabc is not ported yet");  // the new high score's entry (comes with the scores)
		mHighScoresPanel->mHide = false;
	}

	for (int i = 0; i < 4; i++)
		mColorCycles[i].Update();
	float aDelay = (float)(mStartDelay - 0.009999999776482582);
	if (aDelay < 0.0f)
		aDelay = 0;
	mStartDelay = aDelay;

	if (mState < 3) {
		if (!mNoProfile) {
			if (mEndStats == nullptr) {
				if (aDelay <= 0.0f)
					mStart.IncInVal();
				mPlayButton->mVisible = true;
				mProfilePanel->Hide();
			} else {
				mPlayButton->mVisible = false;
			}
		} else if (mEndStats != nullptr) {
			mPlayButton->mVisible = false;
		}
	} else if (!mStart.IncInVal()) {
		// The game starts: the resumed one (0x76046d) when the play button was pressed, else a new one of
		// mGameMode (0x760492(mode, true, false)). Both come with the board.
		if (mApp->mPlayPressed && mGameMode == -1) {
			SDL_Log("WinTwistApp 0x76046d (resume the game) is not ported yet");
			mState = 4;
			return;
		}
		SDL_Log("WinTwistApp 0x760492 (start game mode %d) is not ported yet", mGameMode == -1 ? 1 : mGameMode);
		return;
	} else {
		mStart.SetConstant(mPlayButton->mImageAppear.GetOutVal());
	}

	if (!mPan.HasBeenTriggered() && !mPan.IncInVal()) {
		if (mPanel != PANEL_SCORES)
			RemoveWidget(mHighScoresPanel);
		if (mPanel != PANEL_HELP)
			RemoveWidget(mHelpPanel);
	}
	double aPan = mPan.GetOutVal() < mPan.mOutMax ? mPan.GetOutVal() : mPan.mOutMax;
	int anX = Ftol((double)(mPanX - mPanFromX) * (double)(float)aPan + (double)mPanFromX);
	mX = anX;
	int aY = Ftol((double)(mPanY - mPanFromY) * (double)(float)aPan + (double)mPanFromY);
	mY = aY;
	mPanning = anX != mPanX || aY != mPanY;

	float aShut;
	if (!mApp->mGates->mOpen) {
		aShut = (float)(mGatesShut + 0.009999999776482582);
		if (aShut > 1.0f)
			aShut = 1.0f;
	} else {
		aShut = (float)(mGatesShut - 0.009999999776482582);
		if (aShut < 0.0f)
			aShut = 0.0f;
	}
	mGatesShut = aShut;
	float aRise = (float)(mUnknown1DC + 0.009999999776482582);
	mUnknown1DC = aRise;
	if (aRise > 1.0f)
		mUnknown1DC = 1.0f;
}

void Menu::Draw(Graphics* g)
{
	// The menu draws nothing itself; it leaves the colour state as its panels expect it
	g->SetColorizeImages(true);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColor(Color::White);
}

void Menu::DrawAll(ModalFlags* theFlags, Graphics* g)
{
	// With Direct3D, while mZoom is between 0 and 1, the menu is drawn zoomed about the screen's middle
	TwistWidget::DrawAll(theFlags, g);
}

void Menu::Resize(int, int, int, int)
{
	Gates* aGates = mApp->mGates;
	int aGatesHeight = aGates->mHeight;
	int aGatesWidth = aGates->mWidth;
	int aHeight = aGatesHeight * 3;
	int aWidth = aGatesWidth * 3;
	int aY = Ftol((double)(mApp->mHeight / 2) - (double)aGatesHeight * 1.5);
	int anX = Ftol((double)(mApp->mWidth / 2) - (double)aGatesWidth * 1.5);
	TwistWidget::Resize(anX, aY, aWidth, aHeight);
	mProfileY = Ftol((double)mApp->mHeight * -1.5);
	mUnknown2F4 = mHeight;
}

void Menu::GotFocus()
{
	Widget::GotFocus();
	if (mState == 0)
		SetState(2);
	PlayMusic(-1);
}

void Menu::KeyChar(SexyChar theChar)
{
	Widget::KeyChar(theChar);
}

void Menu::MouseWheel(int theDelta)
{
	if (mChallengeMenu != nullptr)
		mChallengeMenu->MouseWheel(theDelta);
	Widget::MouseWheel(theDelta);
}

void Menu::DialogButtonDepress(int theDialogId, int theButtonId)
{
	if (theDialogId == 1) {
		mApp->KillDialog(1);
	} else if (theDialogId == 2) {
		mApp->KillDialog(2);
		if (theButtonId == Dialog::ID_YES) {
			SoundInstance* aSound = mApp->PlaySoundEx(SOUND_VOICE_GOODBYE, 0, 0, 0, false, true);
			if (aSound != nullptr) {
#ifndef __EMSCRIPTEN__
				// port: on the web the sound plays only once this returns, so the wait is left out there
				while (aSound->IsPlaying())
					SDL_Delay(10);
#endif
				aSound->Release();
			}
			mApp->Shutdown();
		}
	}
}

void Menu::ButtonPress(int, int)
{
	if (!mPanning)
		mApp->PlaySample(SOUND_CLICK);
}

void Menu::ButtonDepress(int theId)
{
	if (mPanning)
		return;
	if (theId != 100)
		mApp->PlaySample(SOUND_MENU_BUTTON_CLICK);
	Gates* aGates = mApp->mGates;
	auto aFixHelpButtons = [&]() {
		aGates->mScoresButton->SetDisabled(mHelpPanel->mPage == 0);
		aGates->mOptionsButton->SetDisabled(mHelpPanel->mPage == 0xf);
	};

	if (theId < 6) {
		if (theId == 5) {
			if (mPanel == PANEL_HELP) {
				mHelpPanel->NextPage();
				aFixHelpButtons();
			} else if (mPanel == PANEL_SCORES) {
				if (mHighScoresPanel->mCategory != -1)
					mHighScoresPanel->SetCategory(-1, false);
			} else {
				mApp->PlaySample(SOUND_DOORLOCK2);
				if (mPanel == PANEL_OPTIONS) {
					mApp->FinishOptionsDialog(true);
					aGates->mOptionsButton->SetFlipped(false, false);
					ShowMain(false);
				} else {
					aGates->mOptionsButton->SetFlipImage(GetImageById(IMAGE_MAINMENU_BACKBUTTON_ORANGE_ID));
					aGates->mOptionsButton->SetFlipped(true, false);
					aGates->mScoresButton->SetGlowOn(false);
					aGates->mQuitButton->SetGlowOn(false);
					aGates->mHelpButton->SetGlowOn(false);
					aGates->mProfileButton->SetGlowOn(false);
					SetPan(mX, 0, false);
					mApp->mHyperspace->SetTransition(4);
					mPanel = PANEL_OPTIONS;
				}
			}
		} else if (theId == 0) {
			if (mNoProfile)
				return;
			mApp->mPlayPressed = true;
			StartGame(-1);
		} else if (theId == 1) {
			if (!aGates->mProfileButton->mFlipped && mPanel != PANEL_SCORES) {
				ShowProfiles();
			} else {
				mApp->PlaySample(SOUND_DOORLOCK3);
				ShowMain(false);
				if (mPanel == PANEL_HELP)
					mOptionsPanel->ApplySettings(true);
			}
		} else if (theId > 1) {
			if (theId < 4) {
				if (mPanel == PANEL_SCORES) {
					if (mHighScoresPanel->mCategory != 1)
						mHighScoresPanel->SetCategory(1, false);
				} else {
					mApp->PlaySample(SOUND_DOORLOCK2);
					if (mPanel == PANEL_HELP) {
						aGates->mProfileButton->SetGlowOn(true);
						aGates->mHelpButton->SetFlipped(false, false);
						aGates->mOptionsButton->SetFlipped(false, false);
						aGates->mScoresButton->SetFlipped(false, false);
						aGates->mScoresButton->SetDisabled(false);
						aGates->mOptionsButton->SetDisabled(false);
						ShowMain(false);
					} else {
						ShowHelp(false);
					}
				}
			} else if (theId == 4) {
				if (mPanel == PANEL_HELP) {
					mHelpPanel->PrevPage();
					aFixHelpButtons();
				} else {
					mApp->PlaySample(SOUND_DOORLOCK2);
					if (mPanel == PANEL_SCORES) {
						aGates->mProfileButton->SetGlowOn(true);
						aGates->mHelpButton->SetFlipped(false, false);
						aGates->mOptionsButton->SetFlipped(false, false);
						aGates->mQuitButton->SetFlipped(false, false);
						aGates->mScoresButton->SetFlipped(false, false);
						ShowMain(false);
					} else {
						ShowScores(false);
					}
				}
			}
		}
	} else if (theId == 6) {
		if (mPanel == PANEL_SCORES) {
			if (mHighScoresPanel->mCategory != 3)
				mHighScoresPanel->SetCategory(3, false);
		} else {
			SexyString aLines = mApp->GetString(GTS(0x7da198), GT(0x7da158));
			SexyString aHeader = mApp->GetString(GTS(0x7da148), GT(0x7da12c));
			TwistDialog* aDialog = static_cast<TwistDialog*>(mApp->DoDialog(2, true, aHeader, aLines, _S(""),
				TwistDialog::BUTTON_MODE_YES | TwistDialog::BUTTON_MODE_NO));
			aDialog->mTwistListener = this;
		}
	} else if (theId == NewUserPanel::BUTTON_OK) {
		if (mNewUserPanel != nullptr && mNewUserPanel->mEditWidget != nullptr) {
			SexyString aName = mNewUserPanel->mEditWidget->mString;
			if (!aName.empty())
				static_cast<EditListener*>(this)->EditWidgetText(0, aName);
		}
	} else if (theId == NewUserPanel::BUTTON_CANCEL && !mApp->mProfile->mName.empty()) {
		RemoveWidget(mNewUserPanel);
		mApp->SafeDeleteWidget(mNewUserPanel);
		mNewUserPanel = nullptr;
	}
	mPanning = mX != mPanX || mY != mPanY;
}

bool Menu::AllowChar(int, SexyChar theChar)
{
	bool isDigit = (unsigned short)(theChar - '0') < 10;
	bool isLower = (unsigned short)(theChar - 'a') < 26;
	bool isUpper = (unsigned short)(theChar - 'A') < 26;
	if (theChar == ' ' || isUpper || isLower || isDigit)
		mApp->PlaySample(SOUND_TYPEWRITER1);
	return theChar == ' ' || isUpper || isLower || isDigit;
}

bool Menu::AllowText(int, const SexyString& theText)
{
	// No name may start with a space
	if (theText.size() != 0 && theText[0] == ' ')
		return false;
	return true;
}

void Menu::EditWidgetText(int, const SexyString& theString)
{
	SexyString aName = Trim(theString);
	if (aName.empty())
		return;
	if (mProfilePanel != nullptr)
		mProfilePanel->mRefresh = true;
	mApp->mWidgetManager->SetFocus(this);
	Profile* aProfile = mApp->mProfile;
	SexyString anOldName = aProfile->mName;
	// port: the original first writes and clears its beta dump (0x7648d2, 0x75e95b; NOTES.md)
	if (!aProfile->Create(aName)) {
		// The name is taken (Create says so in a dialog): back to the player there was
		aProfile->Select(anOldName);
		return;
	}
	if (anOldName == GT(0x7da070))
		aProfile->MarkTutorialDone();
	aProfile->Save();
	aProfile->SaveUsers();
	if (mX == -mApp->mWidth && mY == -mApp->mHeight) {
		// On the main screen the spiral button comes in, and the panel goes a half again as fast
		mPlayButton->mImageAppear.SetCurve(GTS(0x7da090));
		mPlayButton->mAppear.SetConstant(1.0);
		mNewUserPanel->mSlide.mIncRate *= 1.5;
		mApp->PlaySample(SOUND_MENU_BUTTON_CLICK);
	} else {
		static_cast<ButtonListener*>(this)->ButtonDepress(1);
	}
	mApp->mGates->SetButtonsOn(true);
	mNewUserPanel->Hide();
	mNewUserPanel = nullptr;
}

void Menu::ShowNewUser()
{
	if (mNewUserPanel != nullptr)
		return;
	mNewUserPanel = new NewUserPanel(this, this, !mNoProfile);
	int aWidth = GetImageById(mNewUserPanel->mUnknownB0)->mWidth;
	mNewUserPanel->Resize((mApp->mWidth * 3) / 2 - aWidth / 2, Ftol((double)mHeight * 0.699999988079071), aWidth,
		mNewUserPanel->mHeight);
	mApp->mGates->mProfileButton->SetGlowOn(false);
	AddWidget(mNewUserPanel);
	if (mPanel != PANEL_PROFILE) {
		// Away from the profiles it drops in from above, and nothing else takes the mouse
		mNewUserPanel->mSlide.SetCurve(GTS(0x7da0bc));
		mNewUserPanel->mY = Ftol((double)mNewUserPanel->mY - (double)mApp->mHeight * 0.5);
		mWidgetManager->AddBaseModal(mNewUserPanel);
	}
}

void Menu::RefreshPlayer()
{
	mHighScoresPanel->Refresh();
	mChallengeMenu->Unknown6E0DDB(false, true);
}

void Menu::SetPan(int theX, int theY, bool theNow)
{
	mPanX = theX;
	mPanY = theY;
	if (!theNow) {
		mPanFromX = mX;
		mPanFromY = mY;
	} else {
		mPanFromX = theX;
		mPanFromY = theY;
	}
	mPan.SetCurve(GTS(0x7da014));
}

void Menu::SetState(int theState)
{
	if (theState == 2) {
		mStart.SetInVal(0);
		mStart.SetCurve(GTS(0x7da038));
		if (gMenuStartAtEnd) {
			gMenuStartAtEnd = false;
			mStart.SetInVal(mStart.mInMax);
		}
		mApp->mFruitImage = IMAGE_FRUITS;
		mApp->mFruitRect = IMAGE_FRUITS->GetCelRect(0);
	} else if (theState == 3) {
		mStart.SetInVal(0);
		mStart.SetCurve(GTS(0x7da050));
		mApp->mGates->SetOpen(true, false);
		mApp->PlaySample(SOUND_STEAMLOCK1);
	}
	mState = theState;
}

void Menu::ShowMain(bool theNow)
{
	SetPan(-mApp->mWidth, -mApp->mHeight, theNow);
	mApp->mHyperspace->SetTransition(1);
	Gates* aGates = mApp->mGates;
	if (aGates->mOpen)
		aGates->SetOpen(false, false);
	bool noProfile = mNoProfile;
	if (noProfile) {
		aGates->mOptionsButton->SetGlowOn(false);
		aGates->mScoresButton->SetGlowOn(false);
		aGates->mQuitButton->SetGlowOn(false);
	} else {
		aGates->mOptionsButton->SetGlowOn(true);
		aGates->mScoresButton->SetGlowOn(true);
		aGates->mQuitButton->SetGlowOn(true);
	}
	aGates->mHelpButton->SetGlowOn(!noProfile);
	FlipProfileButton(false);
	aGates->mHelpButton->SetFlipped(false, theNow);
	aGates->mOptionsButton->SetFlipped(false, theNow);
	aGates->mQuitButton->SetFlipped(false, theNow);
	aGates->mScoresButton->SetFlipped(false, theNow);
	if (mProfilePanel != nullptr)
		mProfilePanel->mWidgetFlagsMod.mAddFlags |= WIDGETFLAGS_ALLOW_MOUSE;
	if (mPanel == PANEL_PROFILE)
		BringToBack(mProfilePanel);
	mPanel = PANEL_MAIN;
}

void Menu::ShowProfiles()
{
	mApp->PlaySample(SOUND_DOORLOCK2);
	FlipProfileButton(true);
	Gates* aGates = mApp->mGates;
	aGates->mOptionsButton->SetGlowOn(false);
	aGates->mScoresButton->SetGlowOn(false);
	aGates->mHelpButton->SetGlowOn(false);
	aGates->mQuitButton->SetGlowOn(false);
	mProfilePanel->mWidgetFlagsMod.mAddFlags &= ~WIDGETFLAGS_ALLOW_MOUSE;
	BringToFront(mProfilePanel);
	SetPan(mX, mProfileY, false);
	mApp->mHyperspace->SetTransition(5);
	mPanel = PANEL_PROFILE;
}

void Menu::ShowScores(bool theNow)
{
	RemoveWidget(mHighScoresPanel);
	Gates* aGates = mApp->mGates;
	aGates->mHelpButton->SetFlipImage(GetImageById(IMAGE_MAINMENU_SCORESBTN_CLASSICSCORES_ID));
	aGates->mOptionsButton->SetFlipImage(GetImageById(IMAGE_MAINMENU_SCORESBTN_PERSONALSTATS_ID));
	aGates->mQuitButton->SetFlipImage(GetImageById(IMAGE_MAINMENU_SCORESBTN_SPEEDSCORES_ID));
	aGates->mScoresButton->SetFlipImage(GetImageById(IMAGE_MAINMENU_BACKBUTTON_ORANGE_ID));
	aGates->mProfileButton->SetGlowOn(false);
	aGates->mHelpButton->SetFlipped(true, false);
	aGates->mOptionsButton->SetFlipped(true, false);
	aGates->mQuitButton->SetFlipped(true, false);
	aGates->mScoresButton->SetFlipped(true, false);
	SetPan(0, -mApp->mHeight, theNow);
	if (!theNow)
		mApp->mHyperspace->SetTransition(2);
	mHighScoresPanel->mHide = false;
	mPanel = PANEL_SCORES;
	AddWidget(mHighScoresPanel);
}

void Menu::ShowHelp(bool theNow)
{
	Gates* aGates = mApp->mGates;
	aGates->mScoresButton->SetFlipImage(GetImageById(IMAGE_MAINMENU_HELPPREVIOUSBUTTON_ID));
	aGates->mHelpButton->SetFlipImage(GetImageById(IMAGE_MAINMENU_BACKBUTTON_ORANGE_ID));
	aGates->mOptionsButton->SetFlipImage(GetImageById(IMAGE_MAINMENU_HELPNEXTBUTTON_ID));
	aGates->mProfileButton->SetGlowOn(false);
	aGates->mHelpButton->SetFlipped(true, false);
	aGates->mOptionsButton->SetFlipped(true, false);
	aGates->mScoresButton->SetFlipped(true, false);
	aGates->mScoresButton->SetDisabled(true);
	SetPan(mApp->mWidth * -2, -mApp->mHeight, theNow);
	if (!theNow)
		mApp->mHyperspace->SetTransition(2);
	mPanel = PANEL_HELP;
	AddWidget(mHelpPanel);
	mHelpPanel->mUnknown17C = false;
	mHelpPanel->SetPage(0);
}

void Menu::ShowEndStats()
{
	if (mEndStats == nullptr) {
		mEndStats = new EndStats(mApp);
		int aWidth = mEndStats->GetPreferredWidth();
		int aHeight = mEndStats->GetPreferredHeight();
		mEndStats->Resize(mApp->mWidth / 2 - aWidth / 2, mEndStats->mY, aWidth, aHeight);
	}
	mEndStats->mMouseVisible = true;
	mEndStats->mZOrder = mApp->mGates->mZOrder + 10;
	mApp->mWidgetManager->AddWidget(mEndStats);
}

void Menu::ShowChallenges(bool theNow)
{
	mApp->mWidgetManager->RemoveWidget(mChallengeMenu);
	mApp->mWidgetManager->AddWidget(mChallengeMenu);
	mChallengeMenu->Show(theNow ? 4 : 1, false);
}

void Menu::FlipProfileButton(bool theFlipped)
{
	mApp->mGates->mProfileButton->SetFlipped(theFlipped, false);
}

void Menu::PlayMusic(int theMusic)
{
	if (theMusic != -1)
		mMusic = theMusic;
	// WinTwistApp::PlayMusic(mMusic, true, true, 0, 0) comes with the music
}

void Menu::StartGame(int theMode)
{
	mGameMode = theMode;
	SetState(3);
	mPlayButton->mMouseVisible = false;
}
