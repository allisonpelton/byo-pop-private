// WinWSApp.cpp: the application class of Typer Shark Deluxe 1.02 (WinTS.exe): construction, start-up, loading, the
// music, the small virtuals and the helpers of the app. The screens and dialogs it shows are in WinWSAppScreens.cpp,
// the button handler (0x431630) in WinWSAppButtons.cpp.
//
// Original: class Sexy::WinWSApp : public SexyApp, vftable 0x52d734, sizeof 0x930 (WinWSApp.h).

#include "WinWSApp.h"
#include "GameText.h"
#include "Compat2004.h"
#include "Board.h"
#include "TitleScreen.h"
#include "MenuScreen.h"
#include "HighScoreScreen.h"
#include "StatsScreen.h"
#include "MapScreen.h"
#include "TypingTutor.h"
#include "StatusBar.h"
#include "UserProfileMgr.h"
#include "UserProfile.h"
#include "LessonManager.h"
#include "WidgetManager.h"
#include "Dialog.h"
#include "DialogButton.h"
#include "ImageFont.h"
#include "DDImage.h"
#include "MemoryImage.h"
#include "MusicInterface.h"
#include "SDLMusicInterface.h"
#include "SoundManager.h"
#include "SoundInstance.h"
#include "Insets.h"
#include "Color.h"
#include "Common.h"

#include <SDL.h>

using namespace Sexy;

WinWSApp* Sexy::gApp = NULL;        // 0x565380
MTRand* Sexy::gGameRand = NULL;     // 0x565384

