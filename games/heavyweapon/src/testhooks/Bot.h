#ifndef __HEAVYWEAPON_BOT_H__
#define __HEAVYWEAPON_BOT_H__

// port: the port's own bot (docs/TEST_COMMANDS.md, "The bot behind move"): the move a driver makes, and later a bot mod's.
// The original has no bot. The tank drives toward where its gun points and fires while the left button is held, so a
// move is one place for the pointer and whether the button is held there.

namespace Sexy
{

class Armory;
class Board;

struct BotMove
{
	int		mX;			// the pointer, in game pixels
	int		mY;
	bool	mFire;		// the left button held
	int		mTarget;	// the enemy aimed at: its place in the board's list, -1 for none
};

// The next move on theBoard, or false when there is none (no game, the game paused, the level's results up)
bool ChooseBotMove(Board* theBoard, BotMove* theMove);

// The armory's weapon to raise a level next (its panel's place), or -1 once every point is spent or no weapon can rise:
// the lowest weapon on offer, the first of a tie, so the points spread over the weapons
int ChooseArmoryUpgrade(Armory* theArmory);

}

#endif
