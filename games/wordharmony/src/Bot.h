#ifndef __WORDHARMONY_BOT_H__
#define __WORDHARMONY_BOT_H__

// port: the bot behind the move test command (docs/TEST_COMMANDS.md, "The bot behind move"), and later the bot mod's.
// It reads the game and never moves it: no random numbers, no game state written. Its plan is its own (the word it is
// building, where, and how its drags went), kept between calls.

#include <string>
#include <vector>

class Game;

namespace Sexy
{

// One step of a move's input, in game coordinates
struct BotStep
{
	enum Kind { MOVE, DOWN, UP, CLICK, RCLICK, WAIT } mKind;
	int x, y;
	int mN;  // WAIT: updates
};

// A move: what it is ("what", and the word and its letter for a drag), its input, then the updates to wait before
// asking again. No "what": no move.
struct BotAction
{
	BotAction() {}
	BotAction(const char* theWhat, int theUpdates = 1) : mWhat(theWhat), mUpdates(theUpdates) {}
	std::string mWhat;
	std::string mWord;
	int mLetter = -1;
	std::vector<BotStep> mSteps;
	int mUpdates = 1;
	// the MOVE line's JSON ("null" for no move): "input" steps of {"click":{x,y[,button]}}, {"press":{x,y}},
	// {"move":{x,y}}, {"release":{x,y}} and {"wait":updates}, then "updates"
	std::string toJson() const;
};

extern bool gBotDebug;
BotAction BotNext(::Game* theGame);
std::string BotMove(::Game* theGame);

}

#endif
