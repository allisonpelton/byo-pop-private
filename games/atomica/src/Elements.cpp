// The bonus atoms of the five elements (see Atom.h).
// port: only the part that the saved game and the grid need is here; the elements' effects (0x404e30-0x405600) and
// their own pictures (0x404b00) are not ported yet, so an element atom looks and acts as a plain atom of its type.

#include "Atom.h"
#include "AtomicaApp.h"
#include "Grid.h"
#include "Board.h"

using namespace Sexy;

// 0x4043d0
ElementAtom::ElementAtom(Grid* theGrid, int theType) :
	Atom(theGrid, theType == 0 ? 12 : theType, true)
{
	mF_f0 = 0;
	for (int i = 0; i < 5; i++)
	{
		mFlicker[i] = 0;
		mFlickerStep[i] = 0;
	}
	mElement = (mType - 12) / 3;
	mLevel = (mType - 12) % 3;
	mF124 = 0;
	mFlag_e0 = true;
}

// 0x404630
ElementAtom::~ElementAtom()
{
}

// 0x404640
void ElementAtom::Update()
{
	Atom::Update();
}

// 0x404b00
void ElementAtom::Draw(Graphics* g)
{
	Atom::Draw(g);
}

// 0x4055b0: the element's effect (a double click on the atom)
void ElementAtom::OnPlaced()
{
}

// 0x4049d0
void ElementAtom::Save(FILE* f)
{
	Atom::Save(f);
	fwrite(&mF_f0, 4, 1, f);
	fwrite(mFlicker, 4, 5, f);
	fwrite(mFlickerStep, 4, 5, f);
	fwrite(&mElement, 4, 1, f);
	fwrite(&mLevel, 4, 1, f);
	fwrite(&mF124, 4, 1, f);
}

// 0x404a50
void ElementAtom::Load(FILE* f)
{
	Atom::Load(f);
	fread(&mF_f0, 4, 1, f);
	fread(mFlicker, 4, 5, f);
	fread(mFlickerStep, 4, 5, f);
	fread(&mElement, 4, 1, f);
	fread(&mLevel, 4, 1, f);
	fread(&mF124, 4, 1, f);
}
