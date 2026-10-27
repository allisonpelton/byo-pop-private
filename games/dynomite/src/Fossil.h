#ifndef __FOSSIL_H__
#define __FOSSIL_H__

// DynomiteFossil's +0x26c (vtable 0x4fa260, only its destructor; 0x24 bytes; 0x413940-0x413d20): the fragment being
// dug out. The app's picture of a fossil has a mask with one fragment in each colour channel (red, green, blue for
// rounds 1 to 3); the copy here shows only this round's. Place marks the eggs over it as rock, and the board draws the
// fragment and its glow where the grid draws the fragment's cell (colour 50).

namespace Sexy
{

class EggGrid;
class FossilizedApp;
class MaskedPic;
class Pic;
class Color;

class Fossil
{
public:
	FossilizedApp*		mApp;				// +8
	MaskedPic*			mPic;				// +0xc the app's picture of the fossil (not this one's)
	MaskedPic*			mFragment;			// +0x10 a copy showing this round's fragment
	MaskedPic*			mWhole;				// +0x14 a copy showing all three
	Pic*				mGlow;				// +0x18 around the fragment
	int					mX;					// +0x1c the fragment's cell in the picture
	int					mY;					// +0x20

	Fossil(FossilizedApp* theApp);			// 0x413cd0
	virtual ~Fossil();						// 0x413d10

	void				Free();									// 0x413940: the copies and the glow
	void				SetFossil(int theFossil);				// 0x413990: fossil 1-15
	void				SetPos(int x, int y);					// 0x413a50
	// 0x413a70: the eggs over the fragment, with the cell (theCol, theRow) at its mX, mY, become rock (colour 51) and
	// lose their dynamite; that cell becomes the fragment (50)
	void				Place(EggGrid* theGrid, int theCol, int theRow);
	void				MakeGlowFor(const Color& theColor);		// 0x413c80
};

}

#endif
