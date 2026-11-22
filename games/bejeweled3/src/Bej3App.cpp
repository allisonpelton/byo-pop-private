// Sexy::Bej3App: see the header.
#include "Bej3App.h"

#include "DDInterface.h"
#include "PrimeResourceManager.h"
#include "Res.h"
#include "X86Math.h"

#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

using namespace Sexy;

Bej3App* Sexy::gApp = nullptr;

namespace {

// The constructor's choice of art size where nothing else gives one: 600 on a machine with 512 MB of memory or less
// or a desktop under 601 pixels high, else 768 (the original's compat.cfg could name another, GetAppDefaultArtRes;
// the port has no compat.cfg)
int DefaultArtRes(int theMemoryMB, int theDesktopHeight)
{
	if (theMemoryMB < 513 || theDesktopHeight < 601)
		return Bej3App::ART_RES_600;
	return Bej3App::ART_RES_768;
}

}  // namespace

Bej3App::Bej3App()
{
	gApp = this;
	mProdName = "Bejeweled3";
	mRegKey = "PopCap\\Bejeweled3";
	mTitle = _S("Bejeweled 3");
	// +0x7f0: made by the original's SexyAppBase constructor
	mPrimeResourceManager = new PrimeResourceManager(this);

	// port: the machine as the original would see a current one
	SDL_DisplayMode aMode;
	int aDesktopHeight = SDL_GetDesktopDisplayMode(0, &aMode) == 0 ? aMode.h : 1080;
	int aMemoryMB = SDL_GetSystemRAM();
#ifdef __EMSCRIPTEN__
	// port: a browser gives SDL no memory size; navigator.deviceMemory is in gigabytes (at most 8), 4 when unknown
	aMemoryMB = EM_ASM_INT({ return (navigator.deviceMemory || 4) * 1024; });
#endif
	mArtRes = DefaultArtRes(aMemoryMB, aDesktopHeight);
	mHeight = mArtRes;
	mWidth = mArtRes * 4 / 3;
}

// port: the original's app is a global object that lives until the process ends
Bej3App::~Bej3App()
{
	delete mPrimeResourceManager;
	gApp = nullptr;
}

void Bej3App::Init()
{
	SexyApp::Init();
	if (mShutdown)
		return;

	LoadProperties();
	LoadProperties("properties/defaultFramework.xml", true, false);
	LoadProperties("properties/defaultUIConstants.xml", true, false);
	LoadProperties("properties/defaultFilenames.xml", true, false);
	mPrimeResourceManager->mBaseArtRes = ART_RES_1200;
	mPrimeResourceManager->mCurArtRes = mArtRes;

	LoadResourceManifest();

	mPrimeResourceManager->LoadResources("Init");
	if (!ExtractInitResources(mPrimeResourceManager))
		ShowResourceError(true);
	mPrimeResourceManager->LoadResources("Loader");
	if (!ExtractLoaderResources(mPrimeResourceManager))
		ShowResourceError(true);
	if (!Is3DAccelerated()) {
		mPrimeResourceManager->LoadResources("2DOnly");
		if (!Extract2DOnlyResources(mPrimeResourceManager))
			ShowResourceError(true);
	}
}

void Bej3App::LoadingThreadProc()
{
	mNumLoadingThreadTasks = mPrimeResourceManager->GetNumResources("LoadingThread") + 7;
	mPrimeResourceManager->StartLoadResources("LoadingThread");
	while (mPrimeResourceManager->LoadNextResource()) {
		if (mShutdown)
			return;
		mCompletedLoadingThreadTasks++;
	}
	if (!ExtractLoadingThreadResources(mPrimeResourceManager))
		ShowResourceError(false);
}

