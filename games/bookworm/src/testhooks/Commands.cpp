// port: test hooks, the standard script commands (docs/TEST_COMMANDS.md). Not part of the original game. They are
// reachable only from a test (-script, -commands, the web test page's command call); a command that is never given
// changes nothing. Every command answers with one line on stdout: OK <command>, ERROR <command>: <reason>,
// STATE {...} or MOVE {...}.
//
//   state                 STATE {json}: screen (title, menu, help, highscores, game, none), dialog, mode, level, score,
//                         timer (Action's fire timer), fire (Classic's fire meter), bonus, over, busy and the board
//                         (its state, the letters and tile types by row, the selection)
//   goto <screen>         menu (the game selector), options, help, highscores (the Hall of Fame), credits, classic or
//                         action (that mode through the selector's own SelectGameMode: the continue question when a
//                         game is saved). title is an error once it is gone
//   level <n>             level n of the game being played, from level n - 1's threshold through the game's own NextLevel
//   unlock all            ERROR: nothing in the game is locked
//   set <name> <value>    score (the game's and the level's), timer, fire, mode (classic|action: the mode the selector's
//                         button in `move` picks)
//   win                   the level's threshold reached: the level-up and its summary, as a word that reaches it does
//   lose [n]              the game lost as it loses: a bottom fire tile, then its own fire scan (n: after n words of `move`)
//   seed <n>              the C library's generator, the framework's (the 2004 way, as Init seeds it) and the game
//                         generator from its first draw, as at start-up
//   move                  MOVE {json}: the next input of a player who plays the word solver's first hint (Solver.h),
//                         from the title to the Hall of Fame; MOVE null while the game is busy or no move is known.
//                         It changes nothing and draws no number from the game's generators
//
// A MOVE's "input" is in game coordinates, in order: {"click":[x,y]} a left click, {"key":"D"} a key as the DOM names it
// ("Enter"), {"wait":n} n updates. "updates" is how long to wait after it before asking again.

#include "BookWormApp.h"
#include "BWGameSelector.h"
#include "Board.h"
#include "ColorizedButton.h"
#include "Common.h"
#include "EditWidget.h"
#include "Dialog.h"
#include "DialogButton.h"
#include "ButtonWidget.h"
#include "FameWidget.h"
#include "HelpWidget.h"
#include "HyperlinkWidget.h"
#include "LevelUpWidget.h"
#include "Lex.h"
#include "NewUserDialog.h"
#include "OverlayWidget.h"
#include "Res.h"
#include "Solver.h"
#include "StatKeeper.h"
#include "TitleScreen.h"
#include "UserManager.h"
#include "WidgetManager.h"
#include "WordBalloon.h"
#include "Compat2004.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>

using namespace Sexy;

