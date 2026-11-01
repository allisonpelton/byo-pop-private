// Sexy::WinTwistApp (WinTwistApp.h).
#include "WinTwistApp.h"

#include "CustomResourceManager.h"
#include "Buffer.h"
#include "ButtonWidget.h"
#include "EffectsManager.h"
#include "GameText.h"
#include "Gates.h"
#include "GraphicsRecorder.h"
#include "Hyperspace.h"
#include "LoaderScreen.h"
#include "Menu.h"
#include "MenuButtonWidget.h"
#include "MenuPanels.h"
#include "TwistCheckbox.h"
#include "Profile.h"
#include "Ranks.h"
#include "Res.h"
#include "PlayButton.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "TwistDialog.h"
#include "UnderDialogWidget.h"
#include "Widget.h"
#include "WidgetManager.h"

#include <SDL.h>

using namespace Sexy;

WinTwistApp* Sexy::gApp = nullptr;

namespace {

// The art size the constructor picks (0x766ce7): the largest when the machine has the memory, the screen height and
// the video memory for it, 768 for a smaller machine, 600 for the smallest. The original also lets compat.cfg
// (GetAppDefaultArtRes) decide; the port has no compat.cfg.
int ChooseArtRes(bool theAllow1200, int theTotalPhysMB, int theScreenHeight, int theVidMemMB)
{
	if (theAllow1200 && theTotalPhysMB >= 768 && theScreenHeight >= 1200 && theVidMemMB >= 92)
		return WinTwistApp::ART_RES_1200;
	if (theTotalPhysMB > 255 && theScreenHeight > 599 && theVidMemMB > 59)
		return WinTwistApp::ART_RES_768;
	return WinTwistApp::ART_RES_600;
}

}  // namespace

WinTwistApp::WinTwistApp()
{
	gApp = this;
	mProdName = "BejeweledTwist";
	mRegKey = "PopCap\\BejeweledTwist";
	mMusicVolume = 0.85;
	mSfxVolume = 0.85;

	// port: the machine as the original would see a current one: the art size is decided again by Use2DMode
	SDL_DisplayMode aMode;
	int aScreenHeight = SDL_GetDesktopDisplayMode(0, &aMode) == 0 ? aMode.h : 1080;
	mArtRes = ChooseArtRes(mAllowArtRes1200, SDL_GetSystemRAM(), aScreenHeight, 256);
	mScale = (float)mArtRes / 1200.0f;

	mProfile = new Profile();
	delete mResourceManager;
	mResourceManager = new CustomResourceManager(this);
	gGraphicsRecorder = new GraphicsRecorder();

	// port: the framework draws in software, so the game runs as the original does without Direct3D (0x7606de)
	Use2DMode();
}

WinTwistApp::~WinTwistApp()
{
	if (mLoaderScreen != nullptr) {
		mWidgetManager->RemoveWidget(mLoaderScreen);
		delete mLoaderScreen;
	}
	if (mMainMenu != nullptr) {
		mWidgetManager->RemoveWidget(mMainMenu);
		delete mMainMenu;
	}
	if (mGates != nullptr) {
		mWidgetManager->RemoveWidget(mGates);
		delete mGates;
	}
	if (mWindowedButton != nullptr) {
		mWidgetManager->RemoveWidget(mWindowedButton);
		delete mWindowedButton;
	}
	if (mUnderDialog != nullptr) {
		// port: the original never deletes it
		mWidgetManager->RemoveWidget(mUnderDialog);
		delete mUnderDialog;
	}
	delete mProfile;
	delete mRanks;
	if (mHyperspace != nullptr) {
		mWidgetManager->RemoveWidget(mHyperspace);
		delete mHyperspace;
	}
	if (mFXManager != nullptr) {
		mWidgetManager->RemoveWidget(mFXManager);
		delete mFXManager;
	}
	delete gGraphicsRecorder;
	gGraphicsRecorder = nullptr;
	gApp = nullptr;
}

void WinTwistApp::Use2DMode()
{
	if (mArtRes != ART_RES_600) {
		mArtRes = ART_RES_600;
		mScale = 0.5f;
	}
	mWidth = 800;
	mHeight = 600;
	mWindowAspect.Set(4, 3);
}

