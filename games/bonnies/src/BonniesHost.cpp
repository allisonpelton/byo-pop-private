// port: the host for Bonnie's Bookstore (see BonniesHost.h): bonnies::Host over the framework and the platform layer.

#include "BonniesHost.h"
#include "platform/GameOptions.h"
#include "../game/Start.h"
#include "../engine/Host.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "WidgetManager.h"
#include "WebSettings.h"
#include "platform/Audio.h"
#include "platform/ImageLoader.h"
#include "platform/MemReport.h"
#include "platform/MusicLevels.h"
#include "platform/VFS.h"
#include "SDLSoundManager.h"
#ifndef __EMSCRIPTEN__
#include "SurfaceDisplay.h"
#endif

#include <SDL.h>
#include <stb_image.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <deque>
#include <memory>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

// The game's sleep yields to the browser (ASYNCIFY). A page that runs hidden can set Module.bonniesSetTimeout to a
// timer of its own: a hidden tab slows setTimeout down to once a second.
EM_ASYNC_JS(void, bonnies_sleep, (int theMs), {
	await new Promise(function(theResolve) { (Module['bonniesSetTimeout'] || setTimeout)(theResolve, theMs); });
});
#endif

// port (mod): the fixes mod's "steadyClock" is on (BonniesHostApp::DoMainLoop)
static bool gSteadyClock = false;

// The clock the game reads and its sleeps run to: the framework's (real time, or the updates' in a -fast run), or with
// "steadyClock" the updates' in real time
static uint32_t GameClock();
// pumpEvents calls since the framework last ran: with "steadyClock" a game loop that spins on pumpEvents and the clock (the
// engine's waitUntil when 3 ms or less are left) would wait for good, so after kSpin of them the framework runs one update
static int gSpin = 0;
static const int kSpin = 1000;

using namespace Sexy;

BonniesHostApp* Sexy::gHostApp = nullptr;

namespace bonnies
{

static Host* gHost = nullptr;

void setHost(Host* theHost)
{
	gHost = theHost;
}

Host* host()
{
	return gHost;
}

} // namespace bonnies

namespace
{

const int kScreenWidth = 800;
const int kScreenHeight = 600;
const int kChannels = 64;  // FMOD 3 was opened with 64 (docs/ENGINE.md, Sound)

// The game's text is Latin-1, as the original's ANSI file names are on Windows; the browser's file system takes UTF-8,
// so on the web the names of players with an accented letter are written to it and listed from it as
// UTF-8. Natively the bytes go as they are, as the original's do.
std::string FileName(const std::string& theText)
{
#ifdef __EMSCRIPTEN__
	std::string aName;
	for (char c : theText)
	{
		unsigned char u = (unsigned char)c;
		if (u < 0x80)
			aName += c;
		else
		{
			aName += (char)(0xc0 | (u >> 6));
			aName += (char)(0x80 | (u & 0x3f));
		}
	}
	return aName;
#else
	return theText;
#endif
}

std::string GameText(const std::string& theName)
{
#ifdef __EMSCRIPTEN__
	std::string aText;
	for (size_t i = 0; i < theName.size(); i++)
	{
		unsigned char u = (unsigned char)theName[i];
		if (u < 0x80)
			aText += (char)u;
		else if ((u & 0xe0) == 0xc0 && i + 1 < theName.size())
		{
			int aChar = ((u & 0x1f) << 6) | ((unsigned char)theName[++i] & 0x3f);
			aText += aChar < 0x100 ? (char)aChar : '?';
		}
		else
		{
			// a character beyond Latin-1: its continuation bytes
			while (i + 1 < theName.size() && ((unsigned char)theName[i + 1] & 0xc0) == 0x80)
				i++;
			aText += '?';
		}
	}
	return aText;
#else
	return theName;
#endif
}

// A path of the game's ("SavedGames/x.gam") in the user's folder, its folders made when theCreate
std::string UserPath(const std::string& thePath, bool theCreate)
{
	const std::string& aDir = Platform::VFS::Get().GetUserDir();
	std::string aRel = Platform::VFS::Normalize(FileName(thePath));
	std::string aFound = Platform::VFS::FindCaseInsensitive(aDir, aRel);
	if (!aFound.empty() || !theCreate)
		return aFound;
	std::string aPath = aDir + "/" + aRel;
	size_t aSlash = aPath.find_last_of('/');
	if (aSlash != std::string::npos)
		Platform::MakeDirs(aPath.substr(0, aSlash));
	return aPath;
}

std::string Lower(std::string theText)
{
	for (char& c : theText)
		if (c >= 'A' && c <= 'Z')
			c = (char)(c - 'A' + 'a');
	return theText;
}

// FMOD 3's channel: the sample it plays, made paused (FSOUND_PlaySoundEx) and started by FSOUND_SetPaused
struct Channel
{
	std::shared_ptr<Platform::SoundData> mData;
	int mVoice = -1;
	bool mPaused = false;
	float mGain = 1.0f;
	int mFrequency = 0;
	int mSfxID = 0;   // port: the sample's number in the sound log (BYOPOP_SOUNDLOG), as the framework's sounds have
	int mLogNum = 0;  // port: this play's number in the log
};

// FMOD 3's sample: the decoded sound
struct SampleHandle
{
	std::shared_ptr<Platform::SoundData> mData;
	int mSfxID = 0;   // port: the number the sound log gives the sample
};

// The framework's side of bonnies::Host
class BonniesHostImpl : public bonnies::Host
{
public:
	std::deque<bonnies::HostEvent> mEvents;
	unsigned char mKeys[bonnies::SDLK_LAST] = {};
	int mMouseX = 0;
	int mMouseY = 0;
	unsigned char mButtons = 0;
	bool mKeyRepeat = false;
	std::vector<uint32_t> mScreen;
	Channel mChannels[kChannels];
	MemoryImage* mCursorImage = nullptr;
	bool mCursorShown = true;

	BonniesHostImpl() : mScreen(kScreenWidth * kScreenHeight, 0) {}

	void post(const bonnies::HostEvent& theEvent)
	{
		if (gHostApp->mGameRunning && mEvents.size() < 4096)
			mEvents.push_back(theEvent);
	}

	uint32_t getTicks() override
	{
		// port (mod): the fixes mod's "steadyClock": the game's clock runs on the framework's updates, 10 ms each, not on
		// the machine's, so the same seed and the same input play the same game on every run (a -fast run is steady
		// already: its clock is the updates')
		return GameClock();
	}

	// a test run (hidden, on rundesk's desktop) has the focus, as the user's window would, until a script's "focus 0";
	// a -realinput run has only the focus and the mouse its file's events give (docs/REAL_INPUT.md)
	unsigned char getAppState() override
	{
		unsigned char aState = 0;
		if (!gHostApp->mMinimized)
			aState |= bonnies::SDL_APPACTIVE;
		bool aTestFocus = gHostApp->IsTestRun() && !gHostApp->mScriptNoFocus && gHostApp->mRealInputFileName.empty();
		if ((gHostApp->mHasFocus && !gHostApp->mScriptNoFocus) || aTestFocus)
			aState |= bonnies::SDL_APPINPUTFOCUS;
		if ((gHostApp->mMouseInside && !gHostApp->mScriptNoFocus) || aTestFocus)
			aState |= bonnies::SDL_APPMOUSEFOCUS;
		return aState;
	}