// 0x4aa... (Sexy::SexyApp::GetImage's caller 0x41ad70 / 0x428500 / 0x4337a0): after every image load the original
// calls the image's slot 25, Palletize
static void PalletizeImage(Image* theImage)
{
	MemoryImage* aMemoryImage = dynamic_cast<MemoryImage*>(theImage);
	if (aMemoryImage != NULL)
		aMemoryImage->Palletize();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Construction and destruction

// 0x4373b0
WinWSApp::WinWSApp()
{
	gApp = this; // port: WinMain stored it (0x565380)

	// port: WinMain (0x401000) sets the size right after making the app
	mWidth = GAME_WIDTH;
	mHeight = GAME_HEIGHT;

	// port: the texts are read from the game's exe by InitPropertiesHook, which runs after this constructor; the
	// original's mProductVersion is the text at 0x52d868 and the title "Typer Shark Deluxe " (0x52d854) + it, set
	// again from the texts there
	mProductVersion = "1.02";
	mTitle = "Typer Shark Deluxe " + mProductVersion;

	mMessageDialog = NULL;
	mDialog61c = NULL;
	mHintDialog = NULL;
	mMenuScreen = NULL;
	mMapScreen = NULL;
	mAbyssModeDialog = NULL;
	mBoard = NULL;
	mHighScoreScreen = NULL;
	mStatsScreen = NULL;
	mHighScoreCloseButton = NULL;
	mMusic = NULL;
	mLessonManager = NULL;
	mFlag660 = false;
	mUsedUpperCount = 0;
	mUsedLetterCount = 0;
	mFlag670 = false;
	mFlag671 = false;
	mDeleteUserDialog = NULL;
	mMusicSlot = 1;
	mMusicOrder = -1;
	mAbyssDepth = 0;
	mFlag684 = false;
	mFlag685 = false;
	mOpenDialogCount = 0;
	mFlag68c = false;
	mFlag68d = false;
	mDifficulty = 0;
	mLongWordLength = 0;
	mTreasure = 0;
	mStats = new AppStats();    // port: the original's operator new(0x30) is not cleared
	mTypingTutor = NULL;
	mTheme = -1;
	mSoundInstance0 = NULL;
	mSoundInstance1 = NULL;
	mSoundInstance1Copy = NULL;
	mStatsCloseButton = NULL;
	mRotorTorpedoDim = NULL;
	mTorpedoSwimDim = NULL;
	mToxicSwimDim = NULL;
	mToxicGlowSwimDim = NULL;
	mGhostSharkSwimDim = NULL;
	mGhostSharkGlowDim = NULL;
	mCannonballDim = NULL;
	mTitleScreen = NULL;
	mOptionsDialog = NULL;

	mFontGillSansMT10Bold = NULL;
	mFontMyriadCondensed28Bold = NULL;
	mFontMyriadCondensed28BoldGL = NULL;
	mFontMyriadCondensed12Bold = NULL;
	mFontMyriadCondensed14Bold = NULL;
	mFontSwissCheesed24 = NULL;
	mFontUnsteadyOversteer22 = NULL;
	mFontRockwell13 = NULL;
	mFontRockwell13Bold = NULL;
	mFontMyriadCondensed15 = NULL;
	mFontMyriadCondensed15b = NULL;
	mFont12pt = NULL;
	mFont12ptGlow = NULL;
	mFontCourierFinalDraft15Bold = NULL;

	mProdName = "TyperShark";
	mRegKey = "PopCap\\TyperShark";
	mIsWindowed = true;
	mAutoEnable3D = true;

	mFlag8d8 = false;
	mFlag8d9 = false;
	mStatusBar = NULL;
	mSoundInstance38 = NULL;
	mProfileMgr = NULL;
	mUser = NULL;
	mFastCPU = false;
	mHintsAllowed = true;
	mImages = NULL;
	mFlag8f8 = true;
	mField8fc = 0;
	mField900 = 0;
	mField904 = 0;
	mField908 = 0;
	mField90c = 0;
	mFlag910 = false;

	mNumLoadingThreadTasks = NUM_LOADING_TASKS;
}

// 0x4366a0 (the scalar deleting destructor 0x437960 slot 14 frees the object): deletes what the app owns, in the
// original's order. The containers (strings, vectors, maps) free themselves.
WinWSApp::~WinWSApp()
{
	mWidgetManager->MarkAllDirty();

	// the dialogs still open
	static const int aDialogIds[] = {0x30c, 0x30a, 0x451, 0x1f4, 0x8d};
	for (int i = 0; i < 5; i++)
	{
		if (GetDialog(aDialogIds[i]) != NULL)
			KillDialog(aDialogIds[i]);
	}

	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		delete mBoard;
		mBoard = NULL;
	}

	if (mMapScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mMapScreen);
		delete mMapScreen;
		mMapScreen = NULL;
	}

	SetCursor(CURSOR_POINTER);

	if (mTypingTutor != NULL)
		mWidgetManager->RemoveWidget(mTypingTutor);
	delete mTypingTutor;

	for (size_t i = 0; i < mSonarSweepImages.size(); i++)
		delete mSonarSweepImages[i];
	for (size_t i = 0; i < mSonarSweep2Images.size(); i++)
		delete mSonarSweep2Images[i];

	if (mHighScoreCloseButton != NULL)
	{
		mWidgetManager->RemoveWidget(mHighScoreCloseButton);
		delete mHighScoreCloseButton;
	}

	delete mCannonballDim;
	delete mFontRockwell13;
	delete mFontRockwell13Bold;
	delete mFontUnsteadyOversteer22;
	delete mFontSwissCheesed24;
	delete mFontMyriadCondensed28Bold;
	delete mFontMyriadCondensed28BoldGL;
	delete mFontGillSansMT10Bold;
	delete mFontMyriadCondensed12Bold;
	delete mFontMyriadCondensed14Bold;
	delete mFontMyriadCondensed15b;
	delete mFontMyriadCondensed15;

	delete mProfileMgr;     // 0x41f090 then free
	mProfileMgr = NULL;

	delete mFont12pt;
	delete mFont12ptGlow;
	// port: the original never deletes the Courier font (a leak)
	delete mFontCourierFinalDraft15Bold;

	delete mStats;

	if (mStatsScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mStatsScreen);
		delete mStatsScreen;
	}

	if (mHighScoreScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mHighScoreScreen);
		delete mHighScoreScreen;
	}

	if (mMusic != NULL)
	{
		delete mMusic;
		mMusic = NULL;
	}

	if (mMenuScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mMenuScreen);
		delete mMenuScreen;
		mMenuScreen = NULL;
	}

	if (mLessonManager != NULL)
		delete mLessonManager;      // 0x421c50, then free()
	mLessonManager = NULL;

	if (mSoundInstance0 != NULL)
		mSoundInstance0->Release();
	if (mSoundInstance1 != NULL)
		mSoundInstance1->Release();
	if (mSoundInstance38 != NULL)
		mSoundInstance38->Release();

	for (size_t i = 0; i < mColorizedFish.size(); i++)
		delete mColorizedFish[i];

	delete mToxicSwimDim;
	delete mToxicGlowSwimDim;
	delete mGhostSharkGlowDim;
	delete mGhostSharkSwimDim;
	delete mRotorTorpedoDim;
	delete mTorpedoSwimDim;

	// the owned entries of the two shark image tables (the others alias mImages); operator[] as the original,
	// which makes the empty entries
	for (int aType = 0; aType < 6; aType++)
		delete mSharkImages[aType][3];
	delete mSharkImages[9][3];
	delete mSharkImages[9][0];
	delete mSharkImages[9][2];
	delete mSharkImages[10][0];
	delete mSharkImages[10][2];
	delete mSharkImages[3][0];
	delete mSharkImages[3][2];
	delete mSharkImages[10][3];
	delete mPiranhaImages[0][3];
	delete mPiranhaImages[6][3];
	delete mPiranhaImages[2][3];
	delete mPiranhaImages[7][0];
	delete mPiranhaImages[7][2];
	delete mPiranhaImages[3][0];
	delete mPiranhaImages[3][2];
	delete mSharkImages[2][4];
	delete mSharkImages[1][4];
	delete mSharkImages[0][4];
	delete mSharkImages[4][4];

	if (mImages != NULL)
	{
		for (int i = 0; i < NUM_IMAGES; i++)
			delete mImages[i];
		delete[] mImages;
		mImages = NULL;
	}

	if (mStatsCloseButton != NULL)
	{
		mWidgetManager->RemoveWidget(mStatsCloseButton);
		delete mStatsCloseButton;
	}

	// leak fix (the original leaks the game generator, made in Init and never deleted): everything that draws from it is gone by now
	delete gGameRand;
	gGameRand = NULL;

	// (0x42fd10 erases mLongWords, 0x431510 / 0x429780 / 0x427000 destroy mWordLists, mThemes and the name lists: in the port
	// the members' destructors do it)
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Start-up

