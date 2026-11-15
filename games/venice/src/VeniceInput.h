// The game's input: the mouse and keys that the game widget receives, as the scene engine reads them.

#ifndef VENICE_INPUT_H
#define VENICE_INPUT_H

const int KeyStateCount = 256;
const int KeyStateNone = 0;
const int KeyStateNew = 1; // pressed since the game last asked
const int KeyStateHeld = 2;
// the places of the mouse buttons among the key states, and the values they put in the key ring
const int KeyStateLeftButton = 1;
const int KeyStateRightButton = 2;

extern int gKeyStates[KeyStateCount]; // 0xa29c58: by key code; the mouse buttons are 1 (left) and 2 (right)
extern int gKeyRing[KeyStateCount]; // 0xa29858: the latest keys and clicks in the order they came, for the name box
extern int gKeyRingIndex; // 0xa2a1a0: where the latest one is
extern int gMouseX; // the pointer in the window (the game widget's 0x94 and 0x98)
extern int gMouseY;
// whether the first press after the focus returned is left unseen (0xa2a194)
extern bool gIgnoreNextPress;
// the left button as the last update saw it (0xa2ab10)
extern bool gLeftWasDown;

const int KeyRingMouseRight = 2;
const int KeyRingMouseLeft = 1;

void AdvanceKeyRing(); // the first lines of the widget's handlers: the ring's index wraps at 256
void PushKeyRing(int theValue); // AdvanceKeyRing, then the value goes in at the index
bool ConsumeKey(int theKey); // 0x442ed0: true once for a new press, which becomes held
bool LeftButtonPressed(); // 0x442950: true on the update that sees the left button go down
bool RightButtonPressed(); // 0x4429d0: the same for the right button, read from the key states
bool IsLeftButtonDown(); // 0x4429b0

// 0x4428d0: the pointer is in the window and the window is the one that has it (the check of the pointer's y uses the screen's
// width, as the original's does)
bool IsMouseInWindow();
void HideSystemCursor(); // 0x442a50: the game draws its own cursor
void ShowSystemCursor(); // 0x442a70

// 0x442f10: the latest key or click in the ring (0 when none), which leaves the ring
int PopKeyRing();
void ClearKeyRing(); // 0x442e70
bool IsKeyDown(int theKey); // 0x442ea0: the key is pressed or held
// 0x442d60: a key that types a letter, digit or sign (by its key code)
bool IsTypingKey(int theKey);
// 0x442a90: the character a typing key types without shift
int KeyToCharacter(int theKey);
// 0x442bc0: the character that the same key types with shift, for a character that 0x442a90 returned
int ShiftedCharacter(int theCharacter);
const int KeyCodeShift = 0x10;
const int KeyCodeBackspace = 8;

#endif
