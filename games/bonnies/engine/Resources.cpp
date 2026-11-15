// The resource cache (see Resources.h).

#include "Resources.h"
#include "Font.h"
#include "Graphics.h"
#include "Host.h"
#include "Platform.h"
#include "Sound.h"

namespace bonnies
{

Resources* gResources = nullptr;
int gLoadingFont = 0;

namespace
{
Resources::Entry* gLists[3];  // 0x480b10: one list per type
}

// 0x421960
Resources::Resources()
{
	mFreeNow = 0;
	mNoSharing = 0;
	mNextSweep = 0;
	gLists[0] = nullptr;
	gLists[1] = nullptr;
	gLists[2] = nullptr;
}

// 0x421980
int Resources::key(const Str& theName, int theType)
{
	return theType * 1000 + (int)hashName(theName);
}

// 0x4219f0
void* Resources::get(const Str& theName, int theType, FontDesc* theDesc)
{
	int aKey = key(theName, theType);
	Entry* anEntry = gLists[theType];
	if (mNoSharing == 0)
	{
		for (; anEntry != nullptr; anEntry = anEntry->mNext)
		{
			if (anEntry->mKey == aKey)
			{
				anEntry->mCount = anEntry->mCount + 1;
				return anEntry->mResource;
			}
		}
	}
	anEntry = new Entry();
	if (theType == TYPE_FONT)
		anEntry->mResource = loadFont(theDesc);
	else if (theType == TYPE_IMAGE)
		anEntry->mResource = loadImageSet(theName);
	else if (theType == TYPE_SOUND)
		anEntry->mResource = SoundSystem::loadSample(theName);
	anEntry->mDue = 0;
	anEntry->mType = theType;
	anEntry->mCount = 1;
	anEntry->mKey = aKey;
	anEntry->mNext = gLists[theType];
	if (gLists[theType] != nullptr)
		gLists[theType]->mPrev = anEntry;
	anEntry->mPrev = nullptr;
	gLists[theType] = anEntry;
	return anEntry->mResource;
}

// 0x421b30
void Resources::free(Entry* theEntry)
{
	int aType = theEntry->mType;
	if (aType == TYPE_FONT)
	{
		if (gPlatform != nullptr)
			freeFont((Font*)theEntry->mResource);
	}
	else if (aType == TYPE_IMAGE)
	{
		if (gGraphics != nullptr)
			freeImage((Image*)theEntry->mResource);
	}
	else if (aType == TYPE_SOUND && gSound != nullptr)
		SoundSystem::freeSample((Sample*)theEntry->mResource);
	if (theEntry->mPrev != nullptr)
		theEntry->mPrev->mNext = theEntry->mNext;
	if (theEntry->mNext != nullptr)
		theEntry->mNext->mPrev = theEntry->mPrev;
	if (gLists[theEntry->mType] == theEntry)
		gLists[theEntry->mType] = theEntry->mNext;
	delete theEntry;
}

// port: the memory report's figures
Resources::MemoryUse Resources::memoryUse()
{
	MemoryUse aUse;
	for (Entry* anEntry = gLists[TYPE_IMAGE]; anEntry != nullptr; anEntry = anEntry->mNext)
	{
		const Image* anImage = (const Image*)anEntry->mResource;
		if (anImage == nullptr)
			continue;
		aUse.mImageCount++;
		aUse.mImageBytes += anImage->mSurface.capacity() * sizeof(uint32_t) + (size_t)anImage->mFrameCount * sizeof(Frame);
	}
	for (Entry* anEntry = gLists[TYPE_FONT]; anEntry != nullptr; anEntry = anEntry->mNext)
		aUse.mFontCount++;
	for (Entry* anEntry = gLists[TYPE_SOUND]; anEntry != nullptr; anEntry = anEntry->mNext)
		aUse.mSoundCount++;
	return aUse;
}

// 0x421bc0
void Resources::release(void* theResource)
{
	int aType = 0;
	do
	{
		for (Entry* anEntry = gLists[aType]; anEntry != nullptr; anEntry = anEntry->mNext)
		{
			if (anEntry->mResource == theResource)
			{
				int aCount = anEntry->mCount;
				anEntry->mCount = aCount - 1;
				if (aCount - 1 < 1)
				{
					anEntry->mDue = (int)host()->getTicks() + (aType != TYPE_IMAGE ? 0xbd74 : 0) + 0x5dc;
					if (mFreeNow != 0)
						free(anEntry);
				}
				return;
			}
		}
		aType = aType + 1;
	} while (aType < 3);
}

// 0x421c40
Image* Resources::getImage(const Str& theName)
{
	return (Image*)get(theName, TYPE_IMAGE, nullptr);
}

// 0x421cc0
Sample* Resources::getSound(const Str& theName)
{
	return (Sample*)get(theName, TYPE_SOUND, nullptr);
}

// 0x421d30
Font* Resources::getFont(FontDesc* theDesc)
{
	gLoadingFont = gLoadingFont + 1;
	Font* aFont = (Font*)get(theDesc->mName, TYPE_FONT, theDesc);
	gLoadingFont = gLoadingFont - 1;
	return aFont;
}

// 0x421d70
void Resources::releaseAll()
{
	mFreeNow = 1;
	Entry** aList = gLists;
	do
	{
		Entry* anEntry = *aList;
		while (anEntry != nullptr)
		{
			release(anEntry->mResource);
			anEntry = *aList;
		}
		aList = aList + 1;
	} while (aList < gLists + 3);
}

// 0x421db0
void Resources::sweep()
{
	int aStart = (int)host()->getTicks();
	Entry** aList = gLists;
	do
	{
		bool aRestart = false;
		for (Entry* anEntry = *aList; anEntry != nullptr; anEntry = anEntry->mNext)
		{
			if (anEntry->mCount == 0 && anEntry->mDue < (int)host()->getTicks())
			{
				free(anEntry);
				if (0x14 < (int)host()->getTicks() - aStart)
				{
					mNextSweep = (int)host()->getTicks() + 200;
					return;
				}
				aRestart = true;
				break;
			}
		}
		if (aRestart)
			continue;
		aList = aList + 1;
	} while (aList < gLists + 3);
	mNextSweep = (int)host()->getTicks() + 0x9c4;
}

// 0x421e30
void Resources::update()
{
	if (mNextSweep < (int)host()->getTicks())
		sweep();
}

} // namespace bonnies
