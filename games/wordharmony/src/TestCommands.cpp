// port: the standard test commands (docs/TEST_COMMANDS.md), reachable only from a test (-script, -commands, the web test
// page's command call). Each answers on stdout with one line: STATE {...}, MOVE {...} or MOVE null, OK <command>, or
// ERROR <command>: <reason>. A command never given changes nothing; move changes nothing either (src/Bot.cpp). The
// game's notes list them (games/wordharmony/docs/README.md, "Test commands").

#include "WordHarmonyApp.h"
#include "Bot.h"
#include "engine/App.h"
#include "engine/Random.h"
#include "game/Game.h"
#include "game/Hud.h"
#include "game/Screens.h"
#include "game/WordBallSystem.h"

#include <cstdio>
#include <cstdlib>
#include <string>

using namespace Sexy;
using namespace engine;

namespace
{

const char* modeName(int theMode)
{
	switch (theMode)
	{
	case MODE_PUZZLE: return "relax";
	case MODE_SOLITAIRE: return "think";
	case MODE_FREEMATCH: return "tsunami";
	}
	return "";
}

int modeByName(const std::string& theName)
{
	if (theName == "relax")
		return MODE_PUZZLE;
	if (theName == "think")
		return MODE_SOLITAIRE;
	if (theName == "tsunami")
		return MODE_FREEMATCH;
	return -1;
}

// The screen on top: the last of the root window's children that is one of the game's screens
std::string screenName(Game* theGame)
{
	if (theGame->mSaverMode != 0)
		return "saver";
	Window* aRoot = App::instance()->getRootWindow();
	std::string aName = "none";
	for (Node* aChild : aRoot->mChildren)
	{
		if (aChild == (Node*)theGame->mSplash) aName = "title";
		else if (aChild == (Node*)theGame->mMainMenu) aName = "menu";
		else if (aChild == (Node*)theGame->mOptions) aName = "options";
		else if (aChild == (Node*)theGame->mHighScoreScreen) aName = "highscores";
		else if (aChild == (Node*)theGame->mHighScoreEntry) aName = "nameentry";
		else if (aChild == (Node*)theGame->mLevelEnd) aName = "levelend";
		else if (aChild == (Node*)theGame->mPausedScreen) aName = "paused";
		else if (aChild == (Node*)theGame->mGameOverScreen) aName = "gameover";
		else if (aChild == (Node*)theGame->mCongratsScreen) aName = "congrats";
		else if (aChild == (Node*)theGame->mLevelsSelection) aName = "levels";
		else if (aChild == (Node*)theGame->mDifficultySelection) aName = "levels";
		else if (aChild == (Node*)theGame->mSolitaireSelection) aName = "levels";
		else if (aChild == (Node*)theGame->mPuzzleHelp || aChild == (Node*)theGame->mActionHelp
			|| aChild == (Node*)theGame->mSolitaireHelp) aName = "help";
		else if (aChild == (Node*)theGame->mNewGameScreen) aName = "continue";
		else if (aChild == (Node*)theGame->mQuitGameScreen) aName = "quit";
		else if (aChild == (Node*)theGame->mLoadScreen) aName = "loading";
		else if (aChild == (Node*)theGame->mGameScreen) aName = "game";
	}
	return aName;
}

std::string jsonText(const std::string& theText)
{
	std::string aText = "\"";
	for (char c : theText)
	{
		if (c == '"' || c == '\\')
			aText += '\\';
		if ((unsigned char)c >= 0x20)
			aText += c;
	}
	return aText + "\"";
}

// the game's screen point of a point in the ball system
engine::Point toScreen(WordBallSystem* theBalls, const engine::Point& thePoint)
{
	engine::Point anOrigin(0.0f, 0.0f);
	theBalls->toLocal(anOrigin);
	return engine::Point(thePoint.x - anOrigin.x, thePoint.y - anOrigin.y);
}

}