	unsigned char getMouseState(int* theX, int* theY) override
	{
		if (theX != nullptr)
			*theX = mMouseX;
		if (theY != nullptr)
			*theY = mMouseY;
		return mButtons;
	}

	const unsigned char* getKeyState() override
	{
		return mKeys;
	}

	bool pollEvent(bonnies::HostEvent& theEvent) override
	{
		if (mEvents.empty())
			return false;
		theEvent = mEvents.front();
		mEvents.pop_front();
		return true;
	}

	// The framework runs while the game is busy (loading) too, so the window stays responsive. Unfocused, the game waits 2 ms a frame in a loop of pumps (0x420770) that never sleeps; a -fast run's clock moves
	// only when the framework runs, so while a script, or a -realinput run's own focus loss, has taken the focus away a
	// pump runs it once
	void pumpEvents() override
	{
		// (with "steadyClock" as in a -fast run: the framework runs only in the game's sleeps, never at a moment the
		// machine's clock picks, so the updates a frame spans stay the same)
		bool aSteady = gHostApp->mFastMode || gSteadyClock;
		if ((!aSteady && (int)(gHostApp->GetClockTime() - gHostApp->mLastFrameworkRun) >= 100) ||
			(aSteady && (gHostApp->mScriptNoFocus || (!gHostApp->mRealInputFileName.empty() && !gHostApp->mHasFocus))))
		{
			gHostApp->RunFramework(0);
		}
		else if (gSteadyClock && ++gSpin >= kSpin)
		{
			// (at most half a second of real time, for a framework that does not update, paused or minimized)
			int aCount = gHostApp->mUpdateCount;
			uint32_t aGiveUp = SDL_GetTicks() + 500;
			while (gHostApp->mUpdateCount == aCount && !gHostApp->mShutdown && (int)(SDL_GetTicks() - aGiveUp) < 0)
				gHostApp->RunFramework(0);
		}
	}

	void enableKeyRepeat(int theDelay, int theInterval) override
	{
		mKeyRepeat = theDelay != 0;
		(void)theInterval;
	}

	uint32_t* getScreenPixels() override
	{
		return mScreen.data();
	}

	void updateRects(int theCount, const int16_t* theRects) override
	{
		(void)theCount;
		(void)theRects;
		if (gHostApp->mWidget != nullptr)
			gHostApp->mWidget->mScreenChanged = true;
	}

	void setFullScreen(bool theFullScreen) override
	{
		gHostApp->SetGameFullscreen(theFullScreen);
	}

	void setCaption(const std::string& theCaption) override
	{
		gHostApp->mTitle = StringToSexyStringFast(theCaption);
		if (gHostApp->mHWnd != nullptr)
			SDL_SetWindowTitle(gHostApp->mHWnd, theCaption.c_str());
	}

	// SDL 1.2's 32x32 cursor (a set mask bit: black where the data bit is set, else white; a clear one: clear, or
	// inverted where the data bit is set, drawn black). On the desktop it is the window's own cursor, as SDL 1.2's was
	// (BonniesHostApp::EnforceCursor shows it), so that the window's display has nothing to draw over the game's screen. On
	// the web it is the framework's image cursor, which the framework centres on the mouse, so the cursor sits in a 64x64
	// image with its hot spot in the middle.
	void setCursor(const uint8_t* theData, const uint8_t* theMask, int theHotX, int theHotY) override
	{
#ifndef __EMSCRIPTEN__
		gHostApp->SetWindowCursor(SDL_CreateCursor(theData, theMask, 32, 32, theHotX, theHotY));
		gHostApp->SetCursor(mCursorShown ? CURSOR_CUSTOM : CURSOR_NONE);
		return;
#else
		if (mCursorImage == nullptr)
		{
			mCursorImage = new MemoryImage(gHostApp);
			mCursorImage->Create(64, 64);
		}
		ulong* aBits = mCursorImage->GetBits();
		std::fill(aBits, aBits + 64 * 64, 0);
		for (int y = 0; y < 32; y++)
		{
			for (int x = 0; x < 32; x++)
			{
				int aByte = y * 4 + x / 8;
				int aBit = 0x80 >> (x & 7);
				bool aData = (theData[aByte] & aBit) != 0;
				bool aMask = (theMask[aByte] & aBit) != 0;
				int aX = x + 32 - theHotX;
				int aY = y + 32 - theHotY;
				if (aX < 0 || aY < 0 || aX >= 64 || aY >= 64)
					continue;
				if (aMask)
					aBits[aY * 64 + aX] = aData ? 0xff000000 : 0xffffffff;
				else if (aData)
					aBits[aY * 64 + aX] = 0xff000000;
			}
		}
		mCursorImage->BitsChanged();
		gHostApp->SetCursorImage(CURSOR_CUSTOM, mCursorImage);
		gHostApp->SetCursor(mCursorShown ? CURSOR_CUSTOM : CURSOR_NONE);
#endif
	}

	void showCursor(bool theShow) override
	{
		mCursorShown = theShow;
#ifndef __EMSCRIPTEN__
		bool aHasCursor = gHostApp->mWindowCursor != nullptr;
#else
		bool aHasCursor = mCursorImage != nullptr;
#endif
		gHostApp->SetCursor(!theShow ? CURSOR_NONE : aHasCursor ? CURSOR_CUSTOM : CURSOR_POINTER);
	}

	void iconify() override
	{
		if (gHostApp->mHWnd != nullptr && !gHostApp->IsTestRun())
			SDL_MinimizeWindow(gHostApp->mHWnd);
	}

	bool isIconic() override
	{
		return gHostApp->mMinimized;
	}

	void moveWindow(int theX, int theY) override
	{
#ifndef __EMSCRIPTEN__
		if (gHostApp->mHWnd != nullptr && gHostApp->mIsWindowed && !gHostApp->IsTestRun())
			SDL_SetWindowPosition(gHostApp->mHWnd, theX, theY);
#else
		(void)theX;
		(void)theY;
#endif
	}

	bool getCursorPos(int* theX, int* theY) override
	{
#ifndef __EMSCRIPTEN__
		// a -realinput run's mouse is where its events left it, not the real cursor (docs/REAL_INPUT.md)
		if (!gHostApp->mRealInputFileName.empty())
		{
			*theX = mMouseX;
			*theY = mMouseY;
			return true;
		}
		SDL_GetGlobalMouseState(theX, theY);
		return true;
#else
		*theX = mMouseX;
		*theY = mMouseY;
		return true;
#endif
	}