// port: SexyAppBase::Init runs this first. The texts come from the game folder's executable (GameText.h);
// properties/texts.xml, when present (a mod or language pack), overrides any of them by id.
void WinWSApp::InitPropertiesHook()
{
	SexyApp::InitPropertiesHook();

	LoadProperties("properties/texts.xml", false, false);
	if (!LoadGameTexts())
		SDL_Log("TyperShark: no game texts read from WinTS.exe (texts without an override stay empty)");

	// the constructor's version and title (0x4373b0) now that the texts exist
	if (!GT(0x0052d868).empty())
	{
		mProductVersion = GT(0x0052d868);
		mTitle = GT(0x0052d854) + mProductVersion;
	}
}

// 0x41ac70 (named BookWormApp::CheckRegistryAccess in the export): the original tests that it may write the registry
// (the emulated registry always can)
bool WinWSApp::CheckRegistryAccess()
{
	if (!RegistryWriteInteger(GT(0x0052b330), 1))
	{
		if (!mIsWindowed)
			SwitchScreenMode(true, Is3DAccelerated());
		MsgBox(GT(0x0052b2c8), GT(0x0052b31c), MB_ICONERROR);
		return false;
	}
	return true;
}

// the font step shared by Init (0x428500) and LoadingThreadProc (0x435560): the font is stored even when it failed
bool WinWSApp::LoadFont(ImageFont*& theFont, const char* thePath)
{
	theFont = new ImageFont(this, thePath);
	if (!theFont->mFontData->mInitialized)
	{
		Popup(StrFormat(GTC(0x0052c790), thePath));
		mLoadingFailed = true;
		return false;
	}
	theFont->Prepare();
	return true;
}

// 0x428500 slot 16
void WinWSApp::Init()
{
	SexyApp::Init();

	// port: the 2004 SexyApp::Init seeds the C library generator with srand(GetTickCount()); -seed=N test runs seed it
	// with N, as they do the framework's Rand()
	CrtSRand(mHasSeedParam ? (unsigned int) mRandSeed : (unsigned int) GetTickCount());

	// The 2004 SexyApp::Init seeds Rand()'s generator the SAF 1.2 way with its GetTickCount(); the port's
	// SexyAppBase::Init seeded it the 1.3 way. Seeded again here as the original was, so that a seed gives the
	// original's draws (the game generator's seed among them, below) and nothing has drawn from it yet
	{
		MTRand aRand;
		SeedMTRand2004(aRand, (unsigned long) mRandSeed);
		uint32_t aState[MTRAND_N];
		int anIndex;
		aRand.GetState(aState, &anIndex);
		SetRandMTState(aState, anIndex);
	}

	if (!CheckRegistryAccess())
	{
		Shutdown();
		return;
	}

	// the module music interface (BassMusicInterface(mHWnd) in the original), apart from SexyAppBase's own
	mMusic = CreateMusicInterface();
	mMusic->SetVolume(mMusicVolume);

	mTitleScreen = new TitleScreen(this);
	mTitleScreen->Resize(0, 0, mWidth, mHeight);
	mTitleScreen->AddControls();    // TitleScreen::AddControls 0x448f40
	mWidgetManager->AddWidget(mTitleScreen);

	// the game generator: one draw of the framework's generator, seeded the 2004 way
	gGameRand = new MTRand();
	SeedMTRand2004(*gGameRand, (unsigned long) Sexy::Rand());

	mProfileMgr = new UserProfileMgr(this);     // 0x427440
	if (mProfileMgr->GetCurrentUserName() != "")
	{
		mUser = mProfileMgr->LoadUser(mProfileMgr->GetCurrentUserName());   // 0x425f20
		if ((mUser != NULL) && mUser->mFlag1dc)
			mFlag8f8 = false;
	}

	// the images the loading screen needs (the rest come in LoadingThreadProc): the cursors and the dialog parts
	mImages = new Image*[NUM_IMAGES];
	for (int i = 0; i < NUM_IMAGES; i++)
		mImages[i] = NULL;

	for (int i = 0xf2; i < 0xf8; i++)
	{
		mImages[i] = GetImage(std::string("images\\") + gImageNames[i]);
		if (mImages[i] == NULL)
		{
			Popup(GT(0x0052b338) + gImageNames[i] + GT(0x0052b350));
			Shutdown();
			return; // port: the original goes on with the next image after Shutdown, on the freed table
		}
	}

	SetCursorImage(CURSOR_POINTER, mImages[IMAGE_CURSOR1]);
	SetCursorImage(CURSOR_HAND, mImages[IMAGE_CURSOR1 + 1]);
	SetCursorImage(CURSOR_DRAGGING, mImages[IMAGE_CURSOR1 + 2]);
	SetCursorImage(CURSOR_TEXT, mImages[IMAGE_CURSOR1 + 3]);

	if (!LoadFont(mFontRockwell13, "data\\Rockwell13.txt"))
		return;
	LoadFont(mFontRockwell13Bold, "data\\Rockwell13Bold.txt");
}

