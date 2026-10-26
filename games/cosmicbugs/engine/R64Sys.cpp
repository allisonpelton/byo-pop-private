#include "R64Sys.h"

namespace r64
{

namespace
{

SysHost* gHost = nullptr;

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

} // namespace r64
