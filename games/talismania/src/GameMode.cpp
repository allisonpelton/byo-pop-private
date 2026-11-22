#include "GameMode.h"

#include "AtlantisApp.h"
#include "Engine.h"
#include "FrontEnd.h"
#include "GameMgr.h"

// The wait of the CheckNewGame state with no question to ask (the float 0.01, 0x3c23d70a, 0x443110)
static const float CHECK_NEW_GAME_SECONDS = 0.01f;

class GameMode::CheckNewGameState : public engine::State
{
public:
	explicit CheckNewGameState(GameMode* theMode) : mMode(theMode) {}

	// 0x443110 (without a saved game): no screen, a short wait
	void enter() override { mSecondsLeft = CHECK_NEW_GAME_SECONDS; }
	// 0x443cb0: the wait counted down, then the next state (0x443310 for a first game: the main menu popped and the Play
	// state taken)
	void update(engine::UpdateContext* theContext) override
	{
		if (mDone)
			return;
		mSecondsLeft -= (float)theContext->mElapsed;
		if (mSecondsLeft < 0.0f)
		{
			mDone = true;
			mMode->goToPlay();
		}
	}

private:
	GameMode* mMode;
	float mSecondsLeft = 0.0f;
	bool mDone = false;
};

class GameMode::PlayState : public engine::State
{
public:
	explicit PlayState(GameMode* theMode) : mMode(theMode) {}

	// 0x443690: the main menu taken off (the menu interface slot 0x18 with the app +0x48), the game manager made and
	// started (not yet translated)
	void enter() override
	{
		mMode->mGameMgr.reset(new AtlantisGameMgr(mMode->mApp, mMode->mClass));
		mMode->mGameMgr->start();
	}

private:
	GameMode* mMode;
};

GameMode::GameMode(AtlantisApp* theApp, const GameModeClass& theClass) : mApp(theApp), mClass(theClass)
{
}

GameMode::~GameMode() = default;

void GameMode::begin()
{
	mStates.changeState(new CheckNewGameState(this));
}

void GameMode::goToPlay()
{
	mApp->mFrontEnd->hideMainMenu();
	mStates.changeState(new PlayState(this));
}
