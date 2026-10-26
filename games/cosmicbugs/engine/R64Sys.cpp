#include "R64Sys.h"

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

// 0x428f50
void Sys_AddUpdateRect(int x, int y, int w, int h)
{
	if (gHost != nullptr)
		gHost->AddUpdateRect(x, y, w, h);
}

int gPumpMessages = 0;

// 0x428890
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

// WndProc's (0x42a7a0) WM_KEYDOWN and WM_KEYUP (F5 and F6, which toggle the sound and the music there, are the
// host's)
void Sys_KeyEvent(int theKey, int theDown)
{
	if (theKey < 0 || theKey > 0xfe)
		return;
	if (theDown == 0)
	{
		gKeysDown[theKey] = 0;
		return;
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

} // namespace r64
