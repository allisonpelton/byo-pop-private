#include "EngineImpl.h"

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
	: mQuitRequested(false), mWidth(theWidth), mHeight(theHeight), mVirtualClock(false), mVirtualTicks(0)
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
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "%s", aLine);
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