	// a test run sees the same desktop on every machine
	void getDesktop(int* theWidth, int* theHeight, int* theBits) override
	{
		*theWidth = 1920;
		*theHeight = 1080;
		*theBits = 32;
#ifndef __EMSCRIPTEN__
		SDL_DisplayMode aMode;
		if (!gHostApp->IsTestRun() && SDL_GetDesktopDisplayMode(0, &aMode) == 0)
		{
			*theWidth = aMode.w;
			*theHeight = aMode.h;
		}
#endif
	}

	int doubleClickTime() override
	{
		return 500;
	}

	void sleep(int theMs) override
	{
		gHostApp->RunFramework(theMs);
	}

	bool decodeImage(const std::vector<uint8_t>& theData, bonnies::HostSurface& theSurface) override
	{
		int aWidth = 0;
		int aHeight = 0;
		int aComponents = 0;
		if (!stbi_info_from_memory(theData.data(), (int)theData.size(), &aWidth, &aHeight, &aComponents))
			return false;
		int aWanted = aComponents == 4 || aComponents == 2 ? 4 : 3;
		// a JPEG through libjpeg 6b, as SDL_image decodes it in the original, every other image through stb_image
		uint8_t* aPixels = Platform::LoadImageFromMemory(theData.data(), (int)theData.size(), &aWidth, &aHeight, &aComponents,
			aWanted);
		if (aPixels == nullptr)
			return false;
		theSurface.mWidth = aWidth;
		theSurface.mHeight = aHeight;
		theSurface.mBytesPerPixel = aWanted;
		theSurface.mPitch = aWidth * aWanted;
		theSurface.mPixels.assign(aPixels, aPixels + (size_t)aWidth * aHeight * aWanted);
		stbi_image_free(aPixels);
		return true;
	}

	bool readGameFile(const std::string& thePath, std::vector<uint8_t>& theData) override
	{
		return Platform::VFS::Get().ReadAll(thePath, theData);
	}

	bool gameFileExists(const std::string& thePath) override
	{
		return Platform::VFS::Get().Exists(thePath);
	}

	bool readUserFile(const std::string& thePath, std::vector<uint8_t>& theData) override
	{
		std::string aPath = UserPath(thePath, false);
		if (aPath.empty())
			return false;
		FILE* aFile = fopen(aPath.c_str(), "rb");
		if (aFile == nullptr)
			return false;
		theData.clear();
		uint8_t aBuffer[65536];
		size_t aRead;
		while ((aRead = fread(aBuffer, 1, sizeof(aBuffer), aFile)) > 0)
			theData.insert(theData.end(), aBuffer, aBuffer + aRead);
		fclose(aFile);
		return true;
	}

	bool writeUserFile(const std::string& thePath, const std::vector<uint8_t>& theData) override
	{
		std::string aPath = UserPath(thePath, true);
		FILE* aFile = fopen(aPath.c_str(), "wb");
		if (aFile == nullptr)
			return false;
		bool anOk = theData.empty() || fwrite(theData.data(), 1, theData.size(), aFile) == theData.size();
		fclose(aFile);
		gHostApp->UpdateUserDataBackup(true);
		return anOk;
	}

	bool userFileExists(const std::string& thePath) override
	{
		std::string aPath = UserPath(thePath, false);
		return !aPath.empty() && Platform::IsFile(aPath);
	}

	bool removeUserFile(const std::string& thePath) override
	{
		std::string aPath = UserPath(thePath, false);
		if (aPath.empty())
			return false;
		bool anOk = remove(aPath.c_str()) == 0;
		gHostApp->UpdateUserDataBackup(true);
		return anOk;
	}

	// The folder's files in the user's folder, then the game's that the user's has not
	std::vector<std::string> listFiles(const std::string& theDir) override
	{
		std::vector<std::string> aNames;
		std::vector<std::string> aLower;
		std::string aRel = Platform::VFS::Normalize(theDir);
		while (!aRel.empty() && aRel.back() == '/')
			aRel.pop_back();
		std::string aRoots[2] = {Platform::VFS::Get().GetUserDir(), Platform::VFS::Get().GetBaseDir()};
		for (const std::string& aRoot : aRoots)
		{
			// the folder's own name in any case (VFS::FindCaseInsensitive finds files only)
			std::string aDir = aRoot;
			size_t aStart = 0;
			while (!aRel.empty() && !aDir.empty() && aStart <= aRel.size())
			{
				size_t anEnd = aRel.find('/', aStart);
				if (anEnd == std::string::npos)
					anEnd = aRel.size();
				std::string aPart = Lower(aRel.substr(aStart, anEnd - aStart));
				std::string aNext;
				for (const std::string& aName : Platform::ListDirectory(aDir))
					if (Lower(aName) == aPart && Platform::IsDirectory(aDir + "/" + aName))
					{
						aNext = aDir + "/" + aName;
						break;
					}
				aDir = aNext;
				aStart = anEnd + 1;
			}
			if (aDir.empty() || !Platform::IsDirectory(aDir))
				continue;
			std::vector<std::string> aFound = Platform::ListDirectory(aDir);
			std::sort(aFound.begin(), aFound.end(), [](const std::string& a, const std::string& b)
				{ return Lower(a) < Lower(b); });
			for (const std::string& aName : aFound)
			{
				if (!Platform::IsFile(aDir + "/" + aName))
					continue;
				std::string aKey = Lower(aName);
				if (std::find(aLower.begin(), aLower.end(), aKey) != aLower.end())
					continue;
				aLower.push_back(aKey);
				aNames.push_back(GameText(aName));
			}
		}
		return aNames;
	}

	// a test run types the same name on every machine
	std::string userName() override
	{
		if (gHostApp->IsTestRun())
			return "";
#if defined(__EMSCRIPTEN__)
		return "";
#elif defined(_WIN32)
		const char* aName = SDL_getenv("USERNAME");
		return aName != nullptr ? aName : "";
#else
		const char* aName = SDL_getenv("USER");
		return aName != nullptr ? aName : "";
#endif
	}

	std::string getClipboard() override
	{
		return SexyStringToStringFast(gHostApp->GetClipboard());
	}

	void setClipboard(const std::string& theText) override
	{
		if (!gHostApp->IsTestRun())
			SDL_SetClipboardText(theText.c_str());
	}

	void trace(const char* theText) override
	{
		if (gHostApp->mTraceMode)
			SDL_Log("%s", theText);
	}

	// FMOD 3 on the platform's mixer

	// port: BYOPOP_SOUNDLOG, the sound log the framework's sounds write (Sexy::SoundLog, framework/SDLSoundManager.h),
	// in the same lines: "load <id> <file> frames= rate= ch=" for a sample, "instance <id> #<n>" when a channel takes it,
	// "play <id> #<n> vol= pan= pitch= loop=" when FSOUND_SetPaused starts it, "volume", "pitch" and "stop" lines for the
	// channel after that. FMOD 3's volume (0-255) is a straight gain, logged as DirectSound's hundredths of a decibel,
	// which the framework's volume is; it has no pan (always 0) and the game plays no loop. A sample's number is never
	// reused, so a file's name in the log stays unambiguous.
	int mNextSfxID = 0;
	int mNextLogNum = 0;
	// port (the memory report): the samples alive and their decoded bytes
	int mSampleCount = 0;
	size_t mSampleBytes = 0;

