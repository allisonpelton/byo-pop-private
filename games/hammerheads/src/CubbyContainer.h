#ifndef __HAMMERHEADS_CUBBYCONTAINER_H__
#define __HAMMERHEADS_CUBBYCONTAINER_H__

// The list of cubbies that a screen of its own keeps (the original's class Sexy::CubbyContainer, the third base of the bonus
// round and of the shop at +0x84: a vtable with one slot, the picture the cubbies cover their holes with, and the array of
// object references). The game screen's cubbies are Gameplay's own list; these two screens make theirs when they open.

#include <string>
#include <vector>

#include "Object.h"

namespace Sexy
{

class Cubby;
class Image;

class CubbyContainer
{
public:
	std::vector<ObjectRef> mCubbies;		// +0x88 (the array, its capacity at +0x8c and its count at +0x90 are the vector's)

public:
	// slot 0 (0x44cd30 of the bonus round, 0x45df30 of the shop): the picture of the screen, which every cubby takes as the
	// backdrop it hides its creature against
	virtual Image*	GetBackdrop() = 0;

	Cubby*			GetCubby(int theIndex) const;									// 0x4347c0
	// 0x434be0 (called through 0x434c70): a cubby at the place, named, and listed. A random number is drawn for its variant.
	Cubby*			AddCubby(int theX, int theY, const std::string& theName);

protected:
	~CubbyContainer() {}
};

// 0x4015d0: the name of the nth object made by hand of the type ("class Sexy::Cubby" and 90001 make "class Sexy::Cubby_90001")
std::string MakeCubbyName(int theNumber);

}

#endif
