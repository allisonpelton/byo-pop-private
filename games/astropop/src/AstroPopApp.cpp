#include "AstroPopApp.h"
#include "Compat2004.h"
#include "DDInterface.h"
#include "GameText.h"
#include "Board.h"
#include "HallOfFame.h"
#include "LiveSplitObserver.h"
#include "LoaderScreen.h"
#include "MainMenu.h"
#include "OptionsDialog.h"
#include "Particle.h"
#include "SDLMusicInterface.h"
#include "ResourceManager.h"
#include <SDL.h>
#include "WidgetManager.h"
#include "ButtonWidget.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "Graphics.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "Dialog.h"
#include "Checkbox.h"
#include <cmath>
#include <cstdio>
#include <cstring>

using namespace Sexy;

AstroPopApp* Sexy::gApp = NULL;
float Sexy::gSinTable[360];
float Sexy::gCosTable[360];

// 0x427ee0
AstroPopApp::AstroPopApp()
{
	gApp = this; // port: WinMain stored it (0x5dc220)

	mProdName = "AstroPop";
	mProductVersion = "1.0";
	mTitle = "AstroPop Deluxe " + mProductVersion;
	mRegKey = "PopCap\\AstroPop";

	mBoard = NULL;
	mDescriptor = NULL;
	mTutorialOn = false;
	mHintsOn = false;
	mColorblind = false;
	mShowHelpAtStart = false;
	mMainMenu = NULL;
	mHallScreen = NULL;
	mCharSelector = NULL;
	mCreditsScreen = NULL;
	mLoaderScreen = NULL;
	mOldSavesDeleted = false;
	mOldScoresDeleted = false;
	mComboPingTimer = 0;
	mGameUpdates = 0;
	mClassicPilot = 0;
	mSurvivalPilot = 0;
	mOpenDialogs = 0;
	mPilot6e8 = -1;
	mHelpShown = false;
	mHelpScreen = NULL;
	mDialogFont = NULL;
	for (int i = 0; i < 3; i++)
		mGradient[i] = NULL;
	mUser = NULL;
	mEngineLoop = NULL;
	mHyperspaceLoop = NULL;
	mLightningImage = NULL;
	mLightningImage2 = NULL;
	mMusicSlot = 0;
	mMusicOrder = -1;

	// sin and cos of whole degrees: the original multiplies by a float constant (0x562704) in the x87 unit
	for (int i = 0; i < 360; i++)
	{
		gSinTable[i] = (float) sin((double) i * (double) 0.017452778f);
		gCosTable[i] = (float) cos((double) i * (double) 0.017452778f);
	}

	mLoopSounds.assign(NUM_LOOP_SOUNDS, (SoundInstance*) NULL);
	mMusicOrder = -1;
}

// 0x42dc30 slot 14 (body 0x42d820)
AstroPopApp::~AstroPopApp()
{
	LiveSplitObserver::AppDestroyed(this);      // port(autosplit): the autosplit block's last write
	delete mUser;
	if (gApp == this)
		gApp = NULL;
}

// 0x408e90 slot 3 (and 0x4285c0, its entry through the ButtonListener part)
void AstroPopApp::ButtonPress(int theId)
{
	PlaySample(SOUND_BUTTON_DOWN);
}

// 0x408b20 slot 7: the original also sets the x87 precision to 24 bits here (_PC_24; the port's float maths is SSE)
void AstroPopApp::UpdateFrames()
{
	SexyApp::UpdateFrames();
	if (mComboPingTimer > 0)
		mComboPingTimer--;
	if (mTraceMode)
		TraceUpdate();
	LiveSplitObserver::AfterUpdate(this);       // port(autosplit): the autosplit block, after the update
}

// port(autosplit): -livesplitlog=<file> asks for the block and logs it; everything else is the framework's
void AstroPopApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-livesplitlog")
		LiveSplitObserver::SetLogFile(theParamValue);
	else
		SexyApp::HandleCmdLineParam(theParamName, theParamValue);
}

