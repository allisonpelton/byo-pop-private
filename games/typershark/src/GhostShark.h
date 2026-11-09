#ifndef __TYPERSHARK_GHOSTSHARK_H__
#define __TYPERSHARK_GHOSTSHARK_H__

// GhostShark.cpp: the ghost shark: a Shark with its own pictures. It is drawn twice, a glow picture 10 pixels larger on
// each side added over the body (the way ToxicShark glows), and the word is typed over it as on any shark. It swims,
// turns, dies and is shocked as Shark does; only Draw is its own.
// Original: class Sexy::GhostShark : Shark, Enemy, GameObject (all virtual bases), own part with no virtuals and no
// fields at +0 (just the vbptr, vbtable 0x529fec), the GameObject part at +8 (vftable 0x529fbc), the Enemy part at +0xf8
// (0x529fac), the Shark part at +0x1e0 (0x529fa4, 1 virtual); the constructor is 0x4111b0 (its last argument says
// whether it builds the virtual bases itself), the deleting destructor thunk 0x411350 (to 0x4113b0, which runs Shark's
// 0x4011b0 and Enemy's 0x4078b0).
//
// port: the single chain GameObject, Enemy, Shark, GhostShark, so no virtual inheritance. The vtordisp thunks
// (GameObject slots 0 0x411350, 1 0x4113f0 to Shark::Update 0x40b930, 2 0x411390 to 0x4037d0, 3 and 4 0x411360 and
// 0x411370 to the Enemy level 0x402230 and 0x402250, 6 0x411380 to 0x401a90, 7 0x4113a0 to Shark::SetState 0x404c20)
// only adjust the pointer; the others are Shark's.

#include "Shark.h"

namespace Sexy
{

class Graphics;
class Image;

class GhostShark : public Shark
{
public:
	GhostShark(WinWSApp* theApp);													// 0x4111b0
	virtual ~GhostShark();															// deleting thunk 0x411350

	virtual void			Draw(Graphics* g) override;								// GameObject slot 2, 0x4037d0

private:
	void					DrawGlowed(Graphics* g, Image* theGlow, Image* theBody);	// the code the cases of 0x4037d0 repeat
};

}

#endif