// The game's state as one JSON line: the screen on top and the game's values, and while a game is on its balls (in
// screen coordinates, each with its letter, radius, state, special flags and whether it is fixed or in a chain), the
// level's words (RELAX), the bonus word (TSUNAMI) and whether the submit button shows
std::string WordHarmonyApp::TestState()
{
	Game* aGame = Game::sInstance;
	char aBuf[256];
	std::string aState = "{\"screen\":" + jsonText(screenName(aGame));
	std::snprintf(aBuf, sizeof(aBuf), ",\"state\":%d,\"mode\":%s,\"level\":%d,\"score\":%d,\"lettersleft\":%d,\"hints\":%d,"
		"\"over\":%s,\"frame\":%d", aGame->mState, jsonText(modeName(aGame->mMode)).c_str(), aGame->mLevel + 1,
		aGame->mScore, aGame->mLettersLeft, aGame->mHints, aGame->mState == 6 ? "true" : "false", mFrames);
	aState += aBuf;
	WordBallSystem* aBalls = aGame->mBallSystem;
	if (aBalls != nullptr && (aGame->mState == 5 || aGame->mState == 4 || aGame->mState == 7 || aGame->mState == 2))
	{
		std::snprintf(aBuf, sizeof(aBuf), ",\"water\":%.4f,\"submit\":%s,\"held\":%s,\"shortest\":%d,\"bonus\":%s",
			aBalls->mWaterLevel, aBalls->mHud != nullptr && aBalls->mHud->isSubmitShown() ? "true" : "false",
			aBalls->mRope != nullptr ? "true" : "false", aBalls->mShortestWord, jsonText(aBalls->mWord).c_str());
		aState += aBuf;
		aState += ",\"words\":[";
		bool aFirst = true;
		for (auto& aWord : aBalls->mWordIndex)
		{
			aState += (aFirst ? "" : ",") + jsonText(aWord.first);
			aFirst = false;
		}
		aState += "],\"balls\":[";
		aFirst = true;
		for (unsigned int i = 0; i < aBalls->mCount; i++)
		{
			Ball* aBall = aBalls->getBall(i);
			if (aBall->mState == BALL_GONE)
				continue;
			engine::Point aPos = toScreen(aBalls, *aBall->getPosition());
			std::snprintf(aBuf, sizeof(aBuf), "%s{\"i\":%u,\"l\":%s,\"x\":%.1f,\"y\":%.1f,\"r\":%.1f,\"s\":%d,\"f\":%u,"
				"\"fixed\":%s,\"chain\":%s}", aFirst ? "" : ",", i, jsonText(std::string(1, aBall->mLetter)).c_str(),
				aPos.x, aPos.y, aBall->mRadius, aBall->mState, aBall->mSpecial, aBall->mFixed ? "true" : "false",
				aBall->mChain != nullptr ? "true" : "false");
			aState += aBuf;
			aFirst = false;
		}
		aState += "]";
	}
	return aState + "}";
}

bool WordHarmonyApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	const std::string& aCmd = theCommand.mCommand;
	const std::string& anArgs = theCommand.mArgs;
	Game* aGame = Game::sInstance;
	auto ok = [&]() { std::printf("OK %s%s%s\n", aCmd.c_str(), anArgs.empty() ? "" : " ", anArgs.c_str()); };
	auto error = [&](const char* theWhy) { std::printf("ERROR %s: %s\n", aCmd.c_str(), theWhy); };
	bool aKnown = aCmd == "state" || aCmd == "move" || aCmd == "goto" || aCmd == "level" || aCmd == "unlock"
		|| aCmd == "set" || aCmd == "win" || aCmd == "lose" || aCmd == "seed" || aCmd == "botplay"
		|| aCmd == "botdebug";
	if (!aKnown)
		return SexyAppBase::HandleScriptCommand(theCommand);
	if (aGame == nullptr || !mEngineRunning)
	{
		error("the game has not started");
	}
	else if (aCmd == "state")
	{
		std::printf("STATE %s\n", TestState().c_str());
	}
	else if (aCmd == "move")
	{
		std::printf("MOVE %s\n", BotMove(aGame).c_str());
	}
	else if (aCmd == "goto")
	{
		// the game's own buttons' actions: a mode starts as its menu button starts it (its help or its levels first)
		bool aMenu = aGame->mState == 1;
		if (!aMenu)
			error("only from the main menu");
		else if (anArgs == "options")
			aGame->screenAction("options"), ok();
		else if (anArgs == "highscores")
			aGame->screenAction("highscore"), ok();
		else if (anArgs == "help")
			aGame->helpKey(false, aGame->mMode), ok();
		else if (anArgs == "relax")
			aGame->screenAction("newPuzzleGame"), ok();
		else if (anArgs == "think")
			aGame->screenAction("newSolitaireGame"), ok();
		else if (anArgs == "tsunami")
			aGame->screenAction("newActionGame"), ok();
		else
			error("not a screen this game opens directly (options, highscores, help, relax, think, tsunami)");
	}
	else if (aCmd == "level")
	{
		// "level <n> [mode]": as the levels screen's button for level n starts it (the mode's help first when it is on)
		char aMode[32] = "";
		int aLevel = 0;
		int aCount = std::sscanf(anArgs.c_str(), "%d %31s", &aLevel, aMode);
		int aModeIndex = aCount == 2 ? modeByName(aMode) : aGame->mMode;
		if (aCount < 1 || aLevel < 1 || aModeIndex < 0)
			error("level <n> [relax|think|tsunami]");
		else if (aGame->mState != 1)
			error("only from the main menu");
		else
		{
			const char* aPrefix = aModeIndex == MODE_PUZZLE ? "levels" : aModeIndex == MODE_SOLITAIRE ? "solitaire" : "action";
			aGame->mMode = aModeIndex;
			aGame->screenAction(aPrefix + std::to_string(aLevel));
			ok();
		}
	}
	else if (aCmd == "unlock")
	{
		// every levels screen's buttons, as reaching their last levels would
		LevelsSelectionScreen* aScreens[3] = { aGame->mLevelsSelection, aGame->mSolitaireSelection, aGame->mDifficultySelection };
		for (int i = 0; i < 3; i++)
		{
			if (aScreens[i] != nullptr)
				aGame->mSettings.setHighestLevel(i, aScreens[i]->mStep * (aScreens[i]->mCount - 1) + 1);
		}
		aGame->mSettings.save();
		ok();
	}
	else if (aCmd == "set")
	{
		char aName[32] = "";
		double aValue = 0.0;
		if (std::sscanf(anArgs.c_str(), "%31s %lf", aName, &aValue) != 2)
			error("set <name> <value>");
		else if (std::string(aName) == "score")
			aGame->addScore((int)aValue - aGame->mScore), ok();
		else if (std::string(aName) == "hints")
			aGame->mHints = (int)aValue, ok();
		else if (std::string(aName) == "lettersleft")
			aGame->mLettersLeft = (int)aValue, ok();
		else if (std::string(aName) == "water" && aGame->mBallSystem != nullptr)
			aGame->mBallSystem->mWaterLevel = (float)aValue, ok();
		else
			error("score, hints, lettersleft or water");
	}
	else if (aCmd == "win")
	{
		if (aGame->mState != 5)
			error("no level is being played");
		else
			aGame->mBallSystem->levelDone(), ok();
	}
	else if (aCmd == "lose")
	{
		if (aGame->mState != 5)
			error("no level is being played");
		else
			aGame->gameOver(false, false), ok();
	}
	else if (aCmd == "botplay")
	{
		// "botplay <updates>": the bot plays the next updates through the widget's mouse handlers (BotTick)
		int aCount = std::atoi(anArgs.c_str());
		mBotUntil = mFrames + aCount;
		mBotSteps.clear();
		mBotAt = 0;
		mBotWait = 0;
		ok();
	}
	else if (aCmd == "botdebug")
	{
		// the bot's plans on stderr (as WH_BOTDEBUG=1), for a web run that has no environment
		gBotDebug = anArgs != "off";
		ok();
	}
	else if (aCmd == "seed")
	{
		char* anEnd = nullptr;
		unsigned long aSeed = std::strtoul(anArgs.c_str(), &anEnd, 10);
		if (anArgs.empty() || *anEnd != '\0')
			error("seed <n>");
		else
			engine::crtSrand((unsigned int)aSeed), ok();
	}
	std::fflush(stdout);
	return true;
}