namespace
{

const char* const DRIVER_NAME = "Driver";   // the name `move` types into the new-player dialog

// `lose n`: the game is lost (as `lose` does) once `move` has played n words, counted from the command
int gLoseAfterWords = 0;
int gWordsMoved = 0;

void Reply(const std::string& theLine)
{
	printf("%s\n", theLine.c_str());
	fflush(stdout);
}

void Ok(const std::string& theLine)
{
	Reply("OK " + theLine);
}

void Error(const std::string& theLine, const std::string& theReason)
{
	Reply("ERROR " + theLine + ": " + theReason);
}

std::string Num(int theValue)
{
	return std::to_string(theValue);
}

std::string Quote(const std::string& theText)
{
	std::string s = "\"";
	for (char c : theText)
	{
		if ((c == '"') || (c == '\\'))
			s += '\\';
		if ((unsigned char)c >= 0x20)
			s += c;
	}
	return s + "\"";
}

std::string Bool(bool theValue)
{
	return theValue ? "true" : "false";
}

// a point of a widget that a click reaches (nothing in front of it), searched from the middle outwards on a grid
bool Reachable(BookWormApp* theApp, Widget* theWidget, int* theX, int* theY)
{
	if ((theWidget == NULL) || (!theWidget->mVisible) || (theWidget->mDisabled))
		return false;
	Point aPos = theWidget->GetAbsPos();
	for (int aRing = 0; aRing <= 4; aRing++)
		for (int dy = -aRing; dy <= aRing; dy++)
			for (int dx = -aRing; dx <= aRing; dx++)
			{
				if ((abs(dx) != aRing) && (abs(dy) != aRing))
					continue;
				int x = aPos.mX + theWidget->mWidth / 2 + dx * theWidget->mWidth / 10;
				int y = aPos.mY + theWidget->mHeight / 2 + dy * theWidget->mHeight / 10;
				if (theApp->mWidgetManager->GetWidgetAt(x, y, NULL, NULL) == theWidget)
				{
					*theX = x;
					*theY = y;
					return true;
				}
			}
	return false;
}

std::string ClickStep(int theX, int theY)
{
	return "{\"click\":[" + Num(theX) + "," + Num(theY) + "]}";
}

const char* DialogName(int theId)
{
	switch (theId)
	{
	case DIALOG_OPTIONS: return "options";
	case DIALOG_CONTINUE: return "continue";
	case DIALOG_QUIT: return "quit";
	case DIALOG_HINT: return "hint";
	case DIALOG_CREDITS: return "credits";
	case DIALOG_MESSAGE: return "message";
	case DIALOG_SELECT_USER: return "users";
	case DIALOG_DELETE_USER: return "deleteuser";
	case DIALOG_CONFIRM_DELETE_USER: return "confirmdelete";
	case DIALOG_NEW_USER: return "newuser";
	case DIALOG_LEAVE_GAME: return "leave";
	default: return "other";
	}
}

const char* ScreenName(BookWormApp* theApp)
{
	if (theApp->mTitleScreen != NULL)
		return "title";
	if (theApp->mHelpWidget != NULL)
		return "help";
	if (theApp->mFameWidget != NULL)
		return "highscores";
	if (theApp->mGameSelector != NULL)
		return "menu";
	if (theApp->mBoard != NULL)
		return "game";
	return "none";
}

const char* BoardStateName(int theState)
{
	static const char* const NAMES[] = { "playing", "gameovermusic", "gameover", "inactive", "gameoverpending",
		"levelcomplete", "nextlevel", "scrambling" };
	return ((theState >= 0) && (theState <= 7)) ? NAMES[theState] : "?";
}

// the board is in play on screen and takes a click on its tiles now (the -autoplay bot's test)
bool BoardTakesInput(BookWormApp* theApp, Board* theBoard)
{
	if ((theBoard == NULL) || (!theApp->mDialogMap.empty()) || (theApp->mHelpWidget != NULL) ||
		(theApp->mFameWidget != NULL) || (theApp->mGameSelector != NULL))
		return false;
	if (theApp->mUpdateCount + 1 - theApp->mResumeUpdateCnt <= 14)
		return false;
	return Solver::BoardAtRest(theBoard);
}

// a game that has ended and gone back to the selector
bool IsOver(BookWormApp* theApp)
{
	return (theApp->mGameSelector != NULL) && (theApp->mBoard != NULL) && (theApp->mBoard->mState == BOARD_GAMEOVER) &&
		(theApp->mFameWidget == NULL);
}

std::string BoardJson(BookWormApp* theApp, Board* b)
{
	std::string aLetters, aTypes;
	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
	{
		std::string aLetterRow, aTypeRow;
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			Piece* p = b->mBoard[aRow][aCol];
			aLetterRow += ((p != NULL) && (p->mLetter >= 'A') && (p->mLetter <= 'Z')) ? (char)p->mLetter : '.';
			aTypeRow += (p == NULL) ? '.' : (char)('0' + p->mType);
		}
		aLetters += std::string(aRow ? "," : "") + Quote(aLetterRow);
		aTypes += std::string(aRow ? "," : "") + Quote(aTypeRow);
	}
	std::string aSelection;
	for (PieceList::iterator anItr = b->mSelection.begin(); anItr != b->mSelection.end(); ++anItr)
		aSelection += (char)(*anItr)->mLetter;
	return std::string("{\"state\":") + Quote(BoardStateName(b->mState)) + ",\"busy\":" +
		Bool(!BoardTakesInput(theApp, b)) + ",\"letters\":[" + aLetters + "],\"types\":[" + aTypes + "],\"selection\":" +
		Quote(aSelection) + "}";
}

