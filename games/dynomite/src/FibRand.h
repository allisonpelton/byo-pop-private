#ifndef __FIBRAND_H__
#define __FIBRAND_H__

// The app's second generator at +0xf9c (vtable 0x4fc824, 0x431d30-0x431e60): an additive lagged Fibonacci
// generator of 55 30-bit numbers (lags 55 and 24), seeded by a Fibonacci run from the seed and 1.

namespace Sexy
{

class FibRand
{
public:
	int					mI;					// +4
	int					mJ;					// +8
	unsigned int		mState[55];			// +0xc

	FibRand();								// 0x431d30
	virtual ~FibRand();						// 0x431d40

	void				Seed(unsigned int theSeed);			// 0x431d50
	int					Next();								// 0x431da0: 24 bits
	// 0x431e00: 0 to theRange - 1 (the low bits under the next power of two, modulo theRange), negated half the
	// time when isSigned
	int					Random(int theRange, bool isSigned);
};

}

#endif
