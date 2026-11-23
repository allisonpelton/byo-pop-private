#include "App.h"
#include "Exception.h"
#include "Font.h"
#include "Graphics.h"
#include "Host.h"
#include "Sound.h"

using namespace engine;

static App* gApp = nullptr;  // 0x49bfbc (wordharmony)
static Host* gHost = nullptr;

// port: the host that stands in for the Windows layer (Host.h), set by the game's app before the engine starts
void engine::setHost(Host* theHost)
{
	gHost = theHost;
}

// port: the host set by setHost
Host* engine::getHost()
{
	return gHost;
}

// 0x4382f0 (wordharmony)
App::App()
	: mResources(new ResourceManager()), mLastTime(0)
{
	mConfig = AppConfig();
	gApp = this;
}

// 0x4381b0 (wordharmony)
App::~App()
{
	if (gApp == this)
		gApp = nullptr;
}

// 0x435f20 (wordharmony)
App* App::instance()
{
	return gApp;
}

static SoundSystem* gSoundSystem = nullptr;  // 0x49bfc0 (wordharmony)

// 0x435f30 (wordharmony)
SoundSystem* App::getSoundSystem()
{
	return gSoundSystem;
}

// 0x435f40 (wordharmony)
void App::setSoundSystem(SoundSystem* theSoundSystem)
{
	gSoundSystem = theSoundSystem;
}

// 0x437b20 (wordharmony)
Image* App::getImage(const char* theName)
{
	std::string aName = theName;
	Image* anImage = findImage(theName);
	if (anImage == nullptr)
	{
		anImage = createImage(theName, false);
		mImages.insert(std::make_pair(aName, Ref<Image>(anImage)));
	}
	return anImage;
}

// 0x437be0 (wordharmony)
Image* App::getFileImage(const char* theName)
{
	std::string aName = theName;
	Image* anImage = findImage(theName);
	if (anImage == nullptr)
	{
		anImage = createImage(theName, true);
		mImages.insert(std::make_pair(aName, Ref<Image>(anImage)));
	}
	return anImage;
}

// 0x437dc0 (wordharmony): a cached null is loaded again
Font* App::getFont(const char* theName)
{
	std::string aName = theName;
	std::map<std::string, Ref<Font>>::iterator anIt = mFonts.find(aName);
	Font* aFont;
	if (anIt == mFonts.end() || (aFont = anIt->second) == nullptr)
	{
		aFont = createFont(theName);
		mFonts[aName] = aFont;
	}
	return aFont;
}

// 0x445310 (feedingfrenzy)
bool App::addImage(const char* theName, Image* theImage)
{
	std::map<std::string, Ref<Image>>::iterator anIt = mImages.find(theName);
	if (anIt != mImages.end() && anIt->second != nullptr)
		return false;
	mImages[theName] = Ref<Image>(theImage);
	return true;
}

// 0x453f80 (wordharmony), 0x454010 (wordharmony) and the loader 0x457510 (wordharmony): a PNG or a JPEG, and the alpha
// of "<dir>_<name>.png" when that exists. (The original opens the mask through the archive or as a loose file by the
// low byte of the name's address, a slip for theFromFile; the port uses theFromFile.)
Image* App::createImage(const char* theName, bool theFromFile)
{
	std::string aName = theName;
	for (char& c : aName)
	{
		if (c == '\\')
			c = '/';
	}
	size_t aSlash = aName.rfind('/');
	std::string aDir = aSlash == std::string::npos ? "" : aName.substr(0, aSlash + 1);
	std::string aBase = aSlash == std::string::npos ? aName : aName.substr(aSlash + 1);
	size_t aDot = aBase.rfind('.');
	if (aDot != std::string::npos)
		aBase = aBase.substr(0, aDot);
	std::string aMaskName = aDir + "_" + aBase + ".png";

	Ref<InputStream> aStream = theFromFile ? openFile(theName) : openRawStream(theName);
	bool aHasMask = theFromFile ? fileExists(aMaskName.c_str()) : exists(aMaskName.c_str());
	std::vector<uint8_t> aData;
	readAll(aStream, aData);
	std::vector<uint8_t> aMask;
	if (aHasMask)
	{
		Ref<InputStream> aMaskStream = theFromFile ? openFile(aMaskName.c_str()) : openRawStream(aMaskName.c_str());
		readAll(aMaskStream, aMask);
	}
	if (aName.find(".jpg") == std::string::npos && aName.find(".png") == std::string::npos)
		return nullptr;
	Image* anImage = getHost()->createImage(aData, aHasMask ? &aMask : nullptr);
	if (anImage == nullptr)
		throw Exception(std::string("Unable to load image: ") + theName);
	return anImage;
}

// 0x436a70 (wordharmony)
Image* App::findImage(const char* theName)
{
	std::map<std::string, Ref<Image>>::iterator anIt = mImages.find(theName);
	if (anIt != mImages.end())
		return anIt->second;
	return nullptr;
}

// port (the "memreport" test command): the pictures of the image table and their pixels, 4 bytes each
int App::getImageMemory(size_t* theBytes) const
{
	*theBytes = 0;
	for (std::map<std::string, Ref<Image>>::const_iterator anIt = mImages.begin(); anIt != mImages.end(); ++anIt)
		*theBytes += (size_t)anIt->second->getWidth() * anIt->second->getHeight() * 4;
	return (int)mImages.size();
}