void WinTwistApp::Init()
{
	SexyApp::Init();
	if (mShutdown)
		return;
	LoadProperties("properties/config.xml", true, false);
	LoadResourceManifest();
	LoadGameTexts();
	// port: the original reads the scores before the properties; their defaults need the game's texts here
	LoadScores();
	// 0x4a3c20: properties\default.xml, the texts of the high scores' stats page and others (the original reads
	// properties\music.xml just before it, 0x763ca4: it comes with the music)
	LoadProperties();
	mRanks = new Ranks();

	// The player who played last, or else the first one whose profile loads
	SexyString aLastName;
	mProfile->LoadUsers(&aLastName);
	if (!aLastName.empty() && !mProfile->Select(aLastName)) {
		aLastName = _S("");
		if (mProfile->SelectAny()) {
			aLastName = mProfile->mName;
			RegistryWriteString("LastProfileName", SexyStringToString(aLastName));
		}
	}
	// port: 0x764814 would then read PopCap's beta dump for the player (NOTES.md)

	// Without SexyCache's pictures the loader screen comes up, with its music (0x75fc23(0xca))
	mUseSexyCache = false;
	if (!mUseSexyCache && !mInitialLoadDone)
		mResourceManager->LoadResources("LoadScreen");
	mInitialLoadDone = true;
	mResourceManager->LoadResources("LoadScreenNonRelease");
	if (!ExtractLoadScreenNonReleaseResources(mResourceManager))
		ShowResourceError(true);
	if (!Is3DAccelerated()) {
		mResourceManager->LoadResources("2DOnly");
		if (!Extract2DOnlyResources(mResourceManager))
			ShowResourceError(true);
	}
	SetCursorImage(0, IMAGE_CURSOR_POINTER);
	SetCursorImage(1, IMAGE_CURSOR_HAND);
	SetCursorImage(2, IMAGE_CURSOR_DRAGGING);

	mFXManager = new EffectsManager();
	mFXManager->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mFXManager);
	// The original sets up the music visualiser (+0xc6c, z order -100) and loads the 3D models of space here; with
	// -record or -playback it seeds both random number generators from the recording
	if (!mUseSexyCache) {
		ShowHyperspace();
		mLoaderScreen = new LoaderScreen(this);
		mLoaderScreen->Resize(0, 0, mWidth, mHeight);
		mLoaderScreen->mZOrder = 1000;
		mWidgetManager->AddWidget(mLoaderScreen);
	}
	// The layer behind the dialogs (the original makes its Direct3D textures here too, which 2D never uses)
	mUnderDialog = new UnderDialogWidget();
	mUnderDialog->Resize(0, 0, mWidth, mHeight);
	mUnderDialog->mZOrder = 0;
	mWidgetManager->AddWidget(mUnderDialog);
	mUnderDialog->SetVisible(false);
}

void WinTwistApp::PlaySample(int theSoundNum)
{
	if (mPlaySamples)
		PlaySoundEx(theSoundNum, 0, 0, 0, true, true);
}

void WinTwistApp::ShowHyperspace()
{
	if (mHyperspace == nullptr)
		mHyperspace = new Hyperspace();
	mHyperspace->Resize(0, 0, mWidth, mHeight);
	// The music visualiser's z order (-100) less one; the port has no visualiser yet
	mHyperspace->mZOrder = -101;
	mHyperspace->SetTransition(0);
	mWidgetManager->AddWidget(mHyperspace);
}

void WinTwistApp::CreateGates()
{
	mGates = new Gates();
	// The music visualiser's z order (-100) plus 15
	mGates->mZOrder = -100 + 15;
	mGates->Resize(mWidth / 2 - mGates->mWidth / 2, mHeight / 2 - mGates->mHeight / 2, mGates->mWidth,
		mGates->mHeight);
	mGates->SetOpen(true, true);
}

void WinTwistApp::CreateMainMenu()
{
	mMainMenu = new Menu(this);
}

