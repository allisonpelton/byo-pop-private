#ifndef __DYNOMITE_H__
#define __DYNOMITE_H__

// The board (Dynomite, vtable 0x4f7470: the 43 Widget slots and 19 of its own; constructor 0x404030, destructor
// 0x4041e0). The four modes derive from it (DynomiteStomped, DynomitePanic, DynomiteFossil, DynomiteTimeTrial).
// Work in progress: the fields are named as the translated code comes to use them.

#include "PtrList.h"
#include "Widget.h"

namespace Sexy
{

class EggGrid;
class FossilizedApp;

// the board's +0x5c (its +0xc is tested before the dynamite combo gives a stick)
struct BoardPart5c
{
	int					mField00;
	int					mField04;
	int					mField08;
	int					mField0c;
};

class Dynomite : public Widget
{
public:
	FossilizedApp*		mApp;				// +0x58
	BoardPart5c*		mBoard5c;			// +0x5c
	int					mField68;			// +0x68 306
	int					mField6c;			// +0x6c 153
	int					mField70;			// +0x70 250, the launcher's middle
	int					mDifficulty;		// +0x78 1-3 (the side bar's skill picture)
	int					mGameType;			// +0x7c (the side bar's mode picture; 2 shows its sixth button)
	EggGrid*			mGrid;				// +0x8c (0x415770)
	int					mField108;			// +0x108 (a leaving baby sets it to 20)
	bool				mField130;			// +0x130
	bool				mField131;			// +0x131
	int					mPauseCount;		// +0x150

	Dynomite();								// 0x404030
	virtual ~Dynomite();					// 0x4041e0

	void				SetPaused(bool isPaused);		// 0x404fb0
	void				SetActive(bool isActive);		// 0x4047e0
	virtual void		MakeDynamite();					// 0x405010 (slot 51)
	void				Hint();							// 0x40ad90 (the side bar's sixth button)
};

}

#endif