std::string StateJson(BookWormApp* theApp)
{
	Board* b = theApp->mBoard;
	std::string s = std::string("{\"screen\":") + Quote(ScreenName(theApp));
	s += ",\"dialog\":" + (theApp->mDialogMap.empty() ? std::string("null") :
		Quote(DialogName(theApp->mDialogMap.begin()->first)));
	s += ",\"mode\":" + Quote((theApp->mGameMode == GAMEMODE_ACTION) ? "action" : "classic");
	s += ",\"player\":" + Quote(theApp->mUserManager != NULL ? theApp->mUserManager->GetCurrentUserName() : "");
	if (b != NULL)
	{
		s += ",\"level\":" + Num(b->mLevel + 1) + ",\"score\":" + Num(b->mGameStats->mScore) + ",\"timer\":" +
			Num(b->mActionFireTimer);
		char aFire[32];
		sprintf(aFire, "%.3f", b->mFireMeter);
		s += std::string(",\"fire\":") + aFire + ",\"bonus\":" + Quote(b->mBonusWord) + ",\"board\":" + BoardJson(theApp, b);
	}
	s += ",\"over\":" + Bool(IsOver(theApp));
	s += ",\"busy\":" + Bool((b != NULL) && (theApp->mGameSelector == NULL) && (theApp->mDialogMap.empty()) &&
		(theApp->mHelpWidget == NULL) && (theApp->mFameWidget == NULL) && (!BoardTakesInput(theApp, b)));
	return s + "}";
}

// The game lost as the game loses it: a bottom-row tile turned to fire (and in Action the fire countdown run out),
// then the game's own fire scan, which starts the shake, the burn-down, the jingle and the summary
void LoseGame(BookWormApp* theApp, Board* b)
{
	for (int aCol = 0; aCol < BOARD_COLS; aCol++)
	{
		Piece* aPiece = b->mBoard[7][aCol];
		if ((aPiece != NULL) && (!aPiece->mSelected))
		{
			aPiece->mType = TILE_FIRE;
			break;
		}
	}
	if (theApp->mGameMode == GAMEMODE_ACTION)
		b->mActionCountdown = -1;
	b->BurnTiles(false);
}

std::string MoveJson(const std::string& theWhat, const std::string& theInput, int theUpdates, const std::string& theExtra = "")
{
	return "{\"what\":" + Quote(theWhat) + theExtra + ",\"input\":[" + theInput + "],\"updates\":" + Num(theUpdates) + "}";
}

