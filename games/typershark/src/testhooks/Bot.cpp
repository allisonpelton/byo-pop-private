// port: the typing bot behind the `move` test command (docs/TEST_COMMANDS.md). A pure function of a BoardSnapshot, so that a
// later bot mod can reuse it as it is; it holds no state and draws no random number. The snapshot copies what Board::KeyChar
// and Board::TryLetter look at, so the bot types what the game will accept: the first letter picks the creature (the first one
// in the order sharks, piranhas, bonus creatures, boss shots whose word begins with it and which can be typed), every later
// letter must be the next one of the word of the creature the board is locked on.

#include "Bot.h"

#include <cstdio>

namespace Sexy
{

std::string JsonQuote(const std::string& theText)
{
	std::string aResult = "\"";
	for (size_t i = 0; i < theText.size(); i++)
	{
		unsigned char aChar = (unsigned char) theText[i];
		if (aChar == '"')
			aResult += "\\\"";
		else if (aChar == '\\')
			aResult += "\\\\";
		else if (aChar < 0x20)
		{
			char aBuffer[8];
			snprintf(aBuffer, sizeof(aBuffer), "\\u%04x", aChar);
			aResult += aBuffer;
		}
		else
			aResult += (char) aChar;
	}
	return aResult + "\"";
}

namespace
{

// Board::TryLetter and the tests of Board::KeyChar's loops: the creature can take a first letter
bool Typable(const BoardSnapshot& theSnapshot, const BotCreature& theCreature)
{
	if (theCreature.mState != 0 && theCreature.mState != 6)
		return false;
	if (!theCreature.mVisible || theCreature.mWord.empty())
		return false;
	if (theCreature.mList == "bonus")
		return !theCreature.mBlocked && (double) theSnapshot.mHeight > theCreature.mY + 20.0;
	if (theCreature.mList == "shots")
		return theCreature.mState == 0;
	return (double) theSnapshot.mWidth > theCreature.mX + 20.0;
}

BotMove TypingMove(const BotCreature& theCreature, int theTyped)
{
	BotMove aMove;
	if (theTyped >= (int) theCreature.mWord.size())
		return aMove;
	aMove.mValid = true;
	aMove.mWhat = theCreature.mKind;
	aMove.mWord = theCreature.mWord;
	aMove.mTyping = theCreature.mWord.substr(theTyped);
	for (size_t i = 0; i < aMove.mTyping.size(); i++)
		aMove.mInput.push_back(BotStep::Key(std::string(1, aMove.mTyping[i])));
	aMove.mInput.push_back(BotStep::Wait(1));
	return aMove;
}

}  // namespace

BotMove ChooseMove(const BoardSnapshot& theSnapshot)
{
	BotMove aNone;

	if (theSnapshot.mPhase == BoardSnapshot::PHASE_NONE)
	{
		if (theSnapshot.mPrompt.empty())
			return aNone;
		BotMove aMove;
		aMove.mValid = true;
		aMove.mWhat = theSnapshot.mPromptWhat;
		aMove.mInput = theSnapshot.mPrompt;
		return aMove;
	}

	if (theSnapshot.mPhase == BoardSnapshot::PHASE_STREAK)
	{
		if (theSnapshot.mStreakPos >= (int) theSnapshot.mStreakText.size())
			return aNone;
		BotCreature aText;
		aText.mKind = theSnapshot.mPromptWhat.empty() ? "streak" : theSnapshot.mPromptWhat;		// the typing tutor's line is typed the same way
		aText.mWord = theSnapshot.mStreakText;
		return TypingMove(aText, theSnapshot.mStreakPos);
	}

	if (theSnapshot.mPhase == BoardSnapshot::PHASE_CLAMS)
	{
		// a dud clam that is open blocks every letter until it closes
		for (size_t i = 0; i < theSnapshot.mClams.size(); i++)
			if (theSnapshot.mClams[i].mState == 1 && theSnapshot.mClams[i].mType == 0)
				return aNone;
		// the bot knows the clams' kinds (the game hides them until they open): the first closed clam of a winning kind
		for (size_t i = 0; i < theSnapshot.mClams.size(); i++)
			if (theSnapshot.mClams[i].mState == 4 && theSnapshot.mClams[i].mType != 0 && !theSnapshot.mClams[i].mWord.empty())
				return TypingMove(theSnapshot.mClams[i], 0);
		return aNone;
	}

	// the normal play
	if (theSnapshot.mTarget >= 0 && theSnapshot.mTarget < (int) theSnapshot.mCreatures.size())
	{
		// locked on a creature: only its next letter counts
		const BotCreature& aTarget = theSnapshot.mCreatures[theSnapshot.mTarget];
		if (aTarget.mState != 0 && aTarget.mState != 6)
			return aNone;
		return TypingMove(aTarget, theSnapshot.mTypedCount);
	}

	int aBest = -1;
	for (size_t i = 0; i < theSnapshot.mCreatures.size(); i++)
	{
		const BotCreature& c = theSnapshot.mCreatures[i];
		if (c.mList == "bonus" || !Typable(theSnapshot, c))
			continue;
		if (aBest < 0 || c.mX < theSnapshot.mCreatures[aBest].mX)
			aBest = (int) i;
	}
	if (aBest < 0)
	{
		for (size_t i = 0; i < theSnapshot.mCreatures.size() && aBest < 0; i++)
			if (theSnapshot.mCreatures[i].mList == "bonus" && Typable(theSnapshot, theSnapshot.mCreatures[i]))
				aBest = (int) i;
	}
	if (aBest < 0)
		return aNone;

	// the first letter goes to the first creature in the game's order that can take it
	char aLetter = theSnapshot.mCreatures[aBest].mWord[0];
	for (size_t i = 0; i < theSnapshot.mCreatures.size(); i++)
		if (Typable(theSnapshot, theSnapshot.mCreatures[i]) && theSnapshot.mCreatures[i].mWord[0] == aLetter)
		{
			aBest = (int) i;
			break;
		}
	return TypingMove(theSnapshot.mCreatures[aBest], 0);
}

std::string BotMoveToJson(const BotMove& theMove)
{
	if (!theMove.mValid)
		return "MOVE null";
	std::string aLine = "MOVE {\"what\":" + JsonQuote(theMove.mWhat);
	if (!theMove.mWord.empty())
		aLine += ",\"word\":" + JsonQuote(theMove.mWord) + ",\"typing\":" + JsonQuote(theMove.mTyping);
	aLine += ",\"input\":[";
	for (size_t i = 0; i < theMove.mInput.size(); i++)
	{
		const BotStep& aStep = theMove.mInput[i];
		char aBuffer[96];
		if (i > 0)
			aLine += ",";
		if (aStep.mKind == BotStep::KEY)
			aLine += "{\"key\":" + JsonQuote(aStep.mKey) + "}";
		else if (aStep.mKind == BotStep::CLICK)
		{
			snprintf(aBuffer, sizeof(aBuffer), "{\"click\":[%d,%d]}", aStep.mX, aStep.mY);
			aLine += aBuffer;
		}
		else
		{
			snprintf(aBuffer, sizeof(aBuffer), "{\"wait\":%d}", aStep.mWait);
			aLine += aBuffer;
		}
	}
	return aLine + "]}";
}

}  // namespace Sexy
