#include "FishFood.h"
#include "Host.h"
#include "ImageNode.h"
#include "Random.h"
#include "Sound.h"
#include "TextItem.h"

using namespace feedingfrenzy;

FishFood* FishFood::sInstance = nullptr;

namespace
{
const char* const WINDOW_TITLE = "Feeding Frenzy";

// The game's archive (0x430b80) and the files init reads from it
const char* const ARCHIVE_FILE = "FFarchive.saf";
const char* const STRINGS_FILE = "config\\gameStrings.xml";
const char* const STAGE_MANIFEST_FILE = "config\\stages\\stageManifest.xml";

// The logo that slides down over the menus, and the "TM" beside it (0x430180)
const char* const LOGO_IMAGE = "resources\\menus\\logo.png";
const char* const LOGO_FONT = "resources\\menus\\infoText14.xml";
const float LOGO_HIDDEN_Y = -80.0f;
const float LOGO_SHOWN_Y = 50.0f;
const float LOGO_X = 320.0f;
const float LOGO_SLIDE_SECONDS = 0.3f;
const float TM_X = 105.0f;
const float TM_Y = 24.0f;
const float TM_OPACITY = 0.6f;

// The splash screen (0x42cc70): its picture is a loose file, its font the archive's
const char* const SPLASH_IMAGE = "resources\\splash\\gameSplash.jpg";
const char* const SPLASH_FONT = "resources\\hud\\FrenzyFontWhite.xml";
const float LOADING_X = 320.0f;
const float LOADING_Y = 150.0f;
const float LOADING_OPACITY = 0.8f;
const float SPLASH_SECONDS = 2.05f;      // how long it stays at least (0x430180)

// The fade over the whole screen (0x430180)
const float COVER_FADE_SECONDS = 1.0f;

// A picture the loading state fetches every frame (0x42ec30): the image cache keeps it
const char* const PRELOADED_IMAGE = "resources\\backdrops\\SunkenTreasure\\mg_plant2.jpg";

// The steps of the loading state (+0x60)
enum
{
	LOADING_STEP_WAIT = 0,    // the splash's minimum time
	LOADING_STEP_LOAD = 2,    // the menu's stage and screens are made
	LOADING_STEP_STAGE = 3,   // the menu's stage loads
	LOADING_STEP_FADE = 4,    // the splash fades out
	LOADING_STEP_FADED = 5    // (the original's fade callback gets here)
};

// The logo's states (+0xb8)
enum
{
	LOGO_HIDDEN = 0,
	LOGO_SLIDING_IN = 1,
	LOGO_SHOWN = 2,
	LOGO_SLIDING_OUT = 3
};

}

// 0x430ee0
FishFood::FishFood() :
	mState(STATE_LOADING), mStateSeconds(0.0f), mStep(0), mShowFadeSeconds(0.0f), mLogoState(LOGO_HIDDEN),
	mLogoSecondsLeft(-1.0f), mLogoSlideSeconds(LOGO_SLIDE_SECONDS),
	mLogoHiddenPosition(LOGO_X, LOGO_HIDDEN_Y), mLogoShownPosition(LOGO_X, LOGO_SHOWN_Y)
{
	sInstance = this;
	engine::gSignalBuild = true;
	engine::gEarlyBuild = true;
}

// 0x431310
FishFood::~FishFood()
{
	if (sInstance == this)
		sInstance = nullptr;
}

// 0x42b8e0: the window's title, icon, size and frame rate
void FishFood::configure(engine::AppConfig& theConfig)
{
	theConfig.mTitle = WINDOW_TITLE;
	theConfig.mIcon = WINDOW_ICON;
	theConfig.mWidth = SCREEN_WIDTH;
	theConfig.mHeight = SCREEN_HEIGHT;
	theConfig.mFrameRate = FRAME_RATE;
}

// 0x430b80: the settings are read and the display made in the full screen they say (the host has made the window by
// now), then the archive, the game's strings and the stage manifest, and the scene and the splash (startup)
void FishFood::init()
{
	mSettings.load();
	engine::Host* aHost = engine::getHost();
	aHost->setFullScreen(mSettings.isFullScreen());
	engine::App* anApp = engine::App::instance();
	anApp->setUseFiles(false);
	anApp->addArchive(ARCHIVE_FILE);
	mStrings.load(STRINGS_FILE);
	loadStageManifest(STAGE_MANIFEST_FILE, mManifest);
	startup();
}

