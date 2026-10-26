#include "App.h"
#include "Exception.h"
#include "Font.h"
#include "Graphics.h"
#include "Host.h"

using namespace engine;

static App* gApp = nullptr;  // 0x49bfbc
static Host* gHost = nullptr;

void engine::setHost(Host* theHost)
{
	gHost = theHost;
}

Host* engine::getHost()
{
	return gHost;
}

// 0x4382f0
App::App()
	: mResources(new ResourceManager()), mLastTime(0)
{
	mConfig = AppConfig();
	gApp = this;
}

// 0x4381b0
App::~App()
{
	if (gApp == this)
		gApp = nullptr;
}

// 0x435f20
App* App::instance()
{
	return gApp;
}

static SoundSystem* gSoundSystem = nullptr;  // 0x49bfc0

// 0x435f30
SoundSystem* App::getSoundSystem()
{
	return gSoundSystem;
}

// 0x435f40
void App::setSoundSystem(SoundSystem* theSoundSystem)
{
	gSoundSystem = theSoundSystem;
}

// 0x437b20
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

// 0x437be0
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

// 0x437dc0: a cached null is loaded again
Font* App::getFont(const char* theName)
{
	std::string aName = theName;
	std::map<std::string, Ref<Font>>::iterator anIt = mFonts.find(aName);
	Font* aFont;
	if (anIt == mFonts.end() || (aFont = anIt->second) == nullptr)
	{
		aFont = createFont(theName, false);
		mFonts[aName] = aFont;
	}
	return aFont;
}

// 0x437e90
Font* App::getFileFont(const char* theName)
{
	std::string aName = theName;
	std::map<std::string, Ref<Font>>::iterator anIt = mFonts.find(aName);
	Font* aFont;
	if (anIt == mFonts.end() || (aFont = anIt->second) == nullptr)
	{
		aFont = createFont(theName, true);
		mFonts[aName] = aFont;
	}
	return aFont;
}

// 0x453f80, 0x454010 and the loader 0x457510: a PNG or a JPEG, and the alpha of "<dir>_<name>.png" when that exists.
// (The original opens the mask through the archive or as a loose file by the low byte of the name's address, a slip
// for theFromFile; the port uses theFromFile.)
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

// 0x437ca0
bool App::addImage(Image* theImage, const char* theName)
{
	if (findImage(theName) != nullptr)
		return false;
	mImages.insert(std::make_pair(std::string(theName), Ref<Image>(theImage)));
	return true;
}

// 0x436a70
Image* App::findImage(const char* theName)
{
	std::map<std::string, Ref<Image>>::iterator anIt = mImages.find(theName);
	if (anIt != mImages.end())
		return anIt->second;
	return nullptr;
}

void App::log(const char* theText)
{
	getHost()->log(theText);
}

// 0x454180
uint32_t App::getTime()
{
	return getHost()->timeMs();
}

// 0x454230: the code is not used
void App::quit(int theCode)
{
	getHost()->quit();
}

// 0x4540a0, 0x454110
Font* App::createFont(const char* theName, bool theFromFile)
{
	Font* aFont = new Font(mResources);
	if (theFromFile)
		aFont->loadFile(theName);
	else
		aFont->load(theName);
	return aFont;
}

// 0x437f60: a multimap, so names may repeat
void App::registerNode(Node* theNode)
{
	mNodes.insert(std::make_pair(theNode->getName(), theNode));
}

// 0x436ff0
void App::unregisterNode(Node* theNode)
{
	std::pair<std::multimap<std::string, Node*>::iterator, std::multimap<std::string, Node*>::iterator> aRange =
		mNodes.equal_range(theNode->getName());
	for (std::multimap<std::string, Node*>::iterator anIt = aRange.first; anIt != aRange.second; ++anIt)
	{
		if (anIt->second == theNode)
		{
			mNodes.erase(anIt);
			return;
		}
	}
}

Node* App::findNode(const std::string& theName)
{
	std::multimap<std::string, Node*>::iterator anIt = mNodes.find(theName);
	if (anIt != mNodes.end())
		return anIt->second;
	return nullptr;
}

// 0x4552b0, before its loop
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

// 0x4542e0
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

// The run loop's end: the controller's shutdown
void App::shutdown()
{
	if (mController != nullptr)
		mController->shutdown();
}
