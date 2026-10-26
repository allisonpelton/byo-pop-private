// Sexy::SwfMovie: a small player for Flash (.swf) movies, written from the public SWF file format specification.
// Bookworm Adventures plays its four story cutscenes through the Flash ActiveX control; the port cannot use that,
// so this plays what those four files use (vector shapes with solid and gradient fills and strokes, morph shapes,
// text from embedded fonts, sprites, clip layers, color transforms and the actions Play, Stop and GetURL) and
// draws it into a 32-bit pixel buffer. Nothing here is specific to one game. What a file holds that this does not
// play is counted, not played: GetUnsupported lists it.
//
// A movie is played by calling NextFrame at its frame rate and Render whenever the picture is wanted. The movie
// tells its host what it wants through GetURL actions whose URL starts with "FSCommand:" (the Flash control's
// FSCommand event): the listener hears the command, and the host decides what to do. The game's four cutscenes
// send "ready" in frame 0, where they also Play, so they need nothing from the host to start, and "done" in their
// last frame, where they Stop.
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>

namespace Sexy {

class SwfListener {
public:
	virtual ~SwfListener() {}
	// A GetURL action with an "FSCommand:" URL: theCommand is what follows the colon ("ready", "done"), theArgs the target
	virtual void SwfCommand(const std::string& theCommand, const std::string& theArgs) = 0;
};

class SwfMovie {
public:
	SwfListener* mListener = nullptr;

	SwfMovie();
	~SwfMovie();

	// Reads a movie through Platform::VFS (a path in the game's folder, "images\\cutscenes\\intro.swf"). Replaces
	// what the object held; false (and GetError) when the file cannot be read or is not one this player can play.
	bool Load(const std::string& theGamePath);
	const std::string& GetError() const;
	int GetWidth() const;   // pixels
	int GetHeight() const;
	float GetFrameRate() const;
	int GetFrameCount() const;
	int GetCurrentFrame() const;  // of the root timeline, 0-based
	bool IsPlaying() const;
	void Play();
	void Stop();
	void GotoFrame(int theFrame);  // and stays playing or stopped; the frame's actions run (and an FSCommand is told) at once

	// One frame tick, as the Flash player does it: the root timeline (when playing) and every sprite instance (each
	// one its own timeline, playing unless a Stop stopped it) advance one frame, the frame's tags are applied and its
	// actions run (FSCommands go to mListener, when the tick is done). The first call shows frame 0. A timeline that
	// reaches its end plays on from its first frame, and that rebuilds what it shows from frame 0, except for the
	// objects it placed in frame 0 itself, which keep playing.
	void NextFrame();

	// Draws the current frame, scaled to theWidth x theHeight, into theBits (0xAARRGGBB, not premultiplied,
	// theStride pixels per row), every pixel opaque: the background color first. The movie keeps the picture it drew
	// last and, when the size is the same, draws only where the frame differs from it (the picture is the same as
	// if all of it were drawn, and a frame that did not change costs a copy), so call it for every frame shown.
	void Render(uint32_t* theBits, int theWidth, int theHeight, int theStride);
	// Makes the next Render draw everything again
	void Invalidate();
	// The rectangle that the last Render drew over, in pixels of its target: what differs from the picture the
	// call before gave (all of it for the first). False when nothing differs. For a host that uploads only that part.
	bool GetDamage(int& theX, int& theY, int& theWidth, int& theHeight) const;

	// What the file holds that is not played, by name (a tag, an action, a feature) with how often: empty for the
	// game's four cutscenes
	const std::map<std::string, int>& GetUnsupported() const;

	// The display list as text, one object a line, sprites with their own lists indented below them: for tests
	std::string DumpDisplayList() const;

private:
	struct Impl;
	std::unique_ptr<Impl> mImpl;
};

}  // namespace Sexy
