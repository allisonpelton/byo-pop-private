// port: the standard test commands (docs/TEST_COMMANDS.md), reached only from a test's -script, its live command channel
// (-commands, byopop_test_command) or -realinput, through HandleScriptCommand. A command that is never given changes
// nothing: no draw of the game's random numbers, no state. Each answers on stdout with one line: STATE {...},
// MOVE {...}, OK <command> or ERROR <command>: <reason>.
//
//   state              STATE {json}: screen, dialog, mode, level, score, timer, moves, tiles, busy, buttons, ...
//   goto <screen>      menu, options, help (from the main menu), classic [n] (the list, or layout n at once),
//                      escape or map (the adventure's map)
//   level <n>          adventure level n: the player's progress set to the level before it, the map opened and its
//                      Continue pressed, so the level starts as the game starts it (a saved game of the level goes)
//   unlock all         the adventure's progress goes to its last level (nothing else is locked)
//   set <name> <n>     score or progress
//   win                the game's own moves, from the bot's way to the end, until it ends the game
//   lose, seed <n>     ERROR: a game here is never lost, and every deal has its seed from the game's own clock
//   move               MOVE {json}: the bot's next move (Bot.h) and the real input that makes it; changes nothing
//
// The commands that switch screens answer once the new screen has settled (so a driver need not poll), from the
// frame that runs after the command.

#include "Bot.h"
#include "Board.h"
#include "Dialog.h"
#include "Globals.h"
#include "Layer.h"
#include "LayoutSelect.h"
#include "MahjongApp.h"
#include "MainMenu.h"
#include "Manager.h"
#include "MapLayer.h"
#include "Object.h"
#include "PlayLayer.h"
#include "Players.h"
#include "User.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <typeinfo>
#include <vector>

using namespace Sexy;
using namespace mahjongjapan;

namespace {

enum PendingKind
{
	PENDING_NONE,
	PENDING_SCREEN,   // wait until the top layer is theScreen and has settled
	PENDING_MAP,      // wait for the map, then press Continue for a level
	PENDING_BOARD,    // wait for a board whose tiles are in
	PENDING_DIALOG,   // wait for a dialog on top
};

struct Pending
{
	PendingKind mKind = PENDING_NONE;
	std::string mCommand;  // the command line to answer with "OK <command>"
	std::string mScreen;
	int mLevel = -1;       // the level to start from the map
	int mFrames = 0;
};

Pending gPending;
Bot gBot;

void Reply(const std::string& theLine)
{
	printf("%s\n", theLine.c_str());
	fflush(stdout);
}

std::string CommandText(const ScriptCommand& theCommand)
{
	return theCommand.mCommand + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs);
}

void ReplyOK(const std::string& theCommand)
{
	Reply("OK " + theCommand);
}

void ReplyError(const std::string& theCommand, const std::string& theReason)
{
	Reply("ERROR " + theCommand + ": " + theReason);
}

std::string Num(int theValue)
{
	return std::to_string(theValue);
}

// The kind of each layer on the stack, by its class
struct LayerInfo
{
	const char* mName;
	bool mScreen;  // a screen of its own, not a dialog over one
};

