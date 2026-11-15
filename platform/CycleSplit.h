#ifndef __PLATFORM_CYCLESPLIT_H__
#define __PLATFORM_CYCLESPLIT_H__

// The cycle split behind -cyclesplit=<file> and the "cyclesplit" test command (docs/TEST_COMMANDS.md, "The cycle split"): where a
// run's processor time goes, split at the same boundaries as origrun's --count does in the original (docs/ORIGRUN.md, "Counting
// cycles inside the original's functions"), so that a port and its original are compared part by part and not only in total.
//
// The game loop's thread is counted by function, in cycles (QueryThreadCycleTime: the time the thread ran, not the time it waited):
//   update   SexyAppBase::DoUpdateFrames (the script, the widgets' updates, the music's update);
//   draw     SexyAppBase::DrawDirtyStuff, less the present it calls (the dirty scan, BeginFrame, DrawScreen);
//   present  SexyAppBase::Redraw (DDInterface::Present: SDL_RenderPresent and the cursor).
// A part's cycles are exclusive of the counted parts it calls, so a draw's present is not in its draw. Every other thread is listed
// with its cycles, its name (SetThreadDescription) and the module its start address is in; the main thread's remainder (events,
// waking from Sleep) is whatever its total has beyond the three parts. A snapshot also holds the process's cycles, which count
// the threads that ended too, so the parts always add up to the total.
//
// Off by default: every site is a test of one flag. On, a main-thread cycle reading at each boundary (a system call: two for each
// call of a part, about four an update for BookWorm; the file records how many were made and what one cost, 500 to 860 cycles in a
// tight loop but 2 to 5 thousand in the running game, so counting adds about 0.4 to 1 % to BookWorm's total: docs/TEST_COMMANDS.md,
// "What counting costs") and nothing else during the run: snapshots are taken in memory by the "cyclesplit <label>" command and
// written, with one more at exit, as JSON lines when the loop ends.
// Native only (Windows); the web build has no thread cycle counters and compiles every call out.

#include <string>

namespace Platform
{

enum CyclePart
{
	CYCLE_UPDATE,
	CYCLE_DRAW,
	CYCLE_PRESENT,
	CYCLE_NUM_PARTS
};

#ifdef _WIN32

extern bool gCycleSplitOn;      // set by Start(): the only thing a run without -cyclesplit looks at

// Starts counting; the file is written by Finish(). Call it on the game loop's thread, before the loop (the parameter parser does).
// False when it cannot (the log says why).
bool CycleSplitStart(const std::string& theFileName);
bool CycleSplitEnter(CyclePart thePart);    // false when this is not the loop's thread: the matching Leave is then skipped
void CycleSplitLeave();
// A snapshot of the counters, kept in memory (a thread listing and a few reads: a millisecond or two, on the loop's thread, with
// the open parts' time so far credited to them first, so that the snapshot falls exactly between two stretches)
void CycleSplitMark(const std::string& theLabel, int theUpdateCount);
// Stops the readings (the scopes then cost a test of one flag, as without the option) or starts them again, to see what counting
// costs: a run that alternates the two in blocks of updates compares them under the machine's own noise. False without Start().
bool CycleSplitPause(bool thePause);
// The last snapshot ("exit"), then the file. Once.
void CycleSplitFinish(int theUpdateCount);
// The cycles the calling thread has run (QueryThreadCycleTime), for a test's own count of one piece of work.
unsigned long long CurrentThreadCycles();

class CycleScope
{
public:
	explicit CycleScope(CyclePart thePart) : mActive(gCycleSplitOn && CycleSplitEnter(thePart)) {}
	~CycleScope() { if (mActive) CycleSplitLeave(); }
	CycleScope(const CycleScope&) = delete;
	CycleScope& operator=(const CycleScope&) = delete;
private:
	bool mActive;
};

#else

static const bool gCycleSplitOn = false;
inline bool CycleSplitStart(const std::string&) { return false; }
inline void CycleSplitMark(const std::string&, int) {}
inline bool CycleSplitPause(bool) { return false; }
inline void CycleSplitFinish(int) {}
inline unsigned long long CurrentThreadCycles() { return 0; }

class CycleScope
{
public:
	explicit CycleScope(CyclePart) {}
	~CycleScope() {}
};

#endif

}

#endif
