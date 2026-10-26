#ifndef __NINGPO_HINPUT_H__
#define __NINGPO_HINPUT_H__

// HInput (0x426860..0x426ca0): a ring of 128 events that the window procedure (0x433290) fills and the loops of
// PGApp and PGDialog empty. When it is empty, getEvent() sleeps a millisecond and hands out a tick event instead.
// The original could also record the events to journal.bin and play back a demo file (the cvars pg_journal and
// pg_demoFile); port: neither. The port's host posts the events (src/NingPoApp.cpp) during its frame, which runs
// inside the sleep.

#include "Pg.h"

namespace pg
{

class HInput
{
public:
	PGEvent mQueue[128];   // +0x0000
	int mWrite;            // +0x1800
	int mRead;             // +0x1804
	int mMouseX;           // +0x1860 where the mouse was at the last move or left button event
	int mMouseY;           // +0x1864

	HInput();              // 0x426860

	// 0x426930: false when the ring is full; a move to where the mouse already was is dropped
	bool postEvent(const PGEvent& theEvent);
	// 0x426a50 HInput::getEvent: theWait sleeps whenever the ring is empty (otherwise only when the last event is
	// under a millisecond old)
	bool getEvent(PGEvent* theEvent, bool theWait);

private:
	// 0x426bc0: the next event, not taken from the ring
	bool peekEvent(PGEvent* theEvent);
	PyTime mLastTime;      // 0x4c5220 the time of the last event handed out
	int mLastMoveX;        // 0x4c5218
	int mLastMoveY;        // 0x4c521c
};

// The input the host posts to (0x413d24: the app's)
HInput* GetInput();
void SetInput(HInput* theInput);

} // namespace pg

#endif
