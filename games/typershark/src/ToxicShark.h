#ifndef __TYPERSHARK_TOXICSHARK_H__
#define __TYPERSHARK_TOXICSHARK_H__

// ToxicShark.cpp: a glowing shark whose word scrambles: every mScramblePeriod milliseconds the letters not yet typed
// are shuffled (the first letter stays while nothing is typed). It is drawn twice, a glow picture added over the body.
// Original: class Sexy::ToxicShark : Shark (a plain base; Enemy and GameObject are virtual bases), vftable 0x5296e8
// (the Shark part at +0, 1 virtual), 0x5296b4 (GameObject part at +0x38), 0x5296a4 (Enemy part at +0x130), sizeof
// 0x238; the constructor is 0x4109e0, the deleting destructor 0x409bc0 (destructor of the whole object 0x401cf0).
// The field comments give the offsets in the original's object (Shark's own fields end at +0x24).

#include "Shark.h"

namespace Sexy
{

class Graphics;
class WSTimer;

class ToxicShark : public Shark
{
public:
	int						mScramblePeriod;		// +0x28 ctor 1000; TypeLetter sets it by the app's difficulty field mField690 (ms)
	WSTimer*				mTimer;					// +0x2c added to the widget manager, which keeps it counting; removed and deleted by the destructor

public:
	ToxicShark(WinWSApp* theApp);													// 0x4109e0
	virtual ~ToxicShark();															// deleting destructor 0x409bc0

	virtual void			Update() override;										// GameObject slot 1, 0x40d6e0
	virtual void			Draw(Graphics* g) override;								// slot 2, 0x4055f0
	virtual void			TypeLetter() override;									// Enemy slot 2, 0x40d650

private:
	void					DrawGlow(Graphics* g, Image* theGlow);					// the first half of each case of 0x4055f0, written out in each of them
};

}

#endif
