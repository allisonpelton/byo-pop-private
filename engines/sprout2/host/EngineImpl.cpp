#include "EngineImpl.h"
#include "PortCompat.h"
#include "RegistryKey.h"
#include "SimpleSoundDX.h"
#include "SexyAppBase.h"
#include "WidgetManager.h"

#include "platform/Audio.h"
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
	  mVirtualTicks(0), mGameTicks(SDL_GetPerformanceCounter()), mGameTicksPart(0.0), mLastCounter(mGameTicks),
	  mLastGameTimeMS(Sexy::GetGameTimeMS()), mMouse(theWidth, theHeight)
{
}

EngineImpl::~EngineImpl()
{
}

simplesound::SimpleSound* EngineImpl::makeSoundSystem()
{
	// The original starts DirectSound on the window (0x4a2870 (feedingfrenzy2)) and gives none when it cannot; the
	// port's mixer is the platform's, started by the framework
	if (!Platform::Mixer::Get().IsInitialized())
		return nullptr;
	return new simplesounddx::SimpleSoundDX();
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
	uint64_t aCounter = SDL_GetPerformanceCounter();
	uint32_t aGameTimeMS = Sexy::GetGameTimeMS();
	double aSpeed = Sexy::gSexyAppBase->GameSpeed();
	if (aSpeed == Sexy::SexyAppBase::SPEED_UNLIMITED)
		// The framework moves its game clock a frame's time each update, run back to back
		mGameTicksPart += (double)(uint32_t)(aGameTimeMS - mLastGameTimeMS) * (double)getTicksPerSecond() / 1000.0;
	else
		mGameTicksPart += (double)(aCounter - mLastCounter) * aSpeed;
	mLastCounter = aCounter;
	mLastGameTimeMS = aGameTimeMS;
	uint64_t aWholeTicks = (uint64_t)mGameTicksPart;
	mGameTicks += aWholeTicks;
	mGameTicksPart -= (double)aWholeTicks;
	return mGameTicks;
}

uint64_t EngineImpl::getTicksPerSecond()
{
	return SDL_GetPerformanceFrequency();
}

} // namespace engine
