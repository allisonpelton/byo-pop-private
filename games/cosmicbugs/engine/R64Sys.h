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
};

void Sys_SetHost(SysHost* theHost);
SysHost* Sys_Host();

// fopen/fseek/ftell/fread on the whole file, as the loaders do: false when it cannot be read
bool Sys_ReadFile(const char* thePath, std::vector<uint8_t>& theData);
// 0x428f50
void Sys_AddUpdateRect(int x, int y, int w, int h);
// 0x428eb0: the screen's pixel format (gScreenFormat 0x46e5b8 points at it)
const PixelFormat* Sys_ScreenFormat();
void Sys_SetScreenFormat(const PixelFormat* theFmt);

} // namespace r64

#endif