void WinTwistApp::Shutdown()
{
	if (mShutdown)
		return;
	// port: the original first ends a background thread (+0x8e0), adds the session's minutes to the time played
	// (+0x75c; the "MaxTime" property chooses the clock), drops the widget at +0xf24 and a game that may go (0x6a4065);
	// they come with the board
	mWidgetManager->RemoveWidget(mWindowedButton);
	mProfile->Save();
	SexyApp::Shutdown();
}

void WinTwistApp::WriteToRegistry()
{
	SexyApp::WriteToRegistry();
	mProfile->SaveUsers();
	// port: LastScoreName (+0x7fc) comes with the high score's name entry
	if (mLoaded)
		RegistryWriteString("LastProfileName", SexyStringToString(mProfile->mName));
	// port: HighResolution (+0x818), AlwaysUpdate (+0x81a) and ArtRes (when the options changed it, +0xf35) keep
	// the art's size, which the port always chooses as 2D mode does; ModeSeconds0 to 3 (+0x7d4) come with the board
	// and VizSpeed with the visualizer
	RegistryWriteBoolean("CustomCursors", mCustomCursorsEnabled);
	RegistryWriteBoolean("ShowHints", mShowHints);
	RegistryWriteBoolean("DoViz", mDoViz);
}

void WinTwistApp::ReadFromRegistry()
{
	SexyApp::ReadFromRegistry();
	// port: the values the writer leaves out are left out here too
	RegistryReadBoolean("CustomCursors", &mCustomCursorsEnabled);
	RegistryReadBoolean("DoViz", &mDoViz);
	RegistryReadBoolean("ShowHints", &mShowHints);
	// port: "Is3D" off chooses 2D mode (Use2DMode), which the port always uses
	EnableCustomCursors(mCustomCursorsEnabled);
	mProfile->mCustomCursors = mCustomCursorsEnabled;
}

void WinTwistApp::SwitchScreenMode(bool wantWindowed)
{
	SwitchScreenMode(wantWindowed, SexyAppBase::Is3DAccelerated());
}

void WinTwistApp::SwitchScreenMode(bool wantWindowed, bool is3d, bool force)
{
	SexyApp::SwitchScreenMode(wantWindowed, is3d, false);
	(void)force;
	if (mWindowedButton != nullptr)
		mWindowedButton->SetVisible(!mIsWindowed);
	if (mMainMenu != nullptr && mMainMenu->mOptionsPanel != nullptr)
		mMainMenu->mOptionsPanel->mFullscreenCheckbox->mChecked = !wantWindowed;
	if (mOptionsDialog != nullptr)
		static_cast<OptionsPanel*>(mOptionsDialog)->mFullscreenCheckbox->mChecked = !wantWindowed;
	// The original then clears the window's resize-corner flag (Direct3D only) and calls its slot 12 (0x75ef1b),
	// which does nothing more without that flag than the framework's
	PlaceWindowedButton();
}

void WinTwistApp::FinishOptionsDialog(bool theResolution)
{
	SDL_Log("FinishOptionsDialog()");
	OptionsPanel* aDialog = static_cast<OptionsPanel*>(mOptionsDialog);
	if (aDialog == nullptr && mMainMenu != nullptr) {
		mMainMenu->mOptionsPanel->ApplySettings(theResolution);
		return;
	}
	aDialog->ApplySettings(theResolution);
	if (mBoard == nullptr) {
		mWidgetManager->RemoveWidget(aDialog);
		SafeDeleteWidget(aDialog);
		if (mMainMenu != nullptr)
			mWidgetManager->SetFocus(mMainMenu);
	} else {
		// it zooms out over the game, which has the keys again
		aDialog->mZooming = true;
		aDialog->mZoom.SetCurve(GTS(0x7d8c7c));
		mWidgetManager->SetFocus(mBoard);
		mWidgetManager->RemoveBaseModal(mOptionsDialog);
	}
	mOptionsDialog = nullptr;
}

void WinTwistApp::PlaceWindowedButton()
{
	if (mWindowedButton == nullptr)
		return;
	mWindowedButton->Resize(mWidth - mWindowedButton->mNormalRect.mWidth - 5, 5,
		IMAGE_WINDOWEDBUTTON->GetCelWidth(), IMAGE_WINDOWEDBUTTON->GetCelHeight());
	mWindowedButton->SetVisible(!mIsWindowed && !mForceFullscreen);
}

