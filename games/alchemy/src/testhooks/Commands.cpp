// port: test hooks, the standard script commands (docs/TEST_COMMANDS.md), reachable only from a test (-script, and the live
// channel once the framework has it). A command that is never given changes nothing: no draw, no state. Every command
// answers with one line on stdout: OK <command>, ERROR <command>: <reason>, STATE {...} or MOVE {...}.
//
//   state                 STATE {json}: screen, dialog, mode, skill, level, score, forge, over, busy and the board
//   goto <screen>         title and credits are errors (the game has none to go to); menu (the selector), options, help,
//                         highscores (the all-time table), strategic|timetrial [skill 1..3], tutorial: starts that game
//                         through the selector's own button
//   level <n>             board n of the game being played: n - 1 boards cleared, an empty grid (a first board has its stone)
//   unlock all            ERROR: the game has nothing locked
//   set <name> <value>    score, forge (0..3), boards (boards cleared; the difficulty follows)
//   win                   every tile lit: the board is won through CheckWin (the bonus, the pieces vanish, the panel)
//   lose                  the forge overflows: the game's end through GameOver
//   seed <n>              the C library generator (the game's pieces come from the board's, seeded at a game's start from it)
//   move                  MOVE {json}: the bot's next input (Bot.cpp), what it is and the real input that makes it; MOVE null
//                         when there is none; changes nothing and draws no random number of the game's
//
// The `input` list of a MOVE is in game coordinates, in order: {"move":[x,y]} the pointer goes there (the piece hangs
// from it and the board takes the cell from the piece, so a hover comes before a click), {"wait":n} n updates,
// {"click":[x,y]} a left click ("button":"right" a right one) and {"key":"B"} a key press, named as the DOM names it.

#include "WinAlchApp.h"
#include "Autoplay.h"
#include "Board.h"
#include "Bot.h"
#include "Compat2004.h"
#include "ButtonWidget.h"
#include "Checkbox.h"
#include "Dialog.h"
#include "Dialogs.h"
#include "MemReport.h"
#include "HighScores.h"
#include "HyperlinkWidget.h"
#include "Piece.h"
#include "IntroScreen.h"
#include "TitleScreen.h"
#include "Wizard.h"
#include "WidgetManager.h"

#include <cstdio>
#include <cstdlib>
#include <list>
#include <sstream>
#include <string>

using namespace Sexy;

