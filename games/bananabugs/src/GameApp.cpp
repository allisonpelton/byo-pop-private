#include "GameApp.h"

#include "Anim.h"
#include "BBFont.h"
#include "GameWidget.h"
#include "MusicInterface.h"
#include "ObjBase.h"
#include "Pending.h"
#include "Res.h"
#include "ResourceManager.h"
#include "SoundQueue.h"
#include "WidgetManager.h"
#include "XMLParser.h"

#include <SDL.h>

#include <cstring>

using namespace Sexy;

GameApp* Sexy::gApp = nullptr;

// 0x60e17f: partner.xml's DefaultWindowed
static bool gDefaultWindowed = false;

// The music's fades, 0.02 of the volume an update
const double MUSIC_FADE_SPEED = 0.02;

// 0x42c2b0. The original reads the framework's 3D choice ("Is3D") here: in 2D the window is 800x600, in 3D the
// desktop's size (full screen) or a share of it (windowed; see SwitchScreenMode). The port always takes the 800x600
// path (docs/GAME_NOTES.md, deviations).
GameApp::GameApp()
{
	gApp = this;
	mRegKey = "PopCap\\Banana Bugs";
	memset(mPartnerImage, 0, sizeof(mPartnerImage));
	memset(mRegistryKeyText, 0, sizeof(mRegistryKeyText));
	gDefaultWindowed = ReadPartnerFile();
	mRootScaleX = 1.0f;
	mRootScaleY = 1.0f;
	mForcedWidth = -1;
	mForcedHeight = -1;
	mFocused = true;
	mFocusLost = false;
	mGameWidget = nullptr;
	mRedFont = nullptr;
	mFont = nullptr;
	mSoundQueue = nullptr;
	mCurrentMusic = 0;
	mProdName = "Banana Bugs";
	mProductVersion = "1.0";
	mTitle = StringToSexyString(mProdName + " - " + mProductVersion);
	mFullscreenBits = 32;
	mAutoEnable3D = true;
	mForceFullscreen = false;
	// the original also reads its registration's ReferId and RegisterLink here (0x4ed8a0); registration is not ported
	if (gDefaultWindowed)
		mIsWindowed = true;

	mWidth = 800;
	mHeight = 600;
	SetForcedSize(800, 600);
	mRootScaleX = 1.0f;
	mScreenModeKind = 0;
	mRootScaleY = 1.0f;
	gScreenScaleX = (float)((float)mWidth / 1920.0);
	gScreenScaleY = (float)((float)mHeight / 1200.0);
	mDesignScaleX = 2.4f;
	mDesignScaleY = 2.0f;

	mLastWidth = mWidth;
	mLastHeight = mHeight;
	mWindowToDesignX = (float)(1920.0 / (float)mWidth);
	mWindowToDesignY = (float)(1200.0 / (float)mHeight);
	// from the desktop's shape here, and from the window's at the next change of mode
	mWidescreen = DesktopIsWidescreen();
	ObjBase::FollowScreenScale();
	// 0x4f7bc0, the framework's table of software blitters, is the framework's own business in the port
	ObjBase::InitEngine(this);
	ObjBase::SetBounds(this);
	SetCursor(CURSOR_NONE);
}

// 0x42c680
GameApp::~GameApp()
{
	SaveProfiles();
	if (mRedFont != nullptr) {
		delete mRedFont;
		if (mFont != nullptr)
			delete mFont;
		mRedFont = nullptr;
		mFont = nullptr;
	}
	mWidgetManager->RemoveAllWidgets(true, true);
	// the original never deletes the sound queue
	delete mSoundQueue;
	mSoundQueue = nullptr;
	if (gApp == this)
		gApp = nullptr;
}

