#ifndef __BONNIES_HOST_H__
#define __BONNIES_HOST_H__

// What the engine took from SDL 1.2, SDL_image, FMOD 3 and Windows, as one interface the port's host implements
// (src/BonniesApp.cpp) over the framework and the platform layer. The engine calls it where the original called those
// libraries; the names say which call each stands for.

#include <cstdint>
#include <string>
#include <vector>

namespace bonnies
{

// SDL 1.2's keysyms (SDLK_*), which the game compares keys with
enum
{
	SDLK_BACKSPACE = 8, SDLK_TAB = 9, SDLK_RETURN = 13, SDLK_ESCAPE = 27, SDLK_SPACE = 32, SDLK_DELETE = 127,
	SDLK_KP0 = 256, SDLK_KP_ENTER = 271, SDLK_UP = 273, SDLK_DOWN = 274, SDLK_RIGHT = 275, SDLK_LEFT = 276,
	SDLK_INSERT = 277, SDLK_HOME = 278, SDLK_END = 279, SDLK_PAGEUP = 280, SDLK_PAGEDOWN = 281, SDLK_F1 = 282,
	SDLK_NUMLOCK = 300, SDLK_CAPSLOCK = 301, SDLK_SCROLLOCK = 302, SDLK_RSHIFT = 303, SDLK_LSHIFT = 304,
	SDLK_RCTRL = 305, SDLK_LCTRL = 306, SDLK_RALT = 307, SDLK_LALT = 308, SDLK_RMETA = 309, SDLK_LMETA = 310,
	SDLK_PRINT = 316, SDLK_LAST = 323
};

// SDL 1.2's event types and SDL_GetAppState's bits, as the engine reads them
enum
{
	SDL_ACTIVEEVENT = 1, SDL_KEYDOWN = 2, SDL_KEYUP = 3, SDL_MOUSEMOTION = 4, SDL_MOUSEBUTTONDOWN = 5,
	SDL_MOUSEBUTTONUP = 6, SDL_QUIT = 12, SDL_SYSWMEVENT = 13
};
enum
{
	SDL_APPMOUSEFOCUS = 1, SDL_APPINPUTFOCUS = 2, SDL_APPACTIVE = 4
};

struct HostEvent
{
	int mType = 0;
	int mSym = 0;        // a key's keysym
	int mUnicode = 0;    // a key-down's character (SDL_EnableUNICODE)
	int mButton = 0;     // 1 left, 2 middle, 3 right
	int mX = 0;          // the mouse in screen pixels
	int mY = 0;
	int mRelX = 0;       // a motion's relative movement
	int mRelY = 0;
};

// A decoded image as IMG_LoadTyped_RW gave it: 4 bytes per pixel R, G, B, A for a PNG with an alpha channel, 3 bytes
// R, G, B for a PNG without one or a JPEG (mBytesPerPixel), rows mPitch bytes apart
struct HostSurface
{
	int mWidth = 0;
	int mHeight = 0;
	int mBytesPerPixel = 0;
	int mPitch = 0;
	std::vector<uint8_t> mPixels;
};

class Host
{
public:
	virtual ~Host() {}

