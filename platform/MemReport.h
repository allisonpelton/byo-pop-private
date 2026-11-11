#ifndef __PLATFORM_MEMREPORT_H__
#define __PLATFORM_MEMREPORT_H__

// The memory report behind the "memreport" test command (docs/TEST_COMMANDS.md): what a game holds in memory, by kind,
// beside the process's own totals, so that a reader can split a game's memory into its libraries and devices (what the
// process held before it read a game file), its graphics, its sound and the rest, which is the game's state.
//
// A SexyAppFramework port needs nothing: SexyAppBase answers the command and fills the report from its images, its sounds
// and the mixer's songs (a game adds holders of its own in SexyAppBase::AddMemReportHolders). An engine of its own does
// three things: it calls NoteStartMemory() once its window and devices are up and before it reads a game file; in its
// own "memreport" command it fills a MemReport with what its holders keep (AddSongMemory() for the mixer's songs,
// mOthers for holders that are neither pictures nor sound); and it answers with LogMemReport(), so the line is the same
// for every game. Reading the numbers changes nothing in the game.

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace Platform
{

struct MemReport
{
	int mImageCount = 0;
	int mImageTexturedCount = 0;        // how many of them have a texture on the graphics device
	size_t mImagePixelBytes = 0;        // the pictures' own pixels and palettes
	size_t mImageCopyBytes = 0;         // second copies made from them: the blitters' alpha caches, high-resolution pixels
	size_t mImageTextureBytes = 0;      // their textures on the graphics device: an estimate, since the device's memory cannot be read
	int mSoundCount = 0;
	size_t mSoundBytes = 0;             // the sounds' decoded samples
	int mSongCount = 0;
	size_t mSongTrackBytes = 0;         // audio tracks the mixer keeps: an Ogg's file and decode buffer, a WAV's samples
	size_t mSongModuleFileBytes = 0;    // the module files loaded: their players keep a copy of their own, about that size or more
	std::vector<std::pair<std::string, size_t>> mOthers;    // an engine's or a game's own holders: a name and its bytes
};

// Records what the process holds now as its start figure. Call it once, when the libraries are loaded and the window,
// the graphics device and the sound device are up, and before a game file is read; a later call changes nothing.
void NoteStartMemory();

// The process's working set and private bytes now; false where the platform cannot tell (both are then 0). In a
// browser both are the size of the page's heap.
bool GetProcessMemory(size_t* theWorkingSet, size_t* thePrivateBytes);

// The private bytes NoteStartMemory() recorded, 0 when it was never called.
size_t GetStartPrivateBytes();

// Adds the mixer's songs (Platform::Mixer) to the report.
void AddSongMemory(MemReport* theReport);

// The report as one line of JSON, with the process's totals:
// {"images":{"count":n,"textured":n,"pixels":b,"copies":b,"textures":b},"sounds":{"count":n,"bytes":b},
//  "music":{"count":n,"tracks":b,"moduleFiles":b},"others":{"<name>":b,...},
//  "process":{"workingSet":b,"private":b,"privateAtStart":b}}   (every b in bytes)
std::string FormatMemReport(const MemReport& theReport);

// Answers the command: logs "MEMREPORT " and the line above.
void LogMemReport(const MemReport& theReport);

// The process's heaps in words, for a report written to a file: each heap's bytes in use, its blocks by size and its
// free bytes ("" where the platform cannot walk them).
std::string DescribeHeaps();

}

#endif
