// The port's host for Bonnie's Bookstore (see BonniesHost.h): bonnies::Host over the framework and the platform layer.

#include "BonniesHost.h"
#include "../game/Start.h"
#include "../engine/Host.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "WidgetManager.h"
#include "platform/Audio.h"
#include "platform/VFS.h"

#include <SDL.h>
#include <stb_image.h>
#include <algorithm>
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
};

// FMOD 3's sample: the decoded sound
struct SampleHandle
{
	std::shared_ptr<Platform::SoundData> mData;
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
		return gHostApp->GetClockTime();
	}

	// a test run (hidden, on rundesk's desktop) has the focus, as the user's window would, until a script's "focus 0"
	unsigned char getAppState() override
	{
		unsigned char aState = 0;
		if (!gHostApp->mMinimized)
			aState |= bonnies::SDL_APPACTIVE;
		bool aTestFocus = gHostApp->IsTestRun() && !gHostApp->mScriptNoFocus;
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
	// only when the framework runs, so while a script has taken the focus away a pump runs it once
	void pumpEvents() override
	{
		if ((int)(gHostApp->GetClockTime() - gHostApp->mLastFrameworkRun) >= 100 ||
			(gHostApp->mFastMode && gHostApp->mScriptNoFocus))
			gHostApp->RunFramework(0);
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
	// inverted where the data bit is set, drawn black) as the framework's image cursor. The framework centres the
	// image on the mouse, so the cursor sits in a 64x64 image with its hot spot in the middle.
	void setCursor(const uint8_t* theData, const uint8_t* theMask, int theHotX, int theHotY) override
	{
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
	}

	void showCursor(bool theShow) override
	{
		mCursorShown = theShow;
		gHostApp->SetCursor(!theShow ? CURSOR_NONE : mCursorImage != nullptr ? CURSOR_CUSTOM : CURSOR_POINTER);
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
		uint8_t* aPixels = stbi_load_from_memory(theData.data(), (int)theData.size(), &aWidth, &aHeight, &aComponents,
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

	// the framework opens nothing in a test run (it logs "OpenURL in a test run")
	void openURL(const std::string& theURL) override
	{
		gHostApp->SexyAppBase::OpenURL(theURL);
	}

	// MessageBoxA's buttons (MB_OK 0, MB_OKCANCEL 1, MB_YESNOCANCEL 3, MB_YESNO 4) and results (IDOK 1, IDCANCEL 2,
	// IDYES 6, IDNO 7); a test run never blocks on one and takes the first button
	int messageBox(const std::string& theText, const std::string& theCaption, unsigned int theFlags) override
	{
		SDL_Log("%s: %s", theCaption.c_str(), theText.c_str());
		int aKind = theFlags & 0xf;
		SDL_MessageBoxButtonData aButtons[3];
		int aCount = 0;
		auto add = [&](int theId, const char* theText, uint32_t theButtonFlags)
		{
			aButtons[aCount].flags = theButtonFlags;
			aButtons[aCount].buttonid = theId;
			aButtons[aCount].text = theText;
			aCount++;
		};
		if (aKind == 4 || aKind == 3)
		{
			add(6, "Yes", SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT);
			add(7, "No", aKind == 4 ? SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT : 0);
			if (aKind == 3)
				add(2, "Cancel", SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT);
		}
		else
		{
			add(1, "OK", SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT | (aKind == 0 ? SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT : 0));
			if (aKind == 1)
				add(2, "Cancel", SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT);
		}
		if (gHostApp->mFastMode || gHostApp->IsTestRun())
			return aButtons[0].buttonid;
		SDL_MessageBoxData aBox = {};
		aBox.flags = (theFlags & 0x10) != 0 ? SDL_MESSAGEBOX_ERROR : (theFlags & 0x30) == 0x30 ?
			SDL_MESSAGEBOX_WARNING : SDL_MESSAGEBOX_INFORMATION;
		aBox.window = gHostApp->mHWnd;
		aBox.title = theCaption.c_str();
		aBox.message = theText.c_str();
		aBox.numbuttons = aCount;
		aBox.buttons = aButtons;
		int aResult = aButtons[0].buttonid;
		if (SDL_ShowMessageBox(&aBox, &aResult) != 0 || aResult < 0)
			aResult = aKind == 0 ? 1 : 2;
		return aResult;
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

	void* loadSample(const std::string& thePath) override
	{
		std::shared_ptr<Platform::SoundData> aData = Platform::Mixer::Get().LoadSound(thePath);
		if (aData == nullptr)
			return nullptr;
		SampleHandle* aSample = new SampleHandle();
		aSample->mData = aData;
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
					Platform::Mixer::Get().StopVoice(aChannel.mVoice);
				aChannel = Channel();
			}
		}
		delete aSample;
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
		if (aChannel->mVoice >= 0)
			Platform::Mixer::Get().SetVoicePitch(aChannel->mVoice, pitch(*aChannel));
	}

	void setVolume(int theChannel, int theVolume) override
	{
		Channel* aChannel = channel(theChannel);
		if (aChannel == nullptr)
			return;
		aChannel->mGain = std::max(0, std::min(255, theVolume)) / 255.0f;
		if (aChannel->mVoice >= 0)
			Platform::Mixer::Get().SetVoiceGain(aChannel->mVoice, aChannel->mGain, aChannel->mGain);
	}

	void setPaused(int theChannel, bool thePaused) override
	{
		Channel* aChannel = channel(theChannel);
		if (aChannel == nullptr || thePaused || !aChannel->mPaused)
			return;
		aChannel->mPaused = false;
		aChannel->mVoice = Platform::Mixer::Get().StartVoice(aChannel->mData, aChannel->mGain, aChannel->mGain,
			pitch(*aChannel), false);
	}

	void stopSound(int theChannel) override
	{
		Channel* aChannel = channel(theChannel);
		if (aChannel == nullptr)
			return;
		if (aChannel->mVoice >= 0)
			Platform::Mixer::Get().StopVoice(aChannel->mVoice);
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
			Platform::Mixer::Get().SetSongAmplify(aSong, 256.0f / 48.0f);
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
	mScreenImage = new MemoryImage(gHostApp);
	mScreenImage->Create(kScreenWidth, kScreenHeight);
	mScreenChanged = true;
	mWantsFocus = true;
}

BonniesWidget::~BonniesWidget()
{
	delete mScreenImage;
}

void BonniesWidget::Update()
{
	Widget::Update();
	if (mScreenChanged)
		MarkDirty();
}

// The game's screen (0x00RRGGBB) as it was last presented
void BonniesWidget::Draw(Graphics* g)
{
	mScreenChanged = false;
	ulong* aBits = mScreenImage->GetBits();
	const uint32_t* aScreen = gHost.mScreen.data();
	for (int i = 0; i < kScreenWidth * kScreenHeight; i++)
		aBits[i] = 0xff000000 | (aScreen[i] & 0xffffff);
	mScreenImage->BitsChanged();
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
}

BonniesHostApp::~BonniesHostApp()
{
	if (mWidget != nullptr)
	{
		mWidgetManager->RemoveWidget(mWidget);
		delete mWidget;
	}
}

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
		mGameRunning = true;
		bonnies::bonniesMain();
		mGameRunning = false;
		if (gHost.mCursorImage != nullptr)
		{
			SetCursorImage(CURSOR_CUSTOM, nullptr);
			delete gHost.mCursorImage;
			gHost.mCursorImage = nullptr;
		}
		Shutdown();
	}
	SexyAppBase::DoMainLoop();
}

void BonniesHostApp::RunFramework(int theMs)
{
	DWORD anEnd = GetClockTime() + (DWORD)theMs;
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
	} while ((int)(GetClockTime() - anEnd) < 0);
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
// clicks another window and then the game's
bool BonniesHostApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	if (theCommand.mCommand != "focus")
		return false;
	mScriptNoFocus = theCommand.mArgs == "0";
	if (mScriptNoFocus)
		LostFocus();
	else
		GotFocus();
	return true;
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