// the next move, or "" for none
std::string NextMove(BookWormApp* theApp)
{
	int x, y;
	if (theApp->mTitleScreen != NULL)
	{
		if (Reachable(theApp, theApp->mTitleScreen->mPlayLink, &x, &y))
			return MoveJson("play", ClickStep(x, y), 30);
		return "";
	}

	if (!theApp->mDialogMap.empty())
	{
		int anId = theApp->mDialogMap.begin()->first;
		Dialog* aDialog = theApp->mDialogMap.begin()->second;
		switch (anId)
		{
		case DIALOG_NEW_USER:
			{
				std::string anInput;
				NewUserDialog* aNewUser = (NewUserDialog*) aDialog;
				for (size_t i = 0; i < aNewUser->mNameEdit->mString.size(); i++)
					anInput += std::string(anInput.empty() ? "" : ",") + "{\"key\":\"Backspace\"}";
				for (const char* c = DRIVER_NAME; *c != 0; c++)
					anInput += std::string(anInput.empty() ? "" : ",") + "{\"key\":\"" + *c + "\"}";
				anInput += ",{\"key\":\"Enter\"}";
				return MoveJson("name", anInput, 30);
			}
		case DIALOG_HINT:
		case DIALOG_MESSAGE:
			if (Reachable(theApp, aDialog->mYesButton, &x, &y))
				return MoveJson("ok", ClickStep(x, y), 20);
			return "";
		case DIALOG_CONTINUE:
			// a new game, not the saved one
			if (Reachable(theApp, aDialog->mNoButton, &x, &y))
				return MoveJson("newgame", ClickStep(x, y), 30);
			return "";
		default:
			return "";
		}
	}

	if (theApp->mHelpWidget != NULL)
	{
		if (Reachable(theApp, theApp->mHelpWidget->mContinueButton, &x, &y))
			return MoveJson("help", ClickStep(x, y), 30);
		return "";
	}

	if (theApp->mFameWidget != NULL)
	{
		if (Reachable(theApp, theApp->mFameWidget->mContinueButton, &x, &y))
			return MoveJson("fame", ClickStep(x, y), 30);
		return "";
	}

	if (theApp->mGameSelector != NULL)
	{
		BWGameSelector* aSelector = theApp->mGameSelector;
		if ((aSelector->mOpening) || (aSelector->mClosing) || (IsOver(theApp)))
			return "";
		bool isAction = theApp->mGameMode == GAMEMODE_ACTION;
		if (Reachable(theApp, isAction ? aSelector->mActionButton : aSelector->mClassicButton, &x, &y))
			return MoveJson(isAction ? "action" : "classic", ClickStep(x, y), 30);
		return "";
	}

	Board* b = theApp->mBoard;
	if (b == NULL)
		return "";

	LevelUpWidget* aSummary = b->mLevelUpWidget;
	if ((b->mSummaryShown) && (aSummary != NULL))
	{
		if ((aSummary->mFalling) || (aSummary->mSlide != 1.0))
			return "";
		if (Reachable(theApp, aSummary->mContinueButton, &x, &y))
			return MoveJson((b->mState == BOARD_GAMEOVER) ? "gameover" : "summary", ClickStep(x, y), 30);
		return "";
	}

	if (!BoardTakesInput(theApp, b))
		return "";

	// a selection left over: cleared first by a right click is not a click the driver sends; the solver's hint
	// replaces it anyway, since a click on a tile not next to the selection starts a new one
	if ((gLoseAfterWords > 0) && (gWordsMoved >= gLoseAfterWords))
	{
		gLoseAfterWords = 0;
		LoseGame(theApp, b);
		return MoveJson("lose", "{\"wait\":1}", 1);
	}

	SolverWord aHint;
	if (Solver::FirstHint(b, &aHint))
	{
		gWordsMoved++;
		Point aBoardPos = b->GetAbsPos();
		std::string anInput;
		for (size_t i = 0; i < aHint.mPath.size(); i++)
		{
			Piece* p = aHint.mPath[i];
			int aTileX = aBoardPos.mX + b->ColX(p->mCol) + 28;
			int aTileY = aBoardPos.mY + b->RowY(p->mCol, p->mRow) + 28;
			anInput += std::string(i ? "," : "") + ClickStep(aTileX, aTileY);
		}
		// submitted with the Submit button, or when something covers it (a definition balloon) by a second click
		// on the word's last tile, which submits too
		if (Reachable(theApp, b->mSubmitButton, &x, &y))
			anInput += "," + ClickStep(x, y);
		else
		{
			Piece* aLast = aHint.mPath.back();
			anInput += "," + ClickStep(aBoardPos.mX + b->ColX(aLast->mCol) + 28, aBoardPos.mY + b->RowY(aLast->mCol, aLast->mRow) + 28);
		}
		static const char* const KINDS[] = { "fire", "bonus", "best", "longest", "cashin" };
		std::string anExtra = ",\"word\":" + Quote(aHint.mWord) + ",\"score\":" + Num(aHint.mScore) + ",\"kind\":" +
			Quote(((aHint.mKind >= 0) && (aHint.mKind <= 4)) ? KINDS[aHint.mKind] : "?");
		return MoveJson("word", anInput, 10, anExtra);
	}

	// no word on the board: Lex scrambles it
	if ((b->mLex != NULL) && (!b->mLex->IsMoving()) && (Reachable(theApp, b->mLex, &x, &y)))
		return MoveJson("scramble", ClickStep(x, y), 30);
	return "";
}

