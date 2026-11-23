// port: the host for Feeding Frenzy 2 (see FeedingFrenzy2App.h).

#include "FeedingFrenzy2App.h"
#include "BonusItem.h"
#include "DXDrawContext.h"
#include "EngineImpl.h"
#include "FeedingFrenzyTwo.h"
#include "GameData.h"
#include "GameHud.h"
#include "PlayerFish.h"
#include "PlayerSpawn.h"
#include "Stage.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "SimpleSoundDX.h"
#include "Surface.h"
#include "WidgetManager.h"

#include <SDL.h>

#include <cstring>

using namespace Sexy;

FeedingFrenzy2App* Sexy::gApp = nullptr;

namespace
{

// The game's screen (FeedingFrenzyTwo's configuration, 0x48c080 (feedingfrenzy2))
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
// And its frame rate before the splash loads, the rate the game sets after loading (where the original sets its first
// rate is not yet found)
const int FRAME_RATE = 60;

} // namespace

FeedingFrenzy2Widget::FeedingFrenzy2Widget() : mNextFrame(0), mMouseX(0), mMouseY(0)
{
	ClearKeys();
	mScreenImage = new MemoryImage(gApp);
	mScreenImage->Create(SCREEN_WIDTH, SCREEN_HEIGHT);
	mScreenImage->SetImageMode(false, false);
	mScreenImage->mUploadAsIs = true;
}

FeedingFrenzy2Widget::~FeedingFrenzy2Widget()
{
	delete mScreenImage;
}

// The run loop (0x4383b0 (feedingfrenzy2)): a frame runs once the clock passes the time the last one set, else the
// loop sleeps
void FeedingFrenzy2Widget::Update()
{
	Widget::Update();
	if (!gApp->mEngineRunning)
		return;
	if (gApp->mFastMode)
	{
		// port: -fast runs a frame each update, as the original does on origrun's virtual clock, which skips the wait
		// and moves the clock on by it
		gApp->mEngine->advanceClock(gApp->FramePeriod());
	}
	else
	{
		uint64_t aTicks = gApp->mEngine->getTicks();
		if (aTicks < mNextFrame)
			return;
		mNextFrame = aTicks + gApp->FramePeriod();
	}
	gApp->RunFrame();
}

void FeedingFrenzy2Widget::Draw(Graphics* g)
{
	g->DrawImage(mScreenImage, 0, 0);
}

namespace
{

// 0x424730 (feedingfrenzy2): a position kept inside the screen
engine::Vector2<int> clampToScreen(int x, int y)
{
	if (x < 0)
		x = 0;
	else if (x > SCREEN_WIDTH - 1)
		x = SCREEN_WIDTH - 1;
	if (y < 0)
		y = 0;
	else if (y > SCREEN_HEIGHT - 1)
		y = SCREEN_HEIGHT - 1;
	return engine::Vector2<int>(x, y);
}

// The framework's mouse wheel turns in notches; WM_MOUSEWHEEL's delta is in WHEEL_DELTA steps
const int WHEEL_DELTA_PER_NOTCH = 120;

} // namespace

void FeedingFrenzy2Widget::ClearKeys()
{
	std::memset(mKeyIsDown, 0, sizeof(mKeyIsDown));
}

// WM_KEYDOWN: the key-down signal for the first, the repeat signal (with a count of 1) for each after it while the key
// is held, never both
void FeedingFrenzy2Widget::KeyDown(KeyCode theKey)
{
	int aKey = (int)theKey;
	bool aRepeat = false;
	if (aKey >= 0 && aKey < 256)
	{
		aRepeat = mKeyIsDown[aKey];
		mKeyIsDown[aKey] = true;
	}
	if (!gApp->mEngineRunning)
		return;
	if (!aRepeat)
		gApp->mEngine->mKeyDownSignal.emit(aKey);
	else
		gApp->mEngine->mKeyRepeatSignal.emit(aKey, 1);
}

// WM_KEYUP
void FeedingFrenzy2Widget::KeyUp(KeyCode theKey)
{
	int aKey = (int)theKey;
	if (aKey >= 0 && aKey < 256)
		mKeyIsDown[aKey] = false;
	if (gApp->mEngineRunning)
		gApp->mEngine->mKeyUpSignal.emit(aKey);
}

// WM_CHAR
void FeedingFrenzy2Widget::KeyChar(SexyChar theChar)
{
	if (gApp->mEngineRunning)
		gApp->mEngine->mCharSignal.emit((char)theChar);
}

