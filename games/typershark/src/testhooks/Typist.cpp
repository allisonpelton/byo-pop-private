// port: test hook -typist=<file>[;every=K][;start=U][;stop=U][;typo=N][;enter=U] (phase 1 of the identical-play pass,
// games/typershark/tools/phase1): the game's own bot (Bot.cpp) plays by script lines and writes down every line it plays, so the
// same inputs can be sent to the original at the same updates. Each line is `<update> char <c>` or `<update> click <x> <y>`,
// as -script takes them (the framework's script path: the line of update N runs before update N).
//   every=K   a letter every K updates (default 1)       start=U   no input before update U
//   stop=U    no input from update U on (an idle player)  typo=N    every Nth letter typed is a wrong one
//   burst=B   (P1b) with typo=N, the last B letters of every N are wrong (B wrong letters in a row; a penalty piranha needs NumForPenalty of them)
//   enter=U   press Enter (the Shark Zapper) once at update U
//   want=a+b  (P1b) at a fork of the map take the chest of the first site in this list that is on offer (default: the first chest)
//   streak=N  (P1b) type at most N words of the streak game at the end of each level
//   wait=N    (P1b) the space bar that leaves a won level comes N updates after the level is won (default at once)
//   tutorerr=N  (P1b) in the typing tutor every Nth key is a wrong one (a space, Return, a letter or a symbol in turn)
//   win=yes|no  (P1b) the button of the "CONGRATULATIONS!" dialog at the end of the adventure to click (yes: return home, no: keep going)
//   over=yes|no (P1b) the button of the game-over dialog to click (yes: play again, no: the other one)
//   confirm=yes|no  (P1b) the button of a two-button message dialog (CONTINUE? and the like) to click
//   clams=mixed  (P1b) in a bonus level type the clams' letters by a fixed pseudo-random pattern (pearls, empty clams, wrong
//             letters, clams that are open) instead of the bot's ideal play
// The bot reads the board after an update and its input goes in at the next one. Nothing here runs without the option.

#include "TestHooks.h"
#include "WinWSApp.h"
#include "Board.h"
#include "Player.h"
#include "Clam.h"
#include "MapScreen.h"
#include "XWidget.h"
#include "Dialog.h"
#include "DialogButton.h"
#include "TypingTutor.h"
#include "Common.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace Sexy;

namespace
{

struct TypistState
{
	int			mMode;			// -1 not looked at yet, 0 off, 1 on
	FILE*		mFile;
	int			mEvery;
	int			mStart;
	int			mStop;
	int			mTypoEvery;
	int			mTypoBurst;		// wrong letters in a row (default 1)
	int			mEnterAt;
	int			mTypedCount;
	bool		mEnterDone;
	int			mPromptUpdate;	// the update of the last prompt click: the same dialog is not clicked twice in a row
	int			mTutorCorrect;	// the typing tutor's count of right keys at the last key, and the keys since that counted for nothing
	int			mTutorStuck;
	std::vector<int> mWant;		// the sites the map route goes by (P1b)
	int			mStreakMax;		// words of the streak game typed at most in one level, -1: no limit
	int			mStreakWords;	// words begun in this streak game
	bool		mClamsMixed;
	int			mWinButton;		// -1 none, 0 the dialog's yes button, 1 its no button (the end of the adventure)
	int			mOverButton;	// the same for the game-over dialog
	int			mConfirmButton;	// and for a question with two buttons
	int			mTutorErrEvery;	// every Nth key of the typing tutor is a wrong one, 0: never
	int			mTutorKeys;
	int			mContinueWait;	// updates to wait before the space bar of a won level
	int			mContinueSince;	// the update at which the won level was first seen, -1 when none
	unsigned	mLcg;			// the pseudo-random pattern of the mixed clam play