MenuButtonWidget* WinTwistApp::MakeMenuButton(Image* theImage, Image* theCover, int theId, int theX, int theY,
	const SexyString& theLabel, const SexyString& theToolTip)
{
	MenuButtonWidget* aButton = new MenuButtonWidget(theId, theImage, this);
	aButton->mButtonImage = theImage;
	aButton->mOverImage = theImage;
	aButton->mDownImage = theImage;
	aButton->mDisabledImage = theImage;
	aButton->mCover = theCover;
	int aWidth = theImage->GetCelWidth();
	int aHeight = theImage->GetCelHeight();
	if (theImage->mNumRows * theImage->mNumCols < 2) {
		aButton->mNormalRect = Rect(0, 0, theImage->mWidth, theImage->mHeight);
	} else {
		aButton->mNormalRect = theImage->GetCelRect(0);
		aButton->mOverRect = theImage->GetCelRect(1);
		aButton->mDownRect = theImage->GetCelRect(2);
		aButton->mDisabledRect = theImage->GetCelRect(theImage->mNumRows * theImage->mNumCols == 4 ? 3 : 0);
		aButton->mOverAlphaFadeInSpeed = 0.05000000074505806;
		aButton->mOverAlphaSpeed = 0.05000000074505806;
	}
	aButton->mToolTip = theToolTip;
	aButton->mLabel = theLabel;
	aButton->mZOrder = 0;
	aButton->mX = theX;
	aButton->mY = theY;
	aButton->mWidth = aWidth;
	aButton->mHeight = aHeight;
	aButton->SetVisible(true);
	return aButton;
}

void WinTwistApp::StartMainMenu()
{
	// With -record the original starts recording here (app +0xc42)
	if (mUnknownF85) {
		// A classic game at once, as TWISTBOT: 0x760492(1, true, false) comes with the board
		mProfile->Create(GT(0x7da118));
		SDL_Log("WinTwistApp::StartMainMenu: starting a game at once is not ported yet");
		return;
	}
	CreateGates();
	CreateMainMenu();
	if (mResumeGame) {
		// The saved game (app +0x934's mode) is resumed, or the challenges come up: comes with the board
		SDL_Log("WinTwistApp::StartMainMenu: resuming a game is not ported yet");
	}
	// The registration widget (0x73b403, 0x98 bytes, app +0x924, z order 99) comes with the registration code
	if (mWindowedButton == nullptr) {
		mWindowedButton = new ButtonWidget(0x2a, this);
		mWindowedButton->mZOrder = 99;
		mWidgetManager->AddWidget(mWindowedButton);
		mWindowedButton->mButtonImage = IMAGE_WINDOWEDBUTTON;
		mWindowedButton->mNormalRect = IMAGE_WINDOWEDBUTTON->GetCelRect(0);
		mWindowedButton->mOverRect = IMAGE_WINDOWEDBUTTON->GetCelRect(1);
		mWindowedButton->mDownRect = IMAGE_WINDOWEDBUTTON->GetCelRect(2);
	}
	PlaceWindowedButton();
	// 0x7620a1: the gems drawn ahead into app +0x7f4/+0x7f8 come with the board
	if (!mResumeGame)
		ShowMainMenu(false, false, 0);
	mGates->Update();
	mMainMenu->Update();
	ClearUpdateBacklog(false);
}