// 0x4090d0 slot 53: if the options box (id 0x66) is open, its full-screen check box follows the new mode (as a click on it
// would, with the box's listener told)
void AstroPopApp::SwitchScreenMode(bool wantWindowed)
{
	SexyApp::SwitchScreenMode(wantWindowed);
	OptionsDialog* anOptions = (OptionsDialog*) GetDialog(0x66);
	if (anOptions != NULL && anOptions->mFullscreenCheck != NULL)
		anOptions->mFullscreenCheck->SetChecked(!wantWindowed);
}

// port: the 2004 SexyApp makes a BassMusicInterface itself (slot 15, 0x408e30); the port's music interface gets
// BASS 2.0's gain like BookWorm's (docs/FRAMEWORK_2004.md section 6)
MusicInterface* AstroPopApp::CreateMusicInterface()
{
	if (mNoSoundNeeded)
		return new MusicInterface;

	SDLMusicInterface* aMusicInterface = new SDLMusicInterface();
	aMusicInterface->mMaxMusicVolume = 100;
	return aMusicInterface;
}

void AstroPopApp::InitPropertiesHook()
{
	SexyApp::InitPropertiesHook();

	LoadProperties("properties/texts.xml", false, false);
	if (!LoadGameTexts())
		SDL_Log("AstroPop: no game texts read from WinAP.exe or AstroPop.exe (texts without an override stay empty)");

	// The ten lines the quit dialog picks from (the original's constructor stores them, 0x562900 ...): read here, since the
	// texts are loaded only now
	static const unsigned int kGoodbyes[] = {0x562900, 0x5628b8, 0x562884, 0x562840, 0x5627fc, 0x5627e0, 0x5627c0,
		0x562798, 0x562760, 0x562708};
	mGoodbyeMessages.clear();
	for (size_t i = 0; i < sizeof(kGoodbyes) / sizeof(kGoodbyes[0]); i++)
		mGoodbyeMessages.push_back(GT(kGoodbyes[i]));
}

// 0x4254b0: the names under the registry key Users
void AstroPopApp::ListUsers()
{
	mUserNames.clear();
	RegistryGetSubKeys("Users", &mUserNames);
}

// the game's own generator (Particle.h)
MTRand Sexy::gGameRand;

// The display of a 2D run draws a stretch as the original's software screen does (DDImage::StretchBlt, 0x4c68c0): a fast one
// samples the source in 16.16 steps from the rectangle's corner, the column stepped before it is read, and a non-fast one
// is averaged into an image of the destination's size that is then blitted (DDInterface::mSoftwareStretch). In 3D the
// original's textured quads, and the port's GPU, sample at pixel centres. Is3D can differ from one run to the next, so
// the choice is made at each stretch.
class AstroPopDisplay : public DDInterface
{
public:
	AstroPopDisplay(SexyAppBase* theApp) : DDInterface(theApp)
	{
		mFastStretchPreAdd = true;
	}

	virtual void StretchBlt(Image* theImage, const FRect& theDestRect, const Rect& theSrcRect, const Rect* theClipRect, const Color& theColor, int theDrawMode, bool fastStretch, bool mirror = false) override
	{
		mSoftwareStretch = !mIs3D;
		DDInterface::StretchBlt(theImage, theDestRect, theSrcRect, theClipRect, theColor, theDrawMode, fastStretch, mirror);
	}
};

DDInterface* AstroPopApp::CreateDDInterface()
{
	return new AstroPopDisplay(this);
}