// WM_MOUSEMOVE
void FeedingFrenzy2Widget::MouseMove(int x, int y)
{
	if (!gApp->mEngineRunning)
		return;
	engine::Vector2<int> aPoint = clampToScreen(x, y);
	mMouseX = aPoint.x;
	mMouseY = aPoint.y;
	// While the play's mouse is acquired, only where the cursor is kept
	engine::MessageMouse& aMouse = gApp->mEngine->getMessageMouse();
	aMouse.setPosition(aPoint);
	if (aMouse.isAcquired())
		return;
	gApp->mEngine->mMouseMoveSignal.emit(aPoint);
}

void FeedingFrenzy2Widget::MouseDrag(int x, int y)
{
	MouseMove(x, y);
}

// WM_LBUTTONDOWN and WM_RBUTTONDOWN (the framework's buttons: 0 left, 1 right; the original took no middle button,
// and its window class has no double clicks, so a second click is another press). The original captures the mouse
// for the window while a button is down; the framework does the same for its widgets
void FeedingFrenzy2Widget::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	if (!gApp->mEngineRunning)
		return;
	engine::Vector2<int> aPoint = clampToScreen(x, y);
	mMouseX = aPoint.x;
	mMouseY = aPoint.y;
	// While the play's mouse is acquired, told to it alone
	engine::MessageMouse& aMouse = gApp->mEngine->getMessageMouse();
	if (aMouse.isAcquired())
	{
		if (theBtnNum == 0 || theBtnNum == 1)
			aMouse.buttonDown(aPoint, theBtnNum);
		return;
	}
	if (theBtnNum == 0)
		gApp->mEngine->mMouseDownSignal.emit(aPoint);
	else if (theBtnNum == 1)
		gApp->mEngine->mRightDownSignal.emit(aPoint);
}

// WM_LBUTTONUP and WM_RBUTTONUP
void FeedingFrenzy2Widget::MouseUp(int x, int y, int theBtnNum, int theClickCount)
{
	if (!gApp->mEngineRunning)
		return;
	engine::Vector2<int> aPoint = clampToScreen(x, y);
	mMouseX = aPoint.x;
	mMouseY = aPoint.y;
	engine::MessageMouse& aMouse = gApp->mEngine->getMessageMouse();
	if (aMouse.isAcquired())
	{
		if (theBtnNum == 0 || theBtnNum == 1)
			aMouse.buttonUp(aPoint, theBtnNum);
		return;
	}
	if (theBtnNum == 0)
		gApp->mEngine->mMouseUpSignal.emit(aPoint);
	else if (theBtnNum == 1)
		gApp->mEngine->mRightUpSignal.emit(aPoint);
}

// WM_MOUSEWHEEL: at the mouse's place (the original turns the message's screen position into the window's)
void FeedingFrenzy2Widget::MouseWheel(int theDelta)
{
	if (!gApp->mEngineRunning)
		return;
	gApp->mEngine->mMouseWheelSignal.emit(clampToScreen(mMouseX, mMouseY), theDelta * WHEEL_DELTA_PER_NOTCH);
}

FeedingFrenzy2App::FeedingFrenzy2App()
{
	gApp = this;
	mProdName = "FeedingFrenzy2";
	mTitle = "Feeding Frenzy 2 - Shipwreck Showdown";
	// The key the engine's registry keys are below (engines/sprout2/host/RegistryKey.h): Software\<vendor>\<appName> of
	// HKEY_CURRENT_USER, the vendor and the name being the strings of resource/baseStrings.xml
	mRegKey = "GameHouse\\Feeding Frenzy 2";
	mWidth = SCREEN_WIDTH;
	mHeight = SCREEN_HEIGHT;
	mWidget = nullptr;
	mEngine = nullptr;
	mDisplay = nullptr;
	mGame = nullptr;
	mEngineRunning = false;
	mFrames = 0;
	mLastFrameTicks = 0;
}

FeedingFrenzy2App::~FeedingFrenzy2App()
{
	if (mWidget != nullptr)
	{
		mWidgetManager->RemoveWidget(mWidget);
		delete mWidget;
	}
	if (mGame != nullptr)
		mGame->release();
	if (mDisplay != nullptr)
		mDisplay->release();
	delete mEngine;
}

void FeedingFrenzy2App::ReadFromRegistry()
{
	mReadFromRegistry = true;
}

void FeedingFrenzy2App::WriteToRegistry()
{
}

uint64_t FeedingFrenzy2App::FramePeriod() const
{
	return mEngine->getFramePeriod();
}

