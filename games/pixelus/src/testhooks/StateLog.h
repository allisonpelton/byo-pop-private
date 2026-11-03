#pragma once

// Test hook: -statelog (testhooks/Autoplay.cpp)

#include <cstdio>

namespace Sexy
{

class PixelusApp;

// -statelog=<file>: a line "<update> <name> <value>" each time one of the values changes ("-" while it cannot be read,
// as the original's memory watches show it). Pointers are logged as 0 or 1: what matters is that a screen exists.
struct StateLog
{
	enum { NUM_FIELDS = 40 };
	PixelusApp*	mApp;
	FILE*		mFile;
	int			mLast[NUM_FIELDS];

	StateLog(PixelusApp* theApp);
	~StateLog();
	void Update();
};

}
