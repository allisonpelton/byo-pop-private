#include "LuaResourceManager.h"

#include "LuaImage.h"
#include "PopLua.h"

using namespace Sexy;
using namespace LuaLib;

LuaResourceManager* Sexy::gLuaResourceManager = nullptr;

// 0x65d3c0
LuaResourceManager::LuaResourceManager(SexyAppBase* theApp, PopLua* theLua) : ResourceManager(theApp), mLua(theLua)
{
	gLuaResourceManager = this;
}

// 0x65d490
LuaResourceManager::~LuaResourceManager()
{
	if (gLuaResourceManager == this)
		gLuaResourceManager = nullptr;
}

// 0x65d930, and its compiled copy in the game's CustomResManager (0x4bfe70); 0x65dee0 (the set's insert) is inline
// here: mLoaded.insert
void LuaResourceManager::ResourceLoadedHook(BaseRes* theRes)
{
	mLoaded.insert(theRes);
}

// 0x65d950
void LuaResourceManager::ExportToLua()
{
	while (!mLoaded.empty()) {
		BaseRes* aRes = *mLoaded.begin();
		mLoaded.erase(mLoaded.begin());
		if (!aRes->mFromProgram)
			PushResource(aRes);
	}
}

// 0x66ffe0 (making the LuaImage of an image resource) and 0x670080 (making the borrowed LuaFont of a font resource) are
// inline here
void LuaResourceManager::PushResource(BaseRes* theRes)
{
	if (mLua == nullptr || mLua->L == nullptr)
		return;
	switch (theRes->mType) {
	case ResType_Image: {
		ImageRes* anImageRes = (ImageRes*)theRes;
		if ((Image*)anImageRes->mImage == nullptr)
			return;
		LuaImage* anImage = new LuaImage(anImageRes->mImage);
		anImage->mPath = theRes->mPath;
		mLua->PushObject(ClassOf<LuaImage>::info, anImage, true);
		break;
	}
	case ResType_Sound:
		lua_pushint(mLua->L, ((SoundRes*)theRes)->mSoundId);
		break;
	case ResType_Font: {
		Font* aFont = ((FontRes*)theRes)->mFont;
		if (aFont == nullptr)
			return;
		mLua->PushObject(ClassOf<LuaFont>::info, new LuaFont(theRes->mId, aFont), true);
		break;
	}
	}
	mLua->SetPath(theRes->mId);
}

// 0x65d9b0
void LuaResourceManager::DeleteResources(const std::string& theGroup)
{
	if (mLua != nullptr && mLua->L != nullptr) {
		for (ResMap* aMap : {&mImageMap, &mSoundMap, &mFontMap})
			for (auto& anEntry : *aMap) {
				BaseRes* aRes = anEntry.second;
				if (!theGroup.empty() && strcasecmp(aRes->mResGroup.c_str(), theGroup.c_str()) != 0)
					continue;
				mLoaded.erase(aRes);
				lua_pushnil(mLua->L);
				mLua->SetPath(aRes->mId);
			}
	}
	ResourceManager::DeleteResources(theGroup);
}
