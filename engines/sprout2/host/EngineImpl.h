#ifndef __SPROUT2_HOST_ENGINEIMPL_H__
#define __SPROUT2_HOST_ENGINEIMPL_H__

// The port's dxdriver::EngineImpl (its constructor 0x494e30 (feedingfrenzy2), 0x4e8 bytes): the engine's platform
// services over the framework's platform layer, for every game on the engine. The original's window, DirectDraw and
// DirectSound parts and its run loop (0x4383b0 (feedingfrenzy2)) belong to the game's app, which runs the frames;
// this class holds what does not depend on them: the game folder's files, the log and the clock.

#include "Engine.h"
#include "Stream.h"

#include <cstdio>

namespace engine
{

// A file of the game's folder (dxdriver::FileInputStream, vtable 0x5628b4 (feedingfrenzy2), 0x18 bytes: fread,
// fclose, fseek and ftell over fopen(path, "rb"), 0x40e0b0 (feedingfrenzy2))
class FileInputStream : public RandomAccessInputStream
{
public:
	explicit FileInputStream(FILE* theFile) : mFile(theFile) {}
	~FileInputStream() override;

	int read(void* theBuffer, int theSize) override;
	bool close() override;
	bool seek(int theOffset, int theOrigin) override;
	int tell() override;

private:
	FILE* mFile;
};

class EngineImpl : public Engine
{
public:
	EngineImpl(int theWidth, int theHeight);
	~EngineImpl() override;

	int getWidth() override { return mWidth; }
	int getHeight() override { return mHeight; }
	// 0x41c200 (feedingfrenzy2): a relative path is the game folder's
	bool diskFileExists(const std::string& thePath) override;
	// 0x40ad60 (feedingfrenzy2): null when the file cannot be opened
	RandomAccessInputStream* openDiskFile(const std::string& thePath) override;
	// 0x49a920 (feedingfrenzy2): the line goes to the debugger (and to a log file the original opens only for its
	// developers); the port's debugger is SDL's log, at the info level (the console, or stderr)
	void log(const char* theFormat, ...) override;
	// 0x4383a0 and 0x435ba0 (feedingfrenzy2): the original reads QueryPerformanceCounter, then the processor's time
	// stamp once it is calibrated; the port reads SDL's performance counter, or the frames' own clock under -fast
	uint64_t getTicks() override;
	uint64_t getTicksPerSecond() override;
	// 0x48da20 (feedingfrenzy2)
	void setFrameRate(int theFramesPerSecond) override { mFrameRate = theFramesPerSecond; }
	// The frame's period in ticks, 0 for none (the original keeps it at +0x160)
	uint64_t getFramePeriod() { return mFrameRate == 0 ? 0 : getTicksPerSecond() / (uint64_t)mFrameRate; }
	// 0x4a4a00 (feedingfrenzy2): PostQuitMessage; the app ends the run loop when it sees the request
	void quit() override { mQuitRequested = true; }
	// 0x48b200 (feedingfrenzy2): the parent is one of the port's keys
	PersistKey* openPersistKey(PersistKey* theParent, const std::string& theName) override;
	// 0x492cf0 (feedingfrenzy2): the port's registry is the framework's, one tree whatever the root, which holds the
	// application's key only (RegistryKey.h); a key outside it is not opened
	PersistKey* openPersistKey(int theRoot, const std::string& thePath) override;
	// The framework's key state (its widget manager's, kept from the window's messages; the original asks Windows for
	// the key as it is now)
	bool isKeyDown(int theKey) override;
	// The framework's clipboard (its text in Latin-1), false when it is empty
	bool getClipboardText(std::string& theText) override;

	// port: -fast: the clock is the app's, moved on by a frame's period each frame, as on origrun's virtual clock
	void setVirtualClock(bool theVirtualClock) { mVirtualClock = theVirtualClock; }
	void advanceClock(uint64_t theTicks) { mVirtualTicks += theTicks; }

	bool mQuitRequested;

private:
	int mWidth;
	int mHeight;
	int mFrameRate;   // +0x58
	bool mVirtualClock;
	uint64_t mVirtualTicks;
};

} // namespace engine

#endif
