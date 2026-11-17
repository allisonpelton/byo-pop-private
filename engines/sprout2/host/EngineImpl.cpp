#include "EngineImpl.h"
#include "RegistryKey.h"
#include "SexyAppBase.h"
#include "WidgetManager.h"

#include "platform/VFS.h"

#include <SDL.h>
#include <cstdarg>

namespace engine
{

FileInputStream::~FileInputStream()
{
	if (mFile != nullptr)
		fclose(mFile);
}

int FileInputStream::read(void* theBuffer, int theSize)
{
	return (int)fread(theBuffer, 1, (size_t)theSize, mFile);
}

bool FileInputStream::close()
{
	bool aClosed = fclose(mFile) == 0;
	mFile = nullptr;
	return aClosed;
}

bool FileInputStream::seek(int theOffset, int theOrigin)
{
	return fseek(mFile, theOffset, theOrigin) == 0;
}

int FileInputStream::tell()
{
	return (int)ftell(mFile);
}

EngineImpl::EngineImpl(int theWidth, int theHeight)
	: mQuitRequested(false), mWidth(theWidth), mHeight(theHeight), mFrameRate(0), mVirtualClock(false),
	  mVirtualTicks(0)
{
}

EngineImpl::~EngineImpl()
{
}

bool EngineImpl::diskFileExists(const std::string& thePath)
{
	return Platform::VFS::Get().Exists(thePath);
}

RandomAccessInputStream* EngineImpl::openDiskFile(const std::string& thePath)
{
	FILE* aFile = Platform::VFS::Get().Open(thePath, "rb");
	if (aFile == nullptr)
		return nullptr;
	return new FileInputStream(aFile);
}

void EngineImpl::log(const char* theFormat, ...)
{
	char aLine[0x800];
	va_list anArgs;
	va_start(anArgs, theFormat);
	vsnprintf(aLine, sizeof(aLine), theFormat, anArgs);
	va_end(anArgs);
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s", aLine);
}

PersistKey* EngineImpl::openPersistKey(PersistKey* theParent, const std::string& theName)
{
	const std::string& aParentPath = static_cast<RegistryKey*>(theParent)->getPath();
	return new RegistryKey(aParentPath.empty() ? theName : aParentPath + "\\" + theName);
}

PersistKey* EngineImpl::openPersistKey(int theRoot, const std::string& thePath)
{
	std::string aPathBelow;
	if (!RegistryKey::pathBelowApplication(thePath, aPathBelow))
	{
		log("Failed to open registry key %s.\n", thePath.c_str());
		return nullptr;
	}
	return new RegistryKey(aPathBelow);
}

bool EngineImpl::isKeyDown(int theKey)
{
	if (theKey < 0 || theKey >= 0xff)
		return false;
	return Sexy::gSexyAppBase->mWidgetManager->mKeyDown[theKey];
}

bool EngineImpl::getClipboardText(std::string& theText)
{
	theText = Sexy::gSexyAppBase->GetClipboard();
	return !theText.empty();
}

uint64_t EngineImpl::getTicks()
{
	if (mVirtualClock)
		return mVirtualTicks;
	return SDL_GetPerformanceCounter();
}

uint64_t EngineImpl::getTicksPerSecond()
{
	return SDL_GetPerformanceFrequency();
}

} // namespace engine
