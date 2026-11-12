// port: the bot that plays Venice Deluxe (the standard test commands' `move`, docs/TEST_COMMANDS.md; a later bot mod reuses it).
// It reads the game's state, never changes it and draws no random number.
//
// The shooter always points straight up and follows the pointer's x, so a shot is only an x: the bot aims at the lowest slot on
// the screen that the ready jewel fills, and waits when there is none (the level scrolls, or the next jewel is on its way).

#ifndef VENICE_BOT_H
#define VENICE_BOT_H

enum class BotAction
{
	None, // nothing to play: no level is on screen
	Wait, // the level is not ready for a shot
	Shoot, // move the pointer to (mX, mY), wait mUpdatesToAim updates for the shooter to catch up, and click
	Click, // a message over the level (the cupid's) is dismissed by a click at (mX, mY)
	Swap, // the jewel that waits is brought forward by a right click at (mX, mY), the pointer's place
};

struct BotMove
{
	BotAction mAction = BotAction::None;
	int mX = 0;
	int mY = 0;
	int mUpdatesToAim = 0;
};

// whether the bot goes on past a level's end: it clicks a summary screen away (a level that was won or lost goes on to its summary
// by itself), and the next level begins; otherwise it has no move once the play is over
extern bool gBotContinuesPastSummaries;

// the move the bot would make now (a screen with no level in play has none)
BotMove ChooseBotMove();

// whether the play has ended: a summary screen is up, or the level in play has been won or lost
bool PlayIsOver();

#endif