	static int LogVolume(float theGain)
	{
		if (!(theGain > 0.0f))
			return -10000;
		return std::max(-10000, (int)std::lround(2000.0 * std::log10((double)theGain)));
	}

	void* loadSample(const std::string& thePath) override
	{
		std::shared_ptr<Platform::SoundData> aData = Platform::Mixer::Get().LoadSound(thePath);
		if (aData == nullptr)
			return nullptr;
		SampleHandle* aSample = new SampleHandle();
		aSample->mData = aData;
		aSample->mSfxID = mNextSfxID++;
		mSampleCount++;
		mSampleBytes += aData->Bytes();
		SoundLog("load %d %s frames=%d rate=%d ch=%d", aSample->mSfxID, thePath.c_str(), (int)aData->mFrames, aData->mRate,
			aData->mChannels);
		return aSample;
	}

	void freeSample(void* theSample) override
	{
		SampleHandle* aSample = static_cast<SampleHandle*>(theSample);
		if (aSample == nullptr)
			return;
		for (Channel& aChannel : mChannels)
		{
			if (aChannel.mData == aSample->mData)
			{
				if (aChannel.mVoice >= 0)
					stopVoice(aChannel);
				aChannel = Channel();
			}
		}
		mSampleCount--;
		mSampleBytes -= aSample->mData->Bytes();
		delete aSample;
	}

	// the voice of a channel stopped (the sound log's "stop": the framework logs one whenever a started sound is stopped,
	// also when it had ended by itself)
	void stopVoice(const Channel& theChannel)
	{
		SoundLog("stop %d #%d", theChannel.mSfxID, theChannel.mLogNum);
		Platform::Mixer::Get().StopVoice(theChannel.mVoice);
	}

	bool channelBusy(const Channel& theChannel)
	{
		if (theChannel.mData == nullptr)
			return false;
		if (theChannel.mPaused)
			return true;
		return theChannel.mVoice >= 0 && Platform::Mixer::Get().IsVoicePlaying(theChannel.mVoice);
	}

	// FSOUND_FREE: the first channel not playing
	int playSample(void* theSample) override
	{
		SampleHandle* aSample = static_cast<SampleHandle*>(theSample);
		if (aSample == nullptr)
			return -1;
		for (int i = 0; i < kChannels; i++)
		{
			if (!channelBusy(mChannels[i]))
			{
				Channel& aChannel = mChannels[i];
				aChannel = Channel();
				aChannel.mData = aSample->mData;
				aChannel.mPaused = true;
				aChannel.mFrequency = aSample->mData->mRate;
				aChannel.mSfxID = aSample->mSfxID;
				aChannel.mLogNum = ++mNextLogNum;
				SoundLog("instance %d #%d", aChannel.mSfxID, aChannel.mLogNum);
				return i;
			}
		}
		return -1;
	}

	Channel* channel(int theChannel)
	{
		if (theChannel < 0 || theChannel >= kChannels || mChannels[theChannel].mData == nullptr)
			return nullptr;
		return &mChannels[theChannel];
	}

	int getFrequency(int theChannel) override
	{
		Channel* aChannel = channel(theChannel);
		return aChannel != nullptr ? aChannel->mFrequency : 0;
	}

	double pitch(const Channel& theChannel)
	{
		return theChannel.mData->mRate > 0 ? (double)theChannel.mFrequency / theChannel.mData->mRate : 1.0;
	}

	void setFrequency(int theChannel, int theFrequency) override
	{
		Channel* aChannel = channel(theChannel);
		if (aChannel == nullptr)
			return;
		aChannel->mFrequency = theFrequency;
		SoundLog("pitch %d #%d %.4f", aChannel->mSfxID, aChannel->mLogNum, pitch(*aChannel));
		if (aChannel->mVoice >= 0)
			Platform::Mixer::Get().SetVoicePitch(aChannel->mVoice, pitch(*aChannel));
	}

	void setVolume(int theChannel, int theVolume) override
	{
		Channel* aChannel = channel(theChannel);
		if (aChannel == nullptr)
			return;
		aChannel->mGain = std::max(0, std::min(255, theVolume)) / 255.0f;
		if (SoundLogOn())
			SoundLog("volume %d #%d %d", aChannel->mSfxID, aChannel->mLogNum, LogVolume(aChannel->mGain));
		if (aChannel->mVoice >= 0)
			Platform::Mixer::Get().SetVoiceGain(aChannel->mVoice, aChannel->mGain, aChannel->mGain);
	}

	void setPaused(int theChannel, bool thePaused) override
	{
		Channel* aChannel = channel(theChannel);
		if (aChannel == nullptr || thePaused || !aChannel->mPaused)
			return;
		aChannel->mPaused = false;
		if (SoundLogOn())
			SoundLog("play %d #%d vol=%d pan=0 pitch=%.4f loop=0", aChannel->mSfxID, aChannel->mLogNum,
				LogVolume(aChannel->mGain), pitch(*aChannel));
		aChannel->mVoice = Platform::Mixer::Get().StartVoice(aChannel->mData, aChannel->mGain, aChannel->mGain,
			pitch(*aChannel), false);
	}

	void stopSound(int theChannel) override
	{
		Channel* aChannel = channel(theChannel);
		if (aChannel == nullptr)
			return;
		if (aChannel->mVoice >= 0)
			stopVoice(*aChannel);
		*aChannel = Channel();
	}

	bool isPlaying(int theChannel) override
	{
		Channel* aChannel = channel(theChannel);
		return aChannel != nullptr && channelBusy(*aChannel);
	}

	static int song(void* theSong)
	{
		return (int)(intptr_t)theSong;
	}

	void* loadSong(const std::string& thePath) override
	{
		int aSong = Platform::Mixer::Get().LoadSong(thePath);
		// libopenmpt mixes an XM's channels at its sample pre-amp of 48 (soundlib/Sndfile.cpp; MixLevels::Compatible,
		// Sndmix.cpp), FMOD 3 at the full 256: the original's menu music, recorded through origrun, is 5.3 times as
		// loud as the port's was, its sounds as loud
		if (aSong > 0)
			Platform::Mixer::Get().SetSongAmplify(aSong, (float)BYOPOP_FMOD3_XM_GAIN);
		return aSong > 0 ? (void*)(intptr_t)aSong : nullptr;
	}

	void freeSong(void* theSong) override
	{
		if (theSong != nullptr)
			Platform::Mixer::Get().UnloadSong(song(theSong));
	}

	// FMUSIC_PlaySong: from the start, looping (FMUSIC_SetLooping's default)
	void playSong(void* theSong) override
	{
		if (theSong != nullptr)
			Platform::Mixer::Get().PlaySong(song(theSong), 0, 0, true);
	}

