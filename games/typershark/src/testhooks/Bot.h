#pragma once

// Test hook: the typing bot behind the `move` test command (docs/TEST_COMMANDS.md), kept as a pure function of a snapshot of
// the board so that a later bot mod can reuse it unchanged. It reads nothing from the game and draws no random number.
// The snapshot is made by testhooks/Snapshot.cpp from the real Board; the decision is made here.

#include <string>
#include <vector>

namespace Sexy
{

// one step of a move's real input (docs/TEST_COMMANDS.md): a key typed, a click at game pixels, or n updates waited
struct BotStep
{
	enum Kind { KEY, CLICK, WAIT };
	Kind		mKind;
	std::string	mKey;		// KEY: the character as the word has it ("A", " ") or a DOM key name ("Enter")
	int			mX;			// CLICK
	int			mY;
	int			mWait;		// WAIT

	BotStep() : mKind(WAIT), mX(0), mY(0), mWait(1) {}
	static BotStep Key(const std::string& theKey) { BotStep s; s.mKind = KEY; s.mKey = theKey; return s; }
	static BotStep Click(int theX, int theY) { BotStep s; s.mKind = CLICK; s.mX = theX; s.mY = theY; return s; }
	static BotStep Wait(int theUpdates) { BotStep s; s.mKind = WAIT; s.mWait = theUpdates; return s; }
};

// one creature (or boss shot, or clam) of the board, as Board::KeyChar sees it
struct BotCreature
{
	std::string	mKind;			// the class name: Shark, RedShark, Piranha, Clam, BonusCreature, TorpedoBoss shot "Torpedo"...
	std::string	mList;			// "sharks", "piranhas", "bonus", "shots" or "clams"
	std::string	mWord;
	int			mTyped;			// letters typed on it (the board's typed count for the creature it is locked on)
	double		mX;
	double		mY;
	int			mState;
	bool		mVisible;
	bool		mBlocked;		// a bonus creature's mFlag19: Board::KeyChar skips it
	bool		mIsTarget;		// the board is locked on it
	int			mType;			// a clam's kind (0 a dud, 1 and 2 pair up), -1 for the others

	BotCreature() : mTyped(0), mX(0), mY(0), mState(0), mVisible(true), mBlocked(false), mIsTarget(false), mType(-1) {}
};

struct BoardSnapshot
{
	// what the keyboard does on the board right now (Board::KeyChar's branches)
	enum Phase
	{
		PHASE_NONE,		// the board takes no letters: the bot answers with mPrompt, if there is one
		PHASE_WORDS,	// the normal play: type the word of a creature
		PHASE_STREAK,	// the streak game of the diver's state 5: type mStreakText from mStreakPos
		PHASE_CLAMS		// the clams of a treasure level: a letter opens the clam it is written on
	};

	Phase					mPhase;
	int						mWidth;
	int						mHeight;
	std::vector<BotCreature> mCreatures;	// sharks, piranhas, bonus creatures, boss shots: the order in which KeyChar tries them
	std::vector<BotCreature> mClams;
	int						mTarget;		// index in mCreatures of the creature the board is locked on, or -1
	int						mTypedCount;	// letters typed on the target (Board::mTyped)
	std::string				mStreakText;
	int						mStreakPos;
	std::string				mPromptWhat;	// what the obvious step is ("dialog", "map", "continue"), "" when there is none
	std::vector<BotStep>	mPrompt;

	BoardSnapshot() : mPhase(PHASE_NONE), mWidth(640), mHeight(480), mTarget(-1), mTypedCount(0), mStreakPos(0) {}
};

struct BotMove
{
	bool					mValid;
	std::string				mWhat;		// the creature kind, or "dialog", "map", "continue", "streak"
	std::string				mWord;		// the whole word
	std::string				mTyping;	// the letters it types now (the word's letters not typed yet)
	std::vector<BotStep>	mInput;

	BotMove() : mValid(false) {}
};

// the policy: continue the word of the creature the board is locked on; otherwise take the one that threatens soonest (the
// shark, piranha or boss shot nearest the left edge that can be typed), a bonus creature only when none of those can be typed;
// the creature that takes the first letter is the one Board::KeyChar picks (the first in its order with that letter)
BotMove ChooseMove(const BoardSnapshot& theSnapshot);

// "MOVE {...}" or "MOVE null"
std::string BotMoveToJson(const BotMove& theMove);

// a JSON string literal (with the quotes)
std::string JsonQuote(const std::string& theText);

}
