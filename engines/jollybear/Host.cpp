#include "Host.h"

namespace jb
{

static Host* gHost = nullptr;

void SetHost(Host* theHost)
{
	gHost = theHost;
}

Host* GetHost()
{
	return gHost;
}

uint32_t TickCount()
{
	return gHost->TickCount();
}

uint32_t RealTickCount()
{
	return gHost->RealTickCount();
}

uint32_t SeedTick()
{
	return gHost->SeedTick();
}

const uint8_t* FindResourceData(Module theModule, const char* theType, int theId, uint32_t* theSize)
{
	return gHost->FindResource(theModule, theType, 0, theId, theSize);
}

// 0x425820 (mahjongchina), 0x423590 (mahjongjapan)
std::string LoadDllString(int theId)
{
	std::string aText;
	gHost->LoadString(Module::DLL, theId, aText);
	return aText;
}

// 0x425800 (mahjongchina), 0x423570 (mahjongjapan)
std::string LoadExeString(int theId)
{
	std::string aText;
	gHost->LoadString(Module::EXE, theId, aText);
	return aText;
}

} // namespace jb