	void stopSong(void* theSong) override
	{
		if (theSong != nullptr)
			Platform::Mixer::Get().StopSong(song(theSong));
	}

	void setSongVolume(void* theSong, int theVolume) override
	{
		if (theSong != nullptr)
			Platform::Mixer::Get().SetSongGain(song(theSong), std::max(0, std::min(256, theVolume)) / 256.0f);
	}

	void setSongOrder(void* theSong, int theOrder) override
	{
		if (theSong != nullptr)
			Platform::Mixer::Get().PlaySong(song(theSong), theOrder, 0, true);
	}

	int getSongOrder(void* theSong) override
	{
		int anOrder = 0;
		int aRow = 0;
		if (theSong == nullptr || !Platform::Mixer::Get().GetSongPosition(song(theSong), anOrder, aRow))
			return 0;
		return anOrder;
	}

	bool isSongPlaying(void* theSong) override
	{
		return theSong != nullptr && Platform::Mixer::Get().IsSongPlaying(song(theSong));
	}

	void setSongPaused(void* theSong, bool thePaused) override
	{
		if (theSong != nullptr)
			Platform::Mixer::Get().PauseSong(song(theSong), thePaused);
	}
};

BonniesHostImpl gHost;

// The framework's key (a Windows virtual key) as SDL 1.2's keysym; 0 for one SDL 1.2 had no keysym for
int KeySym(int theKey)
{
	if (theKey >= '0' && theKey <= '9')
		return theKey;
	if (theKey >= 'A' && theKey <= 'Z')
		return theKey - 'A' + 'a';
	if (theKey >= KEYCODE_NUMPAD0 && theKey <= KEYCODE_NUMPAD9)
		return bonnies::SDLK_KP0 + theKey - KEYCODE_NUMPAD0;
	if (theKey >= KEYCODE_F1 && theKey <= KEYCODE_F15)
		return bonnies::SDLK_F1 + theKey - KEYCODE_F1;
	switch (theKey)
	{
	case KEYCODE_BACK: return bonnies::SDLK_BACKSPACE;
	case KEYCODE_TAB: return bonnies::SDLK_TAB;
	case KEYCODE_RETURN: return bonnies::SDLK_RETURN;
	case KEYCODE_ESCAPE: return bonnies::SDLK_ESCAPE;
	case KEYCODE_SPACE: return bonnies::SDLK_SPACE;
	case KEYCODE_DELETE: return bonnies::SDLK_DELETE;
	case KEYCODE_PAUSE: return 19;
	case KEYCODE_UP: return bonnies::SDLK_UP;
	case KEYCODE_DOWN: return bonnies::SDLK_DOWN;
	case KEYCODE_RIGHT: return bonnies::SDLK_RIGHT;
	case KEYCODE_LEFT: return bonnies::SDLK_LEFT;
	case KEYCODE_INSERT: return bonnies::SDLK_INSERT;
	case KEYCODE_HOME: return bonnies::SDLK_HOME;
	case KEYCODE_END: return bonnies::SDLK_END;
	case KEYCODE_PRIOR: return bonnies::SDLK_PAGEUP;
	case KEYCODE_NEXT: return bonnies::SDLK_PAGEDOWN;
	case KEYCODE_DECIMAL: return 266;
	case KEYCODE_DIVIDE: return 267;
	case KEYCODE_MULTIPLY: return 268;
	case KEYCODE_SUBTRACT: return 269;
	case KEYCODE_ADD: return 270;
	case KEYCODE_CAPITAL: return bonnies::SDLK_CAPSLOCK;
	case 0x90: return bonnies::SDLK_NUMLOCK;
	case 0x91: return bonnies::SDLK_SCROLLOCK;
	case KEYCODE_SHIFT: return bonnies::SDLK_LSHIFT;
	case KEYCODE_CONTROL: return bonnies::SDLK_LCTRL;
	case KEYCODE_MENU: return bonnies::SDLK_LALT;
	case KEYCODE_SNAPSHOT: return bonnies::SDLK_PRINT;
	case 0xba: return ';';
	case 0xbb: return '=';
	case 0xbc: return ',';
	case 0xbd: return '-';
	case 0xbe: return '.';
	case 0xbf: return '/';
	case 0xc0: return '`';
	case 0xdb: return '[';
	case 0xdc: return '\\';
	case 0xdd: return ']';
	case 0xde: return '\'';
	default: return 0;
	}
}

bonnies::HostEvent MouseEvent(int theType, int theX, int theY, int theButton)
{
	bonnies::HostEvent anEvent;
	anEvent.mType = theType;
	anEvent.mX = std::max(0, std::min(kScreenWidth - 1, theX));
	anEvent.mY = std::max(0, std::min(kScreenHeight - 1, theY));
	anEvent.mRelX = anEvent.mX - gHost.mMouseX;
	anEvent.mRelY = anEvent.mY - gHost.mMouseY;
	anEvent.mButton = theButton;
	return anEvent;
}

// The framework's buttons (0 left, 1 right, 2 middle) as SDL's (1 left, 2 middle, 3 right)
int SDLButton(int theBtnNum)
{
	return theBtnNum == 0 ? 1 : theBtnNum == 1 ? 3 : theBtnNum == 2 ? 2 : 0;
}

void Motion(int x, int y)
{
	bonnies::HostEvent anEvent = MouseEvent(bonnies::SDL_MOUSEMOTION, x, y, 0);
	if (anEvent.mRelX == 0 && anEvent.mRelY == 0)
		return;
	gHost.mMouseX = anEvent.mX;
	gHost.mMouseY = anEvent.mY;
	gHost.post(anEvent);
}

} // namespace

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The widget: SDL 1.2's events and the screen

BonniesWidget::BonniesWidget()
{
	mScreenImage = nullptr;
	mShownWhole = false;
#ifndef __EMSCRIPTEN__
	// the window's display keeps the game's screen itself (CreateDDInterface), so the widget has no picture of it
	if (dynamic_cast<SurfaceDisplay*>(gHostApp->mDDInterface) != nullptr)
	{
		mShown.assign(kScreenWidth * kScreenHeight, 0);
		mScreenChanged = true;
		mWantsFocus = true;
		return;
	}
#endif
	mScreenImage = new MemoryImage(gHostApp);
	mScreenImage->Create(kScreenWidth, kScreenHeight);
	// every pixel is drawn opaque (Draw below): the image is said to be opaque, so that the framework need not look through
	// its 480,000 pixels for alpha after each change, and with every alpha byte 0xff the framework's premultiplying would
	// change nothing, so that it uploads the image as it is, in one piece, instead of converting it in strips (the Mahjong
	// ports' screen)
	mScreenImage->SetImageMode(false, false);
	mScreenImage->mUploadAsIs = true;
	mScreenChanged = true;
	mWantsFocus = true;
}