// 0x42f170 slot 16
void AstroPopApp::Init()
{
	// port: this game's MemoryImage::SlowStretchBlt (0x4b1bf0) divides the colour's weight by the alpha that is there, as the
	// 1.4 framework's has commented out (a stretch onto transparent bits, as the loader's strips, keeps the colour at full strength)
	gSlowStretchRenormalizesAlpha = true;

	// port: the 2004 WidgetManager draws no widget with the fast stretch on; the level-up text asks for it itself
	// (LevelUpText::DrawAt 0x40c8a0), and the atomic explosion's ring and the other stretches average (a 2D run)
	gWidgetsDrawWithoutFastStretch = true;

	SexyApp::Init();
	LoadResourceManifest();   // 0x4d93f0

	// port: the 2004 SexyApp::Init seeds the C library generator and Rand()'s generator (see BookWormApp::Init)
	CrtSRand(mHasSeedParam ? (unsigned int) mRandSeed : (unsigned int) GetTickCount());
	{
		MTRand aRand;
		SeedMTRand2004(aRand, (unsigned long) mRandSeed);
		uint32_t aState[MTRAND_N];
		int anIndex;
		aRand.GetState(aState, &anIndex);
		SetRandMTState(aState, anIndex);
	}

	mTitle = GetString("Title", mTitle);

	// The loading bar counts the four groups loaded by the thread and 0x20 more tasks
	mNumLoadingThreadTasks = mResourceManager->GetNumResources("AllwaysLoaded") + mResourceManager->GetNumResources("Game") +
		mResourceManager->GetNumResources("CharSelector") + 0x20 + mResourceManager->GetNumResources("GameSelector");

	if (!mResourceManager->LoadResources("Init") || !ExtractInitResources(mResourceManager))
		ShowResourceError(true);
	if (!mResourceManager->LoadResources("Required") || !ExtractRequiredResources(mResourceManager))
		ShowResourceError(true);

	SetCursorImage(CURSOR_POINTER, IMAGE_CUSTOM_POINTER);
	SetCursorImage(CURSOR_HAND, IMAGE_CUSTOM_HAND);
	SetCursorImage(CURSOR_DRAGGING, IMAGE_CUSTOM_DRAGGING);
	SetCursorImage(CURSOR_TEXT, IMAGE_CUSTOM_TEXT);

	mLoaderScreen = new LoaderScreen(this);
	mLoaderScreen->Resize(0, 0, mWidth, mHeight);
	mLoaderScreen->MakeSlides();
	mWidgetManager->AddWidget(mLoaderScreen);
	mLoaderScreen->MarkDirty();

	// Version bookkeeping: a profile or a high-score file of a version before 15 is deleted
	std::string aVersion;
	if (!RegistryReadString("APVersion", &aVersion))
		aVersion = mProductVersion;
	RegistryWriteString("APVersion", mProductVersion);

	aVersion = "";
	int aSgfVersion = 0;
	if (RegistryReadString("SGFVersion", &aVersion))
	{
		sscanf(aVersion.c_str(), "%d", &aSgfVersion);
		if (aSgfVersion < 15)
		{
			mOldSavesDeleted = true;
			mOldScoresDeleted = true;
		}
	}
	else
	{
		mOldSavesDeleted = true;
		mOldScoresDeleted = true;
	}
	RegistryWriteString("SGFVersion", StrFormat("%d", 15));

	ListUsers();
	if (mUserNames.empty())
	{
		mOldSavesDeleted = false;
		mOldScoresDeleted = false;
	}

	if (mOldSavesDeleted)
	{
		for (size_t i = 0; i < mUserNames.size(); i++)
		{
			mUser = new UserProfile();
			std::string aName = mUserNames[i];
			mUser->Load(NULL, &aName, true);

			RegistryEraseKey(StrFormat("Users\\%s", aName.c_str()));
			int aNumber = -1;
			if (!mUser->GetInt(PROFILE_NUMBER, &aNumber))
				Popup(GT(0x00563380));
			if (aNumber != -1)
			{
				if (!EraseFile(StrFormat("users\\data%d.usr", aNumber)))
					Popup(GT(0x00563350));
			}
			delete mUser;
			mUser = NULL;
		}
		RegistryEraseKey("Users");
		ListUsers();
	}

	LoadHighScores();

	if (mOldSavesDeleted || mOldScoresDeleted)
	{
		const char* aMessage;
		if (mOldSavesDeleted && !mOldScoresDeleted)
			aMessage = GTC(0x5632e0);
		else if (!mOldSavesDeleted && mOldScoresDeleted)
			aMessage = GTC(0x563288);
		else
			aMessage = GTC(0x563200);
		// original: ShowDialog(1, 0, 0x6c, ...) with the header "" and the footer "RESET" (0x411ba0)
		Popup(aMessage);
	}

	// The music: the same module is loaded twice, one song per slot, so that the songs can crossfade
	for (int i = 0; i < NUM_MUSIC_SLOTS; i++)
	{
		mMusicInterface->LoadMusic(i, "music\\music.mo3");
		mCompletedLoadingThreadTasks++;
	}
	PlaySong(0);

	// 0x429280, the start-up, goes on after the Init with the game's own generator: seeded with the next number of Rand()'s
	SeedMTRand2004(gGameRand, (unsigned long) (unsigned int) Rand());
}

