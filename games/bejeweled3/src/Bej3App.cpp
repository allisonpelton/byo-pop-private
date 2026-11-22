// Sexy::Bej3App: see the header.
#include "Bej3App.h"

#include "Bej3Music.h"
#include "MainMenu.h"
#include "WidgetManager.h"
#include "DDInterface.h"
#include "EncodingParser.h"
#include "HighScores.h"
#include "PrimeResourceManager.h"
#include "Profile.h"
#include "QuestData.h"
#include "SoundInstance.h"
#include "SoundManager.h"
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
	// 0x671490: the high-score manager is made before the framework starts
	HighScoreManager* aHighScoreManager = HighScoreManager::Get();
	SexyApp::Init();
	if (mShutdown)
		return;
	aHighScoreManager->LoadHighScores("users/hiscores.dat");

	LoadProperties();
	LoadProperties("properties/defaultFramework.xml", true, false);
	LoadProperties("properties/defaultUIConstants.xml", true, false);
	LoadProperties("properties/defaultFilenames.xml", true, false);
	mPrimeResourceManager->mBaseArtRes = ART_RES_1200;
	mPrimeResourceManager->mCurArtRes = mArtRes;

	LoadQuestData();
	// port: the Steam statistics object at +0xbac (0x78a740, 0x78a8c0) is left out
	LoadTextFiles();
	LoadProfile();

	LoadResourceManifest();

	// The music (+0xa30) reads its songs, and the loading screen's song is asked for
	if (mBej3Music != nullptr) {
		mBej3Music->LoadMusicFile("properties/music.xml");
		mBej3Music->QueueEvent("Play", "LoadingScreen", false);
	}

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

	// 0x46c420: the three cursors
	SetCursorImage(CURSOR_POINTER, IMAGE_CURSOR_POINTER);
	SetCursorImage(CURSOR_HAND, IMAGE_CURSOR_HAND);
	SetCursorImage(CURSOR_DRAGGING, IMAGE_CURSOR_DRAGGING);
	// port: temple.p3d (0x861b70) is a 3D model; the 3D mode comes later
	// 0x6a54b0: the MainMenu at +0xba0, sized from the art size (offset -160, 1920 by 1200 at 1200) and put on the screen
	mMainMenu = new MainMenu();
	mMainMenu->Resize(-160 * mArtRes / 1200, 0, 1920 * mArtRes / 1200, mArtRes);
	mWidgetManager->AddWidget(mMainMenu);
	// port: the TooltipManager at +0xbf0 (0x67c540), the windowed button, the resize widget, the sound volumes and the
	// Zen file lists come with the next steps of Init
}

// 0x76aaf0: the volume goes through SetVolume, the pan through SetPan and the pitch through AdjustPitch, then the
// instance plays once and frees itself
void Bej3App::PlaySoundEx(int theSoundId, double theVolume, int thePan, double thePitch)
{
	if (IsMuted() || mSoundManager == nullptr)
		return;
	SoundInstance* aSound = mSoundManager->GetSoundInstance(theSoundId);
	if (aSound == nullptr)
		return;
	aSound->SetVolume(theVolume);
	aSound->SetPan(thePan);
	aSound->AdjustPitch(thePitch);
	aSound->Play(false, true);
}

// 0x76cd70: the original makes its CustomBassMusicInterface when the framework asks for the music interface
MusicInterface* Bej3App::CreateMusicInterface()
{
	if (mNoSoundNeeded)
		return SexyApp::CreateMusicInterface();
	mBej3Music = new Bej3Music();
	return mBej3Music;
}

// 0x76bc30: each quest file is read; a file that cannot be read shows the parser's error
void Bej3App::LoadQuestData()
{
	struct QuestFile {
		QuestDataParser** mParser;
		const char* mFileName;
	};
	// the original makes them in this order: quest.cfg, defaultquest.cfg, secret.cfg, speed.cfg
	const QuestFile aQuestFiles[] = {{&mQuestData, "properties/quest.cfg"}, {&mDefaultQuestData, "properties/defaultquest.cfg"},
		{&mSecretQuestData, "properties/secret.cfg"}, {&mSpeedQuestData, "properties/speed.cfg"}};
	for (const QuestFile& aQuestFile : aQuestFiles) {
		delete *aQuestFile.mParser;
		*aQuestFile.mParser = new QuestDataParser();
		if (!(*aQuestFile.mParser)->LoadDescriptor(aQuestFile.mFileName))
			Popup((*aQuestFile.mParser)->mError);
	}
}

