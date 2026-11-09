#ifndef __TYPERSHARK_REDPIRANHA_H__
#define __TYPERSHARK_REDPIRANHA_H__

// RedPiranha.cpp: a piranha with other pictures, a white word and its own text offsets; no virtual is its own.
// Original: class Sexy::RedPiranha : Piranha (a virtual base, as are Enemy and GameObject; the object has no vptr of
// its own, only a vbptr at +0), vftable 0x529ecc (GameObject part at +8), 0x529ebc (Enemy part at +0xf8), 0x529eb4
// (Piranha part at +0x1e0), sizeof 0x210; the constructor is 0x40ff50 (the whole object's destructor is not a
// function of its own). port: Piranha is a plain base here, since no class reaches it twice through RedPiranha.

#include "Piranha.h"

namespace Sexy
{

class RedPiranha : public Piranha
{
public:
	RedPiranha(WinWSApp* theApp, int theType);										// 0x40ff50
	virtual ~RedPiranha();
};

}

#endif