// 0x430180: the scene and its two groups (the screens and the popups), the fade over everything, the logo with its
// "TM", and the splash; the window's signals go to the controller
void FishFood::startup()
{
	engine::App* anApp = engine::App::instance();
	unsigned int aSeed = engine::getHost()->timeMs();
	engine::getHost()->testSeed(aSeed);
	engine::crtSrand(aSeed);
	anApp->mUpdateSignal.connect(this, &FishFood::onAppUpdate);
	anApp->mActivateSignal.connect(this, &FishFood::onAppActivate);
	// 0x45ffd0, then the application's 0x88: the sound system
	engine::SoundSystem* aSounds = engine::getHost()->createSoundSystem();
	anApp->setSoundSystem(aSounds);

	mScene = new engine::Scene();
	mScene->setName("Scene");
	mScene->connect();
	mScene->layout();
	mRoot = new engine::Container();
	mRoot->setName("RootGroup");
	mRoot->setPosition(0.0f, 0.0f);
	mScene->addChild(mRoot);
	mPopups = new engine::Container();
	mPopups->setName("PopupGroup");
	mPopups->setPosition(0.0f, 0.0f);
	mScene->addChild(mPopups);

	mFader = new ScreenFader();
	mFader->setCoverColor(0.0f, 0.0f, 0.0f);
	mFader->setOpacity(1.0f);
	mFader->setDurations(COVER_FADE_SECONDS, -1.0f);
	mFader->setRect(engine::Rect(0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT));

	engine::Image* aLogo = anApp->getImage(LOGO_IMAGE);
	mLogoGroup = new engine::Container();
	mLogoGroup->addChild(new engine::ImageNode(aLogo));
	mScene->addChild(mLogoGroup);
	engine::TextItem* aTrademark = new engine::TextItem();
	mLogoGroup->addChild(aTrademark);
	aTrademark->setFont(anApp->getFont(LOGO_FONT));
	aTrademark->setText("TM");
	aTrademark->setPosition(TM_X, TM_Y);
	aTrademark->setOpacity(TM_OPACITY);
	mLogoGroup->setPosition(mLogoHiddenPosition);

	showSplash();
	mScene->addChild(mSplash);
	mScene->addChild(mFader);
	mSplash->setOpacity(1.0f);
	mStep = LOADING_STEP_WAIT;
	mFader->fadeFromColor();
	mStateSeconds = SPLASH_SECONDS;
	mState = STATE_LOADING;
}

// 0x42cc70: the splash screen's picture with the word "LOADING" under it
void FishFood::showSplash()
{
	engine::App* anApp = engine::App::instance();
	mSplash = new engine::FadeContainer();
	mSplash->addChild(new engine::ImageNode(anApp->getFileImage(SPLASH_IMAGE)));
	engine::TextItem* aLoading = new engine::TextItem();
	mSplash->addChild(aLoading);
	aLoading->setFont(SPLASH_FONT);
	aLoading->setHAlign(1);
	aLoading->setPosition(LOADING_X, LOADING_Y);
	aLoading->setOpacity(LOADING_OPACITY);
	aLoading->setText(mStrings.get("loading"));
}

// 0x42fa80: the sound system's update, the cursor, the popup screens, the logo, and the state's own
void FishFood::onAppUpdate(engine::UpdateInfo& theInfo)
{
	engine::SoundSystem* aSounds = engine::App::getSoundSystem();
	if (aSounds != nullptr)
		aSounds->update((int)(theInfo.mSeconds * 1000.0f));
	updateScreens();
	updateLogo(theInfo);
	switch (mState)
	{
	case STATE_LOADING:
		updateLoading(theInfo);
		break;
	}
}

// 0x42c230: the window's focus lost pauses a game that is running
void FishFood::onAppActivate(bool theActive)
{
	(void)theActive;
}

// 0x42ec30: the splash stays its minimum time, then the menu's stage and screens are made and loaded
void FishFood::updateLoading(engine::UpdateInfo& theInfo)
{
	mStateSeconds -= theInfo.mSeconds;
	engine::App::instance()->getImage(PRELOADED_IMAGE);
	if (mStateSeconds < 0.0f)
	{
		switch (mStep)
		{
		case LOADING_STEP_WAIT:
			mStep = LOADING_STEP_LOAD;
			break;
		default:
			// port: the menu's stage and screens are not translated yet
			break;
		}
	}
}

// 0x42c3a0
void FishFood::setState(int theState)
{
	mState = theState;
}