// 0x42cda0 slot 17 (runs in one go in the port)
void AstroPopApp::LoadingThreadProc()
{
	if (!LoadBonusLevels())
	{
		Popup(GT(0x563120));
		mLoadingFailed = true;
		return;
	}

	static const char* kGroups[] = {"GameSelector", "CharSelector", "AllwaysLoaded"};
	for (int g = 0; g < 3; g++)
	{
		mResourceManager->StartLoadResources(kGroups[g]);
		while (mResourceManager->LoadNextResource())
		{
			mCompletedLoadingThreadTasks++;
			if (mShutdown)
				return;
			mLoaderScreen->MarkDirty();
		}
		if (mResourceManager->HadError())
		{
			mLoadingFailed = true;
			ShowResourceError(false);
			return;
		}
	}

	// Combo fonts: copies of FONT_COMBO and FONT_LARGE_COMBO that show their layers tagged Red, Blue and Yellow
	ImageFont* aFont = dynamic_cast<ImageFont*>(mResourceManager->GetFont("FONT_COMBO"));
	if (aFont == NULL)
	{
		mLoadingFailed = true;
		Popup(GT(0x00563100));
		return;
	}
	{
		static const char* kColours[] = {"Red", "Blue", "Yellow"};
		for (int i = 0; i < 3; i++)
		{
			ImageFont* aCopy = (ImageFont*) aFont->Duplicate();
			aCopy->AddTag(kColours[i]);
			mResourceManager->ReplaceFont(std::string("FONT_COMBO_") + (i == 0 ? "RED" : i == 1 ? "BLUE" : "YELLOW"), aCopy);
		}
	}
	aFont = dynamic_cast<ImageFont*>(mResourceManager->GetFont("FONT_LARGE_COMBO"));
	if (aFont == NULL)
	{
		mLoadingFailed = true;
		Popup(GT(0x005630cc));
		return;
	}
	{
		static const char* kColours[] = {"Red", "Blue", "Yellow"};
		for (int i = 0; i < 3; i++)
		{
			ImageFont* aCopy = (ImageFont*) aFont->Duplicate();
			aCopy->AddTag(kColours[i]);
			mResourceManager->ReplaceFont(std::string("FONT_LARGE_COMBO_") + (i == 0 ? "RED" : i == 1 ? "BLUE" : "YELLOW"), aCopy);
		}
	}

	mResourceManager->StartLoadResources("Game");
	while (mResourceManager->LoadNextResource())
	{
		mCompletedLoadingThreadTasks++;
		if (mShutdown)
			return;
		mLoaderScreen->MarkDirty();
	}
	if (mResourceManager->HadError() || !ExtractGameSelectorResources(mResourceManager) ||
		!ExtractCharSelectorResources(mResourceManager) || !ExtractAllwaysLoadedResources(mResourceManager) ||
		!ExtractGameResources(mResourceManager))
	{
		mLoadingFailed = true;
		ShowResourceError(false);
		return;
	}
	mLoaderScreen->MarkDirty();

	mEngineLoop = mSoundManager->GetSoundInstance(SOUND_ENGINE_LOOP);
	mHyperspaceLoop = mSoundManager->GetSoundInstance(SOUND_HYPERSPACE_LOOP);
	for (int i = 0; i < NUM_LOOP_SOUNDS; i++)
		mLoopSounds[i] = mSoundManager->GetSoundInstance(GetSoundById(0x16a + i));

	// The lightning flash images: IMAGE_WARP_FX_LIGHTENING drawn into two copies, the first mirrored (each row left to right, 0x4cfb50), the second flipped
	// (top to bottom, SexyApp::FlipImage)
	mLightningImage2 = new MemoryImage(this);
	mLightningImage = new MemoryImage(this);
	mLightningImage2->Create(IMAGE_WARP_FX_LIGHTENING->mWidth, IMAGE_WARP_FX_LIGHTENING->mHeight);
	mLightningImage->Create(IMAGE_WARP_FX_LIGHTENING->mWidth, IMAGE_WARP_FX_LIGHTENING->mHeight);
	{
		Graphics g2(mLightningImage2);
		g2.DrawImage(IMAGE_WARP_FX_LIGHTENING, 0, 0);
		Graphics g1(mLightningImage);
		g1.DrawImage(IMAGE_WARP_FX_LIGHTENING, 0, 0);
	}
	MirrorImage(mLightningImage);
	FlipImage(mLightningImage2);

	// Three 1 x 480 ramps (alpha = row / 2) in white, light grey and grey
	for (int i = 0; i < 3; i++)
	{
		mGradient[i] = new MemoryImage(this);
		mGradient[i]->Create(1, 480);
	}
	{
		Graphics g0(mGradient[0]), g1(mGradient[1]), g2(mGradient[2]);
		for (int y = 0; y < 480; y++)
		{
			int anAlpha = y / 2;
			g0.SetColor(Color(255, 255, 255, anAlpha));
			g0.FillRect(0, y, 1, 1);
			g1.SetColor(Color(200, 200, 200, anAlpha));
			g1.FillRect(0, y, 1, 1);
			g2.SetColor(Color(155, 155, 155, anAlpha));
			g2.FillRect(0, y, 1, 1);
		}
	}
	mDialogFont = GetFontById(0xb0);
}