	TypistState() : mMode(-1), mFile(NULL), mEvery(1), mStart(0), mStop(2000000000), mTypoEvery(0), mTypoBurst(1), mEnterAt(-1), mTypedCount(0),
		mEnterDone(false), mPromptUpdate(-100), mTutorCorrect(-1), mTutorStuck(0),
		mStreakMax(-1), mStreakWords(0), mClamsMixed(false), mWinButton(-1), mOverButton(-1), mConfirmButton(-1), mTutorErrEvery(0), mTutorKeys(0), mContinueWait(0), mContinueSince(-1), mLcg(12345) {}
	~TypistState()
	{
		if (mFile != NULL)
			fclose(mFile);
	}
} gTypist;

void ParseOptions(WinWSApp* app)
{
	gTypist.mMode = 0;
	for (size_t i = 0; i < app->mCmdLineArgs.size(); i++)
	{
		const std::string& anArg = app->mCmdLineArgs[i];
		if (anArg.compare(0, 8, "-typist=") != 0)
			continue;
		std::string aSpec = anArg.substr(8);
		if (aSpec.size() >= 2 && aSpec[0] == '"' && aSpec[aSpec.size() - 1] == '"')
			aSpec = aSpec.substr(1, aSpec.size() - 2);
		size_t aSemi = aSpec.find(';');
		std::string aFileName = aSpec.substr(0, aSemi);
		while (aSemi != std::string::npos)
		{
			size_t aNext = aSpec.find(';', aSemi + 1);
			std::string anOption = aSpec.substr(aSemi + 1, aNext == std::string::npos ? std::string::npos : aNext - aSemi - 1);
			size_t anEq = anOption.find('=');
			if (anEq != std::string::npos)
			{
				std::string aName = anOption.substr(0, anEq);
				int aValue = atoi(anOption.c_str() + anEq + 1);
				if (aName == "every")
					gTypist.mEvery = aValue > 0 ? aValue : 1;
				else if (aName == "start")
					gTypist.mStart = aValue;
				else if (aName == "stop")
					gTypist.mStop = aValue;
				else if (aName == "burst")
					gTypist.mTypoBurst = aValue;
				else if (aName == "typo")
					gTypist.mTypoEvery = aValue;
				else if (aName == "enter")
					gTypist.mEnterAt = aValue;
				else if (aName == "win")
					gTypist.mWinButton = anOption.compare(anEq + 1, std::string::npos, "yes") == 0 ? 0 : 1;
				else if (aName == "confirm")
					gTypist.mConfirmButton = anOption.compare(anEq + 1, std::string::npos, "yes") == 0 ? 0 : 1;
				else if (aName == "over")
					gTypist.mOverButton = anOption.compare(anEq + 1, std::string::npos, "yes") == 0 ? 0 : 1;
				else if (aName == "tutorerr")
					gTypist.mTutorErrEvery = aValue;
				else if (aName == "wait")
					gTypist.mContinueWait = aValue;
				else if (aName == "streak")
					gTypist.mStreakMax = aValue;
				else if (aName == "clams")
					gTypist.mClamsMixed = anOption.compare(anEq + 1, std::string::npos, "mixed") == 0;
				else if (aName == "want")
				{
					const char* p = anOption.c_str() + anEq + 1;
					while (*p != 0)
					{
						gTypist.mWant.push_back(atoi(p));
						while (*p != 0 && *p != '+')
							p++;
						if (*p == '+')
							p++;
					}
				}
			}
			aSemi = aNext;
		}
		gTypist.mFile = fopen(aFileName.c_str(), "w");
		if (gTypist.mFile == NULL)
			printf("ERROR typist: cannot open %s\n", aFileName.c_str());
		else
			gTypist.mMode = 1;
		fflush(stdout);
	}
}

void Play(WinWSApp* app, int theUpdate, const std::string& theCommand, const std::string& theArgs)
{
	app->AddScriptCommand(theUpdate, theCommand, theArgs);
	fprintf(gTypist.mFile, "%d %s %s\n", theUpdate, theCommand.c_str(), theArgs.c_str());
	fflush(gTypist.mFile);
}

unsigned NextRandom()
{
	gTypist.mLcg = gTypist.mLcg * 1103515245u + 12345u;
	return (gTypist.mLcg >> 16) & 0x7fff;
}

// the mixed play of the clams: a letter by the fixed pattern, 0 when there is none to type
char MixedClamLetter(const BoardSnapshot& s)
{
	std::vector<char> aPearls, aEmpties, anAny;
	bool aUsed[26] = { false };
	for (size_t i = 0; i < s.mClams.size(); i++)
	{
		const BotCreature& c = s.mClams[i];
		if (c.mWord.size() != 1)
			continue;
		char l = c.mWord[0];
		if (l >= 'A' && l <= 'Z')
			aUsed[l - 'A'] = true;
		anAny.push_back(l);
		if (c.mState == 4)
			(c.mType != 0 ? aPearls : aEmpties).push_back(l);
	}
	unsigned r = NextRandom() % 10;
	if (r < 4 && !aPearls.empty())
		return aPearls[NextRandom() % aPearls.size()];
	if (r < 7 && !aEmpties.empty())
		return aEmpties[NextRandom() % aEmpties.size()];
	if (r < 8)
	{
		for (int k = 0; k < 26; k++)
		{
			int l = (int) (NextRandom() % 26);
			if (!aUsed[l])
				return (char) ('A' + l);
		}
		return 0;
	}
	if (!anAny.empty())
		return anAny[NextRandom() % anAny.size()];
	return 0;
}

// the letter of the chest to take at a fork of the map: that of the first wanted site on offer, else the first chest's
char ChooseChest(MapScreen* theMap)
{
	for (size_t w = 0; w < gTypist.mWant.size(); w++)
		for (size_t i = 0; i < theMap->mVector1b4.size(); i++)
			if (theMap->mVector1b4[i]->mLevel == gTypist.mWant[w])
				return theMap->mVector1b4[i]->mLetter;
	return 0;
}

}  // namespace

