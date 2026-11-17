// Sexy::PrimeResourceManager (Sexy::ResourceManager in the original, 0x2cc bytes, vtable 0x871c8c (bejeweled3); renamed
// because the framework's ResourceManager is the generation before it): the resources of a ResourceGen3 manifest
// (properties\resources.xml, <ResourceManifest version="2">).
//
// What this generation adds to the framework's:
// - Art sizes. A <Resources> group may carry res="600", "768" or "1200": its resources are drawn for that screen size and
//   their ids are stored as "ID|600" and so on, so that each size has its own IMAGE_X. A lookup tries the id with the
//   app's art size (mCurArtRes) first, then the bare id.
// - Composite groups. <CompositeResources id="Main"> names its parts (<Group id="600_Main" res="600"/>, ...); loading
//   it loads the parts whose res is 0 or the app's art size, one after the other.
// - Reference counts. Each resource counts the groups that loaded it; DeleteResources drops a group's count and a
//   resource goes when nothing holds it.
// - Three more types: PopAnim (.pam), PIEffect (.ppf) and RenderEffect (.d3dfx, the 3D mode's shaders). <File>
//   entries are skipped: the game opens those files itself.
// - The generated code's globals: RegisterGlobalPtr gives a resource the address of its global (IMAGE_X and so on),
//   which the resource sets when it loads and clears when it goes.
//
// Not translated: the resource streams (ResStreamsManager, the app's +0x85c), which the PC game never opens (it ships
// no streams file), the "ResMan" cache of decoded images and fonts (a load-time cache with nothing visible), the
// reparse of a running game (ReloadAllResourcesProc, a developer key) and system fonts (the manifest has none).
#pragma once

#include "Common.h"
#include "Image.h"
#include "Point.h"
#include "ResourceManager.h"  // ResourceManagerException, which the Get...Throw functions throw
#include "SharedImage.h"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace Sexy {

class SexyAppBase;
class XMLParser;
class XMLElement;
class Font;
class DDImage;
class PopAnim;
class PIEffect;
class RenderEffectDefinition;

typedef std::map<SexyString, SexyString> XMLParamMap;

class PrimeResourceManager {
public:
	enum ResType {
		ResType_Image = 0,
		ResType_Sound = 1,
		ResType_Font = 2,
		ResType_PopAnim = 3,
		ResType_PIEffect = 4,
		ResType_RenderEffect = 5,
		NUM_RES_TYPES = 6
	};

	struct BaseRes {   // 0xc8 bytes
		PrimeResourceManager* mResourceManager;  // +0x04
		void** mGlobalPtr;                       // +0x08: the generated code's global (RegisterGlobalPtr), or null
		BaseRes* mRefRes;                        // +0x0c: what a "!ref:ID" path names, held while this one is loaded
		int mRefCount;                           // +0x10: loads of the resource that have not been deleted
		bool mDirectLoaded;                      // +0x14: loaded by its own group (not only through a composite or a ref)
		ResType mType;                           // +0x1c
		std::string mId;                         // +0x34: with "|<art size>" when its group has one
		std::string mResGroup;                   // +0x50
		std::string mCompositeResGroup;          // +0x6c: the composite group being parsed, empty for a plain one
		std::string mPath;                       // +0x88: mDefaultPath and the path attribute, or "!program", "!ref:ID"
		int mArtRes;                             // +0xa0: the group's res attribute, 0 for none
		XMLParamMap mXMLAttributes;              // +0xa4
		bool mFromProgram;                       // +0xc4: path="!program", made by the game

		BaseRes(ResType theType);                // 0x45b540 (bejeweled3)
		virtual ~BaseRes() {}
		void ReleaseRefRes();                    // 0x45bee0 (bejeweled3)
		virtual void DeleteResource() {}         // slot 1: frees what the resource holds
		virtual void ApplyConfig() {}            // slot 2: passes the manifest's settings to what was loaded
	};