// 0x4242d0 slot 13: the loading is done, the play button comes up (the update check that follows is out of the port)
void AstroPopApp::LoadingThreadCompleted()
{
	if (!mLoadingFailed)
	{
		mLoaderScreen->mPlayButton->SetVisible(true);
		mLoaderScreen->mPlayButton->SetDisabled(false);
		mLoaderScreen->MarkDirty();
	}
}

// 0x4290d0 slot 33: the original also counts the trial's play time here (out of the port); the player is saved
void AstroPopApp::Shutdown()
{
	SexyApp::Shutdown();
	if (mUser != NULL)
		mUser->Save();
}

// 0x4253e0 slot 62: the game goes on, the music and the loops come back. A screen of the selector or the hall of fame
// goes on too. The loops of the engine and of the hyperspace play again where a game is in the state that has them.
void AstroPopApp::GotFocus()
{
	if (mBoard != NULL)
	{
		mBoard->Pause(false, true);
		PauseMusicForFocus(false);
	}
	else
	{
		PauseMusicForFocus(false);
		SetLoopSoundsMuted(false);
		if (mHallScreen != NULL)
			((HallOfFame*) mHallScreen)->mInactive = false;
		else if (mMainMenu != NULL)
			mMainMenu->mInactive = false;
	}
	if (mEngineLoop != NULL && mBoard != NULL && !mBoard->HasWarpState() && mBoard->mPauseCount <= 0)
		mEngineLoop->Play(true, false);
	if (mHyperspaceLoop != NULL && mBoard != NULL && mBoard->HasWarpState() && mBoard->mPauseCount <= 0)
		mHyperspaceLoop->Play(true, false);
}

// 0x425380 slot 63: the music and the loops stop, a game pauses, the selector and the hall of fame stand still
void AstroPopApp::LostFocus()
{
	PauseMusicForFocus(true);
	SetLoopSoundsMuted(true);
	if (mBoard != NULL)
		mBoard->Pause(true, true);
	else if (mHallScreen != NULL)
		((HallOfFame*) mHallScreen)->mInactive = true;
	else if (mMainMenu != NULL)
		mMainMenu->mInactive = true;
}

