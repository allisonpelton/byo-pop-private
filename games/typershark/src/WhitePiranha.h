#ifndef __TYPERSHARK_WHITEPIRANHA_H__
#define __TYPERSHARK_WHITEPIRANHA_H__

// WhitePiranha.cpp: a piranha with a second word (a second letter to type). StealthWhitePiranha takes it together
// with StealthPiranha.
// Original: class Sexy::WhitePiranha : Piranha (virtual base; so are Enemy and GameObject), vftable 0x529f80 (own
// part, 1 virtual), 0x529f4c (GameObject part at +0x28), 0x529f3c (Enemy part at +0x120), 0x529f34 (Piranha part at
// +0x210), sizeof 0x240; the constructor is 0x410c70 (the export splits it in two: 0x410c93 is its second piece),
// the deleting destructor 0x410ed0. The field comments give the offsets in the original's own part
// (vptr at +0, vbptr at +4).
//
// port: Piranha is a virtual base, as in the original, because StealthWhitePiranha reaches it twice. The most
// derived class constructs Piranha, so every class below WhitePiranha names Piranha(theApp, theType).

#include "Piranha.h"
#include <string>

namespace Sexy
{

class WhitePiranha : public virtual Piranha
{
public:
	std::string				mSecondWord;					// +0x08 the word taken on when mWord is finished; the Board fills it (SetWord2)

public:
	WhitePiranha(WinWSApp* theApp, int theType);									// 0x410c70
	virtual ~WhitePiranha();														// deleting destructor 0x410ed0

	virtual void			SetWord2(std::string theWord);							// own slot 0, 0x410e70
	virtual bool			IsSwimming() override;									// Piranha part slot 0, 0x405f50
	virtual void			SetState(int theState) override;						// GameObject slot 7, 0x405e10
	virtual void			TypeLetter() override;									// Enemy slot 2, 0x40d9c0
};

}

#endif
