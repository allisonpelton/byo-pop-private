#ifndef __TYPERSHARK_STEALTHREDSHARK_H__
#define __TYPERSHARK_STEALTHREDSHARK_H__

// StealthRedShark.cpp: a red shark that starts stealthy; StealthBlackShark with RedShark in place of BlackShark.
// Original: class Sexy::StealthRedShark : RedShark, StealthShark (RedShark: BlackShark; all with the virtual bases
// Shark, Enemy and GameObject), vftable 0x52a0c8 (RedShark's own part at +0, 2 virtuals), 0x52a0bc (StealthShark's
// part at +0x40, 2 virtuals), 0x52a088 (GameObject part at +0x78), 0x52a078 (Enemy part at +0x170), 0x52a070 (Shark
// part at +0x260), sizeof 0x290; the constructor is 0x411b00, the deleting destructor 0x411e00.

#include "RedShark.h"
#include "StealthShark.h"

namespace Sexy
{

class StealthRedShark : public RedShark, public StealthShark
{
public:
	StealthRedShark(WinWSApp* theApp);												// 0x411b00
	virtual ~StealthRedShark();														// deleting destructor 0x411e00

	virtual void			SetState(int theState) override;						// GameObject slot 7, 0x406970
};

}

#endif