namespace {

// The loop of 0x7748e0 over a text file's characters: a line ends at a new line, a carriage return, or the character
// that leaves nothing more to read (which, as in the original, is then not part of the line: a file without a
// last line break loses its last character); an empty line is left out. Returns false when the file cannot be opened.
bool ReadTextLines(const std::string& theFileName, std::vector<std::string>* theLines)
{
	EncodingParser aParser;
	if (!aParser.OpenFile(theFileName))
		return false;
	std::string aLine;
	wchar_t aChar = 0;
	while (true) {
		int aResult = aParser.GetChar(&aChar);
		if (aResult != EncodingParser::GET_CHAR_OK && aLine.empty())
			break;
		if (aChar == '\n' || aChar == '\r' || aParser.EndOfFile()) {
			if (!aLine.empty())
				theLines->push_back(aLine);
			aLine.clear();
		} else {
			aLine = AppendUTF8(aLine, aChar);
		}
	}
	aParser.CloseFile();
	return true;
}

}  // namespace

// 0x7748e0: tips.txt (a tip a line), ranks.txt (a name a line) and badges.txt ("<name>:<goals>" a line, each goal
// "<first>" or "<first>,<second>", the goals split at '/'); a file that cannot be opened shows a box
void Bej3App::LoadTextFiles()
{
	if (!ReadTextLines("properties/tips.txt", &mTips))
		Popup("Failed to open properties\\tips.txt");
	if (!ReadTextLines("properties/ranks.txt", &mRankNames))
		Popup("Failed to open properties\\ranks.txt");
	std::vector<std::string> aBadgeLines;
	if (!ReadTextLines("properties/badges.txt", &aBadgeLines))
		Popup("Failed to open properties\\badges.txt");
	for (const std::string& aBadgeLine : aBadgeLines) {
		size_t aColon = aBadgeLine.find(':');
		if (aColon == std::string::npos)
			continue;
		mBadgeNames.push_back(aBadgeLine.substr(0, aColon));
		std::vector<int> aGoals;
		std::vector<int> aSecondGoals;
		size_t aStart = aColon + 1;
		while (aStart < aBadgeLine.size()) {
			size_t aSlash = aBadgeLine.find('/', aStart);
			std::string aLevel = aBadgeLine.substr(aStart, aSlash == std::string::npos ? std::string::npos : aSlash - aStart);
			size_t aComma = aLevel.find(',');
			if (aComma == std::string::npos) {
				aGoals.push_back(atoi(aLevel.c_str()));
				aSecondGoals.push_back(0);
			} else {
				aGoals.push_back(atoi(aLevel.substr(0, aComma).c_str()));
				aSecondGoals.push_back(atoi(aLevel.substr(aComma + 1).c_str()));
			}
			aStart += aLevel.size() + 1;
		}
		mBadgeGoals.push_back(aGoals);
		mBadgeSecondGoals.push_back(aSecondGoals);
	}
}

// 0x7748e0: the Profile (0x71b530) reads the players (0x71d410); the registry's "LastUser" is chosen (0x71d7c0), else
// the first player who can be loaded (0x71d800)
void Bej3App::LoadProfile()
{
	delete mProfile;
	mProfile = new Profile();
	mProfile->LoadUsers();
	std::string aLastUser;
	RegistryReadString("LastUser", &aLastUser);
	if (!mProfile->SelectUser(StringToWString(aLastUser)))
		mProfile->SelectFirstLoadableUser();
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
	if (mProfile != nullptr)
		RegistryWriteString("LastUser", WStringToString(mProfile->mName));
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