// botplay: the next of the bot's steps before a frame, a new move once the last one and its wait are done
void WordHarmonyApp::BotTick()
{
	if (mBotWait > 0)
	{
		mBotWait--;
		return;
	}
	while (true)
	{
		if (mBotAt >= mBotSteps.size())
		{
			BotAction anAction = BotNext(Game::sInstance);
			mBotSteps = anAction.mSteps;
			mBotAt = 0;
			if (anAction.mWhat.empty())
			{
				mBotWait = 30;
				return;
			}
			mBotSteps.push_back({ BotStep::WAIT, 0, 0, anAction.mUpdates });
			if (anAction.mWhat != "wait")
			{
				mBotMoves++;
				std::printf("BOT %d %d %s %s score %d\n", mFrames, mBotMoves, anAction.mWhat.c_str(), anAction.mWord.c_str(),
					Game::sInstance->mScore);
				std::fflush(stdout);
			}
		}
		const BotStep& st = mBotSteps[mBotAt++];
		// the same input as a -script line: the bot's step before frame mFrames + 1 changes what the state log puts under
		// mFrames + 1, which a script's line for frame mFrames + 2 does (its input is handled before that frame, and
		// logged under the frame before)
		if (mInputLog != nullptr && st.mKind != BotStep::WAIT)
		{
			static const char* const sNames[] = { "move", "down", "up", "click", "rclick" };
			std::fprintf(mInputLog, "%d %s %d %d\n", mFrames + 2, sNames[st.mKind], st.x, st.y);
			std::fflush(mInputLog);
		}
		switch (st.mKind)
		{
		case BotStep::WAIT:
			mBotWait = st.mN - 1;
			return;
		case BotStep::MOVE:
			mWidget->MouseMove(st.x, st.y);
			break;
		case BotStep::DOWN:
			mWidget->MouseMove(st.x, st.y);
			mWidget->MouseDown(st.x, st.y, 0, 1);
			break;
		case BotStep::UP:
			mWidget->MouseMove(st.x, st.y);
			mWidget->MouseUp(st.x, st.y, 0, 1);
			break;
		case BotStep::CLICK:
		case BotStep::RCLICK:
			mWidget->MouseMove(st.x, st.y);
			mWidget->MouseDown(st.x, st.y, st.mKind == BotStep::RCLICK ? 1 : 0, 1);
			mWidget->MouseUp(st.x, st.y, st.mKind == BotStep::RCLICK ? 1 : 0, 1);
			return;
		}
	}
}