namespace
{

void Reply(const std::string& theLine)
{
	printf("%s\n", theLine.c_str());
	fflush(stdout);
}

void Ok(const std::string& theCommand)
{
	Reply("OK " + theCommand);
}

void Error(const std::string& theCommand, const std::string& theReason)
{
	Reply("ERROR " + theCommand + ": " + theReason);
}

std::string Num(int theValue)
{
	char aBuf[32];
	sprintf(aBuf, "%d", theValue);
	return aBuf;
}

std::string Real(double theValue)
{
	char aBuf[32];
	sprintf(aBuf, "%.3f", theValue);
	return aBuf;
}

std::string Bool(bool theValue)
{
	return theValue ? "true" : "false";
}

// the middle of a widget in game coordinates
void Middle(Widget* theWidget, int* theX, int* theY)
{
	Point aPos = theWidget->GetAbsPos();
	*theX = aPos.mX + theWidget->mWidth / 2;
	*theY = aPos.mY + theWidget->mHeight / 2;
}

std::string Pt(Widget* theWidget)
{
	int aX, aY;
	Middle(theWidget, &aX, &aY);
	return "[" + Num(aX) + "," + Num(aY) + "]";
}

// the game's dialog that is up (the first found, in the order of the table) and its name for `state`
Dialog* TopDialog(WinAlchApp* theApp, const char** theName)
{
	static const struct { int mId; const char* mName; } kDialogs[] =
	{
		{DIALOG_RECORD_SCORE, "name"}, {DIALOG_LOAD_GAME, "load"}, {DIALOG_HINT, "hint"}, {DIALOG_QUIT, "quit"},
		{DIALOG_NEW_GAME, "newgame"}, {DIALOG_TUTORIAL, "tutorial"}, {DIALOG_OPTIONS, "options"},
		{DIALOG_HIGH_SCORES, "alltime"}
	};
	for (size_t i = 0; i < sizeof(kDialogs) / sizeof(kDialogs[0]); i++)
	{
		Dialog* aDialog = theApp->GetDialog(kDialogs[i].mId);
		if (aDialog != NULL)
		{
			if (theName != NULL)
				*theName = kDialogs[i].mName;
			return aDialog;
		}
	}
	return NULL;
}

const char* DialogName(WinAlchApp* theApp)
{
	const char* aName = NULL;
	TopDialog(theApp, &aName);
	return aName;
}

const char* ScreenName(WinAlchApp* theApp)
{
	if (theApp->mTitleScreen != NULL)
		return "title";
	if (theApp->GetDialog(DIALOG_OPTIONS) != NULL)
		return "options";
	if (theApp->GetDialog(DIALOG_HIGH_SCORES) != NULL || theApp->mHighScores != NULL)
		return "highscores";
	if (theApp->mHelp != NULL)
		return "help";
	if (theApp->mIntroScreen != NULL)
		return "menu";
	if (theApp->mBoard != NULL)
		return "game";
	return "none";
}

const char* BoardStateName(int theState)
{
	static const char* const kNames[] = {"playing", "clearing", "ending", "gameover", "won", "cleared", "next"};
	return (theState >= 0 && theState <= 6) ? kNames[theState] : "?";
}

const char* ModeName(WinAlchApp* theApp)
{
	if (theApp->mBoard != NULL && theApp->mBoard->mTutorial)
		return "tutorial";
	if (theApp->mBoard != NULL)
		return theApp->mBoard->mTimeTrial ? "timetrial" : "strategic";
	return "none";
}

// the board can take a piece now
bool BoardReady(Board* theBoard)
{
	return theBoard != NULL && theBoard->mState == Board::STATE_PLAYING && theBoard->mCurPiece != NULL && theBoard->mPauseCount == 0 && theBoard->mCoverCount == 0 &&
		!(theBoard->mTutorial && theBoard->mTutorialWaiting);
}

std::string PieceJson(Piece* thePiece)
{
	if (thePiece == NULL)
		return "null";
	return std::string("{\"shape\":") + Num(thePiece->mShape) + ",\"color\":" + Num(thePiece->mColor) + ",\"kind\":\"" +
		(thePiece->mBomb ? "bomb" : thePiece->mSkull ? "stone" : "rune") + "\"}";
}

std::string BoardJson(WinAlchApp* theApp, Board* theBoard)
{
	std::string aJson = std::string("{\"state\":\"") + BoardStateName(theBoard->mState) + "\",\"pieces\":[";
	bool aFirst = true;
	for (int aRow = 0; aRow < Board::ROWS; aRow++)
		for (int aCol = 0; aCol < Board::COLS; aCol++)
		{
			Piece* aPiece = theBoard->mGrid[aRow][aCol];
			if (aPiece == NULL)
				continue;
			aJson += std::string(aFirst ? "" : ",") + "[" + Num(aCol) + "," + Num(aRow) + "," + Num(aPiece->mShape) + "," + Num(aPiece->mColor) +
				"," + (aPiece->mSkull ? "1" : "0") + "]";
			aFirst = false;
		}
	aJson += "],\"lit\":[";
	for (int aRow = 0; aRow < Board::ROWS; aRow++)
	{
		aJson += std::string(aRow ? "," : "") + "\"";
		for (int aCol = 0; aCol < Board::COLS; aCol++)
			aJson += theBoard->mLit[aRow][aCol] ? "1" : "0";
		aJson += "\"";
	}
	int aLitCount = 0;
	for (int aRow = 0; aRow < Board::ROWS; aRow++)
		for (int aCol = 0; aCol < Board::COLS; aCol++)
			if (theBoard->mLit[aRow][aCol])
				aLitCount++;
	aJson += "],\"litCount\":" + Num(aLitCount) + ",\"piece\":" + PieceJson(theBoard->mCurPiece) + ",\"fits\":" + Num(theBoard->mNumValid) +
		",\"moves\":" + Num(theBoard->mMoves) + ",\"forged\":" + Num(theBoard->mDiscards) + ",\"boards\":" + Num(theBoard->mBoardsCleared) +
		",\"shapes\":" + Num(theBoard->mNumShapes) + ",\"colors\":" + Num(theBoard->mNumColors) + ",\"paused\":" +
		Bool(theBoard->mPauseCount > 0 || theBoard->mCoverCount > 0) + ",\"cover\":" + Bool(theBoard->mCoverCount > 0);
	if (theBoard->mTutorial)
		aJson += ",\"tutorial\":{\"step\":" + Num(theBoard->mTutorialStep) + ",\"waiting\":" + Bool(theBoard->mTutorialWaiting) + ",\"count\":" +
			Num(theBoard->mTutorialCount) + ",\"button\":" + (theBoard->mButton != NULL ? Pt(theBoard->mButton) : std::string("null")) + "}";
	if (theBoard->mButton != NULL && !theBoard->mTutorial)
		aJson += ",\"panelButton\":" + Pt(theBoard->mButton);
	return aJson + "}";
}

std::string StateJson(WinAlchApp* theApp)
{
	Board* aBoard = theApp->mBoard;
	std::string aJson = std::string("{\"screen\":\"") + ScreenName(theApp) + "\"";
	if (const char* aDialog = DialogName(theApp))
		aJson += std::string(",\"dialog\":\"") + aDialog + "\"";
	if (Dialog* aTop = TopDialog(theApp, NULL))
	{
		// the buttons of that dialog, for a driver that clicks them
		aJson += ",\"buttons\":{";
		bool aFirst = true;
		if (aTop->mYesButton != NULL)
		{
			aJson += "\"yes\":" + Pt(aTop->mYesButton);
			aFirst = false;
		}
		if (aTop->mNoButton != NULL)
			aJson += std::string(aFirst ? "" : ",") + "\"no\":" + Pt(aTop->mNoButton);
		aJson += "}";
	}
	aJson += ",\"updates\":" + Num(theApp->mUpdateCount);
	aJson += std::string(",\"mode\":\"") + ModeName(theApp) + "\"";
	aJson += ",\"skill\":" + Num(theApp->mSkillLevel);
	if (aBoard != NULL)
	{
		aJson += ",\"level\":" + Num(aBoard->mBoardsCleared + 1) + ",\"score\":" + Num(aBoard->mScore) + ",\"forge\":" + Real(aBoard->mProgress);
		bool isOver = aBoard->mState == Board::STATE_ENDING || aBoard->mState == Board::STATE_GAME_OVER;
		aJson += ",\"over\":" + Bool(isOver);
		aJson += ",\"busy\":" + Bool(!BoardReady(aBoard) || !theApp->mDialogMap.empty() || theApp->mHelp != NULL || theApp->mHighScores != NULL);
		aJson += ",\"board\":" + BoardJson(theApp, aBoard);
	}
	if (theApp->mTitleScreen != NULL)
	{
		aJson += ",\"ready\":" + Bool(theApp->mTitleScreen->mDone);
		if (theApp->mTitleScreen->mDone)
			aJson += ",\"link\":" + Pt(theApp->mTitleScreen->mPlayLink);
	}
	aJson += ",\"active\":" + Bool(theApp->mActive) + ",\"windowed\":" + Bool(theApp->mIsWindowed) + ",\"muted\":" + Bool(theApp->mMuteCount > 0);
	if (theApp->mTitleScreen == NULL)
	{
		// the side panel's buttons, for a driver that clicks them
		aJson += ",\"panel\":{";
		bool aFirst = true;
		for (std::list<ButtonWidget*>::iterator anItr = theApp->mButtons.begin(); anItr != theApp->mButtons.end(); ++anItr)
		{
			int anId = (*anItr)->mId;
			const char* aButton = anId == BUTTON_NEW_GAME ? "new" : anId == BUTTON_OPTIONS ? "options" : anId == BUTTON_FORGE ? "forge" :
				anId == BUTTON_QUIT ? "quit" : anId == BUTTON_HINT ? "hint" : NULL;
			if (aButton == NULL)
				continue;
			aJson += std::string(aFirst ? "" : ",") + "\"" + aButton + "\":" + Pt(*anItr);
			aFirst = false;
		}
		if (theApp->mSoundButton != NULL)
			aJson += std::string(aFirst ? "" : ",") + "\"sound\":" + Pt(theApp->mSoundButton);
		aJson += "}";
	}
	if (OptionsDialog* anOptions = (OptionsDialog*) theApp->GetDialog(DIALOG_OPTIONS))
		aJson += ",\"options\":{\"fullscreen\":" + Pt(anOptions->mFullscreenBox) + ",\"cursors\":" + Pt(anOptions->mCursorsBox) +
			",\"done\":" + Pt(anOptions->mYesButton) + ",\"scores\":" + Pt(anOptions->mScoresButton) + ",\"help\":" +
			Pt(anOptions->mHelpButton) + "}";
	if (IntroScreen* anIntroScreen = theApp->mIntroScreen)
	{
		aJson += ",\"selector\":{\"skillChosen\":" + Num(anIntroScreen->mSkill) + ",\"timeTrialChosen\":" + Bool(anIntroScreen->mTimeTrial) +
			",\"skill1\":" + Pt(anIntroScreen->mSkillButtons[0]) + ",\"skill2\":" + Pt(anIntroScreen->mSkillButtons[1]) + ",\"skill3\":" +
			Pt(anIntroScreen->mSkillButtons[2]) + ",\"strategic\":" + Pt(anIntroScreen->mStrategicButton) + ",\"timetrial\":" + Pt(anIntroScreen->mTimeTrialButton) +
			",\"start\":" + Pt(anIntroScreen->mStartButton) + ",\"tutorial\":" + Pt(anIntroScreen->mTutorialPicButton) + "}";
	}
	if (HighScoreWidget* aHighScores = theApp->mHighScores)
		aJson += ",\"table\":{\"revealed\":" + Bool(aHighScores->mReveal >= aHighScores->mHeight) + ",\"start\":" + Pt(aHighScores->mStartButton) + "}";
	return aJson + "}";
}

// the pointer's hover over a cell, then a click: the input that puts the piece in a cell
std::string PlaceInput(int theCol, int theRow)
{
	int aX, aY;
	BotCellToPointer(theCol, theRow, &aX, &aY);
	return "[{\"move\":[" + Num(aX) + "," + Num(aY) + "]},{\"wait\":2},{\"click\":[" + Num(aX) + "," + Num(aY) + "]}]";
}

std::string ClickInput(Widget* theWidget)
{
	return "[{\"move\":" + Pt(theWidget) + "},{\"wait\":1},{\"click\":" + Pt(theWidget) + "}]";
}

std::string MoveJson(WinAlchApp* theApp)
{
	// the loading screen's link
	if (theApp->mTitleScreen != NULL)
	{
		if (!theApp->mTitleScreen->mDone)
			return "null";
		return "{\"kind\":\"title\",\"input\":" + ClickInput(theApp->mTitleScreen->mPlayLink) + "}";
	}

	// the dialogs the bot answers: a wizard's message, the saved game (yes), the name for the table
	if (Dialog* aHint = theApp->GetDialog(DIALOG_HINT))
		return "{\"kind\":\"dialog\",\"dialog\":\"hint\",\"input\":" + ClickInput(aHint->mYesButton) + "}";
	if (Dialog* aLoad = theApp->GetDialog(DIALOG_LOAD_GAME))
		return "{\"kind\":\"dialog\",\"dialog\":\"load\",\"input\":" + ClickInput(aLoad->mYesButton) + "}";
	if (Dialog* aName = theApp->GetDialog(DIALOG_RECORD_SCORE))
	{
		int aX, aY;
		Middle(aName->mYesButton, &aX, &aY);
		return "{\"kind\":\"name\",\"input\":[{\"key\":\"b\"},{\"key\":\"o\"},{\"key\":\"t\"},{\"move\":[" + Num(aX) + "," + Num(aY) +
			"]},{\"wait\":1},{\"click\":[" + Num(aX) + "," + Num(aY) + "]}]}";
	}
	if (!theApp->mDialogMap.empty())
		return "null";

	// the score table after a game
	if (HighScoreWidget* aHighScores = theApp->mHighScores)
	{
		if (aHighScores->mReveal < aHighScores->mHeight)
			return "null";
		return "{\"kind\":\"scores\",\"input\":" + ClickInput(aHighScores->mStartButton) + "}";
	}
	if (theApp->mHelp != NULL || theApp->mIntroScreen != NULL)
		return "null";

	Board* aBoard = theApp->mBoard;
	if (aBoard == NULL)
		return "null";

	// the board
	if (aBoard->mTutorial && aBoard->mState == Board::STATE_PLAYING && aBoard->mPauseCount == 0 && aBoard->mCoverCount == 0 &&
		(aBoard->mTutorialWaiting || aBoard->mCurPiece == NULL) && aBoard->mButton != NULL)
		return "{\"kind\":\"tutorial\",\"step\":" + Num(aBoard->mTutorialStep) + ",\"input\":" + ClickInput(aBoard->mButton) + "}";
	if (aBoard->mState == Board::STATE_BOARD_CLEARED)
	{
		// the panel's button, once the wizard has come in (the level complete sound plays only then)
		if (aBoard->mButton == NULL || (aBoard->mWizardShown && aBoard->mWizard->mAlpha < 1.0))
			return "null";
		return "{\"kind\":\"cleared\",\"input\":" + ClickInput(aBoard->mButton) + "}";
	}
	if (!BoardReady(aBoard))
		return "null";

	// the bot's dice start from the position, so the same position gives the same move and nothing is drawn from the game's
	unsigned int aSeed = (unsigned int) (aBoard->mMoves * 131 + aBoard->mDiscards * 17 + aBoard->mScore * 7 + aBoard->mBoardsCleared * 3 + aBoard->mNumValid);
	BotChoice aChoice = ChooseBotMove(aBoard, &aSeed, 0);
	if (aChoice.mKind == BotChoice::KIND_FORGE)
		return std::string("{\"kind\":\"forge\",\"input\":[{\"move\":[320,240]},{\"wait\":1},{\"click\":[320,240],\"button\":\"right\"}]}");
	if (aChoice.mKind == BotChoice::KIND_PLACE)
		return "{\"kind\":\"place\",\"cell\":[" + Num(aChoice.mCol) + "," + Num(aChoice.mRow) + "],\"piece\":" + PieceJson(aBoard->mCurPiece) +
			",\"input\":" + PlaceInput(aChoice.mCol, aChoice.mRow) + "}";
	return "null";
}

}

