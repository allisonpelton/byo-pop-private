#ifndef __DYNOMITE_BOT_H__
#define __DYNOMITE_BOT_H__

// port: the bot behind the `move` test command (docs/TEST_COMMANDS.md); the original has none. It reads the board and
// never changes it: every angle the mouse can give the launcher is flown with the grid's ghost probes (as
// FlyingEgg::Update moves an egg, ten steps an update, bouncing off the walls) against the grid coming down as fast as
// it did since the last call, landed as FlyingEgg::Land lands one, and scored by the group of the egg's colour it
// would join (EggGrid::Match's rule). The best shot is the angle and a point of the board the mouse can click to fire
// it.

namespace Sexy
{

class Dynomite;

struct BotShot
{
	int					mAngle;				// the launcher's angle, degrees from up
	int					mCol;				// the cell the egg lands in
	int					mRow;
	int					mColor;				// the loaded egg's colour (0 a bomb)
	int					mGroup;				// the eggs of its colour it joins, itself counted (a bomb: the eggs around it)
	int					mClickX;			// a point of the screen the mouse moves to and clicks to fire it
	int					mClickY;
};

// false when there is no shot (no board, no loaded egg, the launcher frozen, the game over or paused). With
// toTheLine the shot that lands lowest without a match is chosen instead (the tutorial's steps that ask for the eggs
// to cross the line and wait for the loss: a match there clears the rows and spends the step's eggs, and it never ends). With toTheTarget a shot whose flight meets a target (the tutorial's whirley) comes first.
bool BotChooseShot(Dynomite* theBoard, BotShot* theShot, bool toTheLine = false, bool toTheTarget = false);

}

#endif