void Bej3App::WriteToRegistry()
{
	SexyApp::WriteToRegistry();
	if (mArtResChanged)
		RegistryWriteInteger("ArtRes", mArtRes);
	RegistryWriteInteger("VoiceVolume", Ftol(mVoiceVolume * 100.0));
	RegistryWriteInteger("ZenAmbientVolume", Ftol(mZenAmbientVolume * 100.0));
	RegistryWriteInteger("ZenAmbientMusicVolume", Ftol(mZenAmbientMusicVolume * 100.0));
	RegistryWriteInteger("ZenBinauralVolume", Ftol(mZenBinauralVolume * 100.0));
	RegistryWriteInteger("ZenBreathVolume", Ftol(mZenBreathVolume * 100.0));
	RegistryWriteBoolean("AnimateBackground", mAnimateBackground);
	RegistryWriteInteger("TipIdx", mTipIndex);
}

void Bej3App::ReadFromRegistry()
{
	SexyApp::ReadFromRegistry();
	if (!mArtResFromCmdLine) {
		RegistryReadInteger("ArtRes", &mArtRes);
		mHeight = mArtRes;
		mWidth = mArtRes * 4 / 3;
	}
	int aPercent;
	if (RegistryReadInteger("VoiceVolume", &aPercent))
		mVoiceVolume = (double)aPercent / 100.0;
	if (RegistryReadInteger("ZenAmbientVolume", &aPercent))
		mZenAmbientVolume = (double)aPercent / 100.0;
	if (RegistryReadInteger("ZenAmbientMusicVolume", &aPercent))
		mZenAmbientMusicVolume = (double)aPercent / 100.0;
	if (RegistryReadInteger("ZenBinauralVolume", &aPercent))
		mZenBinauralVolume = (double)aPercent / 100.0;
	if (RegistryReadInteger("ZenBreathVolume", &aPercent))
		mZenBreathVolume = (double)aPercent / 100.0;
	RegistryReadBoolean("AnimateBackground", &mAnimateBackground);
	RegistryReadInteger("TipIdx", &mTipIndex);
}

void Bej3App::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	int anArtRes = 0;
	if (theParamName == "-600")
		anArtRes = ART_RES_600;
	else if (theParamName == "-768")
		anArtRes = ART_RES_768;
	else if (theParamName == "-1200")
		anArtRes = ART_RES_1200;
	if (anArtRes == 0) {
		SexyApp::HandleCmdLineParam(theParamName, theParamValue);
		return;
	}
	mWidth = anArtRes * 4 / 3;
	mArtRes = anArtRes;
	mHeight = anArtRes;
	mArtResFromCmdLine = true;
}

void Bej3App::Use2DMode()
{
	if (mArtRes != ART_RES_600)
		mArtRes = ART_RES_600;
	mWidth = 800;
	mHeight = 600;
}

// port: the original sets the 2D mode's size once its display has come up without 3D (0x76b020, slot 85); the port
// knows before it makes the window, which is made at that size
void Bej3App::MakeWindow()
{
	if (!mUse3D)
		Use2DMode();
	SexyApp::MakeWindow();
}

// port: the framework calls its display 3D-accelerated whenever SDL's renderer runs on the GPU, but for the game
// Is3DAccelerated means its 3D mode: a 2D run says no, as the original does without Direct3D
void Bej3App::PostDDInterfaceInitHook()
{
	SexyApp::PostDDInterfaceInitHook();
	if (!mUse3D)
		mDDInterface->mIs3D = false;
}

// The framework's own, after this generation's first step: a name "file|ID" is the image resource ID's picture when the
// manager has one (*isNew is then left as it was), else the file before the bar
SharedImageRef Bej3App::GetSharedImage(const std::string& theFileName, const std::string& theVariant, bool* isNew)
{
	std::string aFileName;
	SharedImageRef anImage = mPrimeResourceManager->GetSharedImageByRef(theFileName, &aFileName);
	if ((DDImage*)anImage != nullptr)
		return anImage;
	return SexyApp::GetSharedImage(aFileName, theVariant, isNew);
}

void Bej3App::LoadResourceManifest()
{
	if (!mPrimeResourceManager->ParseResourcesFile("properties/resources.xml"))
		ShowResourceError(true);
}

void Bej3App::ShowResourceError(bool doExit)
{
	SDL_Log("%s", mPrimeResourceManager->GetErrorText().c_str());
	Popup(mPrimeResourceManager->GetErrorText());
	if (doExit)
		DoExit(0);
}
