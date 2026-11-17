#ifndef __JB_SCREENLOG_H__
#define __JB_SCREENLOG_H__

// The screen's draws of a frame, kept as a list (port only: the original drew each one to the DirectDraw surface or the
// software buffer as it came, and every frame in full). Image::Draw, ScreenFillRect and ScreenFillRectAlpha add a
// command here between Display::BeginFrame and Display::Present instead of drawing; Present then draws the frame.
//
// A frame the engine redraws is one the previous frame's list differs from in a few commands (a tile lit, a label
// changed, a piece moving). Present matches the two lists command by command, in order (the longest common subsequence),
// takes the rects of the commands that have no match in either list, and executes the whole new list again over those
// rects alone, each command clipped to the rect. A pixel outside every one of those rects was reached by the same
// commands, in the same order, in both frames, so it holds the same colour: the frame is the full redraw's, to the last
// pixel (the matched commands draw the same image content: Image::mStamp changes with every change of an image's
// pixels). The rects are what Host::Present is told to show.
//
// This holds when every pixel of the frame is drawn by a command that overwrites it (the unkeyed 24-bit images at
// full opacity and the fills: the layers' backgrounds and bars), which the frame's commands are checked for; a frame
// whose pixels are some of the last frame's, darkened by a fade for one, is drawn in full, as it comes, and so is the
// frame after it. An image changed or freed while commands that use it are waiting (the engine never does it in a draw)
// draws what is waiting at that point, and the frame is shown in full. JB_VERIFY_FRAMES=1 in the environment makes every
// incremental frame be drawn in full as well and compared with the incremental one (the process aborts at a
// difference); JB_NO_SCREENLOG=1 draws every frame in full, one command at a time as the original did.

#include "JBBase.h"

#include <vector>

namespace jb
{

class Image;

struct ScreenCommand
{
	enum Kind : uint8_t
	{
		DRAW_IMAGE,  // Image::Draw: mX, mY the position, mRect the source, mScaleX, mScaleY, mAlpha
		FILL,        // ScreenFillRect: mRect the screen rect (clipped), mColour a COLORREF
		FILL_ALPHA   // ScreenFillRectAlpha: mRect the screen rect (clipped), mAlpha (0 to 255), mColour
	};
	Kind mKind = DRAW_IMAGE;
	bool mKeyed = false;  // the image's mKeyed when it was drawn
	int mX = 0;
	int mY = 0;
	Rect mRect;
	int mScaleX = 100;
	int mScaleY = 100;
	int mAlpha = 0xff;
	uint32_t mColour = 0;
	uint32_t mStamp = 0;  // the image's pixels
	Image* mImage = nullptr;
	Rect mBounds;         // the screen pixels the command changes (never empty)
	uint64_t mHash = 0;
};

namespace screenlog
{

struct Stats
{
	uint64_t mFrames = 0;       // frames presented
	uint64_t mIncremental = 0;  // drawn over the changed rects alone
	uint64_t mIdentical = 0;    // of those, frames without a change
	uint64_t mFull = 0;         // drawn in full
	uint64_t mAborted = 0;      // of those, frames that had to be flushed (an image changed under a waiting command)
	uint64_t mCommands = 0;     // commands of the frames (after the ones before the last full cover were left out)
	uint64_t mDirtyPixels = 0;  // pixels of the changed rects of the incremental frames
	uint64_t mVerified = 0;     // frames compared with a full redraw (JB_VERIFY_FRAMES)
	// what the frames in full followed (those of the last frame kept as it was, the start, a mode change)
	uint64_t mNoCover = 0;      // a frame with no command that covers the screen
	uint64_t mAfterNoCover = 0;
	uint64_t mAfterDirect = 0;  // a draw outside a frame
	uint64_t mAfterInvalidate = 0;
};

// true between Display::BeginFrame and Display::Present, when draws are kept
bool Recording();
// a frame starts (Display::BeginFrame)
void Begin();
// the commands (their images prepared by the caller); false when the draw has no effect
bool RecordDraw(Image* theImage, int theX, int theY, const Rect& theSrc, int theScaleX, int theScaleY, int theAlpha);
// theRect already clipped to the screen
void RecordFill(const Rect& theRect, uint32_t theColour);
void RecordFillAlpha(const Rect& theRect, int theAlpha, uint32_t theColour);
// a draw to the screen outside a frame: the screen is no longer the last frame's
void NoteDirectDraw();
// an image's pixels are about to be freed
void BeforePixelsFreed(Image* theImage);
// the frame's end (Display::Present): the frame is drawn, and theDirty holds the rects of the screen that differ from the
// last frame shown (empty when none does; the whole screen when it is drawn in full)
void End(std::vector<Rect>& theDirty);
// the screen is not the last frame's (a mode change, the surfaces released): the next frame is drawn in full
void Invalidate();
// the display is gone
void Shutdown();
// JB_VERIFY_FRAMES is set
bool Verifying();
// the bytes the frame lists and the shadow screen hold (the memory report, platform/MemReport.h)
size_t MemoryBytes();
const Stats& GetStats();

} // namespace screenlog

} // namespace jb

#endif