// 0x4252b0: lost focus pauses the music slots that play; regained focus resumes them
void AstroPopApp::PauseMusicForFocus(bool lost)
{
	if (mMusicInterface == NULL)
		return;
	if (lost)
	{
		for (int i = 0; i < NUM_MUSIC_SLOTS; i++)
		{
			if (mMusicInterface->IsPlaying(i))
			{
				mPausedSongs.push_back(i);
				mMusicInterface->PauseMusic(i);
			}
		}
		return;
	}
	for (size_t i = 0; i < mPausedSongs.size(); i++)
		mMusicInterface->ResumeMusic(mPausedSongs[i]);
	mPausedSongs.clear();
}

// 0x408db0: the two loops (engine, hyperspace) are muted while the window has no focus
void AstroPopApp::SetLoopSoundsMuted(bool muted)
{
	if (muted)
	{
		if (mEngineLoop != NULL)
			mEngineLoop->SetVolume(0.0);
		if (mHyperspaceLoop != NULL)
			mHyperspaceLoop->SetVolume(0.0);
	}
	else
	{
		if (mEngineLoop != NULL)
			mEngineLoop->SetVolume(1.0);
		if (mHyperspaceLoop != NULL)
			mHyperspaceLoop->SetVolume(1.0);
	}
}

// 0x408b90: two copies of music.mo3 are loaded as songs 0 and 1; a new position fades in on the other copy while the
// current one fades out (positions 9 and 10 are one song; 0x23 and 0x29 give way to 9)
void AstroPopApp::PlaySong(int theOrder)
{
	int aCurrent = mMusicOrder;
	if ((aCurrent == 0x23 || aCurrent == 0x29) && theOrder == 9)
		theOrder = 10;
	if (mMusicInterface == NULL)
		return;
	if (theOrder == aCurrent)
		return;
	if (theOrder == 9)
	{
		if (aCurrent == 10)
			return;
	}
	else if (theOrder == 10)
	{
		if (aCurrent == 9)
			return;
	}

	int aNewSlot = (mMusicSlot == 0) ? 1 : 0;
	if (mMusicInterface->IsPlaying(0) && mMusicInterface->IsPlaying(1))
	{
		mMusicInterface->StopMusic(mMusicSlot);
		mMusicInterface->FadeIn(aNewSlot, theOrder, 0.05f, false);
		mMusicSlot = aNewSlot;
		mMusicOrder = theOrder;
		return;
	}
	if (mMusicInterface->IsPlaying(mMusicSlot))
		mMusicInterface->FadeOut(mMusicSlot, true, 0.05f);
	mMusicInterface->FadeIn(aNewSlot, theOrder, 0.05f, false);
	mMusicSlot = aNewSlot;
	mMusicOrder = theOrder;
}

// 0x431ac0 slot 4: the loader's play button (id 2200) starts the game selector; the rest comes with the screens
// 0x408cc0: a sample panned by the column of the playfield it happens in (the middle ones are not panned)
void AstroPopApp::PlaySampleAt(int theSound, int theColumn)
{
	if (mSoundManager == NULL)
		return;
	float aPan = 0.0f;
	if (theColumn > 5)
		aPan = (float) theColumn * 0.1f;
	else if (theColumn < 5)
		aPan = (float) theColumn * 0.1f * 0.2f - 1.0f;
	PlaySample(theSound, (int) (aPan * 2000.0f));
}

// 0x408d30
void AstroPopApp::PlaySamplePitched(int theSound, int thePitch)
{
	if (mSoundManager == NULL)
		return;
	SoundInstance* anInstance = mSoundManager->GetSoundInstance(theSound);
	if (anInstance != NULL)
	{
		anInstance->AdjustPitch((double) thePitch);
		anInstance->Play(false, true);
	}
}

// 0x409250
void AstroPopApp::PlayComboPing(int thePitch)
{
	if (mComboPingTimer < 1)
	{
		PlaySamplePitched(SOUND_COMBO_PING, thePitch);
		mComboPingTimer = 0x2d;
	}
}

// 0x42c910: the thirty bonus level files data\bonus<n>.xcf are read and their signatures checked. The port leaves the check
// out (docs/GAME_NOTES.md, 5): the Board reads the file of a bonus round itself when the round starts (mBonusRoundFile), and the
// loading screen's progress counts the step.
bool AstroPopApp::LoadBonusLevels()
{
	mCompletedLoadingThreadTasks += 0x20;
	return true;
}

