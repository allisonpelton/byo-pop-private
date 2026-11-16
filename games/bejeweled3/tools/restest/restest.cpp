// port: b3_restest reads Bejeweled 3's manifest (properties/resources.xml) with engines/prime's resource manager and
// loads the groups the game loads, in its order, headless:
//     build/native/b3_restest.exe [-gamedir=<folder>] [-artres=600|768|1200|all] [-parse] [-v]
// For each art size (600 by default, the 2D mode's) it parses the manifest, gives every global of the generated code
// (src/Res.cpp) to the manager, then loads and extracts the groups as Bej3App's Init (0x7748e0) and its loading thread
// (0x772740) do: Init, Loader, 2DOnly (the app here has no 3D) and LoadingThread. It prints the counts of each kind
// in the manifest, each group's resources, how long its load took and whether its extraction got every resource, then
// a hash of the loaded images' sizes and the fonts' heights for comparing builds. -parse stops after the manifest.
// The output names the game's resources: it is for reading and goes into no file of the repository.
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <SDL.h>  // main becomes SDL_main, which the platform's SDL2main calls

#include "DDImage.h"
#include "DDInterface.h"
#include "Font.h"
#include "PrimeResourceManager.h"
#include "SDLSoundManager.h"
#include "SexyAppBase.h"
#include "platform/VFS.h"
#include "platform/openjpeg/JP2Decoder.h"
#include "../../src/Res.h"

using namespace Sexy;

namespace {

std::string gGameDir = "assets/bejeweled3";
bool gVerbose = false;

const char* const kKindNames[PrimeResourceManager::NUM_RES_TYPES] = {
	"Image", "Sound", "Font", "PopAnim", "PIEffect", "RenderEffect"};

typedef bool (*ExtractFunction)(PrimeResourceManager* theManager);

struct GroupStep {
	const char* mGroup;
	ExtractFunction mExtract;
};

// Bej3App::Init's groups, then its loading thread's
const GroupStep kSteps[] = {
	{"Init", ExtractInitResources},
	{"Loader", ExtractLoaderResources},
	{"2DOnly", Extract2DOnlyResources},
	{"LoadingThread", ExtractLoadingThreadResources},
};

unsigned int HashInt(unsigned int theHash, int theValue)
{
	return (theHash ^ (unsigned int)theValue) * 16777619u;  // FNV-1a over whole values
}

unsigned int HashString(unsigned int theHash, const std::string& theText)
{
	for (size_t i = 0; i < theText.size(); i++)
		theHash = HashInt(theHash, (unsigned char)theText[i]);
	return theHash;
}

int RunArtRes(int theArtRes, bool theParseOnly)
{
	int aFailures = 0;
	PrimeResourceManager* aManager = new PrimeResourceManager(gSexyAppBase);
	aManager->mBaseArtRes = 1200;  // Bej3App::Init sets both before it parses
	aManager->mCurArtRes = theArtRes;
	if (!aManager->ParseResourcesFile("properties/resources.xml")) {
		printf("FAIL %d: the manifest: %s\n", theArtRes, aManager->GetErrorText().c_str());
		delete aManager;
		return 1;
	}
	printf("art %d: the manifest has", theArtRes);
	for (int aKind = 0; aKind < PrimeResourceManager::NUM_RES_TYPES; aKind++)
		printf(" %d %s", (int)aManager->mResMaps[aKind].size(), kKindNames[aKind]);
	printf(", %d groups, %d composite groups\n", (int)aManager->mResGroupMap.size(),
		(int)aManager->mCompositeResGroupMap.size());
	if (theParseOnly) {
		delete aManager;
		return 0;
	}

	RegisterResourceGlobals(aManager);
	for (size_t aStep = 0; aStep < sizeof(kSteps) / sizeof(kSteps[0]); aStep++) {
		const GroupStep& aGroupStep = kSteps[aStep];
		int aCount = aManager->GetNumResources(aGroupStep.mGroup);
		std::chrono::steady_clock::time_point aStart = std::chrono::steady_clock::now();
		bool aLoaded = aManager->LoadResources(aGroupStep.mGroup);
		long long aMillis = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - aStart).count();
		bool anExtracted = aLoaded && aGroupStep.mExtract(aManager);
		printf("art %d: %s: %d resources, %s in %lld ms, %s\n", theArtRes, aGroupStep.mGroup, aCount,
			aLoaded ? "loaded" : "load FAILED", aMillis, anExtracted ? "extracted" : "extraction FAILED");
		if (!aLoaded || !anExtracted) {
			aFailures++;
			printf("FAIL %d %s: %s\n", theArtRes, aGroupStep.mGroup, aManager->GetErrorText().c_str());
			aManager->mError.clear();
			aManager->mHasFailed = false;
		}
	}

