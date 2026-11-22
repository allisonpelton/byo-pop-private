// Sexy::PrimeResourceManager: see the header. Bejeweled3.exe's functions from 0x45b540 (bejeweled3) to 0x465600 (bejeweled3), with the
// loading of pictures, sounds and fonts done as the framework's ResourceManager does it on this port (the same calls and
// port notes).
#include "PrimeResourceManager.h"

#include "DDImage.h"
#include "DDInterface.h"
#include "ImageFont.h"
#include "PIEffect.h"
#include "PopAnim.h"
#include "RenderEffectDefinition.h"
#include "SexyAppBase.h"
#include "SoundManager.h"
#include "XMLParser.h"
#include "../platform/ImageLoader.h"

#include <cstring>
#include <exception>

using namespace Sexy;

namespace {

// port: texture format hints (D3DInterface.h), stored in MemoryImage::mD3DFlags as the framework's ResourceManager does
enum D3DImageFlags {
	D3DImageFlag_MinimizeNumSubdivisions = 0x0001,
	D3DImageFlag_UseA4R4G4B4 = 0x0004,
	D3DImageFlag_UseA8R8G8B8 = 0x0008,
	D3DImageFlag_CubeMap = 0x0020,
	D3DImageFlag_VolumeMap = 0x0040,
	D3DImageFlag_NoTriRep = 0x0080,
	D3DImageFlag_Atlas = 0x0201,   // an atlas keeps one piece (MinimizeNumSubdivisions) and 0x200
};

// A missing resource asked for with a Get...Throw: a ResourceManagerException where the build has exceptions, else the end
// of the program, as an uncaught one would be (framework/ResourceManager.cpp)
[[noreturn]] void ThrowResourceError(const std::string& theText)
{
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS)
	throw ResourceManagerException(theText);
#else
	std::terminate();
#endif
}

// 0x45dec0 (bejeweled3): "1,2,3" as numbers
void ReadIntVector(const SexyString& theVal, std::vector<int>& theVector)
{
	theVector.clear();
	std::string::size_type aPos = 0;
	while (true) {
		theVector.push_back(sexyatoi(theVal.c_str() + aPos));
		aPos = theVal.find_first_of(_S(','), aPos);
		if (aPos == std::string::npos)
			break;
		aPos++;
	}
}