LayerInfo InfoOf(jb::Layer* theLayer)
{
	std::string aType = typeid(*theLayer).name();
	auto Is = [&](const char* theName) { return aType.find(theName) != std::string::npos; };
	if (Is("NewPlayerBox"))
		return {"newplayer", false};
	if (Is("PlayerBox"))
		return {"players", false};
	if (Is("MainMenu"))
		return {"menu", true};
	if (Is("LayoutSelect"))
		return {"classic list", true};
	if (Is("MapLayer"))
		return {"map", true};
	if (Is("PlayLayer"))
		return {((PlayLayer*)theLayer)->mMode == 0 ? "escape" : "classic", true};
	if (Is("EndLayer"))
		return {"ending", true};
	if (Is("LevelDone"))
		return {"level done", true};
	if (Is("Splash"))
		return {"splash", true};
	if (Is("PauseLayer"))
		return {"pause", false};
	if (Is("TipBox"))
		return {"tip", false};
	if (Is("NoMovesBox"))
		return {"no moves", false};
	if (Is("ResultBox"))
		return {"result", false};
	if (Is("QuitNote"))
		return {"quit note", false};
	if (Is("RestartBox"))
		return {"restart", false};
	if (Is("FreeMenu") || Is("EscapeMenu"))
		return {"game menu", false};
	if (Is("OptionsNote"))
		return {"options note", false};
	if (Is("OptionsBox"))
		return {"options", false};
	if (Is("StyleBox"))
		return {"tile sets", false};
	if (Is("BackgroundBox"))
		return {"backgrounds", false};
	if (Is("HelpMenu") || Is("HelpFree") || Is("HelpEscape"))
		return {"help", false};
	if (Is("ErrorBox"))
		return {"error", false};
	if (Is("YesNoBox"))
		return {"question", false};
	return {"dialog", false};
}

jb::Layer* TopLayer()
{
	if (jb::gManager == nullptr || jb::gManager->mLayers.empty())
		return nullptr;
	return jb::gManager->mLayers.back();
}

// The topmost layer that is a screen of its own; the dialogs over it are not
jb::Layer* ScreenLayer()
{
	if (jb::gManager == nullptr)
		return nullptr;
	for (size_t i = jb::gManager->mLayers.size(); 0 < i; i--)
		if (InfoOf(jb::gManager->mLayers[i - 1]).mScreen)
			return jb::gManager->mLayers[i - 1];
	return nullptr;
}

template <class T>
T* FindLayer()
{
	if (jb::gManager == nullptr)
		return nullptr;
	for (size_t i = jb::gManager->mLayers.size(); 0 < i; i--)
		if (T* aLayer = dynamic_cast<T*>(jb::gManager->mLayers[i - 1]))
			return aLayer;
	return nullptr;
}

PlayLayer* ActivePlay()
{
	for (size_t i = jb::gManager->mLayers.size(); 0 < i; i--)
		if (PlayLayer* aPlay = dynamic_cast<PlayLayer*>(jb::gManager->mLayers[i - 1]))
			return aPlay;
	return nullptr;
}

// No layer is fading in or out and the manager has no fade of its own
bool Settled()
{
	jb::Layer* aTop = TopLayer();
	return aTop != nullptr && jb::gManager->mFade == 0 && jb::gManager->mNextLayer == nullptr && aTop->mFade == 0;
}

// The board takes no click (Board::OnLButtonDown, 0x40c030)
bool BoardBlocked(Board* theBoard)
{
	if (theBoard == nullptr || !theBoard->mStarted)
		return true;
	return !((theBoard->mFlightCount < 1 || theBoard->mFlightKind == 0 || theBoard->mFlightKind == 6) &&
		theBoard->mPhase == 0 && theBoard->mPhaseLeft < 1);
}

struct Btn
{
	int mCmd, mX, mY, mW, mH;
};

void AddButtons(jb::Object* theObject, std::vector<Btn>& theList)
{
	if (!theObject->IsVisible())
		return;
	if (jb::Button* aButton = dynamic_cast<jb::Button*>(theObject))
	{
		if (aButton->IsEnabled())
		{
			int aPos[2];
			aButton->GetScreenPos(aPos);
			theList.push_back({aButton->mCmd, aPos[0] + aButton->GetFrameWidth() / 2, aPos[1] + aButton->mHeight / 2,
				aButton->GetFrameWidth(), aButton->mHeight});
		}
	}
	for (jb::Object* aChild : theObject->mChildren)
		AddButtons(aChild, theList);
}

// The buttons of the top layer: a hand's clicks go to it, and to nothing under it
std::vector<Btn> TopButtons()
{
	std::vector<Btn> aList;
	jb::Layer* aTop = TopLayer();
	if (aTop != nullptr)
		for (jb::Object* anObject : aTop->mObjects)
			AddButtons(anObject, aList);
	return aList;
}