	struct ImageRes : public BaseRes {   // 0x1bc bytes
		SharedImageRef mImage;
		std::string mAlphaImage;
		std::string mAlphaGridImage;
		std::string mVariant;
		bool mAutoFindAlpha;          // +0x130: not "noalpha"
		bool mPalletize;              // +0x131: not "nopal"
		bool mA4R4G4B4;               // +0x132
		bool mA8R8G8B8;               // +0x133
		bool mDither16;               // +0x134: a 16-bit colour hint, which nothing on this port reads
		bool mDDSurface;              // +0x135
		bool mPurgeBits;              // +0x136: "nobits", or "nobits3d"/"nobits2d" for the mode; never in 2D
		bool mMinimizeSubdivisions;   // +0x137
		bool mCubeMap;                // +0x138
		bool mVolumeMap;              // +0x139
		bool mNoTriRep;               // +0x13a: "notrirep" or "noquadrep"
		bool m2DBig;                  // +0x13b
		bool mIsAtlas;                // +0x13c
		int mRows;                    // +0x140
		int mCols;                    // +0x144
		DWORD mAlphaColor;            // +0x148: the colour of an image that is only a mask, white unless "alphacolor"
		AnimInfo mAnimInfo;           // +0x14c
		Point mOffset;                // +0x128: "x" and "y", where the image's top left is drawn from
		std::string mAtlasName;       // +0x18c: "parent", the atlas image this one is cut from
		int mAtlasX;                  // +0x1ac: "ax", "ay", "aw", "ah", its rectangle there
		int mAtlasY;
		int mAtlasW;
		int mAtlasH;

		ImageRes();                       // 0x45b700 (bejeweled3)
		void DeleteResource() override;   // 0x45bfc0 (bejeweled3)
		void ApplyConfig() override;      // 0x45c000 (bejeweled3)
	};

	struct SoundRes : public BaseRes {   // 0xe0 bytes
		int mSoundId;      // +0xc8: -1 until loaded
		double mVolume;    // +0xd0: the base volume, -1 for the sound manager's own
		int mPanning;      // +0xd8

		SoundRes();
		void DeleteResource() override;   // 0x45c220 (bejeweled3)
		void ApplyConfig() override;      // 0x45c280 (bejeweled3)
	};

	struct FontRes : public BaseRes {   // 0x114 bytes
		Font* mFont;             // +0xc8
		Image* mImage;           // +0xcc
		std::string mImagePath;  // +0xd0
		std::string mTags;       // +0xec

		FontRes();                        // 0x45b7d0 (bejeweled3)
		void DeleteResource() override;   // 0x45c2f0 (bejeweled3)
		void ApplyConfig() override;      // 0x45c350 (bejeweled3)
	};

	struct PopAnimRes : public BaseRes {   // 0xcc bytes
		PopAnim* mPopAnim;   // +0xc8

		PopAnimRes();
		void DeleteResource() override;   // 0x45c4b0 (bejeweled3)
	};

	struct PIEffectRes : public BaseRes {   // 0xcc bytes
		PIEffect* mPIEffect;   // +0xc8

		PIEffectRes();
		void DeleteResource() override;   // 0x45c500 (bejeweled3)
	};

	struct RenderEffectRes : public BaseRes {   // 0xe8 bytes
		RenderEffectDefinition* mRenderEffectDefinition;  // +0xc8
		std::string mSrcFilePath;                          // +0xd4: "srcpath", the shader's source

		RenderEffectRes();
		void DeleteResource() override;
	};

	typedef std::map<std::string, BaseRes*> ResMap;
	typedef std::vector<BaseRes*> ResList;
	typedef std::map<std::string, ResList, StringLessNoCase> ResGroupMap;

	// A part of a composite group: a plain group and the art size it is for (0 for every size)
	struct SubGroup {   // 0x20 bytes
		std::string mGroupName;
		int mArtRes;
	};
	struct CompositeResGroup {
		std::vector<SubGroup> mSubGroups;
	};
	typedef std::map<std::string, CompositeResGroup, StringLessNoCase> CompositeResGroupMap;

