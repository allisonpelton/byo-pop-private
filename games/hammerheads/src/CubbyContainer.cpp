#include "CubbyContainer.h"

#include <cstdio>

#include "Common.h"
#include "Cubby.h"

using namespace Sexy;

// 0x4347c0
Cubby* CubbyContainer::GetCubby(int theIndex) const
{
	return dynamic_cast<Cubby*>(mCubbies[theIndex].Get());
}

// 0x434be0
Cubby* CubbyContainer::AddCubby(int theX, int theY, const std::string& theName)
{
	// 0x4333c0: the variant is drawn before the constructor draws its own wait
	Cubby* aCubby = new Cubby((double)theX, (double)theY, Rand() % 6, GetBackdrop());
	aCubby->SetName(theName);
	mCubbies.push_back(ObjectRef(aCubby));
	return aCubby;
}

// 0x4015d0 (its format is "%s_%05d" with the type's name, as the objects' own made-up names)
std::string Sexy::MakeCubbyName(int theNumber)
{
	char aSuffix[16];
	snprintf(aSuffix, sizeof(aSuffix), "_%05d", theNumber);
	return std::string("class Sexy::Cubby") + aSuffix;
}