	// the loaded images' sizes and cels and the fonts' heights, by id
	unsigned int aHash = 2166136261u;
	int anImages = 0, aFonts = 0;
	for (int anId = 0; anId < RESOURCE_ID_MAX; anId++) {
		std::string aName = GetStringIdById(anId);
		Image* anImage = aManager->GetImage(aName);
		if (anImage != nullptr) {
			anImages++;
			aHash = HashInt(HashInt(HashInt(HashString(aHash, aName), anImage->mWidth), anImage->mHeight),
				anImage->mNumRows * 1000 + anImage->mNumCols);
			if (gVerbose)
				printf("  %s %dx%d %dx%d\n", aName.c_str(), anImage->mWidth, anImage->mHeight, anImage->mNumCols,
					anImage->mNumRows);
		}
		Font* aFont = aManager->GetFont(aName);
		if (aFont != nullptr) {
			aFonts++;
			aHash = HashInt(HashInt(HashString(aHash, aName), aFont->GetHeight()), aFont->GetAscent());
		}
	}
	printf("art %d: %d images and %d fonts loaded, hash %08x\n", theArtRes, anImages, aFonts, aHash);
	delete aManager;
	gSexyAppBase->CleanSharedImages();
	return aFailures;
}

}  // namespace

int main(int argc, char** argv)
{
	std::vector<int> anArtSizes;
	bool aParseOnly = false;
	for (int i = 1; i < argc; i++) {
		if (strncmp(argv[i], "-gamedir=", 9) == 0)
			gGameDir = argv[i] + 9;
		else if (strcmp(argv[i], "-artres=all") == 0) {
			anArtSizes.push_back(600);
			anArtSizes.push_back(768);
			anArtSizes.push_back(1200);
		}
		else if (strncmp(argv[i], "-artres=", 8) == 0)
			anArtSizes.push_back(atoi(argv[i] + 8));
		else if (strcmp(argv[i], "-parse") == 0)
			aParseOnly = true;
		else if (strcmp(argv[i], "-v") == 0)
			gVerbose = true;
		else {
			printf("unknown argument: %s\n", argv[i]);
			return 2;
		}
	}
	if (anArtSizes.empty())
		anArtSizes.push_back(600);

	// The least the framework needs to load resources: the game folder, an app (the images belong to one), a display
	// object that is never started (so the app is in 2D, as the 2D mode is) and a sound manager on SDL's silent driver.
	// Never deleted, so nothing at exit waits on a window or the sound.
	SDL_setenv("SDL_AUDIODRIVER", "dummy", 0);
	Platform::VFS::Get().SetBaseDir(gGameDir);
	Platform::RegisterOpenJPEGImages();
	SexyAppBase* anApp = new SexyAppBase();
	anApp->mDDInterface = new DDInterface(anApp);
	anApp->mSoundManager = new SDLSoundManager();

	int aFailures = 0;
	for (size_t i = 0; i < anArtSizes.size(); i++)
		aFailures += RunArtRes(anArtSizes[i], aParseOnly);
	printf("%s: %d failures\n", aFailures == 0 ? "OK" : "FAILED", aFailures);
	fflush(stdout);
	_exit(aFailures == 0 ? 0 : 1);
}