// 0x41ad70: images 0..0xf1 (Init loaded the last six); the bar advances with each
bool WinWSApp::LoadImages()
{
	int i = 0;
	while (true)
	{
		mImages[i] = GetImage(std::string("images\\") + gImageNames[i]);
		if (mImages[i] == NULL)
			break;
		PalletizeImage(mImages[i]);
		mCompletedLoadingThreadTasks++;
		mTitleScreen->MarkDirty();
		i++;
		if (i > 0xf1)
			return true;
	}

	Popup(GT(0x0052b338) + gImageNames[i] + GT(0x0052b350));
	return false;
}

// 0x435560 slot 17 (runs synchronously in the port): the music, the fonts, the data files (waves, words, gem and
// ship names, themes), the images, the derived images and the sounds. mCompletedLoadingThreadTasks drives the bar.
void WinWSApp::LoadingThreadProc()
{
	for (int i = 0; i < 2; i++)
	{
		mMusic->LoadMusic(i, "music\\Wordshar.mo3");
		mCompletedLoadingThreadTasks++;
		mTitleScreen->MarkDirty();
	}
	ChangeMusic(0x11);

	// the fonts: the bar advances after each
	ImageFont** aFonts[] = {&mFontGillSansMT10Bold, &mFontCourierFinalDraft15Bold, &mFont12pt, &mFont12ptGlow,
		&mFontMyriadCondensed28BoldGL, &mFontMyriadCondensed28Bold, &mFontUnsteadyOversteer22,
		&mFontMyriadCondensed12Bold, &mFontMyriadCondensed14Bold, &mFontMyriadCondensed15, &mFontMyriadCondensed15b,
		&mFontSwissCheesed24};
	const char* aFontPaths[] = {"data\\GillSansMT10Bold.txt", "data\\CourierFinalDraft15Bold.txt", "data\\12ptfont.txt",
		"data\\12ptfontglow.txt", "data\\MyriadCondensedWeb28BoldGL.txt", "data\\MyriadCondensedWeb28Bold.txt",
		"data\\UnsteadyOversteer22.txt", "data\\MyriadCondensedWeb12Bold.txt", "data\\MyriadCondensedWeb14Bold.txt",
		"data\\MyriadCondensedWeb15.txt", "data\\MyriadCondensedWeb15.txt", "data\\SwissCheesed24.txt"};
	for (int i = 0; i < 12; i++)
	{
		if (!LoadFont(*aFonts[i], aFontPaths[i]))
			return;
		mCompletedLoadingThreadTasks++;
		mTitleScreen->MarkDirty();
	}

	// the data files; each reports itself missing with its own file name
	if (!LoadWaves())               // 0x434740
	{
		Popup(StrFormat(GTC(0x0052c790), GTC(0x0052d488)));
		mLoadingFailed = true;
		return;
	}
	mCompletedLoadingThreadTasks++;
	mTitleScreen->MarkDirty();

	if (!LoadWordLists())           // 0x430d20
	{
		Popup(StrFormat(GTC(0x0052c790), GTC(0x0052d478)));
		mLoadingFailed = true;
		return;
	}
	mCompletedLoadingThreadTasks++;
	mTitleScreen->MarkDirty();

	if (!LoadGemNames())            // 0x42b0c0
	{
		Popup(StrFormat(GTC(0x0052c790), GTC(0x0052d468)));
		mLoadingFailed = true;
		return;
	}
	mCompletedLoadingThreadTasks++;
	mTitleScreen->MarkDirty();

	if (!LoadShipNames())           // 0x42b400
	{
		Popup(StrFormat(GTC(0x0052c790), GTC(0x0052d458)));
		mLoadingFailed = true;
		return;
	}
	mCompletedLoadingThreadTasks++;
	mTitleScreen->MarkDirty();

	if (!LoadThemes())              // 0x434d60
	{
		Popup(StrFormat(GTC(0x0052c790), GTC(0x0052d448)));
		mLoadingFailed = true;
		return;
	}
	mCompletedLoadingThreadTasks++;
	mTitleScreen->MarkDirty();

	if (!LoadImages())              // 0x41ad70 (has shown its own message)
	{
		mLoadingFailed = true;
		return;
	}

	MakeDerivedImages();            // 0x4337a0
	mCompletedLoadingThreadTasks++;
	mTitleScreen->MarkDirty();

	for (int i = 0; i < 0x28; i++)
	{
		std::string aPath = std::string("sounds\\") + gSoundNames[i];
		if (!mSoundManager->LoadSound(i, aPath))
		{
			Popup(StrFormat(GTC(0x0052c790), aPath.c_str()));
			mLoadingFailed = true;
			return;
		}
		mCompletedLoadingThreadTasks++;
		mTitleScreen->MarkDirty();
	}

	mLessonManager = NULL;
	mBoard = NULL;

	// 0x462b10 PerfTimer::GetCPUSpeed(): the original treats machines under 695 MHz as slow
	mFastCPU = true;    // port: every machine that runs the port is faster

	mCompletedLoadingThreadTasks += 2;
	mSoundInstance0 = mSoundManager->GetSoundInstance(0);
	mCompletedLoadingThreadTasks++;
	mTitleScreen->MarkDirty();

	mSoundInstance1 = mSoundManager->GetSoundInstance(1);
	mCompletedLoadingThreadTasks++;
	mTitleScreen->MarkDirty();

	mSoundInstance1Copy = mSoundInstance1;
	mSoundInstance38 = mSoundManager->GetSoundInstance(0x26);
	mCompletedLoadingThreadTasks++;
	mTitleScreen->MarkDirty();

	ButtonDepress(0x191);   // the game goes straight on (0x191: what the title screen's continue button would press)
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// The small virtuals

// 0x4127a0 slot 7
void WinWSApp::UpdateFrames()
{
	SexyApp::UpdateFrames();
	if (mMusic != NULL)     // port: the original does not test
		mMusic->Update();
}

// 0x4127c0 slot 47
void WinWSApp::SetMusicVolume(double theVolume)
{
	SexyApp::SetMusicVolume(theVolume);
	if (mMusic != NULL)
		mMusic->SetVolume(mMusicVolume);
}

// 0x412820 slot 3: every button press clicks
void WinWSApp::ButtonPress(int theId)
{
	(void) theId;
	PlaySample(SOUND_BUTTONCLICK);
}

// 0x412b30 slot 20: the plain dialog (Rockwell 13, black text)
Dialog* WinWSApp::NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
	const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	Dialog* aDialog = new Dialog2004(mImages[IMAGE_DIALOG], mImages[IMAGE_DBUTTON], theDialogId, isModal,
		theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);
	aDialog->SetButtonFont(mFontRockwell13);
	aDialog->SetHeaderFont(mFontRockwell13Bold);
	aDialog->SetLinesFont(mFontRockwell13);
	aDialog->SetColor(Dialog::COLOR_LINES, Color(0, 0, 0));
	aDialog->SetColor(Dialog::COLOR_HEADER, Color(0, 0, 0));
	aDialog->SetColor(Dialog::COLOR_FOOTER, Color(0, 0, 0));
	aDialog->mContentInsets = Insets(0x28, 0x14, 0x28, 0x28);
	aDialog->mSpaceAfterHeader = 0x19;
	aDialog->Resize(0xed, 0x32, 0x13c, aDialog->GetPreferredHeight(0x132));
	return aDialog;
}