bool ParseInt(const std::string& theText, int* theValue)
{
	if (theText.empty())
		return false;
	char* anEnd = NULL;
	long aValue = strtol(theText.c_str(), &anEnd, 10);
	if (*anEnd != 0)
		return false;
	*theValue = (int) aValue;
	return true;
}

// the board in play, or an error
Board* PlayingBoard(BookWormApp* theApp, const std::string& theLine)
{
	Board* b = theApp->mBoard;
	if ((b == NULL) || (theApp->mGameSelector != NULL) || (b->mState != BOARD_PLAYING) || (b->mSummaryShown))
	{
		Error(theLine, "no level in play");
		return NULL;
	}
	return b;
}

}

bool BookWormApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	const std::string& aName = theCommand.mCommand;
	std::istringstream anArgs(theCommand.mArgs);
	std::string anArg, aValue;
	anArgs >> anArg >> aValue;
	std::string aLine = aName + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs);

	if (aName == "state")
	{
		Reply("STATE " + StateJson(this));
		return true;
	}

	if (aName == "move")
	{
		std::string aMove = NextMove(this);
		Reply("MOVE " + (aMove.empty() ? std::string("null") : aMove));
		return true;
	}

	if (aName == "goto")
	{
		if (anArg == "title")
		{
			Error(aLine, mTitleScreen != NULL ? "already there" : "the title screen does not come back");
			return true;
		}
		if (mTitleScreen != NULL)
		{
			Error(aLine, "the title screen is still up");
			return true;
		}
		if ((mUserManager == NULL) || (mUserManager->GetCurrentUserName().empty()))
		{
			Error(aLine, "no player yet");
			return true;
		}
		if (anArg == "menu")
			ShowGameSelector();
		else if (anArg == "options")
			ShowOptions();
		else if (anArg == "help")
			ShowHelp();
		else if (anArg == "highscores")
			ShowHallOfFame();
		else if (anArg == "credits")
			ShowCredits();
		else if ((anArg == "classic") || (anArg == "action"))
		{
			if (mGameSelector == NULL)
				ShowGameSelector();
			SelectGameMode((anArg == "action") ? GAMEMODE_ACTION : GAMEMODE_CLASSIC);
		}
		else
		{
			Error(aLine, "no screen " + anArg);
			return true;
		}
		Ok(aLine);
		return true;
	}

	if (aName == "unlock")
	{
		Error(aLine, "nothing in the game is locked");
		return true;
	}

	if (aName == "seed")
	{
		int aSeed;
		if (!ParseInt(anArg, &aSeed))
		{
			Error(aLine, "seed <n>");
			return true;
		}
		CrtSRand((unsigned int) aSeed);
		MTRand aRand;
		SeedMTRand2004(aRand, (unsigned long) aSeed);
		uint32_t aState[MTRAND_N];
		int anIndex;
		aRand.GetState(aState, &anIndex);
		SetRandMTState(aState, anIndex);
		SeedMTRand2004(*gGameRand, (unsigned long) Sexy::Rand());
		Ok(aLine);
		return true;
	}

	if (aName == "set")
	{
		int anInt = 0;
		Board* b = mBoard;
		if (anArg == "mode")
		{
			if ((aValue != "classic") && (aValue != "action"))
				Error(aLine, "set mode classic|action");
			else if ((mBoard != NULL) && (mGameSelector == NULL))
				Error(aLine, "a game is in play");
			else
			{
				mGameMode = (aValue == "action") ? GAMEMODE_ACTION : GAMEMODE_CLASSIC;
				Ok(aLine);
			}
			return true;
		}
		if (!ParseInt(aValue, &anInt))
		{
			Error(aLine, "set <name> <number>");
			return true;
		}
		if ((b = PlayingBoard(this, aLine)) == NULL)
			return true;
		if (anArg == "score")
		{
			// the level's score follows, so the level bar and the level's end see it as a word's points
			int aPrevThreshold = (b->mLevel != 0) ? Board::LevelThreshold((unsigned int)b->mLevel - 1) : 0;
			b->mGameStats->mScore = anInt;
			b->mLevelStats->mScore = anInt - aPrevThreshold;
		}
		else if (anArg == "timer")
			b->mActionFireTimer = anInt;
		else if (anArg == "fire")
			b->mFireMeter = (double) anInt;
		else
		{
			Error(aLine, "no value " + anArg + " (score, timer, fire, mode)");
			return true;
		}
		b->MarkDirty();
		Ok(aLine);
		return true;
	}

	if (aName == "win")
	{
		Board* b = PlayingBoard(this, aLine);
		if (b == NULL)
			return true;
		// the level's threshold reached, as Submit does when a word reaches it
		int aPrevThreshold = (b->mLevel != 0) ? Board::LevelThreshold((unsigned int)b->mLevel - 1) : 0;
		int aThreshold = Board::LevelThreshold((unsigned int)b->mLevel);
		if (b->mGameStats->mScore < aThreshold)
		{
			b->mGameStats->mScore = aThreshold;
			b->mLevelStats->mScore = aThreshold - aPrevThreshold;
		}
		b->mLevelProgress = (double)b->mLevelStats->mScore / (double)(aThreshold - aPrevThreshold);
		b->mOverlay->mBalloon->Show(0);
		b->mState = BOARD_LEVEL_COMPLETE;
		b->mLevelUpPending = true;
		Ok(aLine);
		return true;
	}

	if (aName == "lose")
	{
		if (!anArg.empty())
		{
			int aWords;
			if (!ParseInt(anArg, &aWords) || (aWords < 1))
			{
				Error(aLine, "lose [words]");
				return true;
			}
			gLoseAfterWords = aWords;
			gWordsMoved = 0;
			Ok(aLine);
			return true;
		}
		Board* b = PlayingBoard(this, aLine);
		if (b == NULL)
			return true;
		LoseGame(this, b);
		Ok(aLine);
		return true;
	}

	if (aName == "level")
	{
		int aLevel;
		if (!ParseInt(anArg, &aLevel) || (aLevel < 1) || (aLevel > 42))
		{
			Error(aLine, "level <1..42>");
			return true;
		}
		Board* b = PlayingBoard(this, aLine);
		if (b == NULL)
			return true;
		if (aLevel <= b->mLevel + 1)
		{
			if (aLevel == b->mLevel + 1)
				Ok(aLine);
			else
				Error(aLine, "the game is past level " + anArg);
			return true;
		}
		// level aLevel - 1 just completed: its threshold scored, then the game's own NextLevel (LevelUp)
		b->mLevel = aLevel - 2;
		b->mGameStats->mScore = Board::LevelThreshold((unsigned int)b->mLevel);
		b->NextLevel();
		Ok(aLine);
		return true;
	}

	return SexyApp::HandleScriptCommand(theCommand);
}
