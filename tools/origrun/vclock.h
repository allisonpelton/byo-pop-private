// origrun's virtual clock (--clock virtual): what the game's side (vclock.c, inside the dsound stand-in) and the host
// (origrun.c) share. See docs/ORIGRUN.md, "The virtual clock".
//
// The stand-in creates one named file mapping and two events per game process, named after the run's id (which
// origrun passes in ORIGRUN_CLOCK_ID) and the game's pid, so a game that starts itself again gets a clock of its own:
//   Local\origrun-vclock-<id>-<pid>           the VcShared block
//   Local\origrun-vclock-<id>-<pid>-held      auto-reset; the game sets it each time its main thread is held
//   Local\origrun-vclock-<id>-<pid>-release   auto-reset; origrun sets it to let a held game go on
// The game creates them, so origrun (the same user, or SYSTEM inside the sandbox) can always open them.
#ifndef ORIGRUN_VCLOCK_H
#define ORIGRUN_VCLOCK_H

#include <windows.h>

#define VC_MAGIC   0x4b4c4356u   // "VCLK"
#define VC_VERSION 2
#define VC_RING    16384         // watch changes the game can hand over before origrun drains them

// Virtual time is counted in 100 ns units (a FILETIME's), from the moment the clock was installed.
#define VC_UNITS_PER_MS 10000LL

// Where a hold was taken (VcShared.heldWhere)
enum { VC_AT_NONE, VC_AT_SLEEP, VC_AT_WAIT, VC_AT_MSGWAIT, VC_AT_POLL };
// The barrier's state (VcShared.bar), what an armed one waits for (holdKind), and why holds went off (barrierOffWhy)
enum { VC_BAR_FREE, VC_BAR_ARMED, VC_BAR_HELD };
enum { VC_HOLD_NEXT, VC_HOLD_UPDATE, VC_HOLD_TIME };
enum { VC_OFF_NONE, VC_OFF_CAP, VC_OFF_HOST, VC_OFF_FAILED };

typedef struct VcWatchRec {   // one change of a --watch value, as the game saw it
	LONG update;               // the update counter when it was read (-1: the counter could not be read)
	LONG idx;                  // which watch, 0-3
	LONG value;
	LONG ok;                   // 0: the value could not be read ("-")
} VcWatchRec;

typedef struct VcShared {
	DWORD magic, version, size;
	DWORD pid, driverTid;           // the game's process and the thread whose waits drive the clock
	volatile LONG installed;        // 1 once the import tables are patched
	volatile LONG connected;        // origrun sets 1 once it follows this clock (the first hold waits for it)
	volatile LONG gone;             // the game sets 1 as it exits

	volatile LONGLONG v;            // virtual time since install (100 ns); only the driver thread moves it
	volatile LONG counter;          // the update counter as last read inside the game
	volatile LONG counterOk;

	// The barrier. origrun writes holdKind and its operand, then sets bar to ARMED (and the release event). The game's
	// driver thread checks it as it enters a wait (and at its clock reads once it polls without waiting); where the
	// armed condition is met it claims the barrier by moving bar from ARMED to HELD, and stays held while bar is HELD.
	// origrun lets it go by setting bar to FREE (run free) or ARMED (on to the next barrier), and takes back a barrier the
	// game has not claimed by moving it from ARMED to FREE, so the game never holds after origrun gave up on a hold.
	volatile LONG bar;              // VC_BAR_*
	volatile LONG holdKind;         // VC_HOLD_NEXT: the next wait; _UPDATE: once the counter reached holdUpdate; _TIME:
	volatile LONG holdUpdate;       //   the wait that would take the clock to holdV or past it
	volatile LONGLONG holdV;
	volatile LONG barrierOff;       // the game set 1: it holds no more in this run (a hold ran past its cap, origrun
	volatile LONG barrierOffWhy;    //   ended, or a hold's wait failed: VC_OFF_*), at update barrierOffCounter
	volatile LONG barrierOffCounter;

	// The hold, written by the game
	volatile LONG held;             // 1 while the driver thread is held
	volatile LONG heldSeq;          // counts the holds
	volatile LONG heldCounter;      // the counter at the hold (-1 unreadable)
	volatile LONG heldWhere;        // VC_AT_*
	volatile LONGLONG heldV;        // the clock at the hold
	volatile LONG heldTimeout;      // the timeout (ms) of the wait the hold was taken in

	// Statistics
	volatile LONG waits;            // the driver's timed waits the clock skipped
	volatile LONG passWaits;        // the driver's waits passed to Windows (no timeout, a zero timeout, or done at once)
	volatile LONG polls;            // clock reads that moved the clock (the driver read it with no wait in between)
	volatile LONG maxReads;         // the most clock reads the driver made between two waits
	volatile LONG holds;
	volatile LONGLONG heldReal100;  // wall time spent held (100 ns)
	volatile LONGLONG installReal100;   // the wall clock (FILETIME) at install
	volatile LONGLONG nowReal100;   // the wall clock at the driver's last wait
	volatile LONG otherWaits;       // timed waits of other threads (on the wall clock, as before)
	volatile LONG realWaits;        // the driver's timed waits that took their real time, while another thread of the game worked
	volatile LONGLONG realWait100;  // the game time those waits took
	volatile LONG handleWaits;      // the driver's timed waits on handles, which Windows waits for real (not ready at once)
	volatile LONG handleTimeouts;   // how many of those ran out
	volatile LONGLONG handleWait100;   // the game time they took (their real time)

	// --watch values, read inside the game at each of the driver's waits and whenever the counter moves
	volatile LONG ringHead;         // records written (the game)
	volatile LONG ringTail;         // records drained (origrun)
	volatile LONG ringDropped;
	VcWatchRec ring[VC_RING];
} VcShared;

#endif