BonniesWidget::~BonniesWidget()
{
	delete mScreenImage;
}

#ifdef __EMSCRIPTEN__
// port (test runs): the game's state and a word on its board, for the Browser pane's real-input player
// (games/bonnies/tools/paneplay.js), which calls it with ccall. Read only: it changes nothing in the game.
extern "C" EMSCRIPTEN_KEEPALIVE const char* bonnies_test_state()
{
	static std::string sText;
	sText = bonnies::testState() + bonnies::testWord();
	return sText.c_str();
}

// port (the web build's tests): a test command (docs/TEST_COMMANDS.md) from the page, as -script gives one natively;
// its answer line, which the page logs. Only a test calls it.
extern "C" EMSCRIPTEN_KEEPALIVE const char* bonnies_test_command(const char* theLine)
{
	static std::string sText;
	std::string aLine = theLine;
	size_t aSpace = aLine.find(' ');
	sText = bonnies::testCommand(aLine.substr(0, aSpace), aSpace == std::string::npos ? "" : aLine.substr(aSpace + 1));
	if (sText.empty())
		sText = "ERROR " + aLine + ": no such command";
	return sText.c_str();
}
#endif

// The game presents its screen about 30 times a second whether or not anything moved (a board waiting for the player is
// presented as often as one in motion; two in three presents of the tutorial's board matched the one before), and the same
// screen need not be copied, uploaded and shown again: the framework shows the picture it last drew until it is told
// otherwise. The comparison is on the pixels, so what is on screen never differs from what the game drew. It runs in blocks,
// so that the loop can use the processor's vector instructions, and a block that differs is copied at once, whole, so that the
// screen is read once and written only where it changed (a board in motion changes a part of it, a wait for the player
// nothing) and Draw has nothing left to copy.
bool BonniesWidget::CopyScreenChanges()
{
	const int kBlock = 256;
	static_assert(kScreenWidth * kScreenHeight % kBlock == 0, "the screen is a whole number of blocks");
	ulong* aBits = mScreenImage->GetBits();
	const uint32_t* aScreen = gHost.mScreen.data();
	bool aChanged = false;
	for (int aStart = 0; aStart < kScreenWidth * kScreenHeight; aStart += kBlock)
	{
		uint32_t aDifference = 0;
		for (int i = aStart; i < aStart + kBlock; i++)
			aDifference |= (uint32_t)aBits[i] ^ (0xff000000 | (aScreen[i] & 0xffffff));
		if (aDifference != 0)
		{
			for (int i = aStart; i < aStart + kBlock; i++)
				aBits[i] = 0xff000000 | (aScreen[i] & 0xffffff);
			aChanged = true;
		}
	}
	return aChanged;
}

// The rows of the game's screen that differ from what the window's display last had, handed to it as rects: a run of rows
// that differ is one rect, from the leftmost to the rightmost pixel that differs in it. The display copies only those rects
// into its window and shows them. The first call hands it the whole screen. What is compared is the game's pixels (0x00RRGGBB,
// the top byte is not read), so what is on screen never differs from what the game drew, and a screen that the game
// presented without changing anything shows nothing new.
#ifndef __EMSCRIPTEN__
bool BonniesWidget::SendScreenChanges()
{
	SurfaceDisplay* aDisplay = dynamic_cast<SurfaceDisplay*>(gHostApp->mDDInterface);
	if (aDisplay == nullptr)
		return false;

	const uint32_t* aScreen = gHost.mScreen.data();
	uint32_t* aShown = mShown.data();
	static std::vector<Rect> sRects;
	sRects.clear();
	if (!mShownWhole)
	{
		mShownWhole = true;
		memcpy(aShown, aScreen, sizeof(uint32_t) * kScreenWidth * kScreenHeight);
		aDisplay->SetFrame(aScreen, kScreenWidth * 4, nullptr, -1);
		return true;
	}

	int aRunTop = -1, aRunLeft = 0, aRunRight = 0;
	for (int y = 0; y <= kScreenHeight; y++)
	{
		int aLeft = 0, aRight = -1;
		if (y < kScreenHeight)
		{
			const uint32_t* aRow = aScreen + y * kScreenWidth;
			uint32_t* aOldRow = aShown + y * kScreenWidth;
			uint32_t aDifference = 0;
			for (int x = 0; x < kScreenWidth; x++)
				aDifference |= (aRow[x] ^ aOldRow[x]) & 0xffffff;
			if (aDifference != 0)
			{
				while (((aRow[aLeft] ^ aOldRow[aLeft]) & 0xffffff) == 0)
					aLeft++;
				aRight = kScreenWidth - 1;
				while (((aRow[aRight] ^ aOldRow[aRight]) & 0xffffff) == 0)
					aRight--;
				memcpy(aOldRow + aLeft, aRow + aLeft, sizeof(uint32_t) * (aRight - aLeft + 1));
			}
		}
		if (aRight >= aLeft)
		{
			if (aRunTop < 0)
			{
				aRunTop = y;
				aRunLeft = aLeft;
				aRunRight = aRight;
			}
			else
			{
				aRunLeft = std::min(aRunLeft, aLeft);
				aRunRight = std::max(aRunRight, aRight);
			}
		}
		else if (aRunTop >= 0)
		{
			sRects.push_back(Rect(aRunLeft, aRunTop, aRunRight - aRunLeft + 1, y - aRunTop));
			aRunTop = -1;
		}
	}
	if (sRects.empty())
		return false;
	aDisplay->SetFrame(aScreen, kScreenWidth * 4, sRects.data(), (int)sRects.size());
	return true;
}
#else
bool BonniesWidget::SendScreenChanges()
{
	return false;
}
#endif

void BonniesWidget::Update()
{
	Widget::Update();
	if (mScreenChanged)
	{
		mScreenChanged = false;
		if (mScreenImage == nullptr)
		{
			if (SendScreenChanges())
				MarkDirty();
		}
		else if (CopyScreenChanges())
		{
			mScreenImage->BitsChanged();
			MarkDirty();
		}
	}
#ifndef __EMSCRIPTEN__
	// port (test runs): the game's state for tools/realplay.py, which plays from it with real input (Start.h), written
	// whole to a second file and renamed, so that a reader never sees half of it (it may miss the file for a moment)
	// (SDL_getenv answers in one buffer on Windows, which the next call overwrites)
	static const std::string sStateFile = SDL_getenv("BONNIES_STATE") != nullptr ? SDL_getenv("BONNIES_STATE") : "";
	static uint32_t sStateTime = 0;
	if (!sStateFile.empty() && gHostApp->IsTestRun() && SDL_GetTicks() - sStateTime >= 100)
	{
		sStateTime = SDL_GetTicks();
		std::string aNewFile = sStateFile + ".new";
		if (FILE* aFile = fopen(aNewFile.c_str(), "wb"))
		{
			std::string aState = bonnies::testState();
			fwrite(aState.data(), 1, aState.size(), aFile);
			fclose(aFile);
			std::remove(sStateFile.c_str());
			std::rename(aNewFile.c_str(), sStateFile.c_str());
		}
	}
#endif
}