std::string ButtonsJson()
{
	std::string aJson = "[";
	for (const Btn& aButton : TopButtons())
		aJson += std::string(aJson.size() > 1 ? "," : "") + "{\"cmd\":" + Num(aButton.mCmd) + ",\"x\":" + Num(aButton.mX) +
			",\"y\":" + Num(aButton.mY) + ",\"w\":" + Num(aButton.mW) + ",\"h\":" + Num(aButton.mH) + "}";
	return aJson + "]";
}

// busy as a driver wants it: the screen is changing, or the board does not take a click yet
bool IsBusy()
{
	if (!Settled())
		return true;
	PlayLayer* aPlay = ActivePlay();
	jb::Layer* aTop = TopLayer();
	if (aPlay != nullptr && aTop == aPlay)
		return BoardBlocked(aPlay->mBoard) || aPlay->mBoard->mEscaping;
	return false;
}

int Levels()
{
	return (int)(sizeof(((MapLayer*)nullptr)->mLevels) / sizeof(MapLayer::Level));
}

std::string StateJson()
{
	jb::Layer* aScreen = ScreenLayer();
	jb::Layer* aTop = TopLayer();
	std::string aScreenName = aScreen != nullptr ? InfoOf(aScreen).mName : "none";
	std::string aDialog = "null";
	if (aTop != nullptr && aTop != aScreen)
		aDialog = "\"" + std::string(InfoOf(aTop).mName) + "\"";
	std::string aJson = "{\"screen\":\"" + aScreenName + "\",\"dialog\":" + aDialog;
	aJson += ",\"busy\":" + std::string(IsBusy() ? "true" : "false");
	aJson += ",\"player\":" + std::string(gPlayers.mCurrent == -1 ? "null" : "\"" + std::string(gPlayers.GetName(gPlayers.mCurrent)) + "\"");
	aJson += ",\"progress\":" + Num(gUser.mProgress) + ",\"levels\":" + Num(Levels());
	aJson += ",\"updates\":" + Num(gApp != nullptr ? gApp->mUpdateCount : 0);
	aJson += ",\"layers\":[";
	for (size_t i = 0; jb::gManager != nullptr && i < jb::gManager->mLayers.size(); i++)
		aJson += (i != 0 ? ",\"" : "\"") + std::string(InfoOf(jb::gManager->mLayers[i]).mName) + "\"";
	aJson += "]";
	if (PlayLayer* aPlay = ActivePlay())
	{
		Board* aBoard = aPlay->mBoard;
		bool aEscape = aPlay->mMode == 0;
		aJson += ",\"mode\":" + std::string(aEscape ? "\"escape\"" : "\"classic\"");
		aJson += ",\"level\":" + Num(aPlay->mLevel + 1) + ",\"layout\":" + Num(aBoard != nullptr ? aBoard->mLayoutId : 0);
		aJson += ",\"score\":" + Num(aPlay->mScore) + ",\"timer\":" + Num(aPlay->mPanel != nullptr ? aPlay->mPanel->GetSeconds() : 0);
		if (aBoard != nullptr)
		{
			aJson += ",\"moves\":" + Num(aBoard->mPairs) + ",\"tiles\":" + Num(aBoard->mLeft);
			aJson += ",\"board\":{\"started\":" + std::string(aBoard->mStarted ? "true" : "false") +
				",\"over\":" + (aBoard->mOver ? "true" : "false") + ",\"escaping\":" + (aBoard->mEscaping ? "true" : "false") +
				",\"blocked\":" + (BoardBlocked(aBoard) ? "true" : "false") + ",\"selected\":" +
				(aBoard->mSelected != nullptr ? "true" : "false") + ",\"move\":" + Num(aBoard->mMove) + "}";
		}
	}
	aJson += ",\"buttons\":" + ButtonsJson() + "}";
	return aJson;
}

std::string Click(int theX, int theY)
{
	return "{\"move\":[" + Num(theX) + "," + Num(theY) + "]},{\"click\":[" + Num(theX) + "," + Num(theY) + "]}";
}