void WinTwistApp::ShowMainMenu(bool theEndStats, bool theChallenges, float theDelay)
{
	// With a board the original first takes it away (or, while the board still loads, tells it to come back here):
	// comes with the board
	ShowHyperspace();
	if (mGates == nullptr)
		CreateGates();
	if (mMainMenu == nullptr)
		CreateMainMenu();
	mMainMenu->ShowMain(true);
	if (mGates->mParent == nullptr)
		mWidgetManager->AddWidget(mGates);
	if (mFXManager->mParent == nullptr) {
		mFXManager->mZOrder = -1;
		mWidgetManager->AddWidget(mFXManager);
	}
	mMainMenu->mStartDelay = theDelay;
	if (!theChallenges) {
		mMainMenu->mZOrder = mGates->mZOrder - 1;
		if (!mWidgetManager->HasWidget(mMainMenu))
			mWidgetManager->AddWidget(mMainMenu);
		else
			mMainMenu->AddedToManager(mWidgetManager);
		if (theEndStats) {
			mMainMenu->ShowEndStats();
			mGates->SetOpen(true, true);
		} else if (mLoaderScreen == nullptr) {
			mGates->SetOpen(false, false);
		}
	} else {
		mMainMenu->ShowChallenges(true);
		mGates->SetOpen(true, true);
	}
	mWidgetManager->SetFocus(mMainMenu);
	// 0x764f9c: the high score just made goes into the lists (comes with the scores)
	mUnknownF85 = false;
	if (mWidgetManager->mFocusWidget == nullptr)
		mWidgetManager->SetFocus(mMainMenu);
}

PlayButton* WinTwistApp::GetMainMenuPlayButton()
{
	return mMainMenu != nullptr ? mMainMenu->mPlayButton : nullptr;
}

bool WinTwistApp::LoadTubeResources()
{
	mResourceManager->LoadResources("Tube");
	bool anOk = ExtractTubeResources(mResourceManager);
	if (anOk)
		mTubeLoaded = true;
	else
		ShowResourceError(true);
	return anOk;
}

void WinTwistApp::LoadingThreadProc()
{
	mNumLoadingThreadTasks = mResourceManager->GetNumResources("Sounds") + mResourceManager->GetNumResources("LoadingThread") +
		mResourceManager->GetNumResources("NonResize") + mResourceManager->GetNumResources("InitialMainMenu");
	if (mLoadMode != 2) {
		while (mResourceManager->LoadNextResource()) {
			if (mShutdown)
				return;
			mCompletedLoadingThreadTasks++;
		}
		if (mResourceManager->HadError() || !LoadTubeResources()) {
			ShowResourceError(false);
			mLoadingFailed = true;
			return;
		}
		mResourceManager->StartLoadResources("NonResize");
		while (mResourceManager->LoadNextResource()) {
			if (mShutdown)
				return;
			mCompletedLoadingThreadTasks++;
		}
		mResourceManager->StartLoadResources("LoadingThread");
		while (mResourceManager->LoadNextResource()) {
			if (mShutdown)
				return;
			mCompletedLoadingThreadTasks++;
		}
		if (mResourceManager->HadError()) {
			ShowResourceError(false);
			mLoadingFailed = true;
			return;
		}
		// The original loads its 3D models here (sidebar, saucer, planets, crystals): 3D mode only
	}
	if (mLoadMode != 1) {
		mResourceManager->StartLoadResources("Sounds");
		while (mResourceManager->LoadNextResource())
			mCompletedLoadingThreadTasks++;
		if (mResourceManager->HadError()) {
			ShowResourceError(false);
			mLoadingFailed = true;
			return;
		}
	}
	if ((mLoadMode == 2 || (ExtractLoadingThreadResources(mResourceManager) && ExtractNonResizeResources(mResourceManager))) &&
		(mLoadMode == 1 || ExtractSoundsResources(mResourceManager))) {
		SDL_Log("Loading completed: %d out of %d", mCompletedLoadingThreadTasks, mNumLoadingThreadTasks);
		mLoadingThreadDone = true;
		return;
	}
	ShowResourceError(false);
	mLoadingFailed = true;
}

SoundInstance* WinTwistApp::PlaySoundEx(int theSoundId, int thePan, float thePitch, float theVolume, bool theTrack,
	bool theScaleVolume)
{
	// The board's pause and its pitch changes (board +0x210c, +0x1380, +0x1c8c) come with the board
	if ((!mLoadingThreadCompleted && mUseSexyCache) || mSoundManager == nullptr || !mHasFocus || mSoundsOff)
		return nullptr;
	if (theVolume == 0.0f)
		theVolume = 1.0f;
	SoundInstance* anInstance = mSoundManager->GetSoundInstance(theSoundId);
	if (anInstance == nullptr)
		return nullptr;
	if (thePitch != 0.0f)
		anInstance->AdjustPitch(thePitch);
	if (thePan != 0)
		anInstance->SetPan(thePan);
	if (theTrack)
		mTrackedSounds.push_back({anInstance, (double)theVolume});
	if (theScaleVolume)
		theVolume = theVolume * (float)mSfxVolumeScale;
	anInstance->SetVolume(theVolume);
	anInstance->Play(false, false);
	return anInstance;
}

