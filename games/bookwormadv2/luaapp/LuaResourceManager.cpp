#include "LuaResourceManager.h"

#include "LuaImage.h"
#include "PopLua.h"

using namespace Sexy;
using namespace LuaLib;

LuaResourceManager* Sexy::gLuaResourceManager = nullptr;

LuaResourceManager::LuaResourceManager(SexyAppBase* theApp, PopLua* theLua) : ResourceManager(theApp), mLua(theLua)
{
	gLuaResourceManager = this;
}

LuaResourceManager::~LuaResourceManager()
{
	if (gLuaResourceManager == this)
		gLuaResourceManager = nullptr;
}

void LuaResourceManager::ResourceLoadedHook(BaseRes* theRes)
{
	mLoaded.insert(theRes);
}

void LuaResourceManager::ExportToLua()
{
	while (!mLoaded.empty()) {
		BaseRes* aRes = *mLoaded.begin();
		mLoaded.erase(mLoaded.begin());
		if (!aRes->mFromProgram)
			PushResource(aRes);
	}
}

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