bool WinAlchApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	const std::string& aName = theCommand.mCommand;
	std::istringstream anArgs(theCommand.mArgs);
	std::string anArg;
	anArgs >> anArg;
	std::string aLine = aName + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs);

	if (aName == "state")
	{
		Reply("STATE " + StateJson(this));
		return true;
	}
	if (aName == "goto")
	{
		std::string aSkillArg;
		anArgs >> aSkillArg;
		if (mTitleScreen != NULL)
			Error(aLine, "the title screen is still up");
		else if (anArg == "title" || anArg == "credits")
			Error(aLine, "the game has no " + anArg + " screen to go to");
		else if (anArg == "menu")
		{
			if (mIntroScreen == NULL)
				ShowIntroScreen();
			Ok(aLine);
		}
		else if (anArg == "options")
		{
			if (GetDialog(DIALOG_OPTIONS) == NULL)
				DoOptions();
			Ok(aLine);
		}
		else if (anArg == "help")
		{
			DoHelp();
			Ok(aLine);
		}
		else if (anArg == "highscores")
		{
			if (GetDialog(DIALOG_HIGH_SCORES) == NULL)
				DoHighScoreDialog();
			Ok(aLine);
		}
		else if (anArg == "strategic" || anArg == "timetrial" || anArg == "tutorial")
		{
			int aSkill = aSkillArg.empty() ? 0 : atoi(aSkillArg.c_str());
			if (!aSkillArg.empty() && (aSkill < 1 || aSkill > 3))
				Error(aLine, "the skill is 1 (easy), 2 (average) or 3 (hard)");
			else
			{
				if (mIntroScreen == NULL)
					ShowIntroScreen();
				// the selector's own buttons: the skill, the mode, then the start (or the tutorial) button
				if (anArg == "tutorial")
					mIntroScreen->ButtonDepress(1);
				else
				{
					if (aSkill != 0)
						mIntroScreen->SetSkillLevel(aSkill);
					mIntroScreen->SetTimeTrial(anArg == "timetrial");
					mIntroScreen->ButtonDepress(0);
				}
				Ok(aLine);
			}
		}
		else
			Error(aLine, "unknown screen '" + anArg + "'");
		return true;
	}
	if (aName == "level")
	{
		int aLevel = atoi(anArg.c_str());
		Board* aBoard = mBoard;
		if (aBoard == NULL || aBoard->mTutorial || mHighScores != NULL || aBoard->mState != Board::STATE_PLAYING)
			Error(aLine, "no board is in play");
		else if (aLevel < 1)
			Error(aLine, "the level must be 1 or more");
		else
		{
			// as a board is started: an empty grid with no tile lit, the difficulty of the boards cleared before it
			aBoard->ResetBoard();
			aBoard->mBoardsCleared = aLevel - 1;
			aBoard->SetBoardVars();
			aBoard->mProgress = 0;
			aBoard->mRun = 0;
			aBoard->mHintOn = false;
			aBoard->mHintTimer = 0;
			aBoard->mLightOn = false;
			if (aLevel == 1)
			{
				// a game's first board has the stone in the middle
				Piece* aStone = new Piece(this);
				aStone->mCol = Board::FIRST_COL;
				aStone->mRow = Board::FIRST_ROW;
				aStone->mX = Board::GetColX(Board::FIRST_COL) + aBoard->mX;
				aStone->mY = Board::GetRowY(Board::FIRST_ROW) + aBoard->mY;
				aStone->mSkull = true;
				aBoard->mGrid[Board::FIRST_ROW][Board::FIRST_COL] = aStone;
				aBoard->mLit[Board::FIRST_ROW][Board::FIRST_COL] = true;
			}
			aBoard->RedrawBackground();
			if (aBoard->mCurPiece != NULL)
				aBoard->NextMove();
			aBoard->MarkDirty();
			Ok(aLine);
		}
		return true;
	}
	if (aName == "unlock")
	{
		Error(aLine, "the game has nothing locked");
		return true;
	}
	if (aName == "set")
	{
		std::string aValue;
		anArgs >> aValue;
		if (mBoard == NULL || mBoard->mTutorial)
			Error(aLine, "no game is being played");
		else if (aValue.empty())
			Error(aLine, "set <name> <value>");
		else if (anArg == "score")
		{
			mBoard->mScore = atoi(aValue.c_str());
			mBoard->MarkDirty();
			Ok(aLine);
		}
		else if (anArg == "forge")
		{
			mBoard->mProgress = atof(aValue.c_str());
			mBoard->MarkDirty();
			Ok(aLine);
		}
		else if (anArg == "boards")
		{
			mBoard->mBoardsCleared = atoi(aValue.c_str());
			mBoard->SetBoardVars();
			Ok(aLine);
		}
		else
			Error(aLine, "the game's state keys are score, forge and boards");
		return true;
	}
	if (aName == "win")
	{
		Board* aBoard = mBoard;
		if (aBoard == NULL || aBoard->mTutorial || aBoard->mState != Board::STATE_PLAYING || aBoard->mCurPiece == NULL)
			Error(aLine, "no board is in play");
		else
		{
			// every tile used: the board's own check awards the bonus and wins it
			for (int aRow = 0; aRow < Board::ROWS; aRow++)
				for (int aCol = 0; aCol < Board::COLS; aCol++)
					aBoard->mLit[aRow][aCol] = true;
			aBoard->RedrawBackground();
			aBoard->CheckWin();
			Ok(aLine);
		}
		return true;
	}
	if (aName == "lose")
	{
		Board* aBoard = mBoard;
		if (aBoard == NULL || aBoard->mTutorial || aBoard->mState != Board::STATE_PLAYING)
			Error(aLine, "no game is in play");
		else
		{
			aBoard->EncodeDiscard(true);
			aBoard->mProgress = 3.0;
			aBoard->GameOver();
			Ok(aLine);
		}
		return true;
	}
	if (aName == "seed")
	{
		if (anArg.empty())
			Error(aLine, "seed <n>");
		else
		{
			CrtSRand((unsigned int) atoi(anArg.c_str()));
			Ok(aLine);
		}
		return true;
	}
	// memreport <file>: what the game holds in memory by owner (testhooks/MemReport.h)
	if (aName == "memreport")
	{
		if (theCommand.mArgs.empty())
			Error(aLine, "memreport <file>");
		else if (!WriteMemReport(this, theCommand.mArgs))
			Error(aLine, "cannot write " + theCommand.mArgs);
		else
			Ok(aLine);
		return true;
	}
	if (aName == "move")
	{
		std::string aMove = MoveJson(this);
		Reply(aMove == "null" ? "MOVE null" : "MOVE " + aMove);
		return true;
	}
	return SexyApp::HandleScriptCommand(theCommand);
}
