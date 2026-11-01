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

const uint8_t* FindResourceData(Module theModule, const char* theType, int theId, uint32_t* theSize)
{
	return gHost->FindResource(theModule, theType, 0, theId, theSize);
}

void OpenUrl(const char* theUrl)
{
	gHost->OpenUrl(theUrl);
}

// 0x423590
std::string LoadDllString(int theId)
{
	std::string aText;
	gHost->LoadString(Module::Dll, theId, aText);
	return aText;
}

// 0x423570
std::string LoadExeString(int theId)
{
	std::string aText;
	gHost->LoadString(Module::Exe, theId, aText);
	return aText;
}

} // namespace jb