void FeedingFrenzy2App::Init()
{
	SexyAppBase::Init();
	if (mShutdown)
		return;
	mWidget = new FeedingFrenzy2Widget();
	mWidget->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mWidget);
	mWidgetManager->SetFocus(mWidget);
	mEngine = new engine::EngineImpl(SCREEN_WIDTH, SCREEN_HEIGHT);
	mEngine->setVirtualClock(mFastMode);
	mEngine->setFrameRate(FRAME_RATE);
	mDisplay = new engine::Surface();
	mDisplay->addRef();
	mDisplay->allocate(SCREEN_WIDTH, SCREEN_HEIGHT);
	// The run (0x4383b0 (feedingfrenzy2)) has 0x49bc60 (feedingfrenzy2) make the application and ask for its
	// configuration, then starts it (the screensaver's letters are not yet translated)
	mGame = new FeedingFrenzyTwo();
	mGame->addRef();
	mGame->configure();
	mGame->start(false);
	mLastFrameTicks = mEngine->getTicks();
	mEngineRunning = true;
	// port: the original's window gets WM_ACTIVATE as it opens; the framework has the focus from the start and sends
	// no first activation, so the state it has is passed on
	mEngine->mActivateSignal.emit(mHasFocus);
}

bool FeedingFrenzy2App::HandleScriptCommand(const ScriptCommand& theCommand)
{
	if (theCommand.mCommand == "state")
	{
		logState();
		return true;
	}
	if (theCommand.mCommand == "set")
	{
		setState(theCommand.mArgs);
		return true;
	}
	if (theCommand.mCommand == "win" || theCommand.mCommand == "lose")
	{
		endStage(theCommand.mCommand == "win");
		return true;
	}
	if (theCommand.mCommand != "unlock")
		return false;
	std::string aLine = theCommand.mCommand + " " + theCommand.mArgs;
	if (theCommand.mArgs != "all")
		SDL_Log("ERROR %s: only unlock all", aLine.c_str());
	else if (mGame == nullptr || mGame->mUserData == nullptr || mGame->mUserData->getName().empty())
		SDL_Log("ERROR %s: no player", aLine.c_str());
	else
	{
		mGame->mUserData->unlockAll((int)mGame->mGameData->getStages().getStages().size());
		SDL_Log("OK %s", aLine.c_str());
	}
	return true;
}

void FeedingFrenzy2App::logState()
{
	if (mGame == nullptr || mGame->mGameData == nullptr)
	{
		SDL_Log("ERROR state: no game");
		return;
	}
	GameData* aData = mGame->mGameData.get();
	std::string aPlayer = "null";
	int aPhase = -1;
	int aMultiplier = 0;
	int aShownScore = mGame->mStage ? mGame->mStage->getHud()->getShownScore() : -1;
	float aTimeLeft = mGame->mStage ? mGame->mStage->getTimeLeft() : 0.0f;
	const char* aRun = mGame->mStage == nullptr ? "none" : mGame->mStage->getRunState() == nullptr ? "ended" : "on";
	PlayerFish* aFish = mGame->mStage ? mGame->mStage->getFirstPlayer() : nullptr;
	if (aFish != nullptr)
	{
		char aPlace[64];
		snprintf(aPlace, sizeof(aPlace), "[%.1f,%.1f]", aFish->getPosition().x, aFish->getPosition().y);
		aPlayer = aPlace;
		aPhase = aFish->getSpawn() ? aFish->getSpawn()->getPhaseIndex() : -1;
		aMultiplier = aFish->getFrenzyMultiplier();
	}
	// The bonuses in the stage, where each is
	std::string aBonuses = "[";
	if (mGame->mStage)
	{
		for (BonusItem* aBonus : mGame->mStage->getBonuses())
		{
			char aPlace[64];
			snprintf(aPlace, sizeof(aPlace), "%s[%.1f,%.1f]", aBonuses.size() > 1 ? "," : "", aBonus->getPosition().x,
				aBonus->getPosition().y);
			aBonuses += aPlace;
		}
	}
	aBonuses += "]";
	SDL_Log("STATE {\"mode\":%d,\"level\":%d,\"score\":%d,\"lives\":%d,\"food\":%d,\"phase\":%d,"
	        "\"frenzy\":%d,\"shown\":%d,\"time\":%.2f,\"run\":\"%s\",\"player\":%s,\"bonuses\":%s}",
	        aData->getMode(), aData->getStage(), aData->getScore(), aData->getLives(), aData->getFood(), aPhase,
	        aMultiplier, aShownScore, aTimeLeft, aRun, aPlayer.c_str(), aBonuses.c_str());
}