void WinTwistApp::UpdateFrames()
{
	SexyApp::UpdateFrames();
	// The tracked sounds: the ended ones are released, the others follow mSfxVolumeScale
	for (size_t i = 0; i < mTrackedSounds.size(); i++) {
		TrackedSound& aSound = mTrackedSounds[i];
		if (!aSound.mInstance->IsPlaying()) {
			aSound.mInstance->Release();
			if (aSound.mInstance == mUnmanagedSound)
				mUnmanagedSound = nullptr;
			mTrackedSounds.erase(mTrackedSounds.begin() + i);
			i--;
		} else if (aSound.mInstance != mUnmanagedSound) {
			double aVolume = mSfxVolumeScale * aSound.mVolume;
			if (aSound.mInstance->GetVolume() != aVolume)
				aSound.mInstance->SetVolume(aVolume);
		}
	}
	if (mLoaderScreen != nullptr && mLoaderScreen->mFinished) {
		mWidgetManager->RemoveWidget(mLoaderScreen);
		SafeDeleteWidget(mLoaderScreen);
		mLoaderScreen = nullptr;
		mResourceManager->DeleteResources("LoadScreen");
	}
	// The screen darkens behind the dialogs and the options (and a paused board, which comes with the board)
	if (mDialogMap.size() == 0 && mUnknownF44 == nullptr && mOptionsDialog == nullptr && mBoard == nullptr) {
		float aDim = (float)(mDialogDim - 0.03);
		if (aDim < 0.0f)
			aDim = 0.0f;
		mDialogDim = aDim;
		if (aDim == 0.0f && mUnderDialog->mVisible)
			mUnderDialog->SetVisible(false);
	} else if (mDialogDim < 1.0f) {
		float aDim = (float)(mDialogDim + 0.04);
		if (aDim > 1.0f)
			aDim = 1.0f;
		mDialogDim = aDim;
		if (!mUnderDialog->mVisible) {
			if (mDialogMap.size() != 0) {
				mWidgetManager->PutBehind(mUnderDialog, mDialogList.back());
			} else if (mOptionsDialog != nullptr || mUnknownF44 != nullptr) {
				mWidgetManager->PutBehind(mUnderDialog, mOptionsDialog != nullptr ? mOptionsDialog : mUnknownF44);
			}
			if (mFXManager != nullptr)
				mWidgetManager->PutBehind(mFXManager, mUnderDialog);
			mUnderDialog->SetVisible(true);
			mUnderDialog->mUpdateCnt = 100;
		} else {
			mUnderDialog->MarkDirty();
		}
	}
}

int WinTwistApp::ScoreList(int theGameMode)
{
	int aList = 0;
	if (theGameMode != 1)
		aList = (theGameMode == 3) * 2 - 1;
	return aList;
}

void WinTwistApp::LoadScores()
{
	Buffer aBuffer;  // the original also holds the app's critical section while it reads
	mScoresLoaded = false;
	std::string aPath = GetAppDataFolder() + "users\\scores.dat";
	if (ReadBufferFromFile(aPath, &aBuffer, false) && aBuffer.ReadLong() == 9) {
		for (int i = 0; i < 20; i++) {
			mHighScores[i].mName = WStringToSexyString(aBuffer.ReadUTF8String());
			mHighScores[i].mLevel = aBuffer.ReadLong();
			mHighScores[i].mScore = aBuffer.ReadLong();
		}
	} else {
		SetDefaultScores();
		SaveScores();
	}
	mScoresLoaded = true;
}