// The answer to `move`: the bot's pair, a shuffle when no pair is free, or the dialog's button
void ReplyMove()
{
	jb::Layer* aTop = TopLayer();
	if (aTop == nullptr || IsBusy())
	{
		Reply("MOVE null");
		return;
	}
	PlayLayer* aPlay = ActivePlay();
	if (aPlay != nullptr && aTop == aPlay)
	{
		Board* aBoard = aPlay->mBoard;
		BotMove aMove;
		if (gBot.NextMove(*aBoard, aMove))
		{
			std::string aLine = "MOVE {\"kind\":\"" + std::string(aMove.mSpecial ? "special" : "pair") + "\",\"tiles\":[" +
				Num(aMove.mFirstKind) + "," + Num(aMove.mSecondKind) + "],\"ends\":" + (aMove.mEnds ? "true" : "false") +
				",\"input\":[" + Click(aMove.mX1, aMove.mY1) + "," + Click(aMove.mX2, aMove.mY2) + "]}";
			Reply(aLine);
			return;
		}
		if (!aBoard->mOver && aBoard->mLeft != 0 && aPlay->mShuffle != nullptr && aPlay->mShuffle->IsEnabled())
		{
			int aPos[2];
			aPlay->mShuffle->GetScreenPos(aPos);
			Reply("MOVE {\"kind\":\"shuffle\",\"input\":[" + Click(aPos[0] + aPlay->mShuffle->GetFrameWidth() / 2,
				aPos[1] + aPlay->mShuffle->mHeight / 2) + "]}");
			return;
		}
		Reply("MOVE null");
		return;
	}
	// a dialog that has one answer: its default button (command 1, the OK or Yes), else its first
	std::string aName = InfoOf(aTop).mName;
	if (aName == "map")
	{
		// the map goes on with the level its marker stands at
		for (const Btn& aButton : TopButtons())
			if (aButton.mCmd == 0xe6)
			{
				Reply("MOVE {\"kind\":\"continue\",\"input\":[" + Click(aButton.mX, aButton.mY) + "]}");
				return;
			}
		Reply("MOVE null");
		return;
	}
	if (aName != "question" && aName != "game menu" && aName != "quit note" && aName != "newplayer" && aName != "players" &&
		aName != "restart" && aName != "menu" && aName != "classic list" && aName != "splash" && aName != "ending")
	{
		std::vector<Btn> aList = TopButtons();
		const Btn* aPick = nullptr;
		for (const Btn& aButton : aList)
			if (aButton.mCmd == 1 && aPick == nullptr)
				aPick = &aButton;
		if (aPick == nullptr && !aList.empty())
			aPick = &aList[0];
		if (aPick != nullptr)
		{
			Reply("MOVE {\"kind\":\"dialog\",\"name\":\"" + aName + "\",\"input\":[" + Click(aPick->mX, aPick->mY) + "]}");
			return;
		}
	}
	Reply("MOVE null");
}

bool NeedPlayer(const std::string& theCommand)
{
	if (gPlayers.mCurrent != -1)
		return false;
	ReplyError(theCommand, "no player: make one in the main menu (any button asks for a name)");
	return true;
}

} // namespace