void Sexy::TestTypistStep(WinWSApp* app)
{
	if (gTypist.mMode == 0)
		return;
	if (gTypist.mMode < 0)
	{
		ParseOptions(app);
		if (gTypist.mMode == 0)
			return;
	}
	int aNext = app->mUpdateCount + 1;			// the update the input is read at
	if (aNext < gTypist.mStart || aNext >= gTypist.mStop || aNext % gTypist.mEvery != 0)
		return;
	for (int aKind = 0; aKind < 3; aKind++)
	{
		int aButton = aKind == 0 ? gTypist.mWinButton : aKind == 1 ? gTypist.mOverButton : gTypist.mConfirmButton;
		Dialog* aDialog = app->GetDialog(aKind == 0 ? DIALOG_WIN_GAME : aKind == 1 ? DIALOG_GAME_OVER : DIALOG_MESSAGE);
		if (aButton < 0 || aDialog == NULL || aNext - gTypist.mPromptUpdate < 3)
			continue;
		if (aKind == 2 && aDialog->mNoButton == NULL)
			continue;
		DialogButton* aDialogButton = aButton == 0 ? aDialog->mYesButton : aDialog->mNoButton;
		if (aDialogButton == NULL || !aDialogButton->mVisible)
			continue;
		int aX = aDialogButton->mX + aDialogButton->mWidth / 2;
		int aY = aDialogButton->mY + aDialogButton->mHeight / 2;
		if (aDialogButton->mParent == aDialog)
		{
			aX += aDialog->mX;
			aY += aDialog->mY;
		}
		gTypist.mPromptUpdate = aNext;
		Play(app, aNext, "click", StrFormat("%d %d", aX, aY));
		return;
	}
	if (gTypist.mWinButton >= 0 && app->mMapScreen != NULL && app->mMapScreen->mVisible && app->mMapScreen->mFinal)
		return;			// the map of the last site scrolls away and the CONGRATULATIONS! dialog comes: nothing to press
	if (app->mBoard == NULL && app->mTypingTutor != NULL && app->mTypingTutor->mVisible)
	{
		// the typing tutor: the key that is next, until it stops being accepted (a space at the first or last place of a line
		// is neither right nor wrong: the lesson cannot go on, and the bot gives up after 20 keys that counted for nothing)
		TypingTutor* t = app->mTypingTutor;
		if (t->mState == 2 || !app->mDialogList.empty() || t->mPauseCount > 0)
			return;
		if (t->mCorrectKeys != gTypist.mTutorCorrect)
		{
			gTypist.mTutorCorrect = t->mCorrectKeys;
			gTypist.mTutorStuck = 0;
		}
		else if (++gTypist.mTutorStuck > 20)
			return;
		char c = t->mExpectedChar;
		if (gTypist.mTutorErrEvery > 0 && ++gTypist.mTutorKeys % gTypist.mTutorErrEvery == 0)
		{
			static const char aWrong[] = " \rqz!";
			char w = aWrong[(gTypist.mTutorKeys / gTypist.mTutorErrEvery) % 5];
			if (w == c)
				w = 'k';
			if (w == ' ')
				Play(app, aNext, "char", "SPACE");
			else if (w == '\r')
				Play(app, aNext, "key", "RETURN");
			else
				Play(app, aNext, "char", std::string(1, w));
			return;
		}
		if (c == ' ')
			Play(app, aNext, "char", "SPACE");
		else if (c == '\r' || c == '\n')
			Play(app, aNext, "key", "RETURN");
		else
			Play(app, aNext, "char", std::string(1, c));
		return;
	}
	if (app->mBoard == NULL && app->GetDialog(DIALOG_MESSAGE) == NULL)
		return;

	if (!gTypist.mEnterDone && gTypist.mEnterAt >= 0 && aNext >= gTypist.mEnterAt && app->mBoard != NULL)
	{
		gTypist.mEnterDone = true;
		Play(app, aNext, "key", "RETURN");
		return;
	}

	BoardSnapshot aSnapshot = TestTakeSnapshot(app);
	if (aSnapshot.mPhase == BoardSnapshot::PHASE_CLAMS && gTypist.mClamsMixed)
	{
		char c = MixedClamLetter(aSnapshot);
		if (c != 0)
			Play(app, aNext, "char", std::string(1, c));
		return;
	}
	if (aSnapshot.mPhase == BoardSnapshot::PHASE_STREAK && gTypist.mStreakMax >= 0 && aSnapshot.mStreakPos == 0)
	{
		if (gTypist.mStreakWords >= gTypist.mStreakMax)
			return;
		if (aSnapshot.mStreakPos < (int) aSnapshot.mStreakText.size())
			gTypist.mStreakWords++;
	}
	if (aSnapshot.mPhase != BoardSnapshot::PHASE_STREAK)
		gTypist.mStreakWords = 0;
	BotMove aMove = ChooseMove(aSnapshot);
	if (!aMove.mValid)
		return;
	if (aMove.mWhat == "continue" && gTypist.mContinueWait > 0)
	{
		if (gTypist.mContinueSince < 0)
			gTypist.mContinueSince = aNext;
		if (aNext - gTypist.mContinueSince < gTypist.mContinueWait)
			return;
	}
	if (aMove.mWhat != "continue")
		gTypist.mContinueSince = -1;
	if (aMove.mWhat == "map" && !gTypist.mWant.empty() && app->mMapScreen != NULL && !aMove.mInput.empty() &&
		aMove.mInput[0].mKind == BotStep::KEY && aMove.mInput[0].mKey != " ")
	{
		char aChest = ChooseChest(app->mMapScreen);
		if (aChest != 0)
			aMove.mInput[0].mKey = std::string(1, aChest);
	}
	for (size_t i = 0; i < aMove.mInput.size(); i++)
	{
		const BotStep& aStep = aMove.mInput[i];
		if (aStep.mKind == BotStep::CLICK)
		{
			if (aNext - gTypist.mPromptUpdate < 3)
				return;
			gTypist.mPromptUpdate = aNext;
			Play(app, aNext, "click", StrFormat("%d %d", aStep.mX, aStep.mY));
			return;
		}
		if (aStep.mKind == BotStep::KEY)
		{
			std::string aKey = aStep.mKey;
			if (aKey == " ")
			{
				if (aNext - gTypist.mPromptUpdate < 3)
					return;
				gTypist.mPromptUpdate = aNext;
				Play(app, aNext, "char", "SPACE");
				return;
			}
			if (aKey.size() == 1)
			{
				gTypist.mTypedCount++;
				char c = aKey[0];
				if (gTypist.mTypoEvery > 0 && (gTypist.mTypedCount % gTypist.mTypoEvery == 0 || gTypist.mTypedCount % gTypist.mTypoEvery > gTypist.mTypoEvery - gTypist.mTypoBurst) && c >= 'A' && c <= 'Z')
					c = (char) ('A' + (c - 'A' + 7) % 26);
				Play(app, aNext, "char", std::string(1, c));
				return;
			}
			return;
		}
	}
}
