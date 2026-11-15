// The game's input state (0x442950-0x442f10, and the widget's handlers 0x441190-0x4414a0, which are in VeniceApp.cpp).

#include "VeniceInput.h"

#include "VeniceApp.h"
#include "VeniceGlobals.h"
#include "WidgetManager.h"

#include <cstring>

int gKeyStates[KeyStateCount]; // 0xa29c58
int gKeyRing[KeyStateCount]; // 0xa29858
int gKeyRingIndex = 0; // 0xa2a1a0
int gMouseX = 0;
int gMouseY = 0;
bool gIgnoreNextPress = false; // 0xa2a194
bool gLeftWasDown = false; // 0xa2ab10

// port: the original steps the key ring's index in line wherever it stores a value (the app's mouse and key handlers)
void AdvanceKeyRing()
{
	gKeyRingIndex = gKeyRingIndex + 1;
	if (gKeyRingIndex > 255)
		gKeyRingIndex = 0;
}

// port: the original stores a value in the key ring in line, in the app's mouse and key handlers
void PushKeyRing(int theValue)
{
	AdvanceKeyRing();
	gKeyRing[gKeyRingIndex] = theValue;
}

// 0x442ed0
bool ConsumeKey(int theKey)
{
	int aState = gKeyStates[theKey];
	if (aState == KeyStateNew)
		gKeyStates[theKey] = KeyStateHeld;
	return aState == KeyStateNew;
}

// 0x4429b0 (the widget manager's flags 0xe8: bit 1 is the left button)
bool IsLeftButtonDown()
{
	return (gApp->mWidgetManager->mDownButtons & 1) != 0;
}

// 0x442950
bool LeftButtonPressed()
{
	bool aDown = IsLeftButtonDown();
	bool aPressed = !gLeftWasDown && aDown && !gIgnoreNextPress;
	gLeftWasDown = aDown;
	gIgnoreNextPress = false;
	return aPressed;
}

// 0x4429d0
bool RightButtonPressed()
{
	return ConsumeKey(KeyStateRightButton);
}

bool IsMouseInWindow()
{
	return gApp->mMouseIn && gMouseInside && gMouseX >= 0 && (float)gMouseX < gScreenWidth && gMouseY >= 0 &&
		(float)gMouseY < gScreenWidth;
}

void HideSystemCursor()
{
	gApp->SetCursor(Sexy::CURSOR_NONE);
}

void ShowSystemCursor()
{
	gApp->SetCursor(Sexy::CURSOR_POINTER);
}

int PopKeyRing()
{
	int aKey = gKeyRing[gKeyRingIndex];
	gKeyRing[gKeyRingIndex] = 0;
	gKeyRingIndex = gKeyRingIndex - 1;
	if (gKeyRingIndex < 0)
		gKeyRingIndex = 255;
	return aKey;
}

void ClearKeyRing()
{
	memset(gKeyRing, 0, sizeof(gKeyRing));
	gKeyRingIndex = 0;
}

bool IsKeyDown(int theKey)
{
	return gKeyStates[theKey] != 0;
}

bool IsTypingKey(int theKey)
{
	if ((theKey >= 0x30 && theKey <= 0x39) || (theKey >= 0x41 && theKey <= 0x5a))
		return true;
	switch (theKey) {
	case 0x20:
	case 0xbc:
	case 0xbe:
	case 0xbd:
	case 0xbb:
	case 0xdb:
	case 0xdd:
	case 0xdc:
	case 0xba:
	case 0xde:
	case 0xbf:
	case 0xc0:
		return true;
	}
	return false;
}

int KeyToCharacter(int theKey)
{
	if (theKey >= 0x30 && theKey <= 0x39)
		return theKey;
	if (theKey >= 0x41 && theKey <= 0x5a)
		return theKey + 0x20;
	switch (theKey) {
	case 0xbc: return ',';
	case 0xbe: return '.';
	case 0xbd: return '-';
	case 0xbb: return '=';
	case 0xdb: return '[';
	case 0xdd: return ']';
	case 0xdc: return 0x5c;
	case 0xba: return ';';
	case 0xde: return 0x27;
	case 0xbf: return '/';
	case 0xc0: return '`';
	}
	return theKey;
}

int ShiftedCharacter(int theCharacter)
{
	if (theCharacter >= 0x61 && theCharacter <= 0x7a)
		return theCharacter - 0x20;
	switch (theCharacter) {
	case ',': return '<';
	case '.': return '>';
	case '-': return '_';
	case '=': return '+';
	case '[': return '{';
	case ']': return '}';
	case 0x5c: return '|';
	case ';': return ':';
	case 0x27: return '"';
	case '/': return '?';
	case '`': return '~';
	case '0': return ')';
	case '1': return '!';
	case '2': return '@';
	case '3': return '#';
	case '4': return '$';
	case '5': return '%';
	case '6': return '^';
	case '7': return '&';
	case '8': return '*';
	case '9': return '(';
	}
	return theCharacter;
}