// 0x42ba60: a screen to show at the next update, with the time it fades in
void FishFood::showScreen(engine::ScreenLayout* theScreen, float theFadeSeconds)
{
	mShowingScreen = theScreen;
	theScreen->setVisible(true);
	mShowFadeSeconds = theFadeSeconds;
}

// 0x42baf0: a screen hidden, fading out over the time when there is one
void FishFood::hideScreen(engine::ScreenLayout* theScreen, float theFadeSeconds)
{
	theScreen->setVisible(false);
	if (theFadeSeconds == 0.0f)
		theScreen->clearFlags(engine::NODE_DRAWABLE);
	else
		theScreen->fade(false, theFadeSeconds, 1.0f, true);
}

// 0x42ba80: the screen to hide leaves the popups; the screen to show joins them and fades in
void FishFood::updateScreens()
{
	if (mHidingScreen != nullptr)
	{
		mPopups->removeChild(mHidingScreen);
		mHidingScreen = nullptr;
	}
	if (mShowingScreen != nullptr)
	{
		mPopups->addChild(mShowingScreen);
		mShowingScreen->fade(true, mShowFadeSeconds, 1.0f, false);
		mShowingScreen = nullptr;
	}
}

// 0x42bc40
void FishFood::slideLogoIn()
{
	if (mLogoState == LOGO_HIDDEN)
	{
		mLogoSecondsLeft = mLogoSlideSeconds;
		mLogoState = LOGO_SLIDING_IN;
	}
	else if (mLogoState == LOGO_SLIDING_OUT)
	{
		mLogoState = LOGO_SLIDING_IN;
		mLogoSecondsLeft = mLogoSlideSeconds - mLogoSecondsLeft;
	}
}

// 0x42bc90
void FishFood::slideLogoOut()
{
	if (mLogoState == LOGO_SLIDING_IN)
	{
		mLogoState = LOGO_SLIDING_OUT;
		mLogoSecondsLeft = mLogoSlideSeconds - mLogoSecondsLeft;
	}
	else if (mLogoState == LOGO_SHOWN)
	{
		mLogoSecondsLeft = mLogoSlideSeconds;
		mLogoState = LOGO_SLIDING_OUT;
	}
}

// 0x42bcd0: the logo's place between its two positions follows the slide's time
void FishFood::updateLogo(engine::UpdateInfo& theInfo)
{
	mLogoSecondsLeft -= theInfo.mSeconds;
	if (mLogoSecondsLeft < 0.0f)
		mLogoSecondsLeft = 0.0f;
	float aFraction = 0.0f;
	switch (mLogoState)
	{
	case LOGO_HIDDEN:
		aFraction = 0.0f;
		break;
	case LOGO_SLIDING_IN:
		aFraction = 1.0f - mLogoSecondsLeft / mLogoSlideSeconds;
		if (mLogoSecondsLeft <= 0.0f)
			mLogoState = LOGO_SHOWN;
		break;
	case LOGO_SHOWN:
		aFraction = 1.0f;
		break;
	case LOGO_SLIDING_OUT:
		aFraction = mLogoSecondsLeft / mLogoSlideSeconds;
		if (mLogoSecondsLeft <= 0.0f)
			mLogoState = LOGO_HIDDEN;
		break;
	}
	mLogoGroup->setPosition((mLogoShownPosition.x - mLogoHiddenPosition.x) * aFraction + mLogoHiddenPosition.x,
		(mLogoShownPosition.y - mLogoHiddenPosition.y) * aFraction + mLogoHiddenPosition.y);
}

// 0x42bec0: the sound and music categories are on or off, and the window is in the full screen the settings say
void FishFood::applySettings()
{
	engine::SoundSystem* aSounds = engine::App::getSoundSystem();
	if (aSounds != nullptr)
	{
		aSounds->setCategoryVolume(0, mSettings.isSoundEnabled() ? 1.0f : 0.0f);
		aSounds->setCategoryVolume(1, mSettings.isMusicEnabled() ? 1.0f : 0.0f);
	}
	engine::Host* aHost = engine::getHost();
	if (aHost->isFullScreen() != mSettings.isFullScreen())
		aHost->setFullScreen(mSettings.isFullScreen());
}

// 0x42c9d0
void FishFood::shutdown()
{
	mSettings.save();
}

// 0x42f190: port: the actions are not translated yet
void FishFood::screenAction(std::string theAction)
{
	(void)theAction;
}
