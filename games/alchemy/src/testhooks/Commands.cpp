// Test hooks: the standard script commands (docs/TEST_COMMANDS.md), reachable only from a test (-script, and the live
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
//   win                   every tile lit: the board is won through CheckAllLit (the bonus, the pieces vanish, the panel)
//   lose                  the forge overflows: the game's end through GameOver
//   seed <n>              the C library generator (the game's pieces come from the board's, seeded at a game's start from it)
//   move                  MOVE {json}: the bot's next input (Bot.cpp), what it is and the real input that makes it; MOVE null
//                         when there is none; changes nothing and draws no random number of the game's
//
// The `input` list of a MOVE is in game coordinates, in order: {"move":[x,y]} the pointer goes there (the piece hangs
// from it and the board takes the cell from the piece, so a hover comes before a click), {"wait":n} n updates,
// {"click":[x,y]} a left click ("button":"right" a right one) and {"key":"B"} a key press, named as the DOM names it.

#include "AlchemyApp.h"
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
#include "Selector.h"
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
Dialog* TopDialog(AlchemyApp* theApp, const char** theName)
{
	static const struct { int mId; const char* mName; } kDialogs[] =
	{
		{DIALOG_NAME_ENTRY, "name"}, {DIALOG_LOAD_GAME, "load"}, {DIALOG_HINT, "hint"}, {DIALOG_QUIT, "quit"},
		{DIALOG_NEW_GAME, "newgame"}, {DIALOG_SKILL, "skill"}, {DIALOG_TUTORIAL, "tutorial"}, {DIALOG_OPTIONS, "options"},
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

const char* DialogName(AlchemyApp* theApp)
{
	const char* aName = NULL;
	TopDialog(theApp, &aName);
	return aName;
}

const char* ScreenName(AlchemyApp* theApp)
{
	if (theApp->mTitleScreen != NULL)
		return "title";
	if (theApp->GetDialog(DIALOG_OPTIONS) != NULL)
		return "options";
	if (theApp->GetDialog(DIALOG_HIGH_SCORES) != NULL || theApp->mHighScores != NULL)
		return "highscores";
	if (theApp->mHelp != NULL)
		return "help";
	if (theApp->mSelector != NULL)
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

const char* ModeName(AlchemyApp* theApp)
{
	if (theApp->mBoard != NULL && theApp->mBoard->mTutorial)
		return "tutorial";
	if (theApp->mBoard != NULL)
		return theApp->mBoard->mTimeTrial ? "timetrial" : "strategic";
	return "none";
}

// the board can take a piece now
bool BoardReady(Board* b)
{
	return b != NULL && b->mState == Board::STATE_PLAYING && b->mCurPiece != NULL && b->mPause == 0 && b->mTrialPause == 0 &&
		!(b->mTutorial && b->mTutWaiting);
}

std::string PieceJson(Piece* p)
{
	if (p == NULL)
		return "null";
	return std::string("{\"shape\":") + Num(p->mShape) + ",\"color\":" + Num(p->mColor) + ",\"kind\":\"" +
		(p->mBomb ? "bomb" : p->mSkull ? "stone" : "rune") + "\"}";
}

std::string BoardJson(AlchemyApp* theApp, Board* b)
{
	std::string s = std::string("{\"state\":\"") + BoardStateName(b->mState) + "\",\"pieces\":[";
	bool aFirst = true;
	for (int aRow = 0; aRow < Board::ROWS; aRow++)
		for (int aCol = 0; aCol < Board::COLS; aCol++)
		{
			Piece* p = b->mGrid[aRow][aCol];
			if (p == NULL)
				continue;
			s += std::string(aFirst ? "" : ",") + "[" + Num(aCol) + "," + Num(aRow) + "," + Num(p->mShape) + "," + Num(p->mColor) +
				"," + (p->mSkull ? "1" : "0") + "]";
			aFirst = false;
		}
	s += "],\"lit\":[";
	for (int aRow = 0; aRow < Board::ROWS; aRow++)
	{
		s += std::string(aRow ? "," : "") + "\"";
		for (int aCol = 0; aCol < Board::COLS; aCol++)
			s += b->mLit[aRow][aCol] ? "1" : "0";
		s += "\"";
	}
	int aLitCount = 0;
	for (int aRow = 0; aRow < Board::ROWS; aRow++)
		for (int aCol = 0; aCol < Board::COLS; aCol++)
			if (b->mLit[aRow][aCol])
				aLitCount++;
	s += "],\"litCount\":" + Num(aLitCount) + ",\"piece\":" + PieceJson(b->mCurPiece) + ",\"fits\":" + Num(b->mNumValid) +
		",\"moves\":" + Num(b->mMoves) + ",\"forged\":" + Num(b->mDiscards) + ",\"boards\":" + Num(b->mBoardsCleared) +
		",\"shapes\":" + Num(b->mNumShapes) + ",\"colors\":" + Num(b->mNumColors) + ",\"paused\":" +
		Bool(b->mPause > 0 || b->mTrialPause > 0) + ",\"cover\":" + Bool(b->mTrialPause > 0);
	if (b->mTutorial)
		s += ",\"tutorial\":{\"step\":" + Num(b->mTutStep) + ",\"waiting\":" + Bool(b->mTutWaiting) + ",\"count\":" +
			Num(b->mTutCount) + ",\"button\":" + (b->mButton != NULL ? Pt(b->mButton) : std::string("null")) + "}";
	if (b->mButton != NULL && !b->mTutorial)
		s += ",\"panelButton\":" + Pt(b->mButton);
	return s + "}";
}

std::string StateJson(AlchemyApp* theApp)
{
	Board* b = theApp->mBoard;
	std::string s = std::string("{\"screen\":\"") + ScreenName(theApp) + "\"";
	if (const char* aDialog = DialogName(theApp))
		s += std::string(",\"dialog\":\"") + aDialog + "\"";
	if (Dialog* aTop = TopDialog(theApp, NULL))
	{
		// the buttons of that dialog, for a driver that clicks them
		s += ",\"buttons\":{";
		bool aFirst = true;
		if (aTop->mYesButton != NULL)
		{
			s += "\"yes\":" + Pt(aTop->mYesButton);
			aFirst = false;
		}
		if (aTop->mNoButton != NULL)
			s += std::string(aFirst ? "" : ",") + "\"no\":" + Pt(aTop->mNoButton);
		s += "}";
	}
	s += ",\"updates\":" + Num(theApp->mUpdateCount);
	s += std::string(",\"mode\":\"") + ModeName(theApp) + "\"";
	s += ",\"skill\":" + Num(theApp->mSkillLevel);
	if (b != NULL)
	{
		s += ",\"level\":" + Num(b->mBoardsCleared + 1) + ",\"score\":" + Num(b->mScore) + ",\"forge\":" + Real(b->mProgress);
		bool isOver = b->mState == Board::STATE_ENDING || b->mState == Board::STATE_GAME_OVER;
		s += ",\"over\":" + Bool(isOver);
		s += ",\"busy\":" + Bool(!BoardReady(b) || !theApp->mDialogMap.empty() || theApp->mHelp != NULL || theApp->mHighScores != NULL);
		s += ",\"board\":" + BoardJson(theApp, b);
	}
	if (theApp->mTitleScreen != NULL)
	{
		s += ",\"ready\":" + Bool(theApp->mTitleScreen->mDone);
		if (theApp->mTitleScreen->mDone)
			s += ",\"link\":" + Pt(theApp->mTitleScreen->mPlayLink);
	}
	s += ",\"active\":" + Bool(theApp->mActive) + ",\"windowed\":" + Bool(theApp->mIsWindowed) + ",\"muted\":" + Bool(theApp->mMuteCount > 0);
	if (theApp->mTitleScreen == NULL)
	{
		// the side panel's buttons, for a driver that clicks them
		s += ",\"panel\":{";
		bool aFirst = true;
		for (std::list<ButtonWidget*>::iterator anItr = theApp->mButtons.begin(); anItr != theApp->mButtons.end(); ++anItr)
		{
			int anId = (*anItr)->mId;
			const char* aButton = anId == BUTTON_NEW_GAME ? "new" : anId == BUTTON_OPTIONS ? "options" : anId == BUTTON_FORGE ? "forge" :
				anId == BUTTON_QUIT ? "quit" : anId == BUTTON_HINT ? "hint" : NULL;
			if (aButton == NULL)
				continue;
			s += std::string(aFirst ? "" : ",") + "\"" + aButton + "\":" + Pt(*anItr);
			aFirst = false;
		}
		if (theApp->mSoundButton != NULL)
			s += std::string(aFirst ? "" : ",") + "\"sound\":" + Pt(theApp->mSoundButton);
		s += "}";
	}
	if (OptionsDialog* anOptions = (OptionsDialog*) theApp->GetDialog(DIALOG_OPTIONS))
		s += ",\"options\":{\"fullscreen\":" + Pt(anOptions->mFullscreenBox) + ",\"cursors\":" + Pt(anOptions->mCursorsBox) +
			",\"done\":" + Pt(anOptions->mYesButton) + ",\"scores\":" + Pt(anOptions->mScoresButton) + ",\"help\":" +
			Pt(anOptions->mHelpButton) + "}";
	if (SelectorScreen* sel = theApp->mSelector)
	{
		s += ",\"selector\":{\"skillChosen\":" + Num(sel->mSkill) + ",\"timeTrialChosen\":" + Bool(sel->mTimeTrial) +
			",\"skill1\":" + Pt(sel->mSkillButtons[0]) + ",\"skill2\":" + Pt(sel->mSkillButtons[1]) + ",\"skill3\":" +
			Pt(sel->mSkillButtons[2]) + ",\"strategic\":" + Pt(sel->mStrategicButton) + ",\"timetrial\":" + Pt(sel->mTimeTrialButton) +
			",\"start\":" + Pt(sel->mStartButton) + ",\"tutorial\":" + Pt(sel->mTutorialPicButton) + "}";
	}
	if (HighScoreWidget* hs = theApp->mHighScores)
		s += ",\"table\":{\"revealed\":" + Bool(hs->mReveal >= hs->mHeight) + ",\"start\":" + Pt(hs->mStartButton) + "}";
	return s + "}";
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

std::string MoveJson(AlchemyApp* theApp)
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
	if (Dialog* aName = theApp->GetDialog(DIALOG_NAME_ENTRY))
	{
		int aX, aY;
		Middle(aName->mYesButton, &aX, &aY);
		return "{\"kind\":\"name\",\"input\":[{\"key\":\"b\"},{\"key\":\"o\"},{\"key\":\"t\"},{\"move\":[" + Num(aX) + "," + Num(aY) +
			"]},{\"wait\":1},{\"click\":[" + Num(aX) + "," + Num(aY) + "]}]}";
	}
	if (!theApp->mDialogMap.empty())
		return "null";

	// the score table after a game
	if (HighScoreWidget* hs = theApp->mHighScores)
	{
		if (hs->mReveal < hs->mHeight)
			return "null";
		return "{\"kind\":\"scores\",\"input\":" + ClickInput(hs->mStartButton) + "}";
	}
	if (theApp->mHelp != NULL || theApp->mSelector != NULL)
		return "null";

	Board* b = theApp->mBoard;
	if (b == NULL)
		return "null";

	// the board
	if (b->mTutorial && b->mState == Board::STATE_PLAYING && b->mPause == 0 && b->mTrialPause == 0 &&
		(b->mTutWaiting || b->mCurPiece == NULL) && b->mButton != NULL)
		return "{\"kind\":\"tutorial\",\"step\":" + Num(b->mTutStep) + ",\"input\":" + ClickInput(b->mButton) + "}";
	if (b->mState == Board::STATE_BOARD_CLEARED)
	{
		// the panel's button, once the wizard has come in (the level complete sound plays only then)
		if (b->mButton == NULL || (b->mHelpShown && b->mHelpBox->mAlpha < 1.0))
			return "null";
		return "{\"kind\":\"cleared\",\"input\":" + ClickInput(b->mButton) + "}";
	}
	if (!BoardReady(b))
		return "null";

	// the bot's dice start from the position, so the same position gives the same move and nothing is drawn from the game's
	unsigned int aSeed = (unsigned int) (b->mMoves * 131 + b->mDiscards * 17 + b->mScore * 7 + b->mBoardsCleared * 3 + b->mNumValid);
	BotChoice aChoice = ChooseBotMove(b, &aSeed, 0);
	if (aChoice.mKind == BotChoice::KIND_FORGE)
		return std::string("{\"kind\":\"forge\",\"input\":[{\"move\":[320,240]},{\"wait\":1},{\"click\":[320,240],\"button\":\"right\"}]}");
	if (aChoice.mKind == BotChoice::KIND_PLACE)
		return "{\"kind\":\"place\",\"cell\":[" + Num(aChoice.mCol) + "," + Num(aChoice.mRow) + "],\"piece\":" + PieceJson(b->mCurPiece) +
			",\"input\":" + PlaceInput(aChoice.mCol, aChoice.mRow) + "}";
	return "null";
}

}

bool AlchemyApp::HandleScriptCommand(const ScriptCommand& theCommand)
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
			if (mSelector == NULL)
				ShowSelector();
			Ok(aLine);
		}
		else if (anArg == "options")
		{
			if (GetDialog(DIALOG_OPTIONS) == NULL)
				ShowOptions();
			Ok(aLine);
		}
		else if (anArg == "help")
		{
			ShowHelp();
			Ok(aLine);
		}
		else if (anArg == "highscores")
		{
			if (GetDialog(DIALOG_HIGH_SCORES) == NULL)
				ShowAllTimeScores();
			Ok(aLine);
		}
		else if (anArg == "strategic" || anArg == "timetrial" || anArg == "tutorial")
		{
			int aSkill = aSkillArg.empty() ? 0 : atoi(aSkillArg.c_str());
			if (!aSkillArg.empty() && (aSkill < 1 || aSkill > 3))
				Error(aLine, "the skill is 1 (easy), 2 (average) or 3 (hard)");
			else
			{
				if (mSelector == NULL)
					ShowSelector();
				// the selector's own buttons: the skill, the mode, then the start (or the tutorial) button
				if (anArg == "tutorial")
					mSelector->ButtonDepress(1);
				else
				{
					if (aSkill != 0)
						mSelector->SetSkill(aSkill);
					mSelector->SetTimeTrial(anArg == "timetrial");
					mSelector->ButtonDepress(0);
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
		Board* b = mBoard;
		if (b == NULL || b->mTutorial || mHighScores != NULL || b->mState != Board::STATE_PLAYING)
			Error(aLine, "no board is in play");
		else if (aLevel < 1)
			Error(aLine, "the level must be 1 or more");
		else
		{
			// as a board is started: an empty grid with no tile lit, the difficulty of the boards cleared before it
			b->ClearGrid();
			b->mBoardsCleared = aLevel - 1;
			b->SetDifficulty();
			b->mProgress = 0;
			b->mRun = 0;
			b->mHintOn = false;
			b->mHintTimer = 0;
			b->mLightOn = false;
			if (aLevel == 1)
			{
				// a game's first board has the stone in the middle
				Piece* aStone = new Piece(this);
				aStone->mCol = Board::FIRST_COL;
				aStone->mRow = Board::FIRST_ROW;
				aStone->mX = Board::ColToX(Board::FIRST_COL) + b->mX;
				aStone->mY = Board::RowToY(Board::FIRST_ROW) + b->mY;
				aStone->mSkull = true;
				b->mGrid[Board::FIRST_ROW][Board::FIRST_COL] = aStone;
				b->mLit[Board::FIRST_ROW][Board::FIRST_COL] = true;
			}
			b->RedrawAllCells();
			if (b->mCurPiece != NULL)
				b->NewCurrentPiece();
			b->MarkDirty();
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
			mBoard->SetDifficulty();
			Ok(aLine);
		}
		else
			Error(aLine, "the game's state keys are score, forge and boards");
		return true;
	}
	if (aName == "win")
	{
		Board* b = mBoard;
		if (b == NULL || b->mTutorial || b->mState != Board::STATE_PLAYING || b->mCurPiece == NULL)
			Error(aLine, "no board is in play");
		else
		{
			// every tile used: the board's own check awards the bonus and wins it
			for (int aRow = 0; aRow < Board::ROWS; aRow++)
				for (int aCol = 0; aCol < Board::COLS; aCol++)
					b->mLit[aRow][aCol] = true;
			b->RedrawAllCells();
			b->CheckAllLit();
			Ok(aLine);
		}
		return true;
	}
	if (aName == "lose")
	{
		Board* b = mBoard;
		if (b == NULL || b->mTutorial || b->mState != Board::STATE_PLAYING)
			Error(aLine, "no game is in play");
		else
		{
			b->RecordForge(true);
			b->mProgress = 3.0;
			b->GameOver();
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
	// memreport <file>: what the game holds in memory by owner (testhooks/MemReport.h), for the performance pass
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
