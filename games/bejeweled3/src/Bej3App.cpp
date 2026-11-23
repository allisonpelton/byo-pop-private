// Sexy::Bej3App: see the header.
#include "Bej3App.h"

#include "Bej3Dialog.h"
#include "Bej3Music.h"
#include "Board.h"
#include "GemSilhouettes.h"
#include "ButterflyBoard.h"
#include "IceStormBoard.h"
#include "DiamondMineBoard.h"
#include "PokerBoard.h"
#include "MoveLimitBoard.h"
#include "QuestMenu.h"
#include "TimeLimitBoard.h"
#include "TimeBombBoard.h"
#include "BalanceBoard.h"
#include "FillerBoard.h"
#include "NoLoseBoard.h"
#include "BadMoveLimitBoard.h"
#include "LightningBoard.h"
#include "ZenBoard.h"
#include "MemoryImage.h"
#include "MainMenu.h"
#include "PlayMenu.h"
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
#include <cctype>
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
	// the loaded player is saved as the profile goes (0x71c4b0)
	delete mProfile;
	mProfile = nullptr;
	for (auto& aRow : mGemFrames) {
		for (MemoryImage*& aFrame : aRow) {
			delete aFrame;
			aFrame = nullptr;
		}
	}
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
	GemSilhouettes::Load();   // port: the time gems' bolts follow the gems' edge (0x83a548), which is read from the exe
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

Dialog* Bej3App::NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
	const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	return new Bej3Dialog(theDialogId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);
}

namespace {

// 0x43acd0: the spaces and other white space at both ends cut off
std::wstring TrimmedName(const std::wstring& theName)
{
	size_t aStart = 0;
	while (aStart < theName.size() && iswspace(theName[aStart]))
		aStart++;
	size_t anEnd = theName.size();
	while (anEnd > aStart && iswspace(theName[anEnd - 1]))
		anEnd--;
	return theName.substr(aStart, anEnd - aStart);
}

}  // namespace

void Bej3App::DialogButtonDepress(int theDialogId, int theButtonId)
{
	if (theDialogId == 1) {
		// the new user dialog: OK takes the box's text without the spaces round it; an empty name is refused with a
		// message, a name already there with CreateProfile's, and neither closes the dialog
		NewUserDialog* aDialog = dynamic_cast<NewUserDialog*>(GetDialog(1));
		if (theButtonId == Dialog::ID_YES && aDialog != nullptr) {
			std::wstring aName = TrimmedName(StringToWString(aDialog->mEdit->mString));
			if (aName.empty()) {
				DoDialog(0, true, GetString("83", "ENTER NAME"), GetString("84", "You must enter your name!"),
					GetString("85", "OK"), Dialog::BUTTONS_FOOTER);
				return;
			}
			if (!mProfile->CreateProfile(aName))
				return;
			// port: the exe also updates the Steam statistics object (0x78a4e0, 0x78a460) and the user list dialog's
			// rows (id 4, 0x6774e0 and 0x6779c0): neither exists yet
		}
	}
	// every dialog that has not returned is closed: it fades out and goes (0x76a670)
	Bej3Dialog* aClosing = dynamic_cast<Bej3Dialog*>(GetDialog(theDialogId));
	if (aClosing != nullptr)
		aClosing->StartClose();
}

void Bej3App::DoWelcomeDialog()
{
	SDL_Log("Bej3App::DoWelcomeDialog()");
	WelcomeDialog* aDialog = new WelcomeDialog();
	aDialog->Resize(mWidth / 2 - mArtRes * 0x1c2 / 0x4b0, mArtRes * 0x1a4 / 0x4b0, mArtRes * 900 / 0x4b0,
		mArtRes * 400 / 0x4b0);
	AddDialog(aDialog);
	mWidgetManager->SetFocus(aDialog->mEdit);
	EnterScriptName();
}

