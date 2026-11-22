// PhysObjFactory.h: makes a piece of a level from the number of its class, the way a level file or a saved game names it.

#ifndef __PHYSOBJFACTORY_H__
#define __PHYSOBJFACTORY_H__

namespace Sexy
{

class Board;
class PhysObj;

PhysObj* MakePhysObj(int theClass); // 0x404ca0: a number that is none of PhysObjClass gives a plain PhysObj
Board* GetBoard();					// the game board that is up, or none

} // namespace Sexy

#endif