void FeedingFrenzy2App::endStage(bool isWin)
{
	const char* aName = isWin ? "win" : "lose";
	PlayerFish* aFish = mGame != nullptr && mGame->mStage != nullptr ? mGame->mStage->getFirstPlayer() : nullptr;
	if (aFish == nullptr || aFish->getSpawn() == nullptr || !aFish->getSpawn()->isLive())
	{
		SDL_Log("ERROR %s: no player in play", aName);
		return;
	}
	if (isWin)
	{
		// The food of the last phase's goal: the spawn grows the fish a phase a step, and the stage then ends
		PlayerSpawn* aSpawn = aFish->getSpawn();
		mGame->mGameData->setFood(aSpawn->getPhase(aSpawn->getPhaseCount() - 1).mFoodGoal);
	}
	else
	{
		// The fish's state ended, as a fish that eats it ends it (Fish::beSwallowed): the spawn then finds it dead
		aFish->getStateMgr().changeState(nullptr);
	}
	SDL_Log("OK %s", aName);
}

void FeedingFrenzy2App::setState(const std::string& theArgs)
{
	char aName[32] = {};
	int aValue = 0;
	if (sscanf(theArgs.c_str(), "%31s %d", aName, &aValue) != 2)
	{
		SDL_Log("ERROR set %s: set <name> <value>", theArgs.c_str());
		return;
	}
	if (mGame == nullptr || mGame->mGameData == nullptr)
	{
		SDL_Log("ERROR set %s: no game", theArgs.c_str());
		return;
	}
	GameData* aData = mGame->mGameData.get();
	std::string aKey = aName;
	if (aKey == "score")
	{
		aData->setScore(aValue);
		aData->findNextLifeScore();
	}
	else if (aKey == "lives")
		aData->setLives(aValue);
	else if (aKey == "food")
		aData->setFood(aValue);
	else if (aKey == "time" && mGame->mStage != nullptr)
		mGame->mStage->setTimeLeft((float)aValue);
	else
	{
		SDL_Log("ERROR set %s: only score, lives, food and time (in a stage)", theArgs.c_str());
		return;
	}
	SDL_Log("OK set %s", theArgs.c_str());
}

void FeedingFrenzy2App::GotFocus()
{
	SexyAppBase::GotFocus();
	if (mEngineRunning)
		mEngine->mActivateSignal.emit(true);
}

void FeedingFrenzy2App::LostFocus()
{
	SexyAppBase::LostFocus();
	if (mWidget != nullptr)
		mWidget->ClearKeys();
	if (mEngineRunning)
		mEngine->mActivateSignal.emit(false);
}

void FeedingFrenzy2App::RunFrame()
{
	uint64_t aFrameStart = mEngine->getTicks();
	engine::UpdateContext aContext;
	float aTicks = (float)(int64_t)(aFrameStart - mLastFrameTicks);
	aContext.mElapsed = engine::minOf(0.33f, aTicks / (float)mEngine->getTicksPerSecond());
	aContext.mFrame = (uint64_t)mFrames;
	aContext.mDetailLevel = mEngine->getDetailLevel();
	mFrames++;
	// While the play's mouse is acquired, its move since the last frame (0x42b8c0 (feedingfrenzy2))
	if (mEngine->getMessageMouse().isAcquired())
		mEngine->getMessageMouse().sendMotion();
	// port: the sound's update runs from the window's timer every 100 ms (WM_TIMER 1, 0x424780 (feedingfrenzy2)),
	// here on the game's clock; the only sound system is the host's
	simplesound::SimpleSound* aSound = simplesound::getSimpleSound();
	if (aSound != nullptr)
	{
		double aMilliseconds = aTicks * 1000.0 / (double)mEngine->getTicksPerSecond();
		static_cast<simplesounddx::SimpleSoundDX*>(aSound)->advanceTimer(aMilliseconds);
	}
	mEngine->mUpdateSignal.emit(&aContext);

	// The frame drawn into the display through a context, its first state at the origin, unscaled and opaque; the
	// original draws and presents only while the display is ready (DirectDraw's surface not lost)
	{
		engine::DXDrawContext aDrawContext(mDisplay);
		aDrawContext.setDetailLevel(mEngine->getDetailLevel());
		mEngine->mDrawSignal.emit(&aDrawContext);
	}
	// The display's slot 0x14, its flip: port: the pixels copied to the framework's screen image, opaque as the
	// display's back buffer is
	const uint32_t* aSource = mDisplay->getPixels();
	uint32_t* aTarget = (uint32_t*)mWidget->mScreenImage->GetBits();
	for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
		aTarget[i] = aSource[i] | 0xff000000;
	mWidget->mScreenImage->BitsChanged();
	mWidget->MarkDirty();
	mLastFrameTicks = aFrameStart;

	if (mEngine->mQuitRequested)
		Shutdown();
}

void FeedingFrenzy2App::Shutdown()
{
	mEngineRunning = false;
	SexyAppBase::Shutdown();
}
