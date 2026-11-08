#include "R64Sys.h"
#include "R64Xml.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace r64
{

namespace
{

SysHost* gHost = nullptr;
const PixelFormat* gScreenFormat = nullptr; // 0x46e5b8

// 0x7078c0..0x707e6c in the original's order, because Key_Push writes past its queue: the queue (100 keys, Key_Pop
// takes from the newest down and wraps to the top), the right button's press latch, the keys held from 0x707a68,
// and the queue's count
int gInput[363];
int* const gKeyQueue = gInput;               // 0x7078c0
int& gRightLatch = gInput[105];              // 0x707a64
int* const gKeysDown = gInput + 106;         // 0x707a68  [255]
int& gKeyCount = gInput[362];                // 0x707e68
int gMouseLeft = 0;                          // 0x708158
int gMouseRight = 0;                         // 0x708190
int gMouseMiddle = 0;                        // 0x708114
int gLeftLatch = 0;                          // 0x708118
int gMiddleLatch = 0;                        // 0x707f7c

} // namespace

void Sys_SetHost(SysHost* theHost)
{
	gHost = theHost;
}

SysHost* Sys_Host()
{
	return gHost;
}

bool Sys_ReadFile(const char* thePath, std::vector<uint8_t>& theData)
{
	theData.clear();
	return gHost != nullptr && gHost->ReadFile(thePath, theData);
}

// 0x429430
void Sys_AddUpdateRect(int x, int y, int w, int h)
{
	if (gHost != nullptr)
		gHost->AddUpdateRect(x, y, w, h);
}

int gPumpMessages = 0;

// 0x428d70
void Sys_SetMousePos(int x, int y)
{
	if (gHost != nullptr)
		gHost->SetMousePos(x, y);
}

int Mouse_State(int theButton)
{
	if ((char)theButton == 1)
		return gMouseLeft;
	if ((char)theButton == 2)
		return gMouseRight;
	return (char)theButton == 3 ? gMouseMiddle : 0;
}

int Mouse_Pressed(int theButton)
{
	int aResult = 0;
	if ((char)theButton == 1)
	{
		if (gMouseLeft == 0 || gLeftLatch != 0)
			aResult = 0;
		else
		{
			gLeftLatch = 1;
			aResult = gMouseLeft;
		}
		if (gMouseLeft == 0)
		{
			gLeftLatch = gMouseLeft;
			return aResult;
		}
	}
	else if ((char)theButton == 2)
	{
		if (gMouseRight == 0 || gRightLatch != 0)
			aResult = 0;
		else
		{
			gRightLatch = 1;
			aResult = gMouseRight;
		}
		if (gMouseRight == 0)
		{
			gRightLatch = gMouseRight;
			return aResult;
		}
	}
	else if ((char)theButton == 3)
	{
		if (gMouseMiddle == 0 || gMiddleLatch != 0)
			aResult = 0;
		else
		{
			gMiddleLatch = 1;
			aResult = gMouseMiddle;
		}
		if (gMouseMiddle == 0)
			gMiddleLatch = gMouseMiddle;
	}
	return aResult;
}

int Key_Pop()
{
	int aKey = gKeyQueue[gKeyCount];
	gKeyQueue[gKeyCount] = 0;
	gKeyCount = gKeyCount - 1;
	if (gKeyCount < 0)
		gKeyCount = 99;
	return aKey;
}

// (the original also sets the word at the key's index from the queue's start, as here; its one caller pushes 8)
void Key_Push(int theKey)
{
	gKeyCount = gKeyCount + 1;
	gKeyQueue[gKeyCount] = theKey;
	if (0 <= theKey && theKey < 362)
		gInput[theKey] = 1;
}

void Key_Clear()
{
	std::memset(gKeyQueue, 0, 400);
	gKeyCount = 0;
}

int Key_Down(int theKey)
{
	return gKeysDown[theKey] != 0;
}

// 0x429020: Sys_KeyEvent sets a slot to 1 only from 0, so a held key answers once
int Key_Pressed(int theKey)
{
	if (gKeysDown[theKey] == 1)
	{
		gKeysDown[theKey] = 2;
		return 1;
	}
	return 0;
}

void Mouse_Update(int* x, int* y)
{
	if (gHost != nullptr)
		gHost->GetMousePos(x, y);
	else
	{
		*x = 0;
		*y = 0;
	}
}

// WndProc's (0x42ac80) WM_KEYDOWN and WM_KEYUP: F5 turns the sound on or off, F6 the music, F7 switches between the
// window and the full screen, F8 between stretched and centred
void Sys_KeyEvent(int theKey, int theDown)
{
	if (theKey < 0 || theKey > 0xfe)
		return;
	if (theDown == 0)
	{
		gKeysDown[theKey] = 0;
		return;
	}
	switch (theKey)
	{
	case 0x74:
		Sound_SetOn(Sound_On() == 1 ? 0 : 1);
		break;
	case 0x75:
		Music_SetOn(Music_On() == 1 ? 0 : 1);
		break;
	case 0x76:
		Screen_SetFullscreen(Screen_Fullscreen() == 0 ? 1 : 0);
		break;
	case 0x77:
		Screen_SetCentered(Screen_Centered() == 0 ? 1 : 0);
		break;
	}
	gKeyCount = gKeyCount + 1;
	if (99 < gKeyCount)
		gKeyCount = 0;
	int aHeld = gKeysDown[theKey];
	gKeyQueue[gKeyCount] = theKey;
	if (aHeld == 0)
		gKeysDown[theKey] = 1;
}

// Its button messages: a release also clears the button's press latch
void Sys_MouseButton(int theButton, int theDown)
{
	if (theButton == 1)
	{
		gMouseLeft = theDown;
		if (theDown == 0)
			gLeftLatch = 0;
	}
	else if (theButton == 2)
	{
		gMouseRight = theDown;
		if (theDown == 0)
			gRightLatch = 0;
	}
	else if (theButton == 3)
	{
		gMouseMiddle = theDown;
		if (theDown == 0)
			gMiddleLatch = 0;
	}
}

const PixelFormat* Sys_ScreenFormat()
{
	return gScreenFormat;
}

void Sys_SetScreenFormat(const PixelFormat* theFmt)
{
	gScreenFormat = theFmt;
}

bool Sys_FileExists(const char* thePath)
{
	std::vector<uint8_t> aData;
	return Sys_ReadFile(thePath, aData);
}

const char* Sys_SavePath(const char* theName)
{
	return gHost != nullptr ? gHost->SavePath(theName) : theName;
}

void Sys_SaveDone()
{
	if (gHost != nullptr)
		gHost->SaveDone();
}

void Sys_UserName(char* theName, int theSize)
{
	if (gHost != nullptr)
		gHost->UserName(theName, theSize);
	else if (theSize > 0)
		theName[0] = 0;
}

namespace
{

int gFullscreen = 1;                         // 0x460b14
int gWaveOn = 1;                             // 0x460b18
int gMusicOn = 1;                            // 0x460b1c
int gActive = 1;                             // 0x707a50  the window is the active one
int gSoundReady = 0;                         // 0x707a54  BASS started (the port: a host)
int gCentered = 0;                           // 0x7081a0  cp.cfg's desktopResolution
int gReopen = 0;                             // 0x7081a4  the screen is opened again at the next PumpMessages
int gMusic = 0;                              // 0x7081ac  a music is loaded
void (*gScreenChange)() = nullptr;           // 0x7081b4
int gToggleFullscreen = 0;                   // 0x7081bc
int gToggleCentered = 0;                     // 0x7081c0
std::vector<int> gSamples;                   // 0x7081c4  the host's ids, by the game's

} // namespace

uint32_t TimeMs()
{
	return gHost != nullptr ? gHost->TimeMs() : 0;
}

bool Sys_FixOn(const char* theFix)
{
	return gHost != nullptr && gHost->FixOn(theFix);
}

// (SleepEx; the port's frames are the host's, so nothing waits)
void Sys_Sleep(int)
{
}

// (0x7081b0: the port never loses the screen, see PumpMessages)
void SetScreenLostCallback(void (*)(const PixelFormat*))
{
}

void SetScreenChangeCallback(void (*theCallback)())
{
	gScreenChange = theCallback;
}

void Screen_SetFullscreen(int theOn)
{
	if (gFullscreen != theOn)
	{
		gToggleFullscreen = 1;
		gReopen = 1;
	}
}

int Screen_Fullscreen()
{
	return gFullscreen;
}

void Screen_SetCentered(int theOn)
{
	if (gCentered != theOn)
	{
		gToggleCentered = 1;
		if (gFullscreen == 1)
			gReopen = 1;
	}
}

int Screen_Centered()
{
	return gCentered;
}

// (the original waits for the display's refresh when theWait is 1)
void Present(int theWait)
{
	if (gHost != nullptr)
		gHost->Present(theWait);
}

int LockScreen(ScreenLock* theLock)
{
	uint8_t* aBits = nullptr;
	int aPitch = 0;
	if (gHost == nullptr || !gHost->LockScreen(&aBits, &aPitch))
		return 0;
	theLock->unk00 = 0;
	theLock->w = 800;
	theLock->h = 600;
	theLock->pitch = aPitch;
	theLock->bits = aBits;
	return 1;
}

void UnlockScreen()
{
	if (gHost != nullptr)
		gHost->UnlockScreen();
}

// The port's screen keeps its format, so opening it again loses nothing: the lost callback (which converts the art
// to a new format) is not called, only the change one (which has everything drawn again)
int PumpMessages()
{
	if (gHost != nullptr)
		Sys_Activate(gHost->IsActive() ? 1 : 0);
	if (gPumpMessages != 1 && gActive == 0)
		return 0;
	if (gToggleCentered == 1)
	{
		gToggleCentered = 0;
		gCentered = gCentered == 0 ? 1 : 0;
		SaveConfig();
	}
	if (gReopen != 0)
	{
		int x, y;
		Mouse_Update(&x, &y);
		if (gToggleFullscreen == 1)
		{
			gToggleFullscreen = 0;
			gFullscreen = gFullscreen == 0 ? 1 : 0;
			SaveConfig();
		}
		if (gHost != nullptr)
			gHost->SetScreenMode(gFullscreen, gCentered);
		if (gScreenChange != nullptr)
			gScreenChange();
		Sys_SetMousePos(x, y);
		gReopen = 0;
	}
	return 1;
}

void LoadConfig()
{
	std::FILE* aFile = std::fopen(Sys_SavePath("cp.cfg"), "rb");
	if (aFile == nullptr)
	{
		SaveConfig();
		return;
	}
	std::fseek(aFile, 0, SEEK_END);
	long aSize = std::ftell(aFile);
	std::fseek(aFile, 0, SEEK_SET);
	char* aData = (char*)std::malloc(aSize + 1);
	if (aData == nullptr)
	{
		std::fclose(aFile);
		return;
	}
	std::memset(aData, 0, aSize + 1);
	if (std::fread(aData, aSize, 1, aFile) == 0)
	{
		std::fclose(aFile);
		std::free(aData);
		return;
	}
	std::fclose(aFile);
	XmlTag aTag;
	Xml_NextTag(aData, &aTag);
	gFullscreen = Xml_GetInt(&aTag, "fullscreen");
	gCentered = Xml_GetInt(&aTag, "desktopResolution");
	gWaveOn = Xml_GetInt(&aTag, "wave");
	gMusicOn = Xml_GetInt(&aTag, "music");
	std::free(aData);
}

void SaveConfig()
{
	std::FILE* aFile = std::fopen(Sys_SavePath("cp.cfg"), "wb");
	if (aFile == nullptr)
		return;
	std::fprintf(aFile, "<CP_CONFIG\n");
	std::fprintf(aFile, "    fullscreen=\"%d\"\n", gFullscreen);
	std::fprintf(aFile, "    desktopResolution=\"%d\"\n", gCentered);
	std::fprintf(aFile, "    wave=\"%d\"\n", gWaveOn);
	std::fprintf(aFile, "    music=\"%d\"\n", gMusicOn);
	std::fprintf(aFile, "/>\n");
	std::fclose(aFile);
	Sys_SaveDone();
}

// 0x42aaf0
int Sound_Pause()
{
	if (gSoundReady != 0)
	{
		if (gMusic != 0)
			gHost->PauseMusic();
		Sound_StopAll();
	}
	return 1;
}

// WndProc's WM_ACTIVATEAPP, WM_SETFOCUS and WM_KILLFOCUS, when the window's state changes: the keys held are let go,
// and the music pauses (with the sounds stopped) while the window is inactive
void Sys_Activate(int theActive)
{
	if (theActive == gActive)
		return;
	gActive = theActive;
	std::memset(gKeysDown, 0, 0x3fc);
	if (theActive != 0)
		Music_Resume();
	else
		Sound_Pause();
}

int Sample_Load(const char* thePath)
{
	if (gSoundReady != 0)
	{
		int aSound = gHost->LoadSound(thePath);
		if (aSound != -1)
		{
			gSamples.push_back(aSound);
			return (int)gSamples.size() + -1;
		}
	}
	return -1;
}

// (an id outside the list stops the original)
int Sound_Play(int theSound)
{
	if (gSoundReady != 0 && gWaveOn != 0 && (unsigned)theSound < gSamples.size())
		gHost->PlaySound(gSamples[theSound]);
	return 1;
}

int Sound_StopAll()
{
	if (gSoundReady != 0)
		gHost->StopSounds();
	return 1;
}

int Sound_On()
{
	return gWaveOn;
}

void Sound_SetOn(int theOn)
{
	gWaveOn = theOn;
	SaveConfig();
}

int SoundList_Find(SoundList* theList, const char* thePath)
{
	if (theList->current == nullptr)
		return 0;
	theList->current = theList->head;
	for (;;)
	{
		SoundEntry* anEntry = theList->current;
		if (std::strcmp(thePath, anEntry->name) == 0)
			return anEntry->sound;
		if (anEntry->next == nullptr)
		{
			theList->current = theList->head;
			return 0;
		}
		theList->current = anEntry->next;
	}
}

namespace
{

// 0x428ad0: a new entry at the tail, made current
void SoundList_Append(SoundList* theList)
{
	SoundEntry* anEntry = new SoundEntry();
	if (theList->current != nullptr)
	{
		anEntry->prev = theList->tail;
		anEntry->next = nullptr;
		theList->tail->next = anEntry;
		theList->current = anEntry;
		theList->tail = anEntry;
		return;
	}
	anEntry->next = nullptr;
	anEntry->prev = nullptr;
	theList->head = anEntry;
	theList->current = anEntry;
	theList->tail = anEntry;
}

// 0x428bb0: the entry for thePath, appended when the list has none; null when it was there (it is current then)
SoundEntry* SoundList_Add(SoundList* theList, const char* thePath)
{
	if (theList->current != nullptr)
	{
		theList->current = theList->head;
		for (;;)
		{
			SoundEntry* anEntry = theList->current;
			if (std::strcmp(thePath, anEntry->name) == 0)
				return nullptr;
			if (anEntry->next == nullptr)
			{
				theList->current = theList->head;
				break;
			}
			theList->current = anEntry->next;
		}
	}
	SoundList_Append(theList);
	std::strcpy(theList->current->name, thePath);
	theList->count = theList->count + 1;
	return theList->current;
}

} // namespace

int SoundList_Load(SoundList* theList, const char* thePath)
{
	SoundEntry* anEntry = SoundList_Add(theList, thePath);
	if (anEntry != nullptr)
	{
		int aSound = Sample_Load(thePath);
		anEntry->sound = aSound;
		return aSound;
	}
	return theList->current->sound;
}

// BASS's music volume is a percentage
int Music_SetVolume(int theVolume)
{
	if (gHost != nullptr)
		gHost->SetMusicVolume((theVolume * 100) / 256);
	return 1;
}

int Music_Load(const char* thePath)
{
	if (gSoundReady == 0)
		return 1;
	gMusic = gHost->LoadMusic(thePath) ? 1 : 0;
	return gMusic != 0;
}

int Music_Free()
{
	if (gSoundReady != 0 && gMusic != 0)
		gHost->FreeMusic();
	return 1;
}

int Music_Play()
{
	if (gSoundReady != 0)
	{
		if (gMusic == 0)
			return 0;
		if (gMusicOn != 0)
			gHost->PlayMusic(1);
	}
	return 1;
}

int Music_Resume()
{
	if (gSoundReady != 0 && gMusicOn != 0)
		gHost->PlayMusic(0);
	return 1;
}

int Music_On()
{
	return gMusicOn;
}

void Music_SetOn(int theOn)
{
	if (theOn == 0 && gSoundReady != 0 && gMusic != 0)
		gHost->PauseMusic();
	gMusicOn = theOn;
	SaveConfig();
	if (gMusicOn == 1 && gSoundReady != 0 && gMusic != 0)
		gHost->PlayMusic(1);
}

// (the original also sets BASS's output to 60%: the host's volume)
void Sound_Start()
{
	gSoundReady = gHost != nullptr ? 1 : 0;
}

void Sound_Shutdown()
{
	if (gSoundReady != 0)
	{
		gHost->StopSounds();
		if (gMusic != 0)
			gHost->PauseMusic();
		Music_Free();
	}
	gSamples.clear();
	gSoundReady = 0;
}

} // namespace r64