// The constructor's test of the desktop (0x600564 and 0x600568 hold its size): wider than 4:3. The HUD's panels sit
// further left on a wide desktop, even in the 800x600 window, until the mode first changes.
bool GameApp::DesktopIsWidescreen()
{
	bool aStarted = SDL_WasInit(SDL_INIT_VIDEO) == 0;
	if (aStarted && SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
		return false;
	SDL_DisplayMode aMode;
	bool aWide = false;
	if (SDL_GetDesktopDisplayMode(0, &aMode) == 0 && aMode.h > 0)
		aWide = 133 < (aMode.w * 100) / aMode.h;
	if (aStarted)
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
	return aWide;
}

// 0x42c740: properties/partner.xml. Returns DefaultWindowed; the RegistryKey replaces the registry key, and the
// PartnerImage names a distributor's logo. ProdName and PartnerName are read and not used.
bool GameApp::ReadPartnerFile()
{
	XMLParser aParser;
	XMLElement anElement;
	std::string anId = "none";
	bool aDefaultWindowed = false;
	aParser.OpenFile("properties\\partner.xml");
	while (aParser.NextElement(&anElement)) {
		if (anElement.mType == XMLElement::TYPE_END) {
			if (SexyStringToString(anElement.mValue) == "/Properties")
				break;
		} else if (anElement.mType == XMLElement::TYPE_START) {
			anId = SexyStringToString(anElement.mAttributes[_S("id")]);
		} else if (anElement.mType == XMLElement::TYPE_ELEMENT) {
			std::string aText = SexyStringToString(anElement.mValue);
			if (anId == "DefaultWindowed") {
				aDefaultWindowed = aText == "true";
			} else if (anId == "ProdName") {
			} else if (anId == "RegistryKey") {
				strncpy(mRegistryKeyText, aText.c_str(), sizeof(mRegistryKeyText) - 1);
				mRegKey = mRegistryKeyText;
			} else if (anId == "PartnerName") {
			} else if (anId == "PartnerImage") {
				strncpy(mPartnerImage, aText.c_str(), sizeof(mPartnerImage) - 1);
			}
		}
	}
	return aDefaultWindowed;
}

// 0x42cfb0
void GameApp::HideCursor()
{
	gApp->SetCursor(CURSOR_NONE);
}

// 0x42cfc0
void GameApp::ShowCursor()
{
	gApp->SetCursor(CURSOR_POINTER);
}

// 0x42cfd0: the song fades out and the new one in
void GameApp::ChangeMusic(int theSong)
{
	if (theSong != mCurrentMusic) {
		mMusicInterface->FadeOut(mCurrentMusic, true, MUSIC_FADE_SPEED);
		mMusicInterface->FadeIn(theSong, 0, MUSIC_FADE_SPEED, false);
		mCurrentMusic = theSong;
	}
}

// 0x42d030. The original also moves an old "userdata" folder into the user's application data (the port's user
// folder is main's), loads its JPEG 2000 codec (j2k-codec.dll) with the codec's key (the port's codec is OpenJPEG,
// main.cpp), and then reads the resource manifest.
void GameApp::Init()
{
	SexyApp::Init();
	LoadResourceManifest();
}

// 0x42d1f0: the sounds, then the fonts
void GameApp::LoadingThreadProc()
{
	mResourceManager->StartLoadResources("SoundFX");
	while (mResourceManager->LoadNextResource()) {
		mCompletedLoadingThreadTasks++;
		if (mShutdown)
			return;
	}
	if (!mResourceManager->HadError() && LoadSoundIds(mResourceManager)) {
		mRedFont = BBFont::Create("fonts/BradyBunchRemastered52_red.txt", true);
		mFont = BBFont::Create("fonts/BradyBunchRemastered52.txt", false);
		return;
	}
	ShowResourceError(false);
	mLoadingFailed = true;
}

// 0x42d330: the music, the sound queue and the game's widget, then the engine's root at the window's centre and
// the game's globals: the particle layer, the high scores, the particles' picture and the players
void GameApp::LoadingThreadCompleted()
{
	if (mLoadingFailed)
		return;
	mMusicInterface->LoadMusic(MUSIC_LEVEL_1, "sounds/music1.ogg");
	mMusicInterface->LoadMusic(MUSIC_LEVEL_2, "sounds/music2.ogg");
	mMusicInterface->LoadMusic(MUSIC_LEVEL_3, "sounds/music3.ogg");
	mMusicInterface->LoadMusic(MUSIC_LEVEL_4, "sounds/music4.ogg");
	mMusicInterface->LoadMusic(MUSIC_META, "sounds/meta_music.ogg");
	mMusicInterface->LoadMusic(MUSIC_MAIN_MENU, "sounds/mainmenu.ogg");
	mMusicInterface->LoadMusic(MUSIC_STATS, "sounds/statsriff.ogg");
	mMusicInterface->LoadMusic(MUSIC_VICTORY, "sounds/victorymusic.ogg");
	mMusicInterface->LoadMusic(MUSIC_INTRO, "sounds/music_intro.ogg");
	mMusicInterface->LoadMusic(MUSIC_STATS_2, "sounds/statsriff2.ogg");
	mMusicInterface->LoadMusic(MUSIC_LEVEL_5, "sounds/music5.ogg");
	mMusicInterface->FadeIn(MUSIC_META, 0, MUSIC_FADE_SPEED, false);
	mCurrentMusic = MUSIC_META;
	mSoundQueue = new SoundQueue(this);
	mGameWidget = new GameWidget(this);
	mGameWidget->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mGameWidget);
	mWidgetManager->SetFocus(mGameWidget);
	mForceFullscreen = false;
	ObjBase::sRoot->SetState(STATE_SCALE_X, mRootScaleX);
	ObjBase::sRoot->SetState(STATE_SCALE_Y, mRootScaleY);
	ObjBase::sRoot->SetState(STATE_X, (float)((double)mWidth * 0.5 - 0.5));
	ObjBase::sRoot->SetState(STATE_Y, (float)((double)mHeight * 0.5 - 0.5));
	ObjBase::sRoot->ComputeDrawn();
	CreateParticleLayer();
	InitHighScores();
	LoadParticles();
	LoadProfiles();
}

