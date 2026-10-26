#ifndef __COSMICBUGS_R64SYS_H__
#define __COSMICBUGS_R64SYS_H__

// Retro64's system layer (0x428000..0x42a800, 0x42e1d0..): what the original asked of Windows, DirectDraw,
// DirectInput and BASS. The port's host (src/CosmicBugsApp) implements SysHost on the shared platform layer.

#include <cstdint>
#include <vector>

namespace r64
{

struct PixelFormat;

class SysHost
{
public:
	virtual ~SysHost() {}
	// A whole file of the game's; thePath is relative to the game's folder, with either separator
	virtual bool ReadFile(const char* thePath, std::vector<uint8_t>& theData) = 0;
	// 0x42e6e0 (through 0x428f50): a rectangle of the screen that changed, for the next present
	virtual void AddUpdateRect(int x, int y, int w, int h) {}
	// 0x428890: puts the mouse pointer at (x, y) of the game's screen (the original moves the system's)
	virtual void SetMousePos(int x, int y) {}
	// 0x429e40: where the mouse pointer is on the game's screen (the original reads the system's and takes off the
	// window's place)
	virtual void GetMousePos(int* x, int* y) { *x = 0; *y = 0; }
};

void Sys_SetHost(SysHost* theHost);
SysHost* Sys_Host();

// fopen/fseek/ftell/fread on the whole file, as the loaders do: false when it cannot be read
bool Sys_ReadFile(const char* thePath, std::vector<uint8_t>& theData);
// 0x428f50
void Sys_AddUpdateRect(int x, int y, int w, int h);
// 0x428890
void Sys_SetMousePos(int x, int y);
// 0x70819c: the original's PumpMessages (0x42a070) pumps the window's messages only while it is 1, and it picks
// which focus messages pause the sound; only the demo's power-ups set it (the port's host does neither)
extern int gPumpMessages;

// The input (0x4289d0..0x428b20), which the window's messages fill: the mouse buttons held, the keys held (by
// virtual-key code) and a ring of the last 100 keys pressed. The host feeds them its events with Sys_KeyEvent and
// Sys_MouseButton.
int Mouse_State(int theButton);          // 0x4289d0: 1 the left button, 2 the right, 3 the middle: held
int Mouse_Pressed(int theButton);        // 0x428a00: once for each press
int Key_Pop();                           // 0x428aa0: the queue's newest key, 0 when it is empty
void Key_Push(int theKey);               // 0x428ad0
void Key_Clear();                        // 0x428b00: empties the queue (not the keys held)
int Key_Down(int theKey);                // 0x428b20: held
void Mouse_Update(int* x, int* y);       // 0x429e40: the pointer on the game's screen
void Sys_KeyEvent(int theKey, int theDown);
void Sys_MouseButton(int theButton, int theDown);
// 0x428eb0: the screen's pixel format (gScreenFormat 0x46e5b8 points at it)
const PixelFormat* Sys_ScreenFormat();
void Sys_SetScreenFormat(const PixelFormat* theFmt);

} // namespace r64

#endif
