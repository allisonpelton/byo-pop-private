#include "FibRand.h"

using namespace Sexy;

// 0x431d30
FibRand::FibRand()
{
	// port: the original leaves the state to Seed
	mI = 0;
	mJ = 0;
	for (int i = 0; i < 55; i++)
		mState[i] = 0;
}

// 0x431d40
FibRand::~FibRand()
{
}

// 0x431d50
void FibRand::Seed(unsigned int theSeed)
{
	unsigned int* aState = mState;
	*aState = theSeed & 0x3fffffff;
	mI = 0;
	mJ = 0x1f;
	mState[1] = 1;
	int i = 0x35;
	do
	{
		aState[2] = aState[0] + aState[1] & 0x3fffffff;
		aState++;
		i--;
	} while (i != 0);
}

// 0x431da0
int FibRand::Next()
{
	int i = mI;
	int j = mJ;
	unsigned int aValue = mState[j] + mState[i] & 0x3fffffff;
	mState[i] = aValue;
	i++;
	if (i == 0x37)
		i = 0;
	j++;
	if (j == 0x37)
		j = 0;
	mJ = j;
	mI = i;
	return (int) aValue >> 6;
}

// 0x431e00
int FibRand::Random(int theRange, bool isSigned)
{
	int aMask = 2;
	if (2 < theRange)
	{
		do
		{
			aMask = aMask * 2;
		} while (aMask < theRange);
	}
	int aValue = (int) ((unsigned int) Next() & (aMask - 1U)) % theRange;
	if (isSigned == true)
	{
		if (Random(2, false) == 1)
			aValue = -aValue;
	}
	return aValue;
}