// 0x42d940. The original asks for the window's information (GetWindowInfo) and does not use it, then runs
// SexyAppBase's frames, skipping SexyApp's (the update check's)
void GameApp::UpdateFrames()
{
	SexyAppBase::UpdateFrames();
}

// 0x42d970: the cursor is let go, the scene runs one update (at frame 0), and the next update pauses the game
void GameApp::LostFocus()
{
	mFocused = false;
	// the original frees the cursor from the window (ClipCursor); the port never confines it
	ObjBase::UpdateAll(0);
	mFocusLost = true;
}

// 0x42d9a0: the mouse and the keys start over (the original also asks for the window's information, unused)
void GameApp::GotFocus()
{
	mFocused = true;
	ObjBase::sMouseHeld = false;
	ObjBase::sKeyPressed = 0;
	ObjBase::sKeyHeld = 0;
}

// 0x42d9e0: the cursor to a point of the window, while the game has the focus (no point is negative). The framework's
// WarpMouse does nothing in a test run or on the web.
void GameApp::WarpCursor(int x, int y)
{
	if (-1 < x) {
		if (mFocused)
			WarpMouse(x, y);
	}
}

// 0x42da30: a size the window keeps in every mode, or -1 for the desktop's
void GameApp::SetForcedSize(int theWidth, int theHeight)
{
	mForcedWidth = theWidth;
	mForcedHeight = theHeight;
	// the original then takes the desktop's size when theWidth is negative (0x42cdb0), which only the 3D path does
}

// 0x42da50: the window at the forced size in either mode (the original's 3D path, at the desktop's size, is not
// ported; see the constructor). If the size changed, every picture is made again. The scene runs one update at the
// new size before the framework changes the mode.
void GameApp::SwitchScreenMode(bool wantWindowed, bool is3d, bool force)
{
	bool aReload = false;
	mRootScaleX = 1.0f;
	mRootScaleY = 1.0f;
	mWidth = mForcedWidth;
	mHeight = mForcedHeight;
	if (mScreenModeKind == 1 && (mLastWidth != mWidth || mLastHeight != mHeight))
		aReload = true;
	mScreenModeKind = 0;
	gScreenScaleX = (float)((float)mWidth / 1920.0);
	gScreenScaleY = (float)((float)mHeight / 1200.0);
	mDesignScaleX = 2.4f;
	mDesignScaleY = 2.0f;

	mWindowToDesignX = (float)(1920.0 / (float)mWidth);
	mWindowToDesignY = (float)(1200.0 / (float)mHeight);
	ObjBase::FollowScreenScale();
	mWidescreen = 133 < (mWidth * 100) / mHeight;
	if (aReload) {
		force = true;
		AnimReload::Begin();
		mLastWidth = mWidth;
		mLastHeight = mHeight;
	}
	ObjBase::sRoot->SetState(STATE_SCALE_X, mRootScaleX);
	ObjBase::sRoot->SetState(STATE_SCALE_Y, mRootScaleY);
	ObjBase::sRoot->SetState(STATE_X, (float)((double)mWidth * 0.5 - 0.5));
	ObjBase::sRoot->SetState(STATE_Y, (float)((double)mHeight * 0.5 - 0.5));
	ObjBase::sRoot->ComputeDrawn();
	int anUpdateLevel = ObjBase::sUpdateLevel;
	ObjBase::SetUpdateLevel(0);
	int aFrame = 0;
	if (mGameWidget != nullptr) {
		mGameWidget->mFrame++;
		aFrame = mGameWidget->mFrame;
	}
	ObjBase::UpdateAll(aFrame);
	ObjBase::SetUpdateLevel(anUpdateLevel);
	SexyApp::SwitchScreenMode(wantWindowed, is3d, force);
	if (mGameWidget != nullptr)
		mGameWidget->Resize(0, 0, mWidth, mHeight);
}