	std::set<std::string, StringLessNoCase> mLoadedGroups;   // +0x04
	ResMap mResMaps[NUM_RES_TYPES];                          // +0x24: one for each ResType
	std::map<std::string, BaseRes*> mResFromPathMap;         // +0xe4: each resource by its path in lower case
	std::map<void**, BaseRes*> mGlobalPtrMap;                // +0x104: each registered global's resource
	XMLParser* mXMLParser;                                   // +0x1b8
	std::string mError;                                      // +0x128
	bool mHasFailed;                                         // +0x1d8
	SexyAppBase* mApp;                                       // +0x1dc
	std::string mCurCompositeResGroup;                       // +0x1e0: the composite group being parsed or loaded
	std::string mCurResGroup;                                // +0x1fc
	int mCurResGroupArtRes;                                  // +0x218: the res of the group being parsed
	std::string mDefaultPath;                                // +0x21c: SetDefaults' path
	std::string mDefaultIdPrefix;                            // +0x238: SetDefaults' idprefix
	bool mAllowMissingProgramResources;                      // +0x254
	bool mAllowAlreadyDefinedResources;                      // +0x255: for a reparse while running
	bool mHadAlreadyDefinedError;                            // +0x256
	ResGroupMap mResGroupMap;                                // +0x25c
	ResList* mCurResGroupList;                               // +0x27c
	size_t mCurResGroupListIndex;                            // +0x280: the next resource of mCurResGroupList to load
	CompositeResGroupMap mCompositeResGroupMap;              // +0x288
	int mCurCompositeSubGroupIndex;                          // +0x2a8: the part of mCurCompositeResGroup being loaded
	int mBaseArtRes;                                         // +0x2ac: the art size the animations were made for
	int mCurArtRes;                                          // +0x2b0: the app's art size

	explicit PrimeResourceManager(SexyAppBase* theApp);   // 0x45c610 (bejeweled3)
	virtual ~PrimeResourceManager();                        // 0x45c860 (bejeweled3)

	bool ParseResourcesFile(const std::string& theFilename);  // 0x461a90 (bejeweled3)
	std::string GetErrorText() { return mError; }
	bool HadError() { return mHasFailed; }                    // 0x45d170 (bejeweled3)
	bool IsGroupLoaded(const std::string& theGroup);

	int GetNumResources(const std::string& theGroup);         // 0x464710 (bejeweled3): of every type

	virtual void StartLoadResources(const std::string& theGroup, bool fromComposite = false);  // 0x4642f0 (bejeweled3)
	virtual bool LoadNextResource();                          // 0x4640f0 (bejeweled3): false when the group is done
	virtual void ResourceLoadedHook(BaseRes* theRes) {}       // 0x5ad6c0 (bejeweled3): the shared empty function
	virtual bool LoadResources(const std::string& theGroup);  // 0x464480 (bejeweled3)
	virtual void DeleteResources(const std::string& theGroup);  // 0x45d0e0 (bejeweled3)

	// The resource of that type and id at the app's art size (or with no art size). theArtRes, when not 0, is the art
	// size the caller was generated for: another one is an error. theOptional returns nothing instead of throwing for a
	// resource that is not in the manifest.
	virtual SharedImageRef GetImageThrow(const std::string& theId, int theArtRes, bool theOptional = false);  // 0x464a80 (bejeweled3)
	virtual int GetSoundThrow(const std::string& theId);                                  // 0x464cf0 (bejeweled3)
	virtual Font* GetFontThrow(const std::string& theId, int theArtRes);                  // 0x464e00 (bejeweled3)
	virtual PopAnim* GetPopAnimThrow(const std::string& theId);                           // 0x464fc0 (bejeweled3)
	virtual PIEffect* GetPIEffectThrow(const std::string& theId);                         // 0x4650d0 (bejeweled3)
	virtual RenderEffectDefinition* GetRenderEffectThrow(const std::string& theId);       // 0x4651e0 (bejeweled3)

