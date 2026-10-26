#include "R64Sys.h"

namespace r64
{

namespace
{

SysHost* gHost = nullptr;
const PixelFormat* gScreenFormat = nullptr; // 0x46e5b8

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

const PixelFormat* Sys_ScreenFormat()
{
	return gScreenFormat;
}

void Sys_SetScreenFormat(const PixelFormat* theFmt)
{
	gScreenFormat = theFmt;
}

} // namespace r64
