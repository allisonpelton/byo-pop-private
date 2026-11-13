#ifndef __HAMMERHEADS_INTROCREATURE_H__
#define __HAMMERHEADS_INTROCREATURE_H__

// The gnome a level introduces: the file gnomedata\<level>.txt of a level that has one names it (its first line is a number), the
// level's intro page shows it, and the game-over box shows it as a dark shape with a question mark over it. The functions are the
// original's 0x457c30, 0x457c60, 0x457100 and 0x457870.

#include <string>

namespace Sexy
{

class Creature;

// 0x457c30: the file of a level
std::string GetIntroFileName(int theLevel);

// 0x457c60: whether the level has a file
bool HasIntroFile(int theLevel);

// 0x457100: the creature that goes with a number of the file (not the creature kinds' numbers: 2 is the shop sign, 3 the strong
// gnome, 0x10 the bouncing one, and so on); null for a number that is none of them
Creature* MakeIntroCreature(int theIntroIndex);

// 0x457870: where an intro creature stands on the page (theY: the height of its feet below the top of the page; theMarkOffsetY: how
// much further the question mark of the game-over box is moved down for it)
void GetIntroPlacement(const Creature* theCreature, int& theY, int& theMarkOffsetY);

}

#endif
