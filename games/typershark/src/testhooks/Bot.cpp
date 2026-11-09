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
		unsigned char c = (unsigned char) theText[i];
		if (c == '"')
			aResult += "\\\"";
		else if (c == '\\')
			aResult += "\\\\";
		else if (c < 0x20)
		{
			char aBuffer[8];
			snprintf(aBuffer, sizeof(aBuffer), "\\u%04x", c);
			aResult += aBuffer;
		}
		else
			aResult += (char) c;
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

BotMove ChooseMove(const BoardSnapshot& s)
{
	BotMove aNone;

	if (s.mPhase == BoardSnapshot::PHASE_NONE)
	{
		if (s.mPrompt.empty())
			return aNone;
		BotMove aMove;
		aMove.mValid = true;
		aMove.mWhat = s.mPromptWhat;
		aMove.mInput = s.mPrompt;
		return aMove;
	}

	if (s.mPhase == BoardSnapshot::PHASE_STREAK)
	{
		if (s.mStreakPos >= (int) s.mStreakText.size())
			return aNone;
		BotCreature aText;
		aText.mKind = s.mPromptWhat.empty() ? "streak" : s.mPromptWhat;		// the typing tutor's line is typed the same way
		aText.mWord = s.mStreakText;
		return TypingMove(aText, s.mStreakPos);
	}

	if (s.mPhase == BoardSnapshot::PHASE_CLAMS)
	{
		// a dud clam that is open blocks every letter until it closes
		for (size_t i = 0; i < s.mClams.size(); i++)
			if (s.mClams[i].mState == 1 && s.mClams[i].mType == 0)
				return aNone;
		// the bot knows the clams' kinds (the game hides them until they open): the first closed clam of a winning kind
		for (size_t i = 0; i < s.mClams.size(); i++)
			if (s.mClams[i].mState == 4 && s.mClams[i].mType != 0 && !s.mClams[i].mWord.empty())
				return TypingMove(s.mClams[i], 0);
		return aNone;
	}

	// the normal play
	if (s.mTarget >= 0 && s.mTarget < (int) s.mCreatures.size())
	{
		// locked on a creature: only its next letter counts
		const BotCreature& aTarget = s.mCreatures[s.mTarget];
		if (aTarget.mState != 0 && aTarget.mState != 6)
			return aNone;
		return TypingMove(aTarget, s.mTypedCount);
	}

	int aBest = -1;
	for (size_t i = 0; i < s.mCreatures.size(); i++)
	{
		const BotCreature& c = s.mCreatures[i];
		if (c.mList == "bonus" || !Typable(s, c))
			continue;
		if (aBest < 0 || c.mX < s.mCreatures[aBest].mX)
			aBest = (int) i;
	}
	if (aBest < 0)
	{
		for (size_t i = 0; i < s.mCreatures.size() && aBest < 0; i++)
			if (s.mCreatures[i].mList == "bonus" && Typable(s, s.mCreatures[i]))
				aBest = (int) i;
	}
	if (aBest < 0)
		return aNone;

	// the first letter goes to the first creature in the game's order that can take it
	char aLetter = s.mCreatures[aBest].mWord[0];
	for (size_t i = 0; i < s.mCreatures.size(); i++)
		if (Typable(s, s.mCreatures[i]) && s.mCreatures[i].mWord[0] == aLetter)
		{
			aBest = (int) i;
			break;
		}
	return TypingMove(s.mCreatures[aBest], 0);
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
		const BotStep& s = theMove.mInput[i];
		char aBuffer[96];
		if (i > 0)
			aLine += ",";
		if (s.mKind == BotStep::KEY)
			aLine += "{\"key\":" + JsonQuote(s.mKey) + "}";
		else if (s.mKind == BotStep::CLICK)
		{
			snprintf(aBuffer, sizeof(aBuffer), "{\"click\":[%d,%d]}", s.mX, s.mY);
			aLine += aBuffer;
		}
		else
		{
			snprintf(aBuffer, sizeof(aBuffer), "{\"wait\":%d}", s.mWait);
			aLine += aBuffer;
		}
	}
	return aLine + "]}";
}

}  // namespace Sexy