	// SDL_GetTicks
	virtual uint32_t getTicks() = 0;
	// SDL_GetAppState
	virtual unsigned char getAppState() = 0;
	// SDL_GetMouseState: the buttons (bit 0 left, 1 middle, 2 right) and the position in screen pixels
	virtual unsigned char getMouseState(int* theX, int* theY) = 0;
	// SDL_GetKeyState: one byte per keysym, SDLK_LAST of them
	virtual const unsigned char* getKeyState() = 0;
	// SDL_PumpEvents and SDL_PollEvent
	virtual bool pollEvent(HostEvent& theEvent) = 0;
	// SDL_PumpEvents (and SDL_PollEvent(NULL), which takes no event)
	virtual void pumpEvents() = 0;
	// SDL_EnableKeyRepeat
	virtual void enableKeyRepeat(int theDelay, int theInterval) = 0;
	// The 800x600 screen surface's pixels (0xAARRGGBB, 800 per row) the engine draws into
	virtual uint32_t* getScreenPixels() = 0;
	// SDL_UpdateRects (theCount 0: SDL_UpdateRect of the whole screen): the rectangles become visible
	virtual void updateRects(int theCount, const int16_t* theRects) = 0;
	// SDL_SetVideoMode's full screen flag, and whether the window is full screen
	virtual void setFullScreen(bool theFullScreen) = 0;
	// SDL_WM_SetCaption
	virtual void setCaption(const std::string& theCaption) = 0;
	// SDL_CreateCursor and SDL_SetCursor: a 32x32 cursor from its data and mask bits; SDL_ShowCursor
	virtual void setCursor(const uint8_t* theData, const uint8_t* theMask, int theHotX, int theHotY) = 0;
	virtual void showCursor(bool theShow) = 0;
	// SDL_WM_IconifyWindow; GetWindowPlacement's SW_SHOWMINIMIZED
	virtual void iconify() = 0;
	virtual bool isIconic() = 0;
	// SetWindowPos (the window's top left corner); GetCursorPos (the cursor on the desktop)
	virtual void moveWindow(int theX, int theY) = 0;
	virtual bool getCursorPos(int* theX, int* theY) = 0;
	// The desktop's size and bits per pixel (GetWindowRect and GetDeviceCaps of the desktop window)
	virtual void getDesktop(int* theWidth, int* theHeight, int* theBits) = 0;
	// GetDoubleClickTime
	virtual int doubleClickTime() = 0;
	// Sleep (only when the original slept while waiting for the next frame)
	virtual void sleep(int theMs) = 0;
	// IMG_LoadTyped_RW over a file's bytes
	virtual bool decodeImage(const std::vector<uint8_t>& theData, HostSurface& theSurface) = 0;
	// _open/_read through the platform's VFS: a game file's bytes (a path relative to the game folder)
	virtual bool readGameFile(const std::string& thePath, std::vector<uint8_t>& theData) = 0;
	virtual bool gameFileExists(const std::string& thePath) = 0;
	// The saved games and preferences (SavedGames/...): read and written in the user's folder
	virtual bool readUserFile(const std::string& thePath, std::vector<uint8_t>& theData) = 0;
	virtual bool writeUserFile(const std::string& thePath, const std::vector<uint8_t>& theData) = 0;
	virtual bool userFileExists(const std::string& thePath) = 0;
	virtual bool removeUserFile(const std::string& thePath) = 0;
	// _findfirst/_findnext over a folder: the names of its files, the user's and the game's, in the folder's order
	virtual std::vector<std::string> listFiles(const std::string& theDir) = 0;
	// GetUserNameA
	virtual std::string userName() = 0;
	// ShellExecuteA "open"
	virtual void openURL(const std::string& theURL) = 0;
	// MessageBoxA: the original's flags and result (1 OK, 2 cancel, 6 yes, 7 no)
	virtual int messageBox(const std::string& theText, const std::string& theCaption, unsigned int theFlags) = 0;
	// The clipboard's text (CF_TEXT)
	virtual std::string getClipboard() = 0;
	virtual void setClipboard(const std::string& theText) = 0;
	// The original's trace lines (FUN_0041f5d0 discards them; kept for -trace)
	virtual void trace(const char* theText) { (void)theText; }

	// FMOD 3, as the engine uses it: samples by handle, channels by number, one module song at a time
	virtual void* loadSample(const std::string& thePath) = 0;            // FSOUND_Sample_Load
	virtual void freeSample(void* theSample) = 0;                         // FSOUND_Sample_Free
	virtual int playSample(void* theSample) = 0;                          // FSOUND_PlaySoundEx (paused)
	virtual int getFrequency(int theChannel) = 0;                         // FSOUND_GetFrequency
	virtual void setFrequency(int theChannel, int theFrequency) = 0;      // FSOUND_SetFrequency
	virtual void setVolume(int theChannel, int theVolume) = 0;            // FSOUND_SetVolume (0-255)
	virtual void setPaused(int theChannel, bool thePaused) = 0;           // FSOUND_SetPaused
	virtual void stopSound(int theChannel) = 0;                           // FSOUND_StopSound
	virtual bool isPlaying(int theChannel) = 0;                           // FSOUND_IsPlaying
	virtual void* loadSong(const std::string& thePath) = 0;               // FMUSIC_LoadSong
	virtual void freeSong(void* theSong) = 0;                             // FMUSIC_FreeSong
	virtual void playSong(void* theSong) = 0;                             // FMUSIC_PlaySong
	virtual void stopSong(void* theSong) = 0;                             // FMUSIC_StopSong
	virtual void setSongVolume(void* theSong, int theVolume) = 0;         // FMUSIC_SetMasterVolume (0-256)
	virtual void setSongOrder(void* theSong, int theOrder) = 0;           // FMUSIC_SetOrder
	virtual int getSongOrder(void* theSong) = 0;                          // FMUSIC_GetOrder
	virtual bool isSongPlaying(void* theSong) = 0;                        // FMUSIC_IsPlaying
	virtual void setSongPaused(void* theSong, bool thePaused) = 0;        // FMUSIC_SetPaused
};

void setHost(Host* theHost);
Host* host();

} // namespace bonnies

#endif