// 0x412d20 slot 18: the settings are written with the mute count at 0
void WinWSApp::WriteToRegistry()
{
	int aMuteCount = mMuteCount;
	mMuteCount = 0;
	SexyApp::WriteToRegistry();
	mMuteCount = aMuteCount;
}

// 0x4269e0 slot 63: the music pauses and the board with it
void WinWSApp::LostFocus()
{
	SexyApp::LostFocus();
	PauseMusic(true);
	if (mBoard != NULL)
		mBoard->SetPaused(true);    // Board::SetPaused 0x442100
}

// 0x426a10 slot 62
void WinWSApp::GotFocus()
{
	SexyApp::GotFocus();
	PauseMusic(false);
	if (mBoard != NULL)
		mBoard->SetPaused(false);   // 0x442100
}

// port: the 2004 SexyApp creates a BassMusicInterface (BASS 2.0), which sets the module music's global volume to the
// full music volume: the port's music interface gets the full range (docs/FRAMEWORK_2004.md section 6, 4.2). Both the
// framework's own interface and the app's (mMusic) are made here.
MusicInterface* WinWSApp::CreateMusicInterface()
{
	if (mNoSoundNeeded)
		return new MusicInterface;

	SDLMusicInterface* aMusicInterface = new SDLMusicInterface();
	aMusicInterface->mMaxMusicVolume = 100;
	return aMusicInterface;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Music

// 0x426920: true remembers (and pauses) the songs that play, false resumes the remembered ones
void WinWSApp::PauseMusic(bool thePause)
{
	if (thePause)
	{
		int aSong = 0;
		do
		{
			if (mMusic->IsPlaying(aSong))
			{
				mPausedMusic.push_back(aSong);
				mMusic->PauseMusic(aSong);
			}
			aSong++;
		} while (aSong < 2);
		return;
	}

	size_t i = 0;
	while (i < mPausedMusic.size())
	{
		mMusic->ResumeMusic(mPausedMusic[i]);
		i++;
	}
	mPausedMusic.clear();   // (the original frees the vector's memory)
}

// 0x414350: plays module order theOrder on the other of the two songs and stops the one that plays; false when it is
// already playing (or the board is in a mode that keeps its music). The orders: 0x11 menu, 0x2e map, 0x25 / 0x2b
// board themes, 0x22 ...
bool WinWSApp::ChangeMusic(int theOrder)
{
	double aSpeed = 0.005;
	bool aFlag = false;

	if ((theOrder == 0x22) && (mMusicOrder == 0x22))
		return false;

	int aPrevOrder = mMusicOrder;
	if (aPrevOrder == theOrder)
		return false;

	if ((theOrder == 0x25) || (theOrder == 0x2b))
	{
		if (mBoard->mPlayer->mState == 3)   // +0xd68 -> +0xb8
			return true;
	}

	if (theOrder == 0x25)
	{
		aFlag = false;
		aSpeed = 0.0;
	}
	else if (theOrder == 0x2b)
	{
		aFlag = true;
		aSpeed = 0.0;
	}

	if (((aPrevOrder == 0x11) || (aPrevOrder == 0x14)) && (theOrder == 0))
		aSpeed = 0.0025;
	else if (theOrder == 0x2e)
	{
		aSpeed = 0.015;
		aFlag = true;
	}

	if (mLessonManager != NULL)
	{
		if (mLessonManager->IsBonusLevel() && (theOrder == 0x11) && (aPrevOrder == 0x2e))    // LessonManager::IsBonusLevel 0x4137d0
		{
			if (mMusic->IsPlaying(0))
				return false;
			if (mMusic->IsPlaying(1))
				return false;
		}
	}

	if (aSpeed == 0.0)
		aSpeed = 0.8;

	int aNewSlot = (mMusicSlot == 0);
	if (mMusic->IsPlaying(0) && mMusic->IsPlaying(1))
		mMusic->StopMusic(mMusicSlot);
	else if (mMusic->IsPlaying(mMusicSlot))
	{
		if (aSpeed != 0.0)
			mMusic->StopMusic(mMusicSlot);
		else
			mMusic->FadeOut(mMusicSlot, true, aSpeed);  // (not reached: aSpeed is never 0 here)
	}

	if (aSpeed == 0.0)
		mMusic->PlayMusic(aNewSlot, theOrder, aFlag);   // (not reached: aSpeed is never 0 here)
	else
		mMusic->FadeIn(aNewSlot, theOrder, aSpeed, aFlag);

	mMusicSlot = aNewSlot;
	mMusicOrder = theOrder;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Small helpers

// 0x412830: the treasure of the game (10..15), drawn the first time it is asked for
int WinWSApp::GetTreasureIndex()
{
	if (mTreasure == 0)
	{
		unsigned int aRand = gGameRand->NextNoAssert();
		mTreasure = aRand % 6 + 10;
	}
	return mTreasure;
}

// 0x41b920: the name of the treasure; the table (0x55f948) is Res.cpp's gTreasureNameAddrs, indexed with
// GetTreasureIndex() - 10 (the original's table holds the six names from index 10 on)
std::string WinWSApp::GetTreasureName()
{
	int anIndex = GetTreasureIndex();
	return GT(gTreasureNameAddrs[anIndex - 10]);
}

// 0x41b4e0: a character that has not been handed out yet. theOnlyLetters: only A..Z (at most 26 of them); otherwise
// letters, digits 1..9 and four symbols (39 in all; after the first 16 draws also the digits and symbols). "" when the
// limit is reached. A hit on a character already used draws again.
std::string WinWSApp::RandomLetter(bool theOnlyLetters)
{
	if (!theOnlyLetters)
	{
		if (mUsedLetterCount + mUsedUpperCount > 0x26)
			return "";
	}
	else if (mUsedUpperCount > 0x19)
		return "";

	char aChar = 0;
	while (true)
	{
		unsigned int aRand;
		unsigned int aModulus;
		if ((!theOnlyLetters) && (mUsedLetterCount < 0x10))
		{
			aRand = gGameRand->NextNoAssert();
			aModulus = 0x27;
		}
		else
		{
			aRand = gGameRand->NextNoAssert();
			aModulus = 0x1a;
		}
		unsigned int aValue = (unsigned int) ((unsigned long long) aRand % (unsigned long long) aModulus);

		if (aValue < 0x1a)
			aChar = (char) (aValue + 'A');
		else if (aValue < 0x23)
			aChar = (char) (aValue + 0x17);     // '1'..'9'
		else
		{
			switch (aValue)
			{
			case 0x23: aChar = 0x23; break;     // '#'
			case 0x24: aChar = 0x24; break;     // '$'
			case 0x25: aChar = 0x26; break;     // '&'
			case 0x26: aChar = 0x29; break;     // ')'
			}
		}

		size_t aPos = 0;
		if (mUsedLetters.size() == 0)
			break;
		while (true)
		{
			if (mUsedLetters[aPos] == aChar)
				break;
			aPos++;
			if (aPos >= mUsedLetters.size())
				goto NotUsed;
		}
	}

NotUsed:
	mUsedLetters.append(1, aChar);
	if ((aChar > '@') && (aChar < '['))
		mUsedUpperCount++;
	mUsedLetterCount++;
	return std::string(1, aChar);
}

// 0x41d210: takes the first occurrence of theLetter out of mUsedLetters
void WinWSApp::RemoveLetter(char theLetter)
{
	size_t aPos = 0;
	if (mUsedLetters.size() != 0)
	{
		do
		{
			if (mUsedLetters[aPos] == theLetter)
			{
				std::string aRest = mUsedLetters.substr(0, aPos);
				aRest.append(mUsedLetters.substr(aPos + 1, mUsedLetters.size() - aPos - 1));
				mUsedLetters = aRest;
				return;
			}
			aPos++;
		} while (aPos < mUsedLetters.size());
	}
}

// 0x41d7d0: the points of the game so far
int WinWSApp::GetPointsSoFar()
{
	if (mStats != NULL)
		return mStats->mPointsSoFar;

	Popup("***********No valid save data in WinWSApp::GetPointsSoFar************");   // 0x52c318 (a debug message; 0x4ad5f0)
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// The derived images

// 0x4337a0: copies of the loaded images the entities draw: colourised (the small fish, the glowing and red sharks),
// mirrored (the left-facing sharks) and flipped (the sonar sweeps). Every copy is palletized as it is made.
void WinWSApp::MakeDerivedImages()
{
	// a colourised copy of mImages[theImage]
	struct Local
	{
		static Image* Colorize(WinWSApp* theApp, int theImage, const Color& theColor)
		{
			Image* aCopy = theApp->CreateColorizedImage(theApp->mImages[theImage], theColor);
			PalletizeImage(aCopy);
			return aCopy;
		}
		// FUN_00412870: CopyImage, then Palletize
		static Image* Copy(WinWSApp* theApp, int theImage)
		{
			Image* aCopy = theApp->CopyImage(theApp->mImages[theImage]);
			PalletizeImage(aCopy);
			return aCopy;
		}
		// FUN_004ac0b0 MirrorImage, then Palletize
		static Image* Mirror(WinWSApp* theApp, Image* theImage)
		{
			theApp->MirrorImage(theImage);
			PalletizeImage(theImage);
			return theImage;
		}
	};

	// the small fish in three colours each
	mColorizedFish.push_back(Local::Colorize(this, IMAGE_SMALLCARN, Color(0x32, 0x32, 0xff, 0xff)));
	mColorizedFish.push_back(Local::Colorize(this, IMAGE_SMALLCARN, Color(0x82, 0x82, 200, 0xff)));
	mColorizedFish.push_back(Local::Colorize(this, IMAGE_SMALLCARN, Color(0xb9, 0xb9, 0x9b, 0xff)));
	mColorizedFish.push_back(Local::Colorize(this, IMAGE_SMALLGOLDFISH, Color(0x32, 0x32, 0xff, 0xff)));
	mColorizedFish.push_back(Local::Colorize(this, IMAGE_SMALLGOLDFISH, Color(0x82, 0x82, 200, 0xff)));
	mColorizedFish.push_back(Local::Colorize(this, IMAGE_SMALLGOLDFISH, Color(0xb9, 0xb9, 0x9b, 0xff)));

	// the darkened, mirrored projectiles and ghosts
	mRotorTorpedoDim = Local::Mirror(this, Local::Colorize(this, IMAGE_ROTOR_TORPEDO, Color(200, 200, 200)));
	mCannonballDim = Local::Mirror(this, Local::Colorize(this, IMAGE_CANNONBALL, Color(200, 200, 200)));
	mTorpedoSwimDim = Local::Mirror(this, Local::Colorize(this, IMAGE_TORPEDO_SWIM, Color(200, 200, 200)));
	mToxicSwimDim = Local::Mirror(this, Local::Colorize(this, IMAGE_TOXIC_BASIC_SWIM, Color(200, 200, 200)));
	mToxicGlowSwimDim = Local::Mirror(this, Local::Colorize(this, IMAGE_TOXIC_GLOW_SWIM, Color(200, 200, 200)));
	mGhostSharkSwimDim = Local::Mirror(this, Local::Colorize(this, IMAGE_SHARK_GHOST_SWIM, Color(200, 200, 200)));
	mGhostSharkGlowDim = Local::Mirror(this, Local::Colorize(this, IMAGE_SHARK_GHOST_GLOW_SWIM, Color(200, 200, 200)));

	// the shark table: [type][role], role 0 swim, 2 death, 3 mirrored swim, 4 red
	mSharkImages[2][3] = Local::Mirror(this, Local::Copy(this, IMAGE_SHARK_RED_SWIM));
	mSharkImages[2][4] = Local::Colorize(this, IMAGE_SHARK_RED_SWIM, Color(200, 0, 0));
	mSharkImages[9][0] = Local::Colorize(this, IMAGE_SHARK_RED_SWIM, Color(0xff, 0xff, 0xff, 0x80));
	mSharkImages[9][2] = Local::Colorize(this, IMAGE_SHARK_RED_DEATH, Color(0xff, 0xff, 0xff, 0x80));
	mSharkImages[0][0] = mImages[IMAGE_SHARK_BASIC_SWIM];
	mSharkImages[0][3] = Local::Mirror(this, Local::Copy(this, IMAGE_SHARK_BASIC_SWIM));
	mSharkImages[0][4] = Local::Colorize(this, IMAGE_SHARK_BASIC_SWIM, Color(200, 0, 0));
	mSharkImages[0][2] = mImages[IMAGE_SHARK_BASIC_DEATH];
	mSharkImages[3][0] = Local::Colorize(this, IMAGE_SHARK_BASIC_SWIM, Color(0xff, 0xff, 0xff, 0x80));
	mSharkImages[3][2] = Local::Colorize(this, IMAGE_SHARK_BASIC_DEATH, Color(0xff, 0xff, 0xff, 0x80));
	mSharkImages[1][3] = Local::Mirror(this, Local::Copy(this, IMAGE_SHARK_BLACK_SWIM));
	mSharkImages[1][4] = Local::Colorize(this, IMAGE_SHARK_BLACK_SWIM, Color(200, 0, 0));
	mSharkImages[10][0] = Local::Colorize(this, IMAGE_SHARK_BLACK_SWIM, Color(0xff, 0xff, 0xff, 0x80));
	mSharkImages[10][2] = Local::Colorize(this, IMAGE_SHARK_BLACK_DEATH, Color(0xff, 0xff, 0xff, 0x80));
	mSharkImages[4][2] = mImages[IMAGE_TOXIC_BASIC_DEATH];
	mSharkImages[4][4] = Local::Colorize(this, IMAGE_TOXIC_BASIC_SWIM, Color(200, 0, 0));

	// the piranha table
	mPiranhaImages[0][3] = Local::Mirror(this, Local::Copy(this, IMAGE_PIRANHA_BASIC_SWIM));
	mPiranhaImages[3][0] = Local::Colorize(this, IMAGE_PIRANHA_BASIC_SWIM, Color(0xff, 0xff, 0xff, 0x80));
	mPiranhaImages[3][2] = Local::Colorize(this, IMAGE_PIRANHA_BASIC_DEATH, Color(0xff, 0xff, 0xff, 0x80));
	mPiranhaImages[6][3] = Local::Mirror(this, Local::Copy(this, IMAGE_PIRANHA_WHITE_SWIM));
	mPiranhaImages[2][3] = Local::Mirror(this, Local::Copy(this, IMAGE_GUMBO_SWIM));
	mPiranhaImages[7][0] = Local::Colorize(this, IMAGE_PIRANHA_WHITE_SWIM, Color(0xff, 0xff, 0xff, 0x80));
	mPiranhaImages[7][2] = Local::Colorize(this, IMAGE_PIRANHA_WHITE_DEATH, Color(0xff, 0xff, 0xff, 0x80));

	// the two sonar sweeps, mirrored and flipped
	for (int i = 0; i < 5; i++)
	{
		Image* aImage = Local::Mirror(this, Local::Copy(this, IMAGE_SONAR_SWEEP_0 + i));
		FlipImage(aImage);
		PalletizeImage(aImage);
		mSonarSweepImages.push_back(aImage);
	}
	for (int i = 0; i < 5; i++)
	{
		Image* aImage = Local::Mirror(this, Local::Copy(this, IMAGE_SONAR_SWEEP2_0 + i));
		FlipImage(aImage);
		PalletizeImage(aImage);
		mSonarSweep2Images.push_back(aImage);
	}
}