// The game's screen (0x00RRGGBB) as it was last presented: the picture already holds it (Update), except for a screen the
// game presented since the last update, or the first one; and the framework uploads the picture only when it changed
void BonniesWidget::Draw(Graphics* g)
{
	if (mScreenImage == nullptr)
	{
		// (the window's display shows the screen itself)
		if (mScreenChanged)
		{
			mScreenChanged = false;
			SendScreenChanges();
		}
		return;
	}
	if (mScreenChanged)
	{
		mScreenChanged = false;
		if (CopyScreenChanges())
			mScreenImage->BitsChanged();
	}
	g->DrawImage(mScreenImage, 0, 0);
}

// SDL_EnableUNICODE: the character goes with its key's key-down, which the framework sent just before; a character
// without one (a script's "char") comes as a key of its own
void BonniesWidget::KeyChar(SexyChar theChar)
{
	int aChar = (int)(unsigned)theChar;
	if (aChar == 0)
		return;
	if (!gHost.mEvents.empty())
	{
		bonnies::HostEvent& aLast = gHost.mEvents.back();
		if (aLast.mType == bonnies::SDL_KEYDOWN && (aLast.mUnicode == 0 || aLast.mUnicode == aChar))
		{
			aLast.mUnicode = aChar;
			return;
		}
	}
	bonnies::HostEvent anEvent;
	anEvent.mType = bonnies::SDL_KEYDOWN;
	anEvent.mSym = aChar < 0x80 ? (aChar >= 'A' && aChar <= 'Z' ? aChar - 'A' + 'a' : aChar) : 0;
	anEvent.mUnicode = aChar;
	gHost.post(anEvent);
	anEvent.mType = bonnies::SDL_KEYUP;
	anEvent.mUnicode = 0;
	gHost.post(anEvent);
}

void BonniesWidget::KeyDown(KeyCode theKey)
{
	int aSym = KeySym(theKey);
	if (aSym == 0)
		return;
	// a held key repeats only once SDL_EnableKeyRepeat asked for it
	if (gHost.mKeys[aSym] != 0 && !gHost.mKeyRepeat)
		return;
	gHost.mKeys[aSym] = 1;
	bonnies::HostEvent anEvent;
	anEvent.mType = bonnies::SDL_KEYDOWN;
	anEvent.mSym = aSym;
	// keys without a character: SDL 1.2 gave the control characters of Backspace, Tab, Enter and Esc
	if (aSym == bonnies::SDLK_BACKSPACE || aSym == bonnies::SDLK_TAB || aSym == bonnies::SDLK_RETURN ||
		aSym == bonnies::SDLK_ESCAPE)
		anEvent.mUnicode = aSym;
	gHost.post(anEvent);
}

void BonniesWidget::KeyUp(KeyCode theKey)
{
	int aSym = KeySym(theKey);
	if (aSym == 0)
		return;
	gHost.mKeys[aSym] = 0;
	bonnies::HostEvent anEvent;
	anEvent.mType = bonnies::SDL_KEYUP;
	anEvent.mSym = aSym;
	gHost.post(anEvent);
}

void BonniesWidget::MouseMove(int x, int y)
{
	Motion(x, y);
}

void BonniesWidget::MouseDrag(int x, int y)
{
	Motion(x, y);
}

void BonniesWidget::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	(void)theClickCount;
	int aButton = SDLButton(theBtnNum);
	if (aButton == 0)
		return;
	Motion(x, y);
	gHost.mButtons |= (unsigned char)(1 << (aButton - 1));
	gHost.post(MouseEvent(bonnies::SDL_MOUSEBUTTONDOWN, x, y, aButton));
}

void BonniesWidget::MouseUp(int x, int y, int theBtnNum, int theClickCount)
{
	(void)theClickCount;
	int aButton = SDLButton(theBtnNum);
	if (aButton == 0)
		return;
	Motion(x, y);
	gHost.mButtons &= (unsigned char)~(1 << (aButton - 1));
	gHost.post(MouseEvent(bonnies::SDL_MOUSEBUTTONUP, x, y, aButton));
}

// SDL 1.2 sent the wheel as buttons 4 and 5, which the engine ignores
void BonniesWidget::MouseWheel(int theDelta)
{
	(void)theDelta;
}

void BonniesWidget::MouseEnter()
{
	Widget::MouseEnter();
	gHostApp->mMouseInside = true;
	bonnies::HostEvent anEvent;
	anEvent.mType = bonnies::SDL_ACTIVEEVENT;
	gHost.post(anEvent);
}