// port: test only. The new user dialog (id 1) gets the name a script gave, typed into its box, and its OK is pressed (the box's Enter,
// 0x673b80, does the same), so the game goes on as for a player who has typed a name: the profile is made and the dialog fades out
void Bej3App::EnterScriptName()
{
	NewUserDialog* aDialog = dynamic_cast<NewUserDialog*>(GetDialog(1));
	if (mScriptName.empty() || aDialog == nullptr)
		return;
	aDialog->mEdit->mString = mScriptName;
	mScriptName.clear();
	aDialog->ButtonDepress(Dialog::ID_YES);
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

// 0x7737d0 (bejeweled3)
void Bej3App::StartClassic()
{
	StartBoard(new ClassicBoard(), {0x15});
}

// 0x773100 (bejeweled3): Zen. The board is made by 0x7126f0, sized, reset (slot 85) and started like Classic's. port: the exe's mode
// start also makes an 0x88-byte object first (the Zen mode's own state) and asks the mantras' window to open: both are left out
void Bej3App::StartZen()
{
	StartBoard(new ZenBoard(), {0});
}

// 0x773440 (bejeweled3): Lightning. The board is made by 0x718090; before it is reset (slot 85, 0x718500) the settings of the first quest
// of speed.cfg ("Lightning") are copied into the board's map (0x404870), "Title" among them
void Bej3App::StartLightning()
{
	LightningBoard* aBoard = new LightningBoard();
	if (mSpeedQuestData != nullptr && !mSpeedQuestData->mQuests.empty())
		aBoard->mSettings = mSpeedQuestData->mQuests[0].mSettings;
	aBoard->mSettings["Title"] = "Lightning";
	StartBoard(aBoard, {0, 10});
}

// 0x774060 (bejeweled3): a secret mode. The quest (secret.cfg's, by index: 0 Poker, 1 Butterflies, 2 Inferno, 3 Dig) says by its setting
// Challenge which board is made: the exe compares it with the names in this order, ignoring case: TIME BOMB, TIME BOMB (REAL TIME), INFERNO,
// BALANCE, BUTTERFLIES or BUTTERFLY (0x6dcdf0), FILLER, MOVE LIMIT, TIME LIMIT, NO LOSE, BAD MOVE LIMIT, POKER (0x6da1c0) and DIG (0x737420). The quest's settings
// are copied into the board's map (0x404870) and the secret flag (+0x3184, the call's fourth argument) is set before the board is reset
// (slot 85) and dealt (slot 135). port: only the boards that are made (Poker, Butterflies, Ice Storm, Diamond Mine)
bool Bej3App::StartSecretMode(int theIndex)
{
	if (mSecretQuestData == nullptr || theIndex < 0 || theIndex >= (int)mSecretQuestData->mQuests.size())
		return false;
	const std::map<std::string, std::string>& aSettings = mSecretQuestData->mQuests[theIndex].mSettings;
	auto aChallengeIter = aSettings.find("Challenge");
	std::string aChallenge = aChallengeIter == aSettings.end() ? std::string() : aChallengeIter->second;
	if (aChallenge.size() >= 2 && aChallenge.front() == '"' && aChallenge.back() == '"')
		aChallenge = aChallenge.substr(1, aChallenge.size() - 2);
	for (char& aChar : aChallenge)
		aChar = (char)toupper((unsigned char)aChar);
	QuestBoard* aBoard = nullptr;
	if (aChallenge == "BUTTERFLIES" || aChallenge == "BUTTERFLY")
		aBoard = new ButterflyBoard();
	if (aChallenge == "POKER")
		aBoard = new PokerBoard();
	if (aChallenge == "INFERNO")
		aBoard = new IceStormBoard();
	if (aChallenge == "DIG")
		aBoard = new DigBoard();   // 0x737420
	if (aBoard == nullptr)
		return false;
	aBoard->mSettings = aSettings;
	aBoard->mIsSecretMode = true;
	// 0x774860: the help of the secret mode
	int aHelpId = aChallenge == "POKER" ? 0x12 : aChallenge == "INFERNO" ? 0x14 : aChallenge == "DIG" ? 0x16 : 0x11;
	StartBoard(aBoard, {aHelpId});
	return true;
}

// 0x774060 (bejeweled3) with the secret flag 0: a quest of the map. The quest (quest.cfg's, by index) says by its setting Challenge which board is made (the names
// are compared as for the secret modes, see StartSecretMode); its settings are copied into the board's map and its index is the board's +0x3234 before the board is reset
// (slot 85) and dealt (slot 135). port: every challenge of the exe's list is made (Real Time Bomb's class is not: see TimeBombBoard); the quest map stays up when the
// challenge is not
bool Bej3App::StartQuest(int theIndex)
{
	if (mQuestData == nullptr || theIndex < 0 || theIndex >= (int)mQuestData->mQuests.size())
		return false;
	// port: a test's changes of settings (the quest command) are made before the challenge is read, so one can change the board
	std::map<std::string, std::string> aSettings = mQuestData->mQuests[(size_t)theIndex].mSettings;
	for (const auto& aChange : mQuestSettingChanges)
		aSettings[aChange.first] = aChange.second;
	auto aChallengeIter = aSettings.find("Challenge");
	std::string aChallenge = aChallengeIter == aSettings.end() ? std::string() : aChallengeIter->second;
	if (aChallenge.size() >= 2 && aChallenge.front() == '"' && aChallenge.back() == '"')
		aChallenge = aChallenge.substr(1, aChallenge.size() - 2);
	for (char& aChar : aChallenge)
		aChar = (char)toupper((unsigned char)aChar);
	QuestBoard* aBoard = nullptr;
	if (aChallenge == "TIME BOMB" || aChallenge == "TIME BOMB (REAL TIME)") {
		TimeBombBoard* aBombBoard = new TimeBombBoard();   // 0x869dc4, by 0x6dd440 with 0 or 1; the class of the real-time bombs (0x869954) is never made
		aBombBoard->mRealTime = aChallenge == "TIME BOMB (REAL TIME)";
		aBoard = aBombBoard;
	}
	else if (aChallenge == "INFERNO")
		aBoard = new IceStormBoard();   // 0x8685ec, by 0x6e2d90
	else if (aChallenge == "BALANCE")
		aBoard = new BalanceBoard();   // 0x86789c, by 0x6dd090
	else if (aChallenge == "BUTTERFLIES" || aChallenge == "BUTTERFLY")
		aBoard = new ButterflyBoard();   // 0x867d0c, by 0x6dcdf0
	else if (aChallenge == "FILLER")
		aBoard = new FillerBoard();   // 0x86817c, by 0x6dcc00
	else if (aChallenge == "MOVE LIMIT")
		aBoard = new MoveLimitBoard();   // 0x868a84
	else if (aChallenge == "TIME LIMIT")
		aBoard = new TimeLimitBoard();   // 0x86a234
	else if (aChallenge == "DIG")
		aBoard = new DigBoard();   // 0x8657d4, by 0x737420
	else if (aChallenge == "NO LOSE")
		aBoard = new NoLoseBoard();   // 0x868ef4, by 0x6da7a0
	else if (aChallenge == "BAD MOVE LIMIT")
		aBoard = new BadMoveLimitBoard();   // 0x86742c, by 0x6dd370
	else if (aChallenge == "POKER")
		aBoard = new PokerBoard();   // by 0x6da1c0
	if (aBoard == nullptr) {
		SDL_Log("Quest: the challenge \"%s\" is not made yet", aChallenge.c_str());
		return false;
	}
	aBoard->mSettings = aSettings;
	aBoard->mIsSecretMode = false;
	aBoard->mQuestIndex = theIndex;
	// the quest map goes (the exe keeps it under the game; the port makes a new one when it comes back)
	if (mQuestMenu != nullptr) {
		mWidgetManager->RemoveWidget(mQuestMenu);
		SafeDeleteWidget(mQuestMenu);
		mQuestMenu = nullptr;
	}
	StartBoard(aBoard);
	return true;
}

// 0x773b70 (bejeweled3): with theDeleteBoard the app's board (+0xbe8) goes; the quest map (0x693760) is made if there is none, 1920 wide, as high as the art and 160 (of
// 1200) left of the screen's edge, put on the screen (+0xbd8) with the focus, and the music's events FadeOut and Play of "QuestMenu" are queued. port: a map that is up is
// made again, so it shows the profile as it is
void Bej3App::ShowQuestMenu(bool theDeleteBoard)
{
	if (theDeleteBoard && mBoard != nullptr) {
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = nullptr;
	}
	if (mQuestMenu != nullptr) {
		mWidgetManager->RemoveWidget(mQuestMenu);
		SafeDeleteWidget(mQuestMenu);
		mQuestMenu = nullptr;
	}
	mQuestMenu = new QuestMenu();
	mQuestMenu->Resize(mArtRes * -160 / 0x4b0, 0, mArtRes * 1920 / 0x4b0, mArtRes);
	mWidgetManager->AddWidget(mQuestMenu);
	mWidgetManager->SetFocus(mQuestMenu);
	if (mBej3Music != nullptr) {
		mBej3Music->QueueEvent("FadeOut", "", false);
		mBej3Music->QueueEvent("Play", "QuestMenu", false);
	}
}

// 0x697640 (bejeweled3), the id 2: the quest map (and the board under it) goes and the play menu comes back (0x7716e0)
void Bej3App::LeaveQuestMenu()
{
	if (mQuestMenu != nullptr) {
		mWidgetManager->RemoveWidget(mQuestMenu);
		SafeDeleteWidget(mQuestMenu);
		mQuestMenu = nullptr;
	}
	if (mBoard != nullptr) {
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = nullptr;
	}
	ShowPlayMenu();
}

// the common part of the starts (0x7737d0, 0x773100, 0x773440, 0x774060)
void Bej3App::StartBoard(Board* theBoard, const std::vector<int>& theHelpIds)
{
	if (mGemFrames[0][7] == nullptr)
		MakeGemFrames();
	// the board of the game before goes (the exe also lets go of another screen at +0xba4: nothing there is made yet)
	if (mBoard != nullptr) {
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = nullptr;
	}
	mBoard = theBoard;
	mBoard->Resize(0, 0, mWidth, mHeight);
	mBoard->ResetGame();
	for (int aHelpId : theHelpIds)
		mBoard->RequestHelp(aHelpId);   // slot 213 (0x743f40), after the reset (slot 85) and before the board is added (0x7737d0, 0x773100, 0x773440)
	// 0x76cde0 asks whether a saved game of the mode is to be resumed: there is none, so the new game starts
	mWidgetManager->AddWidget(mBoard);
	mWidgetManager->SetFocus(mBoard);
	mBoard->NewGame();
	if (mBej3Music != nullptr) {
		mBej3Music->QueueEvent("FadeOut", "", false);
		mBej3Music->QueueEvent("Play", mBoard->GetModeName(), false);
	}
}

// 0x7716e0 (bejeweled3): the play menu (+0x270 of the main menu) is made visible, its mode ball (+0x488) goes back from the middle
// by the curve at +0x490 (0x689a10: 150 updates), and the music fades out and the menu's song plays. port: left out until their
// step: the picture's fade (slot 82 of the board's background, 0x77dde0), the ball's own two curves and the labels' curves
// (0x689a10, 0x684660) and the other screen at +0xba4, which is not made
void Bej3App::ShowPlayMenu()
{
	PlayMenu* aPlayMenu = mMainMenu->mPlayMenu;
	aPlayMenu->SetVisible(true);
	aPlayMenu->mHideCurve.SetCurve("b;0,1,0.006667,1,~###         ~####");
	if (mBej3Music != nullptr) {
		mBej3Music->QueueEvent("FadeOut", "", false);
		mBej3Music->QueueEvent("Play", "MainMenu", false);
	}
}