	SharedImageRef GetImage(const std::string& theId);   // null when not loaded
	int GetSound(const std::string& theId);              // -1 when not loaded
	Font* GetFont(const std::string& theId);
	Point GetImageOffset(const std::string& theId);      // the image's x= and y=, 0 when none
	// What the app's GetSharedImage does first with a name "file|ID" (0x46ff40 (bejeweled3)): the image resource ID's
	// picture, loaded if nothing holds it; null when the name has no bar or the manager has no such picture, with
	// *theFileName set to the file to load instead
	SharedImageRef GetSharedImageByRef(const std::string& theName, std::string* theFileName);

	// The generated code's global for that resource (of any type, at the app's art size): set to the resource
	// whenever it loads, cleared when it goes. The manifest must be parsed and mCurArtRes set first.
	void RegisterGlobalPtr(const std::string& theId, void** theGlobalPtr);  // 0x462000 (bejeweled3)
	// the id of the resource whose file is thePath (either slash, any case), or "" for none
	std::string GetIdByPath(const std::string& thePath);                     // 0x4647a0 (bejeweled3)

	void SetAllowMissingProgramImages(bool allow) { mAllowMissingProgramResources = allow; }

protected:
	bool Fail(const std::string& theErrorText);                                    // 0x45d180 (bejeweled3)
	BaseRes* GetBaseRes(ResType theType, const std::string& theId);              // 0x45cfb0 (bejeweled3)

	virtual bool ParseCommonResource(XMLElement& theElement, BaseRes* theRes, ResMap& theMap);  // 0x45d360 (bejeweled3)
	virtual bool ParseSoundResource(XMLElement& theElement);          // 0x45db30 (bejeweled3)
	virtual bool ParseImageResource(XMLElement& theElement);          // 0x45dfa0 (bejeweled3)
	virtual bool ParseFontResource(XMLElement& theElement);           // 0x45f6a0 (bejeweled3)
	virtual bool ParsePopAnimResource(XMLElement& theElement);        // 0x45fda0 (bejeweled3)
	virtual bool ParsePIEffectResource(XMLElement& theElement);       // 0x45fef0 (bejeweled3)
	virtual bool ParseRenderEffectResource(XMLElement& theElement);   // 0x460040 (bejeweled3)
	virtual bool ParseSetDefaults(XMLElement& theElement);            // 0x460310 (bejeweled3)
	virtual bool ParseResources();                                    // 0x460640 (bejeweled3)
	bool DoParseResources();                                          // 0x460d00 (bejeweled3)
	// a resource that was parsed again (mAllowAlreadyDefinedResources): the one already there, with the new path
	BaseRes* ReplaceRedefinedResource(BaseRes* theNewRes, ResMap& theMap);

	virtual void DeleteResources(ResMap& theMap, const std::string& theGroup);  // 0x45cc40 (bejeweled3)
	void Deref(BaseRes* theRes);                                                 // 0x465600 (bejeweled3)
	int GetNumResources(const std::string& theGroup, ResMap& theMap);            // 0x464590 (bejeweled3)

	bool LoadAlphaGridImage(ImageRes* theRes, DDImage* theImage);   // 0x462140 (bejeweled3)
	bool LoadAlphaImage(ImageRes* theRes, DDImage* theImage);       // 0x462370 (bejeweled3)
	virtual bool DoLoadImage(ImageRes* theRes);                     // 0x462500 (bejeweled3)
	virtual bool DoLoadFont(FontRes* theRes);                       // 0x463590 (bejeweled3)
	virtual bool DoLoadSound(SoundRes* theRes);                     // 0x462d30 (bejeweled3)
	virtual bool DoLoadPopAnim(PopAnimRes* theRes);                 // 0x463000 (bejeweled3)
	virtual bool DoLoadPIEffect(PIEffectRes* theRes);               // 0x463210 (bejeweled3)
	virtual bool DoLoadRenderEffect(RenderEffectRes* theRes);       // 0x463400 (bejeweled3)
	// loads theRes; *isLoaded is set when there was nothing to load (a resource the game makes)
	virtual bool DoLoadResource(BaseRes* theRes, bool* isLoaded);   // 0x463e70 (bejeweled3)
};

}  // namespace Sexy
