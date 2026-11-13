#ifndef __HAMMERHEADS_SAVEDGAMES_H__
#define __HAMMERHEADS_SAVEDGAMES_H__

// The saved game of a player (0x469300-0x46e500 and 0x471380-0x473710): a game in progress that "Continue" picks up.

namespace Sexy
{

// 0x46d620: writes the game on screen into the player's saved game
void StoreGameSnapshot();

}

#endif