void BonniesWidget::MouseLeave()
{
	Widget::MouseLeave();
	gHostApp->mMouseInside = false;
	bonnies::HostEvent anEvent;
	anEvent.mType = bonnies::SDL_ACTIVEEVENT;
	gHost.post(anEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The app

BonniesHostApp::BonniesHostApp()
{
	gHostApp = this;
	mProdName = "BonniesBookstore";
	mTitle = "Bonnie's Bookstore Deluxe";
	mRegKey = "PopCap\\BonniesBookstore";
	mWidth = kScreenWidth;
	mHeight = kScreenHeight;
	mWidget = nullptr;
	mGameRunning = false;
	mApplyingGameMode = false;
	mMouseInside = false;
	mLastFrameworkRun = 0;
	mScriptNoFocus = false;
#ifndef __EMSCRIPTEN__
	mWindowCursor = nullptr;
#endif
}

BonniesHostApp::~BonniesHostApp()
{
	if (mWidget != nullptr)
	{
		mWidgetManager->RemoveWidget(mWidget);
		delete mWidget;
	}
#ifndef __EMSCRIPTEN__
	// the game's screen buffer goes with the game
	if (SurfaceDisplay* aDisplay = dynamic_cast<SurfaceDisplay*>(mDDInterface))
		aDisplay->SetFrame(nullptr, 0, nullptr, -1);
#endif
}

#ifndef __EMSCRIPTEN__
DDInterface* BonniesHostApp::CreateDDInterface()
{
	// test only: SURFACE_DISPLAY=off keeps the framework's own display and the widget's picture (the path before the
	// SurfaceDisplay), to measure the two against each other
	const char* aMode = SDL_getenv("SURFACE_DISPLAY");
	if ((aMode != nullptr) && (strcmp(aMode, "off") == 0))
		return SexyAppBase::CreateDDInterface();
	return new SurfaceDisplay(this);
}

// The game's own cursor is the window's cursor, as SDL 1.2's was, not a picture that the framework draws over the screen (a
// drawn cursor image keeps the window's display from showing the screen through the window's own surface): while the
// mouse is in the window and the game has asked for its cursor, the window shows it, and the framework's other cursors
// (the pointer outside the window, none when the game hides it) are as they were.
void BonniesHostApp::EnforceCursor()
{
	if ((mWindowCursor != nullptr) && (mCursorNum == CURSOR_CUSTOM) && (mMouseIn) && (!mSEHOccured))
	{
		if (SDL_GetCursor() != mWindowCursor)
			SDL_SetCursor(mWindowCursor);
		SDL_ShowCursor(SDL_ENABLE);
		return;
	}
	SexyAppBase::EnforceCursor();
}

// The window's cursor made from the game's bitmap (nullptr: none). The one before it is freed once the window shows the new
// one (SDL_FreeCursor puts the default cursor in place of one that is still showing).
void BonniesHostApp::SetWindowCursor(SDL_Cursor* theCursor)
{
	SDL_Cursor* anOldCursor = mWindowCursor;
	mWindowCursor = theCursor;
	EnforceCursor();
	if (anOldCursor != nullptr)
		SDL_FreeCursor(anOldCursor);
}
#endif

void BonniesHostApp::Init()
{
	SexyAppBase::Init();
	if (mShutdown)
		return;
	bonnies::setHost(&gHost);
	mWidget = new BonniesWidget();
	mWidget->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mWidget);
	mWidgetManager->SetFocus(mWidget);
}

// The game's own loop (0x401000 WinMain: bonniesMain); the framework runs in its sleeps. When it ends the framework
// ends too: on the web its loop then finishes the page's run (the saves' backup, Module.onGameExit).
void BonniesHostApp::DoMainLoop()
{
	if (!mShutdown)
	{
		bonnies::loadLetterOdds();
		bonnies::loadBooks();
		gSteadyClock = !mFastMode && Platform::FixOn("steadyClock");
		bonnies::gSkipLogos = Byopop::SiteSkipLogos();
		mGameRunning = true;
		bonnies::bonniesMain();
		mGameRunning = false;
#ifndef __EMSCRIPTEN__
		SetWindowCursor(nullptr);
#else
		if (gHost.mCursorImage != nullptr)
		{
			SetCursorImage(CURSOR_CUSTOM, nullptr);
			delete gHost.mCursorImage;
			gHost.mCursorImage = nullptr;
		}
#endif
		Shutdown();
	}
	SexyAppBase::DoMainLoop();
}

static uint32_t GameClock()
{
	// (once the framework is shut down no update comes, so the game's last waits as it quits run on the machine's clock)
	if (gSteadyClock && !gHostApp->mShutdown)
		return 1000 + (uint32_t)gHostApp->mUpdateCount * (uint32_t)gHostApp->mFrameTime;
	return gHostApp->GetClockTime();
}

void BonniesHostApp::RunFramework(int theMs)
{
	// the game's clock: with "steadyClock" a sleep lasts whole updates, so a frame of the game always spans as many
	DWORD anEnd = GameClock() + (DWORD)theMs;
	do
	{
		// the framework was shut down (a script's quit): the game is asked to quit, as the close button does
		if (mShutdown)
		{
			bonnies::HostEvent anEvent;
			anEvent.mType = bonnies::SDL_QUIT;
			gHost.post(anEvent);
			return;
		}
		if (mExitToTop)
			mExitToTop = false;
#ifdef __EMSCRIPTEN__
		// as SexyAppBase::EmscriptenMainLoop: step until a frame was drawn or nothing is due, then yield
		int aDrawCount = mDrawCount;
		for (int aStep = 0; aStep < 1000 && !mShutdown; aStep++)
		{
			UpdateAppStep(nullptr);
			if (mDrawCount != aDrawCount || mNothingDue)
				break;
		}
		UpdateUserDataBackup(false);
		bonnies_sleep(1);
#else
		UpdateAppStep(nullptr);
#endif
		mLastFrameworkRun = GetClockTime();
		gSpin = 0;
	} while ((int)(GameClock() - anEnd) < 0);
}

// WM_CLOSE: SDL_QUIT goes to the game, which asks or quits
void BonniesHostApp::CloseRequestAsync()
{
	if (mGameRunning)
	{
		bonnies::HostEvent anEvent;
		anEvent.mType = bonnies::SDL_QUIT;
		gHost.post(anEvent);
	}
	else
		SexyAppBase::CloseRequestAsync();
}

// SDL_ACTIVEEVENT: the engine reads the focus from SDL_GetAppState
void BonniesHostApp::GotFocus()
{
	SexyAppBase::GotFocus();
	bonnies::HostEvent anEvent;
	anEvent.mType = bonnies::SDL_ACTIVEEVENT;
	gHost.post(anEvent);
}

void BonniesHostApp::LostFocus()
{
	SexyAppBase::LostFocus();
	bonnies::HostEvent anEvent;
	anEvent.mType = bonnies::SDL_ACTIVEEVENT;
	gHost.post(anEvent);
}

// A script's "focus 0" and "focus 1" (a test hook): the window loses the focus and gets it back, as when the player
// clicks another window and then the game's. The standard test commands (docs/TEST_COMMANDS.md) are the game's
// (bonnies::testCommand), answered with one line on stdout.
bool BonniesHostApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	if (theCommand.mCommand != "focus")
	{
		std::string aReply = bonnies::testCommand(theCommand.mCommand, theCommand.mArgs);
		if (aReply.empty())
			return false;
		printf("%s\n", aReply.c_str());
		fflush(stdout);
		return true;
	}
	mScriptNoFocus = theCommand.mArgs == "0";
	if (mScriptNoFocus)
		LostFocus();
	else
		GotFocus();
	return true;
}

// docs/TEST_COMMANDS.md, "memreport": what the game holds beside the framework's own pictures: its cache's images, which
// the game draws with itself and never hands to the framework, the decoded samples FSOUND's stand-in keeps, the
// dictionary and the 800x600 screen buffer the game draws into (the framework's copy of it is one of its pictures)
void BonniesHostApp::AddMemReportHolders(Platform::MemReport* theReport)
{
	std::vector<std::pair<std::string, size_t>> aRows;
	bonnies::memoryHolders(&aRows);
	for (const std::pair<std::string, size_t>& aRow : aRows)
		theReport->mOthers.push_back(aRow);
	theReport->mOthers.push_back({"samples (" + std::to_string(gHost.mSampleCount) + ", decoded to 16 bits)", gHost.mSampleBytes});
	theReport->mOthers.push_back({"screen buffer (32 bits)", gHost.mScreen.capacity() * sizeof(uint32_t)});
}

// Alt+Enter (and the web page's full screen button) change the mode as the game's own option does
void BonniesHostApp::SwitchScreenMode(bool wantWindowed, bool is3d, bool force)
{
	SexyAppBase::SwitchScreenMode(wantWindowed, is3d, force);
}

// On the web the page's own button makes the game full screen: a browser enters it only on the player's click
void BonniesHostApp::SetGameFullscreen(bool theFullscreen)
{
#ifdef __EMSCRIPTEN__
	return;
#endif
	if (theFullscreen != mIsWindowed)
		return;
	mApplyingGameMode = true;
	SwitchScreenMode(!theFullscreen);
	mApplyingGameMode = false;
}