// port: slot 0x58, the log, which does nothing in the release builds; the port's goes to the host
void App::log(const char* theText)
{
	getHost()->log(theText);
}

// 0x461a50 (pizzafrenzy): the signal build's slot 0x30 (0x461830 (pizzafrenzy)) opens the file, which is
// openRawStream's (the archive, or a loose file when the files are used)
bool App::loadSound(const std::string& theName, const std::string& theFile)
{
	SoundSystem* aSystem = getSoundSystem();
	if (aSystem == nullptr)
		return false;
	if (aSystem->isLoaded(theName.c_str()))
		return true;
	Ref<InputStream> aStream = openRawStream(theFile.c_str());
	return aSystem->load(theName.c_str(), aStream);
}

// 0x461b70 (pizzafrenzy)
bool App::loadSoundStream(const std::string& theName, const std::string& theFile)
{
	SoundSystem* aSystem = getSoundSystem();
	if (aSystem == nullptr)
		return false;
	if (aSystem->isLoaded(theName.c_str()))
		return true;
	Ref<InputStream> aStream = openRawStream(theFile.c_str());
	return aSystem->loadStream(theName.c_str(), aStream);
}

// 0x461c90 (pizzafrenzy)
Sound* App::createSound(const std::string& theName, bool theStatic, int theCategory)
{
	SoundSystem* aSystem = getSoundSystem();
	if (aSystem == nullptr)
		return nullptr;
	return aSystem->createSound(theName.c_str(), theStatic, theCategory);
}

// 0x454180 (wordharmony)
uint32_t App::getTime()
{
	return getHost()->timeMs();
}

// 0x4108a0 (feedingfrenzy): SetCursorPos, with the position the game reads from then on
void App::warpMousePosition(int theX, int theY)
{
	mMousePosition = Point((float)theX, (float)theY);
	getHost()->warpCursor(theX, theY);
}

// 0x454230 (wordharmony): the code is not used
void App::quit(int theCode)
{
	getHost()->quit();
}

// 0x4540a0 (wordharmony) (0x454110 (wordharmony), the same from a loose file, is left out)
Font* App::createFont(const char* theName)
{
	Font* aFont = new Font(mResources);
	aFont->load(theName);
	return aFont;
}

// 0x4552b0 (wordharmony), before its loop
void App::start(Controller* theController)
{
	mController = theController;
	mController->configure(mConfig);
	if (mConfig.mTitle == nullptr || mConfig.mFrameRate == 0)
		throw Exception("Invalid configuration.");
	mResources->setUseFiles(mConfig.mUseFiles);
	mRootWindow = new Window();
	mController->init();
	mLastTime = getTime();
}

// 0x4bd790 (pizzafrenzy)
std::string App::loadString(int theId)
{
	return getHost()->loadString(theId);
}

// 0x4bda70 (pizzafrenzy), and 0x4bdf20 (pizzafrenzy) up to its loop: the window and the display are the host's; the
// configure checks the same two fields as the earlier build's
void App::startSignalled(Controller* theController)
{
	mController = theController;
	mController->configure(mConfig);
	if (mConfig.mTitle == nullptr || mConfig.mFrameRate == 0)
		throw Exception("Invalid configuration.");
	mController->init();
	mLastTicks = getHost()->timeTicks();
}

// 0x4bd210 (pizzafrenzy): the frame's update and its drawing (the frame times, +0x220 and +0x240, are kept for a
// statistics display the game never shows); the host waits for the next frame's time (the start of this one and a
// period, 0x4bc870 (pizzafrenzy))
void App::frameSignalled(Graphics* g)
{
	UpdateInfo anInfo;
	anInfo.mOrigin = Point(0, 0);
	uint64_t aNow = getHost()->timeTicks();
	anInfo.mSeconds = (float)(int64_t)(aNow - mLastTicks) / (float)(int64_t)getHost()->ticksPerSecond();
	if (0.33f < anInfo.mSeconds)
		anInfo.mSeconds = 0.33f;
	mLastTicks = aNow;
	mUpdateSignal.emit(anInfo);
	if (g != nullptr)
		mDrawSignal.emit(g);
}

// 0x4542e0 (wordharmony)
void App::frame(Graphics* g)
{
	UpdateInfo anInfo;
	anInfo.mOrigin = Point(0, 0);
	uint32_t aNow = getTime();
	float anElapsed = (float)(aNow - mLastTime);
	anInfo.mSeconds = anElapsed / (float)getTicksPerSecond();
	if (0.33f < anInfo.mSeconds)
		anInfo.mSeconds = 0.33f;
	mLastTime = aNow;
	mController->update(anInfo);
	mRootWindow->update(anInfo);
	mRootWindow->layout();
	if (g != nullptr)
		mRootWindow->draw(g);
}

// port: the run loop's end, the controller's shutdown (after the loop of 0x4552b0 (wordharmony))
void App::shutdown()
{
	if (mController != nullptr)
		mController->shutdown();
}