namespace Sexy
{

// The frame after a command that waits for its screen (called from MahjongApp::RunFrame while it has one)
void MahjongApp::TestFrame()
{
	if (gPending.mKind == PENDING_NONE || jb::gManager == nullptr)
		return;
	if (60 * 40 < ++gPending.mFrames)
	{
		ReplyError(gPending.mCommand, "timed out (" + StateJson() + ")");
		gPending.mKind = PENDING_NONE;
		return;
	}
	if (!Settled())
		return;
	jb::Layer* aTop = TopLayer();
	switch (gPending.mKind)
	{
	case PENDING_SCREEN:
		if (aTop != nullptr && InfoOf(aTop).mName == gPending.mScreen)
		{
			ReplyOK(gPending.mCommand);
			gPending.mKind = PENDING_NONE;
		}
		break;
	case PENDING_MAP:
		if (MapLayer* aMap = dynamic_cast<MapLayer*>(aTop))
		{
			if (aMap->mContinue == nullptr || !aMap->mContinue->IsVisible() || !aMap->mContinue->IsEnabled() || aMap->mShowing)
				break;
			if (gPending.mLevel < 0)
			{
				ReplyOK(gPending.mCommand);
				gPending.mKind = PENDING_NONE;
				break;
			}
			gPending.mKind = PENDING_BOARD;
			gPending.mFrames = 0;
			aMap->OnCommand(0xe6);
		}
		break;
	case PENDING_BOARD:
	{
		PlayLayer* aPlay = ActivePlay();
		if (aPlay != nullptr && aPlay->mBoard != nullptr && aPlay->mBoard->mStarted && !BoardBlocked(aPlay->mBoard) &&
			(aTop == aPlay || InfoOf(aTop).mName == std::string("tip")))
		{
			ReplyOK(gPending.mCommand);
			gPending.mKind = PENDING_NONE;
		}
		break;
	}
	case PENDING_DIALOG:
		if (aTop != nullptr && !InfoOf(aTop).mScreen)
		{
			ReplyOK(gPending.mCommand);
			gPending.mKind = PENDING_NONE;
		}
		break;
	default:
		break;
	}
}

bool MahjongApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	const std::string& aCommand = theCommand.mCommand;
	const std::string& anArgs = theCommand.mArgs;
	bool aKnown = aCommand == "state" || aCommand == "move" || aCommand == "goto" || aCommand == "level" ||
		aCommand == "unlock" || aCommand == "set" || aCommand == "win" || aCommand == "lose" || aCommand == "seed";
	if (!aKnown)
		return false;
	std::string aText = CommandText(theCommand);
	if (!mGameStarted || mGameEnded || jb::gManager == nullptr)
	{
		ReplyError(aText, "the game has not started");
		return true;
	}
	if (aCommand == "state")
	{
		Reply("STATE " + StateJson());
		return true;
	}
	if (aCommand == "move")
	{
		ReplyMove();
		return true;
	}
	if (gPending.mKind != PENDING_NONE)
	{
		ReplyError(aText, "the last command is still waiting for its screen (" + gPending.mCommand + ")");
		return true;
	}
	if (aCommand == "lose")
	{
		ReplyError(aText, "a game here is never lost: a board with no pair left offers the shuffle");
		return true;
	}
	if (aCommand == "seed")
	{
		ReplyError(aText, "no seed: a classic deal comes from the clock and a level's has its own in the stage table");
		return true;
	}
	if (aCommand == "unlock")
	{
		if (anArgs != "all")
			ReplyError(aText, "unlock all");
		else if (NeedPlayer(aText))
			;
		else
		{
			gUser.mProgress = Levels() - 1;
			gUser.Save();
			ReplyOK(aText);
		}
		return true;
	}
	if (aCommand == "set")
	{
		std::string aName = anArgs.substr(0, anArgs.find(' '));
		int aValue = anArgs.find(' ') == std::string::npos ? 0 : atoi(anArgs.c_str() + anArgs.find(' ') + 1);
		PlayLayer* aPlay = ActivePlay();
		if (aName == "score" && aPlay != nullptr)
		{
			aPlay->AddScore(aValue - aPlay->mScore);
			ReplyOK(aText);
		}
		else if (aName == "progress" && gPlayers.mCurrent != -1 && 0 <= aValue && aValue <= Levels())
		{
			gUser.mProgress = aValue;
			ReplyOK(aText);
		}
		else
			ReplyError(aText, "score (in a game) or progress (0-" + Num(Levels()) + ", with a player)");
		return true;
	}
	if (aCommand == "win")
	{
		PlayLayer* aPlay = ActivePlay();
		if (aPlay == nullptr || aPlay->mBoard == nullptr || TopLayer() != aPlay || aPlay->mBoard->mOver ||
			aPlay->mBoard->mEscaping || !aPlay->mBoard->mStarted || aPlay->mBoard->mPhase != 0)
		{
			ReplyError(aText, "no game on the board to win");
			return true;
		}
		std::vector<BotMove> aMoves;
		if (!gBot.Solve(*aPlay->mBoard, aMoves) || aMoves.empty())
		{
			ReplyError(aText, "the bot found no way to the end");
			return true;
		}
		Board* aBoard = aPlay->mBoard;
		aBoard->ClearSelection();
		aBoard->StartClock();
		for (BotMove& aMove : aMoves)
			aBoard->Take(aMove.mFirst, aMove.mSecond);
		ReplyOK(aText);
		return true;
	}
	if (aCommand == "level")
	{
		int aLevel = atoi(anArgs.c_str());
		if (aLevel < 1 || Levels() < aLevel)
			ReplyError(aText, "level 1-" + Num(Levels()));
		else if (NeedPlayer(aText))
			;
		else
		{
			std::string aPath;
			gUser.GetEscapeSave(aLevel - 1, aPath);
			remove(aPath.c_str());
			gUser.mProgress = aLevel - 1;
			gUser.Save();
			gPending = Pending();
			gPending.mKind = PENDING_MAP;
			gPending.mCommand = aText;
			gPending.mLevel = aLevel - 1;
			jb::gManager->SwitchTo(new PlayLayer(0));
		}
		return true;
	}
	// goto
	std::string aTarget = anArgs.substr(0, anArgs.find(' '));
	std::string aRest = anArgs.find(' ') == std::string::npos ? "" : anArgs.substr(anArgs.find(' ') + 1);
	gPending = Pending();
	gPending.mCommand = aText;
	if (aTarget == "menu" || aTarget == "title")
	{
		gPending.mKind = PENDING_SCREEN;
		gPending.mScreen = "menu";
		if (jb::gManager->mLayers.empty() || InfoOf(TopLayer()).mName != std::string("menu"))
			jb::gManager->SwitchTo(new MainMenu());
	}
	else if (aTarget == "escape" || aTarget == "map")
	{
		if (NeedPlayer(aText))
			return true;
		gPending.mKind = PENDING_MAP;
		jb::gManager->SwitchTo(new PlayLayer(0));
	}
	else if (aTarget == "classic")
	{
		if (NeedPlayer(aText))
			return true;
		if (aRest.empty())
		{
			gPending.mKind = PENDING_SCREEN;
			gPending.mScreen = "classic list";
			jb::gManager->SwitchTo(new LayoutSelect());
		}
		else
		{
			int aLayout = atoi(aRest.c_str());
			if (aLayout < 1 || gLayoutCount < aLayout)
			{
				ReplyError(aText, "layout 1-" + Num(gLayoutCount));
				return true;
			}
			Layout aTemp;
			aTemp.Load(GetLayoutId(aLayout - 1));
			gUser.mSelectedLayout = aLayout - 1;
			PlayLayer* aNew = new PlayLayer(1);
			aNew->SetLayout(aLayout - 1, false, GetLayoutId(aLayout - 1), aTemp.mName.c_str());
			gPending.mKind = PENDING_BOARD;
			gPending.mLevel = 0;
			jb::gManager->SwitchTo(aNew);
		}
	}
	else if (aTarget == "options" || aTarget == "help")
	{
		MainMenu* aMenu = dynamic_cast<MainMenu*>(TopLayer());
		if (aMenu == nullptr)
		{
			ReplyError(aText, aTarget + " opens from the main menu (goto menu first)");
			return true;
		}
		if (aTarget == "options" && NeedPlayer(aText))
			return true;
		gPending.mKind = PENDING_DIALOG;
		aMenu->OnCommand(aTarget == "options" ? 0x83 : 0x87);
	}
	else
	{
		ReplyError(aText, "menu, options, help, classic [layout], escape or map");
		return true;
	}
	return true;
}

} // namespace Sexy