bool HasAttribute(XMLElement& theElement, const SexyChar* theName)
{
	return theElement.mAttributes.find(theName) != theElement.mAttributes.end();
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// The resources

PrimeResourceManager::BaseRes::BaseRes(ResType theType)
{
	mResourceManager = nullptr;
	mGlobalPtr = nullptr;
	mRefRes = nullptr;
	mRefCount = 0;
	mDirectLoaded = false;
	mType = theType;
	mArtRes = 0;
	mFromProgram = false;
}

void PrimeResourceManager::BaseRes::ReleaseRefRes()
{
	if (mRefRes != nullptr) {
		mResourceManager->Deref(mRefRes);
		mRefRes = nullptr;
	}
}

PrimeResourceManager::ImageRes::ImageRes() : BaseRes(ResType_Image)
{
	mAutoFindAlpha = true;
	mPalletize = true;
	mA4R4G4B4 = false;
	mA8R8G8B8 = false;
	mDither16 = false;
	mDDSurface = false;
	mPurgeBits = false;
	mMinimizeSubdivisions = false;
	mCubeMap = false;
	mVolumeMap = false;
	mNoTriRep = false;
	m2DBig = false;
	mIsAtlas = false;
	mRows = 1;
	mCols = 1;
	mAlphaColor = 0xffffff;
	mAtlasX = 0;
	mAtlasY = 0;
	mAtlasW = 0;
	mAtlasH = 0;
}

void PrimeResourceManager::ImageRes::DeleteResource()
{
	if (mRefRes != nullptr)
		ReleaseRefRes();
	if (mGlobalPtr != nullptr)
		*mGlobalPtr = nullptr;
	mImage.Release();
}

void PrimeResourceManager::ImageRes::ApplyConfig()
{
	if (mRefRes != nullptr)
		return;   // the resource it names has its own
	DDImage* anImage = (DDImage*)mImage;
	if (anImage == nullptr)
		return;

	anImage->mD3DFlags = 0;
	if (mNoTriRep)
		anImage->mD3DFlags = D3DImageFlag_NoTriRep;
	anImage->mNumRows = mRows;
	anImage->mNumCols = mCols;
	anImage->mNumTotal = mRows * mCols;   // port: the framework's images also keep the cel count
	if (mA4R4G4B4)
		anImage->mD3DFlags |= D3DImageFlag_UseA4R4G4B4;
	if (mA8R8G8B8)
		anImage->mD3DFlags |= D3DImageFlag_UseA8R8G8B8;
	if (mMinimizeSubdivisions)
		anImage->mD3DFlags |= D3DImageFlag_MinimizeNumSubdivisions;
	if (mCubeMap)
		anImage->mD3DFlags |= D3DImageFlag_CubeMap;
	else if (mVolumeMap)
		anImage->mD3DFlags |= D3DImageFlag_VolumeMap;
	if (mAnimInfo.mAnimType != AnimType_None) {
		delete anImage->mAnimInfo;
		anImage->mAnimInfo = new AnimInfo(mAnimInfo);
	}
	if (mIsAtlas)
		anImage->mD3DFlags |= D3DImageFlag_Atlas;
	// "parent": the original draws the image from its atlas's rectangle; no image of the manifest has one, so the
	// port keeps only the setting (0x45c000 (bejeweled3) sets the image's atlas image and rectangle)

	anImage->CommitBits();
	anImage->mPurgeBits = mPurgeBits;
	if (mDDSurface) {
		anImage->CommitBits();
		if (!anImage->mHasAlpha) {
			anImage->mWantDDSurface = true;
			anImage->mPurgeBits = true;
		}
	}
	if (anImage->mPurgeBits)
		anImage->PurgeBits();
}

// port: the exe has no constructor of its own for it; ParseSoundResource (0x45db30 (bejeweled3)) sets these fields
PrimeResourceManager::SoundRes::SoundRes() : BaseRes(ResType_Sound)
{
	mSoundId = -1;
	mVolume = -1.0;
	mPanning = 0;
}

void PrimeResourceManager::SoundRes::DeleteResource()
{
	if (mRefRes != nullptr)
		ReleaseRefRes();
	else if (mSoundId != -1)
		mResourceManager->mApp->mSoundManager->ReleaseSound(mSoundId);
	mSoundId = -1;
	if (mGlobalPtr != nullptr)
		*(int*)mGlobalPtr = 0;
}

void PrimeResourceManager::SoundRes::ApplyConfig()
{
	if (mSoundId == -1 || mRefRes != nullptr)
		return;
	if (mVolume > 0.0)
		mResourceManager->mApp->mSoundManager->SetBaseVolume(mSoundId, mVolume);
	if (mPanning != 0)
		mResourceManager->mApp->mSoundManager->SetBasePan(mSoundId, mPanning);
}

PrimeResourceManager::FontRes::FontRes() : BaseRes(ResType_Font)
{
	mFont = nullptr;
	mImage = nullptr;
}

void PrimeResourceManager::FontRes::DeleteResource()
{
	if (mRefRes != nullptr)
		ReleaseRefRes();
	delete mFont;
	mFont = nullptr;
	if (mGlobalPtr != nullptr)
		*mGlobalPtr = nullptr;
}

void PrimeResourceManager::FontRes::ApplyConfig()
{
	ImageFont* anImageFont = dynamic_cast<ImageFont*>(mFont);
	if (anImageFont == nullptr || mTags.empty())
		return;
	char aBuf[1024];
	strncpy(aBuf, mTags.c_str(), sizeof(aBuf) - 1);
	aBuf[sizeof(aBuf) - 1] = 0;
	const char* aTag = strtok(aBuf, ", \r\n\t");
	while (aTag != nullptr) {
		anImageFont->AddTag(aTag);
		aTag = strtok(nullptr, ", \r\n\t");
	}
	anImageFont->Prepare();
}

// port: the exe has no constructor of its own for it; ParsePopAnimResource (0x45fda0 (bejeweled3)) sets these fields
PrimeResourceManager::PopAnimRes::PopAnimRes() : BaseRes(ResType_PopAnim)
{
	mPopAnim = nullptr;
}

void PrimeResourceManager::PopAnimRes::DeleteResource()
{
	delete mPopAnim;
	mPopAnim = nullptr;
	if (mGlobalPtr != nullptr)
		*mGlobalPtr = nullptr;
}

// port: the exe has no constructor of its own for it; ParsePIEffectResource (0x45fef0 (bejeweled3)) sets these fields
PrimeResourceManager::PIEffectRes::PIEffectRes() : BaseRes(ResType_PIEffect)
{
	mPIEffect = nullptr;
}

void PrimeResourceManager::PIEffectRes::DeleteResource()
{
	delete mPIEffect;
	mPIEffect = nullptr;
	if (mGlobalPtr != nullptr)
		*mGlobalPtr = nullptr;
}

// port: the exe has no constructor of its own for it; ParseRenderEffectResource (0x460040 (bejeweled3)) sets these fields
PrimeResourceManager::RenderEffectRes::RenderEffectRes() : BaseRes(ResType_RenderEffect)
{
	mRenderEffectDefinition = nullptr;
}

// port: frees the definition and clears the global, as the other kinds' DeleteResource do
void PrimeResourceManager::RenderEffectRes::DeleteResource()
{
	delete mRenderEffectDefinition;
	mRenderEffectDefinition = nullptr;
	if (mGlobalPtr != nullptr)
		*mGlobalPtr = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
// The manager

PrimeResourceManager::PrimeResourceManager(SexyAppBase* theApp)
{
	mXMLParser = nullptr;
	mHasFailed = false;
	mApp = theApp;
	mCurResGroupArtRes = 0;
	mAllowMissingProgramResources = false;
	mAllowAlreadyDefinedResources = false;
	mHadAlreadyDefinedError = false;
	mCurResGroupList = nullptr;
	mCurResGroupListIndex = 0;
	mCurCompositeSubGroupIndex = 0;
	mBaseArtRes = 0;
	mCurArtRes = 0;
}

// port: every resource lets its picture, font or sound go before any is deleted, since a resource that refers to another
// (mRefRes) lets that one go too, and it may come earlier in the maps; DeleteResource does nothing a second time
PrimeResourceManager::~PrimeResourceManager()
{
	for (int aType = 0; aType < NUM_RES_TYPES; aType++)
		for (ResMap::iterator anItr = mResMaps[aType].begin(); anItr != mResMaps[aType].end(); ++anItr)
			anItr->second->DeleteResource();
	for (int aType = 0; aType < NUM_RES_TYPES; aType++) {
		for (ResMap::iterator anItr = mResMaps[aType].begin(); anItr != mResMaps[aType].end(); ++anItr)
			delete anItr->second;
		mResMaps[aType].clear();
	}
	delete mXMLParser;
}

bool PrimeResourceManager::Fail(const std::string& theErrorText)
{
	if (!mHasFailed) {
		mHasFailed = true;
		if (mXMLParser == nullptr) {
			mError = theErrorText;
			return false;
		}
		int aLineNum = mXMLParser->GetCurrentLineNum();
		char aLineNumStr[16];
		snprintf(aLineNumStr, sizeof(aLineNumStr), "%d", aLineNum);
		mError = theErrorText;
		if (aLineNum > 0)
			mError += std::string(" on Line ") + aLineNumStr;
		if (!mXMLParser->GetFileName().empty())
			mError += " in File '" + mXMLParser->GetFileName() + "'";
	}
	return false;
}

// port: the framework's, for the game's checks of a group
bool PrimeResourceManager::IsGroupLoaded(const std::string& theGroup)
{
	return mLoadedGroups.find(theGroup) != mLoadedGroups.end();
}

PrimeResourceManager::BaseRes* PrimeResourceManager::GetBaseRes(ResType theType, const std::string& theId)
{
	ResMap& aMap = mResMaps[theType];
	ResMap::iterator anItr = aMap.find(StrFormat("%s|%d", theId.c_str(), mCurArtRes));
	if (anItr == aMap.end())
		anItr = aMap.find(theId);
	if (anItr == aMap.end())
		return nullptr;
	return anItr->second;
}

///////////////////////////////////////////////////////////////////////////////
// Parsing the manifest

bool PrimeResourceManager::ParseCommonResource(XMLElement& theElement, BaseRes* theRes, ResMap& theMap)
{
	mHadAlreadyDefinedError = false;
	theRes->mResourceManager = this;
	theRes->mGlobalPtr = nullptr;

	const SexyString& aPath = theElement.mAttributes[_S("path")];
	if (aPath.empty())
		return Fail("No path specified.");

	theRes->mXMLAttributes = theElement.mAttributes;
	theRes->mFromProgram = false;
	if (aPath[0] == _S('!')) {
		theRes->mPath = SexyStringToStringFast(aPath);
		if (aPath == _S("!program"))
			theRes->mFromProgram = true;
	} else {
		theRes->mPath = mDefaultPath + SexyStringToStringFast(aPath);
		mResFromPathMap[StringToLower(theRes->mPath)] = theRes;
	}

	std::string anId;
	XMLParamMap::iterator anItr = theElement.mAttributes.find(_S("id"));
	if (anItr == theElement.mAttributes.end())
		anId = mDefaultIdPrefix + GetFileName(theRes->mPath, true);
	else
		anId = mDefaultIdPrefix + SexyStringToStringFast(anItr->second);
	if (mCurResGroupArtRes != 0)
		anId = StrFormat("%s|%d", anId.c_str(), mCurResGroupArtRes);

	theRes->mResGroup = mCurResGroup;
	theRes->mCompositeResGroup = mCurCompositeResGroup;
	theRes->mId = anId;
	theRes->mArtRes = mCurResGroupArtRes;

	std::pair<ResMap::iterator, bool> aRet = theMap.insert(ResMap::value_type(anId, theRes));
	if (!aRet.second) {
		mHadAlreadyDefinedError = true;
		return Fail("Resource already defined.");
	}
	mCurResGroupList->push_back(theRes);
	return true;
}

// port: the branch each Parse...Resource function of the exe repeats when a reparse finds a resource defined again
PrimeResourceManager::BaseRes* PrimeResourceManager::ReplaceRedefinedResource(BaseRes* theNewRes, ResMap& theMap)
{
	mError = "";
	mHasFailed = false;
	BaseRes* anOldRes = theMap[theNewRes->mId];
	anOldRes->mPath = theNewRes->mPath;
	anOldRes->mXMLAttributes = theNewRes->mXMLAttributes;
	delete theNewRes;
	return anOldRes;
}

bool PrimeResourceManager::ParseSoundResource(XMLElement& theElement)
{
	SoundRes* aRes = new SoundRes();
	if (!ParseCommonResource(theElement, aRes, mResMaps[ResType_Sound])) {
		if (!mHadAlreadyDefinedError || !mAllowAlreadyDefinedResources) {
			delete aRes;
			return false;
		}
		aRes = (SoundRes*)ReplaceRedefinedResource(aRes, mResMaps[ResType_Sound]);
	}

	XMLParamMap::iterator anItr = theElement.mAttributes.find(_S("volume"));
	if (anItr != theElement.mAttributes.end())
		sexysscanf(anItr->second.c_str(), _S("%lf"), &aRes->mVolume);
	anItr = theElement.mAttributes.find(_S("pan"));
	if (anItr != theElement.mAttributes.end())
		sexysscanf(anItr->second.c_str(), _S("%d"), &aRes->mPanning);
	aRes->ApplyConfig();
	return true;
}

bool PrimeResourceManager::ParseImageResource(XMLElement& theElement)
{
	if (theElement.mAttributes[_S("id")].empty())
		return true;   // the original skips an image with no id

	ImageRes* aRes = new ImageRes();
	if (!ParseCommonResource(theElement, aRes, mResMaps[ResType_Image])) {
		if (!mHadAlreadyDefinedError || !mAllowAlreadyDefinedResources) {
			delete aRes;
			return false;
		}
		aRes = (ImageRes*)ReplaceRedefinedResource(aRes, mResMaps[ResType_Image]);
	}

	aRes->mPalletize = !HasAttribute(theElement, _S("nopal"));
	aRes->mA4R4G4B4 = HasAttribute(theElement, _S("a4r4g4b4"));
	aRes->mDDSurface = HasAttribute(theElement, _S("ddsurface"));
	bool is3D = mApp->Is3DAccelerated();
	aRes->mPurgeBits = HasAttribute(theElement, _S("nobits")) ||
		(is3D && HasAttribute(theElement, _S("nobits3d"))) ||
		(!is3D && HasAttribute(theElement, _S("nobits2d")));
	if (aRes->mPurgeBits && !mApp->Is3DAccelerated())
		aRes->mPurgeBits = false;   // the 2D renderer draws from the pixels
	aRes->mA8R8G8B8 = HasAttribute(theElement, _S("a8r8g8b8"));
	aRes->mDither16 = HasAttribute(theElement, _S("dither16"));
	aRes->mMinimizeSubdivisions = HasAttribute(theElement, _S("minsubdivide"));
	aRes->mAutoFindAlpha = !HasAttribute(theElement, _S("noalpha"));
	aRes->mCubeMap = HasAttribute(theElement, _S("cubemap"));
	aRes->mVolumeMap = HasAttribute(theElement, _S("volumemap"));
	aRes->mNoTriRep = HasAttribute(theElement, _S("notrirep")) || HasAttribute(theElement, _S("noquadrep"));
	aRes->m2DBig = HasAttribute(theElement, _S("2dbig"));
	aRes->mIsAtlas = HasAttribute(theElement, _S("atlas"));

	XMLParamMap::iterator anItr = theElement.mAttributes.find(_S("alphaimage"));
	if (anItr != theElement.mAttributes.end())
		aRes->mAlphaImage = mDefaultPath + SexyStringToStringFast(anItr->second);

	aRes->mAlphaColor = 0xffffff;
	anItr = theElement.mAttributes.find(_S("alphacolor"));
	if (anItr != theElement.mAttributes.end())
		sexysscanf(anItr->second.c_str(), _S("%x"), &aRes->mAlphaColor);

	aRes->mOffset = Point(0, 0);
	anItr = theElement.mAttributes.find(_S("x"));
	if (anItr != theElement.mAttributes.end())
		aRes->mOffset.mX = sexyatoi(anItr->second.c_str());
	anItr = theElement.mAttributes.find(_S("y"));
	if (anItr != theElement.mAttributes.end())
		aRes->mOffset.mY = sexyatoi(anItr->second.c_str());

	anItr = theElement.mAttributes.find(_S("variant"));
	if (anItr != theElement.mAttributes.end())
		aRes->mVariant = SexyStringToStringFast(anItr->second);

	anItr = theElement.mAttributes.find(_S("alphagrid"));
	if (anItr != theElement.mAttributes.end())
		aRes->mAlphaGridImage = mDefaultPath + SexyStringToStringFast(anItr->second);

	anItr = theElement.mAttributes.find(_S("rows"));
	aRes->mRows = (anItr != theElement.mAttributes.end()) ? sexyatoi(anItr->second.c_str()) : 1;
	anItr = theElement.mAttributes.find(_S("cols"));
	aRes->mCols = (anItr != theElement.mAttributes.end()) ? sexyatoi(anItr->second.c_str()) : 1;

	aRes->mAtlasX = 0;
	aRes->mAtlasY = 0;
	aRes->mAtlasW = 0;
	aRes->mAtlasH = 0;
	anItr = theElement.mAttributes.find(_S("parent"));
	if (anItr != theElement.mAttributes.end()) {
		aRes->mAtlasName = SexyStringToStringFast(anItr->second);
		aRes->mAtlasX = sexyatoi(theElement.mAttributes[_S("ax")].c_str());
		aRes->mAtlasY = sexyatoi(theElement.mAttributes[_S("ay")].c_str());
		aRes->mAtlasW = sexyatoi(theElement.mAttributes[_S("aw")].c_str());
		aRes->mAtlasH = sexyatoi(theElement.mAttributes[_S("ah")].c_str());
	}

	if (aRes->mCubeMap) {
		if (aRes->mRows * aRes->mCols != 6)
			return Fail("Invalid CubeMap definition; must have 6 cells (check rows & cols values).");
	} else if (aRes->mVolumeMap) {
		unsigned int aCount = (unsigned int)(aRes->mRows * aRes->mCols);
		if (aCount == 0 || (aCount & (aCount - 1)) != 0)
			return Fail("Invalid VolumeMap definition; must have a pow2 cell count (check rows & cols values).");
	}

	anItr = theElement.mAttributes.find(_S("anim"));
	AnimType anAnimType = AnimType_None;
	if (anItr != theElement.mAttributes.end()) {
		const SexyChar* aType = anItr->second.c_str();
		if (sexystricmp(aType, _S("none")) == 0)
			anAnimType = AnimType_None;
		else if (sexystricmp(aType, _S("once")) == 0)
			anAnimType = AnimType_Once;
		else if (sexystricmp(aType, _S("loop")) == 0)
			anAnimType = AnimType_Loop;
		else if (sexystricmp(aType, _S("pingpong")) == 0)
			anAnimType = AnimType_PingPong;
		else
			return Fail("Invalid animation type.");
	}
	aRes->mAnimInfo.mAnimType = anAnimType;
	if (anAnimType != AnimType_None) {
		int aNumCels = std::max(aRes->mRows, aRes->mCols);
		int aBeginDelay = 0;
		int anEndDelay = 0;
		anItr = theElement.mAttributes.find(_S("framedelay"));
		if (anItr != theElement.mAttributes.end())
			aRes->mAnimInfo.mFrameDelay = sexyatoi(anItr->second.c_str());
		anItr = theElement.mAttributes.find(_S("begindelay"));
		if (anItr != theElement.mAttributes.end())
			aBeginDelay = sexyatoi(anItr->second.c_str());
		anItr = theElement.mAttributes.find(_S("enddelay"));
		if (anItr != theElement.mAttributes.end())
			anEndDelay = sexyatoi(anItr->second.c_str());
		anItr = theElement.mAttributes.find(_S("perframedelay"));
		if (anItr != theElement.mAttributes.end())
			ReadIntVector(anItr->second, aRes->mAnimInfo.mPerFrameDelay);
		anItr = theElement.mAttributes.find(_S("framemap"));
		if (anItr != theElement.mAttributes.end())
			ReadIntVector(anItr->second, aRes->mAnimInfo.mFrameMap);
		aRes->mAnimInfo.Compute(aNumCels, aBeginDelay, anEndDelay);
	}

	aRes->ApplyConfig();
	return true;
}

bool PrimeResourceManager::ParseFontResource(XMLElement& theElement)
{
	FontRes* aRes = new FontRes();
	if (!ParseCommonResource(theElement, aRes, mResMaps[ResType_Font])) {
		if (!mHadAlreadyDefinedError || !mAllowAlreadyDefinedResources) {
			delete aRes;
			return false;
		}
		aRes = (FontRes*)ReplaceRedefinedResource(aRes, mResMaps[ResType_Font]);
	}

	XMLParamMap::iterator anItr = theElement.mAttributes.find(_S("image"));
	if (anItr != theElement.mAttributes.end())
		aRes->mImagePath = SexyStringToStringFast(anItr->second);
	anItr = theElement.mAttributes.find(_S("tags"));
	if (anItr != theElement.mAttributes.end())
		aRes->mTags = SexyStringToStringFast(anItr->second);
	// "!sys:" (a Windows font) is not ported: the manifest has none
	if (strncmp(aRes->mPath.c_str(), "!sys:", 5) == 0)
		return Fail("SysFont not supported: " + aRes->mPath);

	aRes->ApplyConfig();
	return true;
}

bool PrimeResourceManager::ParsePopAnimResource(XMLElement& theElement)
{
	PopAnimRes* aRes = new PopAnimRes();
	if (!ParseCommonResource(theElement, aRes, mResMaps[ResType_PopAnim])) {
		if (!mHadAlreadyDefinedError || !mAllowAlreadyDefinedResources) {
			delete aRes;
			return false;
		}
		aRes = (PopAnimRes*)ReplaceRedefinedResource(aRes, mResMaps[ResType_PopAnim]);
	}
	aRes->ApplyConfig();
	return true;
}

bool PrimeResourceManager::ParsePIEffectResource(XMLElement& theElement)
{
	PIEffectRes* aRes = new PIEffectRes();
	if (!ParseCommonResource(theElement, aRes, mResMaps[ResType_PIEffect])) {
		if (!mHadAlreadyDefinedError || !mAllowAlreadyDefinedResources) {
			delete aRes;
			return false;
		}
		aRes = (PIEffectRes*)ReplaceRedefinedResource(aRes, mResMaps[ResType_PIEffect]);
	}
	aRes->ApplyConfig();
	return true;
}

bool PrimeResourceManager::ParseRenderEffectResource(XMLElement& theElement)
{
	RenderEffectRes* aRes = new RenderEffectRes();
	if (!ParseCommonResource(theElement, aRes, mResMaps[ResType_RenderEffect])) {
		if (!mHadAlreadyDefinedError || !mAllowAlreadyDefinedResources) {
			delete aRes;
			return false;
		}
		aRes = (RenderEffectRes*)ReplaceRedefinedResource(aRes, mResMaps[ResType_RenderEffect]);
	}
	aRes->mSrcFilePath.clear();
	const SexyString& aSrcPath = theElement.mAttributes[_S("srcpath")];
	if (!aSrcPath.empty())
		aRes->mSrcFilePath = mDefaultPath + SexyStringToStringFast(aSrcPath);
	aRes->ApplyConfig();
	return true;
}

bool PrimeResourceManager::ParseSetDefaults(XMLElement& theElement)
{
	XMLParamMap::iterator anItr = theElement.mAttributes.find(_S("path"));
	if (anItr != theElement.mAttributes.end())
		mDefaultPath = RemoveTrailingSlash(SexyStringToStringFast(anItr->second)) + '/';
	anItr = theElement.mAttributes.find(_S("idprefix"));
	if (anItr != theElement.mAttributes.end())
		mDefaultIdPrefix = RemoveTrailingSlash(SexyStringToStringFast(anItr->second));
	return true;
}

bool PrimeResourceManager::ParseResources()
{
	for (;;) {
		XMLElement anElement;
		if (!mXMLParser->NextElement(&anElement))
			return false;

		if (anElement.mType == XMLElement::TYPE_START) {
			bool aParsed;
			if (anElement.mValue == _S("Image"))
				aParsed = ParseImageResource(anElement);
			else if (anElement.mValue == _S("Sound"))
				aParsed = ParseSoundResource(anElement);
			else if (anElement.mValue == _S("Font"))
				aParsed = ParseFontResource(anElement);
			else if (anElement.mValue == _S("PopAnim"))
				aParsed = ParsePopAnimResource(anElement);
			else if (anElement.mValue == _S("PIEffect"))
				aParsed = ParsePIEffectResource(anElement);
			else if (anElement.mValue == _S("RenderEffect"))
				aParsed = ParseRenderEffectResource(anElement);
			else if (anElement.mValue == _S("File"))
				aParsed = true;   // the game opens these itself
			else if (anElement.mValue == _S("SetDefaults"))
				aParsed = ParseSetDefaults(anElement);
			else
				return Fail("Invalid Section '" + SexyStringToStringFast(anElement.mValue) + "'");
			if (!aParsed)
				return false;

			if (!mXMLParser->NextElement(&anElement))
				return false;
			if (anElement.mType != XMLElement::TYPE_END)
				return Fail("Unexpected element found.");
		} else if (anElement.mType == XMLElement::TYPE_ELEMENT) {
			return Fail("Element Not Expected '" + SexyStringToStringFast(anElement.mValue) + "'");
		} else if (anElement.mType == XMLElement::TYPE_END) {
			return true;
		}
	}
}

bool PrimeResourceManager::DoParseResources()
{
	if (!mXMLParser->HasFailed()) {
		for (;;) {
			XMLElement anElement;
			if (!mXMLParser->NextElement(&anElement))
				break;

			if (anElement.mType == XMLElement::TYPE_START) {
				if (anElement.mValue == _S("Resources")) {
					mCurResGroup = SexyStringToStringFast(anElement.mAttributes[_S("id")]);
					mCurResGroupList = &mResGroupMap[mCurResGroup];
					if (mCurResGroup.empty()) {
						Fail("No id specified.");
						break;
					}
					mCurCompositeResGroup = SexyStringToStringFast(anElement.mAttributes[_S("parent")]);
					const SexyString& anArtRes = anElement.mAttributes[_S("res")];
					mCurResGroupArtRes = anArtRes.empty() ? 0 : sexyatoi(anArtRes.c_str());
					if (!ParseResources())
						break;
				} else if (anElement.mValue == _S("CompositeResources")) {
					std::string anId = SexyStringToStringFast(anElement.mAttributes[_S("id")]);
					if (anId.empty()) {
						Fail("No id specified on CompositeGroup.");
						break;
					}
					CompositeResGroup& aCompositeGroup = mCompositeResGroupMap[anId];
					for (;;) {
						XMLElement aChild;
						if (!mXMLParser->NextElement(&aChild))
							break;
						if (aChild.mType == XMLElement::TYPE_START) {
							if (aChild.mValue != _S("Group")) {
								Fail("Invalid Section '" + SexyStringToStringFast(aChild.mValue) + "' within CompositeGroup");
								break;
							}
							SubGroup aSubGroup;
							aSubGroup.mGroupName = SexyStringToStringFast(aChild.mAttributes[_S("id")]);
							const SexyString& anArtRes = aChild.mAttributes[_S("res")];
							aSubGroup.mArtRes = anArtRes.empty() ? 0 : sexyatoi(anArtRes.c_str());
							aCompositeGroup.mSubGroups.push_back(aSubGroup);
							if (!mXMLParser->NextElement(&aChild)) {
								Fail("Group end expected");
								break;
							}
							if (aChild.mType != XMLElement::TYPE_END) {
								Fail("Unexpected element found.");
								break;
							}
						} else if (aChild.mType == XMLElement::TYPE_ELEMENT) {
							Fail("Element Not Expected '" + SexyStringToStringFast(aChild.mValue) + "'");
							break;
						} else if (aChild.mType == XMLElement::TYPE_END) {
							break;
						}
					}
					if (mHasFailed)
						break;
				} else {
					Fail("Invalid Section '" + SexyStringToStringFast(anElement.mValue) + "'");
					break;
				}
			} else if (anElement.mType == XMLElement::TYPE_ELEMENT) {
				Fail("Element Not Expected '" + SexyStringToStringFast(anElement.mValue) + "'");
				break;
			}
		}
	}

	if (mXMLParser->HasFailed())
		Fail(SexyStringToStringFast(mXMLParser->GetErrorText()));
	delete mXMLParser;
	mXMLParser = nullptr;
	return !mHasFailed;
}

bool PrimeResourceManager::ParseResourcesFile(const std::string& theFilename)
{
	mXMLParser = new XMLParser();
	if (!mXMLParser->OpenFile(theFilename))
		Fail("Resource file not found: " + theFilename);

	XMLElement anElement;
	while (!mXMLParser->HasFailed()) {
		if (!mXMLParser->NextElement(&anElement))
			Fail(SexyStringToStringFast(mXMLParser->GetErrorText()));
		if (anElement.mType == XMLElement::TYPE_START) {
			if (anElement.mValue != _S("ResourceManifest"))
				break;
			XMLParamMap::iterator aVersion = anElement.mAttributes.find(_S("version"));
			if (aVersion == anElement.mAttributes.end() || sexyatoi(aVersion->second.c_str()) != 2)
				Fail("Expecting ResourceManifest tag with version 2; please make sure your version of ResourceGen3 is current");
			return DoParseResources();
		}
	}
	Fail("Expecting ResourceManifest tag");
	return DoParseResources();
}

///////////////////////////////////////////////////////////////////////////////
// Loading

int PrimeResourceManager::GetNumResources(const std::string& theGroup, ResMap& theMap)
{
	int aCount = 0;
	for (ResMap::iterator anItr = theMap.begin(); anItr != theMap.end(); ++anItr) {
		BaseRes* aRes = anItr->second;
		if (aRes->mArtRes != 0 && aRes->mArtRes != mCurArtRes)
			continue;
		if (!theGroup.empty() && aRes->mResGroup != theGroup && aRes->mCompositeResGroup != theGroup)
			continue;
		if (!aRes->mFromProgram)
			aCount++;
	}
	return aCount;
}

int PrimeResourceManager::GetNumResources(const std::string& theGroup)
{
	int aCount = 0;
	for (int aType = 0; aType < NUM_RES_TYPES; aType++)
		aCount += GetNumResources(theGroup, mResMaps[aType]);
	return aCount;
}

void PrimeResourceManager::StartLoadResources(const std::string& theGroup, bool fromComposite)
{
	if (!fromComposite) {
		mError = "";
		mHasFailed = false;
		mCurCompositeResGroup.clear();
		mCurCompositeSubGroupIndex = 0;
		CompositeResGroupMap::iterator aComposite = mCompositeResGroupMap.find(theGroup);
		if (aComposite != mCompositeResGroupMap.end()) {
			mCurCompositeResGroup.clear();
			mCurResGroupList = nullptr;
			mCurCompositeResGroup = theGroup;
			std::vector<SubGroup>& aSubGroups = aComposite->second.mSubGroups;
			for (int i = 0; i < (int)aSubGroups.size(); i++) {
				SubGroup& aSubGroup = aSubGroups[i];
				if (!aSubGroup.mGroupName.empty() && (aSubGroup.mArtRes == 0 || aSubGroup.mArtRes == mCurArtRes)) {
					mCurCompositeSubGroupIndex = i;
					StartLoadResources(aSubGroup.mGroupName, true);
					return;
				}
			}
			return;
		}
	}
	mCurResGroup = theGroup;
	mCurResGroupList = &mResGroupMap[theGroup];
	mCurResGroupListIndex = 0;
}

bool PrimeResourceManager::DoLoadResource(BaseRes* theRes, bool* isLoaded)
{
	*isLoaded = false;
	if (theRes->mFromProgram) {
		*isLoaded = true;
		return true;
	}
	switch (theRes->mType) {
	case ResType_Image:
		return DoLoadImage((ImageRes*)theRes);
	case ResType_Sound:
		return DoLoadSound((SoundRes*)theRes);
	case ResType_Font:
		return DoLoadFont((FontRes*)theRes);
	case ResType_PopAnim:
		return DoLoadPopAnim((PopAnimRes*)theRes);
	case ResType_PIEffect:
		return DoLoadPIEffect((PIEffectRes*)theRes);
	case ResType_RenderEffect:
		return DoLoadRenderEffect((RenderEffectRes*)theRes);
	default:
		return false;
	}
}

bool PrimeResourceManager::LoadNextResource()
{
	if (HadError() || mCurResGroupList == nullptr)
		return false;

	while (mCurResGroupListIndex < mCurResGroupList->size()) {
		BaseRes* aRes = (*mCurResGroupList)[mCurResGroupListIndex++];
		bool isLoaded = true;
		bool aResult = true;
		if (aRes->mRefCount == 0)
			aResult = DoLoadResource(aRes, &isLoaded);
		aRes->mRefCount++;
		aRes->mDirectLoaded = true;
		if (!isLoaded)
			return aResult;
	}

	// the group is done: the next part of the composite group being loaded
	if (!mCurCompositeResGroup.empty()) {
		CompositeResGroupMap::iterator aComposite = mCompositeResGroupMap.find(mCurCompositeResGroup);
		if (aComposite != mCompositeResGroupMap.end()) {
			std::vector<SubGroup>& aSubGroups = aComposite->second.mSubGroups;
			for (int i = mCurCompositeSubGroupIndex + 1; i < (int)aSubGroups.size(); i++) {
				SubGroup& aSubGroup = aSubGroups[i];
				if (!aSubGroup.mGroupName.empty() && (aSubGroup.mArtRes == 0 || aSubGroup.mArtRes == mCurArtRes)) {
					mCurCompositeSubGroupIndex = i;
					StartLoadResources(aSubGroup.mGroupName, true);
					return LoadNextResource();
				}
			}
		}
	}
	return false;
}

bool PrimeResourceManager::LoadResources(const std::string& theGroup)
{
	mError = "";
	mHasFailed = false;
	StartLoadResources(theGroup, false);
	while (LoadNextResource()) {
	}
	if (HadError())
		return false;
	mLoadedGroups.insert(theGroup);
	return true;
}

void PrimeResourceManager::Deref(BaseRes* theRes)
{
	theRes->mRefCount--;
	if (theRes->mRefCount == 0)
		theRes->DeleteResource();
}

void PrimeResourceManager::DeleteResources(ResMap& theMap, const std::string& theGroup)
{
	if (theGroup.empty()) {
		for (ResMap::iterator anItr = theMap.begin(); anItr != theMap.end(); ++anItr) {
			if (anItr->second->mDirectLoaded) {
				anItr->second->mDirectLoaded = false;
				Deref(anItr->second);
			}
		}
		return;
	}

	CompositeResGroupMap::iterator aComposite = mCompositeResGroupMap.find(theGroup);
	if (aComposite == mCompositeResGroupMap.end()) {
		for (ResMap::iterator anItr = theMap.begin(); anItr != theMap.end(); ++anItr) {
			if (anItr->second->mResGroup == theGroup && anItr->second->mDirectLoaded) {
				anItr->second->mDirectLoaded = false;
				Deref(anItr->second);
			}
		}
		return;
	}

	std::vector<SubGroup>& aSubGroups = aComposite->second.mSubGroups;
	for (size_t i = 0; i < aSubGroups.size(); i++) {
		if (aSubGroups[i].mGroupName.empty())
			continue;
		for (ResMap::iterator anItr = theMap.begin(); anItr != theMap.end(); ++anItr) {
			if (anItr->second->mResGroup == aSubGroups[i].mGroupName && anItr->second->mDirectLoaded) {
				anItr->second->mDirectLoaded = false;
				Deref(anItr->second);
			}
		}
	}
}

void PrimeResourceManager::DeleteResources(const std::string& theGroup)
{
	for (int aType = 0; aType < NUM_RES_TYPES; aType++)
		DeleteResources(mResMaps[aType], theGroup);
	mLoadedGroups.erase(theGroup);
}

///////////////////////////////////////////////////////////////////////////////
// Loading each type

bool PrimeResourceManager::LoadAlphaGridImage(ImageRes* theRes, DDImage* theImage)
{
	// port: ImageLib::GetImage is Platform::LoadGameImage, as in the framework's ResourceManager
	Platform::LoadedImage anAlphaImage;
	if (!Platform::LoadGameImage(theRes->mAlphaGridImage, anAlphaImage, true))
		return Fail(StrFormat("Failed to load image: %s", theRes->mAlphaGridImage.c_str()));

	int aNumRows = theRes->mRows;
	int aNumCols = theRes->mCols;
	int aCelWidth = theImage->mWidth / aNumCols;
	int aCelHeight = theImage->mHeight / aNumRows;
	if (anAlphaImage.mWidth != aCelWidth || anAlphaImage.mHeight != aCelHeight)
		return Fail(StrFormat("GridAlphaImage size mismatch between %s and %s", theRes->mPath.c_str(),
			theRes->mAlphaGridImage.c_str()));

	MemoryImage::ApplyAlphaGrid(theImage->mBits, theImage->mWidth, theImage->mHeight, aNumRows, aNumCols,
		anAlphaImage.mBits.data(), anAlphaImage.mWidth, anAlphaImage.mHeight);
	theImage->BitsChanged();
	return true;
}

bool PrimeResourceManager::LoadAlphaImage(ImageRes* theRes, DDImage* theImage)
{
	Platform::LoadedImage anAlphaImage;
	if (!Platform::LoadGameImage(theRes->mAlphaImage, anAlphaImage, true))
		return Fail(StrFormat("Failed to load image: %s", theRes->mAlphaImage.c_str()));
	if (anAlphaImage.mWidth != theImage->mWidth || anAlphaImage.mHeight != theImage->mHeight)
		return Fail(StrFormat("AlphaImage size mismatch between %s and %s", theRes->mPath.c_str(),
			theRes->mAlphaImage.c_str()));

	MemoryImage::ApplyAlphaImage(theImage->mBits, theImage->mWidth * theImage->mHeight, anAlphaImage.mBits.data());
	theImage->BitsChanged();
	return true;
}

bool PrimeResourceManager::DoLoadImage(ImageRes* theRes)
{
	if (strncmp(theRes->mPath.c_str(), "!ref:", 5) == 0) {
		std::string aRefName = theRes->mPath.substr(5);
		BaseRes* aRefRes = GetBaseRes(ResType_Image, aRefName);
		if (aRefRes == nullptr)
			return Fail("Ref Image not found: " + aRefName);
		bool isLoaded;
		if (aRefRes->mRefCount == 0 && !DoLoadResource(aRefRes, &isLoaded))
			return false;
		aRefRes->mRefCount++;
		theRes->ReleaseRefRes();
		theRes->mRefRes = aRefRes;
		theRes->mImage = ((ImageRes*)aRefRes)->mImage;
		if (theRes->mGlobalPtr != nullptr)
			*theRes->mGlobalPtr = (Image*)theRes->mImage;
		ResourceLoadedHook(theRes);
		return true;
	}

	// port: the original takes the picture from its "ResMan" cache of decoded files when it is there (and writes it
	// there after a load); the port decodes the file each time, as the framework does
	bool isNew;
	SharedImageRef aSharedImageRef = mApp->GetSharedImage(theRes->mPath, theRes->mVariant, &isNew);
	DDImage* anImage = (DDImage*)aSharedImageRef;
	if (anImage == nullptr)
		return Fail(StrFormat("Failed to load image: %s", theRes->mPath.c_str()));

	if (isNew) {
		// port: the file's pixels with these alpha steps can be made again after they are dropped (MemoryImage.h,
		// mBitsSource), when nothing else changed them since the file gave them
		MemoryImage::BitsSource* aSource = nullptr;
		if (anImage->BitsSourceValid() && (!theRes->mAlphaImage.empty() || !theRes->mAlphaGridImage.empty())) {
			aSource = new MemoryImage::BitsSource(*anImage->mBitsSource);
			aSource->mAlphaPath = theRes->mAlphaImage;
			aSource->mAlphaGridPath = theRes->mAlphaGridImage;
			aSource->mGridRows = theRes->mRows;
			aSource->mGridCols = theRes->mCols;
		}
		if (!theRes->mAlphaImage.empty() && !LoadAlphaImage(theRes, anImage)) {
			delete aSource;
			return false;
		}
		if (!theRes->mAlphaGridImage.empty() && !LoadAlphaGridImage(theRes, anImage)) {
			delete aSource;
			return false;
		}
		if (aSource != nullptr)
			anImage->SetBitsSource(aSource);
	}

	if (theRes->mPalletize)
		anImage->Palletize();   // port: there are no DirectDraw surfaces; the framework's Palletize keeps the colours

	theRes->mImage = aSharedImageRef;
	theRes->ApplyConfig();
	if (theRes->mGlobalPtr != nullptr)
		*theRes->mGlobalPtr = anImage;
	ResourceLoadedHook(theRes);
	// port: the picture's texture now, and its pixels gone until something needs them (DDInterface::UploadLoadedImage)
	if (isNew && anImage->mDDInterface != nullptr)
		anImage->mDDInterface->UploadLoadedImage(anImage);
	return true;
}

bool PrimeResourceManager::DoLoadSound(SoundRes* theRes)
{
	if (strncmp(theRes->mPath.c_str(), "!ref:", 5) == 0) {
		std::string aRefName = theRes->mPath.substr(5);
		BaseRes* aRefRes = GetBaseRes(ResType_Sound, aRefName);
		bool isLoaded;
		if (aRefRes != nullptr && aRefRes->mRefCount == 0 && !DoLoadResource(aRefRes, &isLoaded))
			return false;
		if (aRefRes == nullptr || ((SoundRes*)aRefRes)->mSoundId == -1)
			return Fail("Ref sound not found: " + aRefName);
		aRefRes->mRefCount++;
		theRes->ReleaseRefRes();
		theRes->mRefRes = aRefRes;
		theRes->mSoundId = ((SoundRes*)aRefRes)->mSoundId;
		if (theRes->mGlobalPtr != nullptr)
			*(int*)theRes->mGlobalPtr = theRes->mSoundId;
		ResourceLoadedHook(theRes);
		return true;
	}

	int aSoundId = mApp->mSoundManager->GetFreeSoundId();
	if (aSoundId < 0)
		return Fail("Out of free sound ids");
	if (!mApp->mSoundManager->LoadSound(aSoundId, theRes->mPath))
		return Fail(StrFormat("Failed to load sound: %s", theRes->mPath.c_str()));
	theRes->mSoundId = aSoundId;
	if (theRes->mGlobalPtr != nullptr)
		*(int*)theRes->mGlobalPtr = aSoundId;
	theRes->ApplyConfig();
	ResourceLoadedHook(theRes);
	return true;
}

bool PrimeResourceManager::DoLoadFont(FontRes* theRes)
{
	// a font may name another file for the app's art size: path600="!ref:FONT_HUMANST9"
	std::string aPath = theRes->mPath;
	XMLParamMap::iterator anArtPath = theRes->mXMLAttributes.find(StrFormat(_S("path%d"), mCurArtRes));
	if (anArtPath != theRes->mXMLAttributes.end())
		aPath = SexyStringToStringFast(anArtPath->second);

	Font* aFont = nullptr;
	if (theRes->mImagePath.empty()) {
		if (strncmp(aPath.c_str(), "!ref:", 5) == 0) {
			std::string aRefName = aPath.substr(5);
			BaseRes* aRefRes = GetBaseRes(ResType_Font, aRefName);
			bool isLoaded;
			if (aRefRes != nullptr && aRefRes->mRefCount == 0 && !DoLoadResource(aRefRes, &isLoaded))
				return false;
			if (aRefRes == nullptr || ((FontRes*)aRefRes)->mFont == nullptr)
				return Fail("Ref Font not found: " + aRefName);
			aRefRes->mRefCount++;
			theRes->ReleaseRefRes();
			theRes->mRefRes = aRefRes;
			aFont = ((FontRes*)aRefRes)->mFont->Duplicate();
		} else {
			// port: the original takes a font it read before from its "ResMan" cache
			aFont = new ImageFont(mApp, aPath);
		}
	} else {
		Image* anImage = mApp->GetImage(theRes->mImagePath);
		if (anImage == nullptr)
			return Fail(StrFormat("Failed to load image: %s", theRes->mImagePath.c_str()));
		theRes->mImage = anImage;
		aFont = new ImageFont(anImage, aPath);
	}

	ImageFont* anImageFont = dynamic_cast<ImageFont*>(aFont);
	if (anImageFont != nullptr) {
		if (anImageFont->mFontData == nullptr || !anImageFont->mFontData->mInitialized) {
			delete aFont;
			return Fail(StrFormat("Failed to load font: %s", aPath.c_str()));
		}
		if (!theRes->mTags.empty()) {
			char aBuf[1024];
			strncpy(aBuf, theRes->mTags.c_str(), sizeof(aBuf) - 1);
			aBuf[sizeof(aBuf) - 1] = 0;
			const char* aTag = strtok(aBuf, ", \r\n\t");
			while (aTag != nullptr) {
				anImageFont->AddTag(aTag);
				aTag = strtok(nullptr, ", \r\n\t");
			}
			anImageFont->Prepare();
		}
	}

	theRes->mFont = aFont;
	if (theRes->mGlobalPtr != nullptr)
		*theRes->mGlobalPtr = aFont;
	theRes->ApplyConfig();
	ResourceLoadedHook(theRes);
	return true;
}

bool PrimeResourceManager::DoLoadPopAnim(PopAnimRes* theRes)
{
	// port: the original holds the manager's lock (+0x2b4) while it loads one, which the port's manager does not have
	PopAnim* anAnim = new PopAnim(0, nullptr);
	anAnim->mImageScale = (float)mCurArtRes / (float)mBaseArtRes;
	anAnim->mDrawScale = (float)mCurArtRes / (float)mBaseArtRes;
	anAnim->LoadAnimation(theRes->mPath);
	// an animation fails only with an error text: a file the player does not read sets none and loads empty
	if (!anAnim->GetError().empty()) {
		std::string anError = "PopAnim loading error: " + anAnim->GetError() + " on file " + theRes->mPath;
		delete anAnim;
		return Fail(anError);
	}
	if (theRes->mGlobalPtr != nullptr)
		*theRes->mGlobalPtr = anAnim;
	theRes->mPopAnim = anAnim;
	return true;
}

bool PrimeResourceManager::DoLoadPIEffect(PIEffectRes* theRes)
{
	PIEffect* anEffect = new PIEffect();
	anEffect->LoadEffect(theRes->mPath);
	if (!anEffect->mError.empty()) {
		std::string anError = "PIEffect loading error: " + anEffect->mError + " on file " + theRes->mPath;
		delete anEffect;
		return Fail(anError);
	}
	if (theRes->mGlobalPtr != nullptr)
		*theRes->mGlobalPtr = anEffect;
	theRes->mPIEffect = anEffect;
	return true;
}

bool PrimeResourceManager::DoLoadRenderEffect(RenderEffectRes* theRes)
{
	RenderEffectDefinition* aDefinition = new RenderEffectDefinition();
	if (!aDefinition->LoadFromFile(theRes->mPath, theRes->mSrcFilePath)) {
		delete aDefinition;
		return Fail("RenderEffect loading error on file " + theRes->mPath);
	}
	theRes->mRenderEffectDefinition = aDefinition;
	if (theRes->mGlobalPtr != nullptr)
		*theRes->mGlobalPtr = aDefinition;
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Getting resources

SharedImageRef PrimeResourceManager::GetImageThrow(const std::string& theId, int theArtRes, bool theOptional)
{
	if (theArtRes != 0 && theArtRes != mCurArtRes) {
		std::string anError = StrFormat("Attempted to load image of incorrect art resolution %d (expected %d): %s",
			theArtRes, mCurArtRes, theId.c_str());
		Fail(anError);
		ThrowResourceError(anError);
	}
	BaseRes* aRes = GetBaseRes(ResType_Image, theId);
	if (aRes != nullptr) {
		ImageRes* anImageRes = (ImageRes*)aRes;
		if ((DDImage*)anImageRes->mImage != nullptr)
			return anImageRes->mImage;
		if (!mAllowMissingProgramResources) {
			theOptional = false;
		} else {
			theOptional = aRes->mFromProgram;
		}
	}
	if (theOptional)
		return SharedImageRef();
	std::string anError = StrFormat("Image resource not found: %s", theId.c_str());
	Fail(anError);
	ThrowResourceError(anError);
}

int PrimeResourceManager::GetSoundThrow(const std::string& theId)
{
	BaseRes* aRes = GetBaseRes(ResType_Sound, theId);
	if (aRes != nullptr) {
		SoundRes* aSoundRes = (SoundRes*)aRes;
		if (aSoundRes->mSoundId != -1)
			return aSoundRes->mSoundId;
		if (mAllowMissingProgramResources && aRes->mFromProgram)
			return -1;
	}
	std::string anError = StrFormat("Sound resource not found: %s", theId.c_str());
	Fail(anError);
	ThrowResourceError(anError);
}

Font* PrimeResourceManager::GetFontThrow(const std::string& theId, int theArtRes)
{
	if (theArtRes != 0 && theArtRes != mCurArtRes) {
		std::string anError = StrFormat("Attempted to load font of incorrect art resolution %d (expected %d): %s",
			theArtRes, mCurArtRes, theId.c_str());
		Fail(anError);
		ThrowResourceError(anError);
	}
	BaseRes* aRes = GetBaseRes(ResType_Font, theId);
	if (aRes != nullptr) {
		FontRes* aFontRes = (FontRes*)aRes;
		if (aFontRes->mFont != nullptr)
			return aFontRes->mFont;
		if (mAllowMissingProgramResources && aRes->mFromProgram)
			return nullptr;
	}
	std::string anError = StrFormat("Font resource not found: %s", theId.c_str());
	Fail(anError);
	ThrowResourceError(anError);
}

PopAnim* PrimeResourceManager::GetPopAnimThrow(const std::string& theId)
{
	BaseRes* aRes = GetBaseRes(ResType_PopAnim, theId);
	if (aRes != nullptr) {
		PopAnimRes* aPopAnimRes = (PopAnimRes*)aRes;
		if (aPopAnimRes->mPopAnim != nullptr)
			return aPopAnimRes->mPopAnim;
		if (mAllowMissingProgramResources && aRes->mFromProgram)
			return nullptr;
	}
	std::string anError = StrFormat("PopAnim resource not found: %s", theId.c_str());
	Fail(anError);
	ThrowResourceError(anError);
}

PIEffect* PrimeResourceManager::GetPIEffectThrow(const std::string& theId)
{
	BaseRes* aRes = GetBaseRes(ResType_PIEffect, theId);
	if (aRes != nullptr) {
		PIEffectRes* anEffectRes = (PIEffectRes*)aRes;
		if (anEffectRes->mPIEffect != nullptr)
			return anEffectRes->mPIEffect;
		if (mAllowMissingProgramResources && aRes->mFromProgram)
			return nullptr;
	}
	std::string anError = StrFormat("PIEffect resource not found: %s", theId.c_str());
	Fail(anError);
	ThrowResourceError(anError);
}

RenderEffectDefinition* PrimeResourceManager::GetRenderEffectThrow(const std::string& theId)
{
	BaseRes* aRes = GetBaseRes(ResType_RenderEffect, theId);
	if (aRes != nullptr) {
		RenderEffectRes* anEffectRes = (RenderEffectRes*)aRes;
		if (anEffectRes->mRenderEffectDefinition != nullptr)
			return anEffectRes->mRenderEffectDefinition;
		if (mAllowMissingProgramResources && aRes->mFromProgram)
			return nullptr;
	}
	std::string anError = StrFormat("RenderEffectDefinition resource not found: %s", theId.c_str());
	Fail(anError);
	ThrowResourceError(anError);
}

// 0x46ff40 (bejeweled3), its first part: this generation's SexyAppBase::GetSharedImage takes a name "file|ID" as the
// image resource ID when the manager has one with a picture, which it loads if nothing holds it; else the file
SharedImageRef PrimeResourceManager::GetSharedImageByRef(const std::string& theName, std::string* theFileName)
{
	size_t aBar = theName.find('|');
	if (aBar == std::string::npos) {
		*theFileName = theName;
		return SharedImageRef();
	}
	SharedImageRef anImage;
	BaseRes* aRes = GetBaseRes(ResType_Image, theName.substr(aBar + 1));
	if (aRes != nullptr) {
		// 0x465540 (bejeweled3): a reference to the resource for the length of the call
		bool isLoaded = false;
		if (aRes->mRefCount == 0)
			DoLoadResource(aRes, &isLoaded);
		aRes->mRefCount++;
		anImage = ((ImageRes*)aRes)->mImage;
		Deref(aRes);
	}
	if ((DDImage*)anImage == nullptr)
		*theFileName = theName.substr(0, aBar);
	return anImage;
}

// port: the framework's getters that never throw, for the game's optional resources
SharedImageRef PrimeResourceManager::GetImage(const std::string& theId)
{
	BaseRes* aRes = GetBaseRes(ResType_Image, theId);
	if (aRes == nullptr)
		return SharedImageRef();
	return ((ImageRes*)aRes)->mImage;
}

// port: as GetImage
int PrimeResourceManager::GetSound(const std::string& theId)
{
	BaseRes* aRes = GetBaseRes(ResType_Sound, theId);
	if (aRes == nullptr)
		return -1;
	return ((SoundRes*)aRes)->mSoundId;
}

// port: as GetImage
Font* PrimeResourceManager::GetFont(const std::string& theId)
{
	BaseRes* aRes = GetBaseRes(ResType_Font, theId);
	if (aRes == nullptr)
		return nullptr;
	return ((FontRes*)aRes)->mFont;
}

// 0x462ce0 (bejeweled3): the image's offset; the original returns its address, which does not change after parsing
Point PrimeResourceManager::GetImageOffset(const std::string& theId)
{
	BaseRes* aRes = GetBaseRes(ResType_Image, theId);
	if (aRes == nullptr)
		return Point(0, 0);
	return ((ImageRes*)aRes)->mOffset;
}

void PrimeResourceManager::RegisterGlobalPtr(const std::string& theId, void** theGlobalPtr)
{
	for (int aType = 0; aType < NUM_RES_TYPES; aType++) {
		BaseRes* aRes = GetBaseRes((ResType)aType, theId);
		if (aRes != nullptr) {
			aRes->mGlobalPtr = theGlobalPtr;
			mGlobalPtrMap[theGlobalPtr] = aRes;
		}
	}
}

// port: the exe's lookup in FrameHit (0x445c00 (bejeweled3)) of a path in mResFromPathMap, and the test of the resource
// handle (0x45bfa0 (bejeweled3): a particle effect resource that has its effect)
PIEffect* PrimeResourceManager::FindPIEffectByPath(const std::string& thePath)
{
	std::map<std::string, BaseRes*>::const_iterator anItr = mResFromPathMap.find(StringToLower(thePath));
	if (anItr == mResFromPathMap.end() || anItr->second->mType != ResType_PIEffect)
		return nullptr;
	return ((PIEffectRes*)anItr->second)->mPIEffect;
}

// port: see FindPIEffectByPath; the lookup is GetBaseRes (0x45cfb0 (bejeweled3))
PIEffect* PrimeResourceManager::FindPIEffect(const std::string& theId)
{
	BaseRes* aRes = GetBaseRes(ResType_PIEffect, theId);
	return aRes != nullptr ? ((PIEffectRes*)aRes)->mPIEffect : nullptr;
}

std::string PrimeResourceManager::GetIdByPath(const std::string& thePath)
{
	std::string aPath = StringToLower(thePath);
	for (size_t i = 0; i < aPath.size(); i++) {
		if (aPath[i] == '/')
			aPath[i] = '\\';
	}
	for (int aType = 0; aType < NUM_RES_TYPES; aType++) {
		for (ResMap::iterator anItr = mResMaps[aType].begin(); anItr != mResMaps[aType].end(); ++anItr) {
			if (StringToLower(anItr->second->mPath) == aPath)
				return anItr->second->mId;
		}
	}
	return "";
}
