#include "Engine.h"
#include "Component.h"
#include "Font.h"
#include "QDParser.h"
#include "SimpleSound.h"
#include "ResourceManager.h"
#include "Stream.h"
#include "Surface.h"

namespace engine
{

namespace
{

Engine* gEngine = nullptr;

} // namespace

Engine* getEngine()
{
	return gEngine;
}

// 0x495f30 (feedingfrenzy2); the original's WinMain sets the global (0x4a3980 (feedingfrenzy2))
Engine::Engine() : mResourceManager(new ResourceManager()), mDetailLevel(0)
{
	mResourceManager->addRef();
	gEngine = this;
}

// 0x4929b0 (feedingfrenzy2)
Engine::~Engine()
{
	mResourceManager->release();
	if (gEngine == this)
		gEngine = nullptr;
}

void Engine::addArchive(const std::string& thePath)
{
	mResourceManager->addArchive(thePath);
}

bool Engine::fileExists(const std::string& thePath)
{
	return mResourceManager->exists(thePath);
}

void Engine::setUseLooseFiles(bool theUseLooseFiles)
{
	mResourceManager->mUseLooseFiles = theUseLooseFiles;
}

InputStream* Engine::openFile(const std::string& thePath)
{
	if (!mResourceManager->mUseLooseFiles)
		return mResourceManager->openArchivedFile(thePath);
	return openDiskFile(thePath);
}

Reader* Engine::openReader(const std::string& thePath)
{
	InputStream* aStream = openFile(thePath);
	if (!aStream)
		return nullptr;
	return new InputStreamReader(aStream);
}

Reader* Engine::openDiskReader(const std::string& thePath)
{
	RandomAccessInputStream* aStream = getEngine()->openDiskFile(thePath);
	if (!aStream)
		return nullptr;
	return new InputStreamReader(aStream);
}

void Engine::setCheckDigests(bool theCheckDigests)
{
	mResourceManager->mCheckDigests = theCheckDigests;
}

void Engine::setSoundSystem(simplesound::SimpleSound* theSoundSystem)
{
	simplesound::setSimpleSound(theSoundSystem);
}

bool Engine::registerSound(const std::string& theName, const std::string& thePath)
{
	simplesound::SimpleSound* aSound = simplesound::getSimpleSound();
	if (aSound == nullptr)
		return false;
	if (aSound->isRegistered(theName))
		return true;
	// The file passed on through a SoundInputStreamAdapter (vtable 0x560d54 (feedingfrenzy2)), the sound system's
	// reader of an engine's stream; port: the stream itself
	Ref<InputStream> aStream = openFile(thePath);
	if (!aStream)
		return false;
	return aSound->registerSound(theName, aStream.get());
}

bool Engine::registerSoundStream(const std::string& theName, const std::string& thePath)
{
	simplesound::SimpleSound* aSound = simplesound::getSimpleSound();
	if (aSound == nullptr)
		return false;
	if (aSound->isRegistered(theName))
		return true;
	Ref<InputStream> aStream = openFile(thePath);
	if (!aStream)
		return false;
	return aSound->registerStream(theName, aStream.get());
}

simplesound::SoundHandle* Engine::getSoundHandle(const std::string& theName, int theSetupValue, int theCategory)
{
	simplesound::SimpleSound* aSound = simplesound::getSimpleSound();
	if (aSound == nullptr)
		return nullptr;
	return aSound->getHandle(theName, theSetupValue, theCategory);
}

bool Engine::parseXml(const std::string& thePath, DocHandler* theHandler)
{
	Ref<Reader> aReader = openReader(thePath);
	if (!aReader)
		return false;
	return QDParser::parse(theHandler, aReader);
}

bool Engine::parseDiskXml(const std::string& thePath, DocHandler* theHandler)
{
	Ref<Reader> aReader = openDiskReader(thePath);
	if (!aReader)
		return false;
	return QDParser::parse(theHandler, aReader);
}

Surface* Engine::getImage(const std::string& thePath)
{
	Surface* anImage = findImage(thePath);
	if (anImage)
		return anImage;
	anImage = loadImage(thePath);
	mImages[thePath] = anImage;
	return anImage;
}

Surface* Engine::getDiskImage(const std::string& thePath)
{
	Surface* anImage = findImage(thePath);
	if (anImage)
		return anImage;
	anImage = loadDiskImage(thePath);
	mImages[thePath] = anImage;
	return anImage;
}

// An entry whose count is 1 is held by the map alone; erasing it releases it
template <class T>
static void releaseUnused(std::map<std::string, Ref<T>>& theMap)
{
	typename std::map<std::string, Ref<T>>::iterator anIt = theMap.begin();
	while (anIt != theMap.end())
	{
		if (anIt->second->refCount() == 1)
			anIt = theMap.erase(anIt);
		else
			++anIt;
	}
}

void Engine::releaseUnusedImages()
{
	releaseUnused(mImages);
}

void Engine::releaseUnusedFonts()
{
	releaseUnused(mFonts);
}

Font* Engine::getFont(const std::string& thePath)
{
	std::map<std::string, Ref<Font>>::iterator anIt = mFonts.find(thePath);
	if (anIt != mFonts.end())
		return anIt->second;
	Font* aFont = createFont(thePath);
	mFonts[thePath] = aFont;
	return aFont;
}

Surface* Engine::loadImage(const std::string& thePath)
{
	Surface* anImage = new Surface();
	anImage->load(thePath, false);
	return anImage;
}

Surface* Engine::loadDiskImage(const std::string& thePath)
{
	Surface* anImage = new Surface();
	anImage->load(thePath, true);
	return anImage;
}

bool Engine::addImage(Surface* theImage, const std::string& thePath)
{
	if (findImage(thePath))
		return false;
	mImages[thePath] = theImage;
	return true;
}

Surface* Engine::findImage(const std::string& thePath)
{
	std::map<std::string, Ref<Surface>>::iterator anIt = mImages.find(thePath);
	if (anIt == mImages.end())
		return nullptr;
	return anIt->second;
}

Font* Engine::createFont(const std::string& thePath)
{
	Font* aFont = new Font(this);
	aFont->load(thePath);
	return aFont;
}

void Engine::addNamedComponent(const std::string& theName, Component* theComponent)
{
	mNamedComponents.insert(std::make_pair(theName, theComponent));
}

void Engine::removeNamedComponent(Component* theComponent)
{
	if (mNamedComponents.empty())
		return;
	std::pair<std::multimap<std::string, Component*>::iterator, std::multimap<std::string, Component*>::iterator> aRange =
		mNamedComponents.equal_range(theComponent->getName());
	for (std::multimap<std::string, Component*>::iterator anIt = aRange.first; anIt != aRange.second; ++anIt)
	{
		if (anIt->second == theComponent)
		{
			mNamedComponents.erase(anIt);
			return;
		}
	}
}

Component* Engine::findNamedComponent(const std::string& theName)
{
	std::multimap<std::string, Component*>::iterator anIt = mNamedComponents.lower_bound(theName);
	if (anIt == mNamedComponents.end() || anIt->first != theName)
		return nullptr;
	return anIt->second;
}

} // namespace engine