void WinTwistApp::SaveScores()
{
	Buffer aBuffer;
	aBuffer.WriteLong(9);
	for (int i = 0; i < 20; i++) {
		aBuffer.WriteUTF8String(SexyStringToWString(mHighScores[i].mName));
		aBuffer.WriteLong(mHighScores[i].mLevel);
		aBuffer.WriteLong(mHighScores[i].mScore);
	}
	WriteBufferToFile(GetAppDataFolder() + "users\\scores.dat", &aBuffer);
}

void WinTwistApp::SetDefaultScores()
{
	// Both lists take the same ten levels; the names and scores run on through the two
	int i = 0;
	for (int aList = 0; aList < 2; aList++) {
		for (int aRow = 0; aRow < 10; aRow++, i++) {
			mHighScores[i].mLevel = GameInt(GTB(0x850ea0, 40) + aRow * 4);
			mHighScores[i].mName = GT(GameInt(GTP(0x850e50, 20) + i * 4));
			mHighScores[i].mScore = GameInt(GTB(0x850e00, 80) + i * 4);
		}
	}
}

Dialog* WinTwistApp::NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
	const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	TwistDialog* aDialog = new TwistDialog(IMAGE_SIZEABLEDIALOG, IMAGE_SIZEABLEBUTTON, theDialogId, isModal,
		theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);
	aDialog->mTwistListener = this;
	aDialog->PopUp();
	return aDialog;
}

Dialog* WinTwistApp::DoDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
	const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	Dialog* anOldDialog = GetDialog(theDialogId);
	if (anOldDialog != nullptr) {
		KillDialog(theDialogId);
		anOldDialog->mParent->RemoveWidget(anOldDialog);
		SafeDeleteWidget(anOldDialog);
		// port: the original leaves the old dialog among the app's dialogs, where AddDialog would find it deleted;
		// no dialog of the game is ever asked for twice
		mDialogMap.erase(theDialogId);
		mDialogList.remove(anOldDialog);
	}
	TwistDialog* aDialog = static_cast<TwistDialog*>(
		NewDialog(theDialogId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode));
	double aHalf = mWidth * 0.5;
	int aPreferred = aDialog->GetPreferredWidth();
	aDialog->mWidth = (int)((double)aPreferred <= aHalf ? aHalf : (double)aPreferred);
	int aHeight = aDialog->GetPreferredHeight(aDialog->mWidth);
	aDialog->mHeight = aHeight;
	if (mOptionsDialog != nullptr)
		aDialog->mZOrder = mOptionsDialog->mZOrder + 1;
	aDialog->mX = mWidth / 2 - aDialog->mWidth / 2;
	aDialog->mY = (int)((double)(mHeight / 2) - aHeight * (double)0.55f);
	AddDialog(theDialogId, aDialog);
	return aDialog;
}

bool WinTwistApp::KillDialog(int theDialogId)
{
	Dialog* aDialog = GetDialog(theDialogId);
	if (aDialog != nullptr)
		static_cast<TwistDialog*>(aDialog)->Close();
	return aDialog != nullptr;
}

void WinTwistApp::DialogButtonDepress(int theDialogId, int theButtonId)
{
	switch (theDialogId) {
	case 3:
		FinishOptionsDialog(true);
		return;
	case 4: {
		// The options' question: the game's options dialog's, or else the main menu's panel's
		OptionsPanel* aPanel = static_cast<OptionsPanel*>(mOptionsDialog);
		if (aPanel == nullptr)
			aPanel = mMainMenu->mOptionsPanel;
		aPanel->Answer(theButtonId != Dialog::ID_CANCEL);
		break;
	}
	case 0xb:
		// The board's question (on OK, by the game's mode: 0x6c9987 or 0x6c7a75; then 0x6af2be) comes with the board
		DialogThirdButtonDepress(0xb);
		return;
	case 0x28:
		// The original looks the dialog up and drops it; unless Cancel, it clears the profile's +0x1b0 and calls
		// 0x734087 and 0x6c78a5, which come with the board
		GetDialog(0x28);
		KillDialog(0x28);
		return;
	case 0x2d:
		return;
	}
	KillDialog(theDialogId);
}

void WinTwistApp::DialogThirdButtonDepress(int theDialogId)
{
	KillDialog(theDialogId);
}
