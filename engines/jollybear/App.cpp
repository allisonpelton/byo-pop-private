#include "App.h"

#include "EngineRes.h"
#include "Host.h"
#include "Object.h"

#include <cctype>
#include <cstdlib>
#include <cstring>

namespace jb
{

App* gApp = nullptr;
std::string gCompany, gProductKey, gTitle, gExeName, gExeFolder, gUserFolder, gCommonFolder;
bool gWideAvailable = false;
bool gSoundsOn = true, gMusicOn = true, gHardwareAccel = false, gFullScreen = false, gWideScreen = false;
int gSoundsVolume = 100, gMusicVolume = 50;

// 0x428f40 (mahjongchina), 0x4290c0 (mahjongjapan)
AppBase::AppBase(int theProductId, int theTitleId) : mProductId(theProductId), mTitleId(theTitleId)
{
}

// 0x429030 (mahjongchina), 0x4291b0 (mahjongjapan)
int AppBase::RegGetInt(const char* theSection, const char* theName, int theDefault, bool theMachine)
{
	if (!mUseDefaults && !gCompany.empty())
	{
		int aValue;
		if (GetHost()->RegReadInt(theSection, theName, theMachine, aValue))
			return aValue;
	}
	return theDefault;
}

// 0x4292b0 (mahjongchina), 0x429470 (mahjongjapan)
bool AppBase::RegGetBool(const char* theSection, const char* theName, bool theDefault, bool theMachine)
{
	return RegGetInt(theSection, theName, theDefault ? 1 : 0, theMachine) == 1;
}

// 0x4291c0 (mahjongchina), 0x429360 (mahjongjapan)
bool AppBase::RegSetInt(const char* theSection, const char* theName, int theValue)
{
	if (gCompany.empty())
		return false;
	return GetHost()->RegWriteInt(theSection, theName, theValue);
}

// 0x4292e0 (mahjongchina), 0x4294a0 (mahjongjapan)
void AppBase::RegSetBool(const char* theSection, const char* theName, bool theValue)
{
	RegSetInt(theSection, theName, theValue ? 1 : 0);
}

// 0x429610 (mahjongchina), 0x429800 (mahjongjapan)
void AppBase::SetupNames()
{
	if (mProductId != 0)
	{
		gTitle = LoadExeString(mProductId);
		MakeProductKey();
	}
	if (mTitleId != 0)
		gTitle = LoadExeString(mTitleId);
	gCompany = GetCompany();
	// the original builds its exe's name in; the game sets gExeName before the app starts
	if (gTitle.empty())
	{
		gTitle = gExeName;
		MakeProductKey();
	}
	if (gExeFolder.empty())
		gExeFolder = GetHost()->ExeFolder();
	gUserFolder = GetHost()->FolderPath(0x1c);
	if (gUserFolder.empty())
		gUserFolder = GetHost()->FolderPath(0x1a);
	gCommonFolder = GetHost()->FolderPath(0x23);
	if (gCommonFolder.empty())
	{
		gCommonFolder = GetHost()->FolderPath(0x2e);
		if (gCommonFolder.empty())
			gCommonFolder = GetHost()->FolderPath(0x1a);
	}
}

// 0x429ac0 (mahjongjapan)
void AppBase::MakeProductKey()
{
	gProductKey.clear();
	int aLen = (int)gTitle.size();
	int i = 0;
	if (0 < aLen)
	{
		do
		{
			char aChar = gTitle[i];
			if (aChar == ' ' || isalnum((uint8_t)aChar))
				gProductKey += aChar;
			i = i + 1;
		} while (i < aLen);
	}
}

// 0x429960 (mahjongchina), 0x429ca0 (mahjongjapan)
bool AppBase::HasCmdFlag(const char* theName)
{
	int aPos = 0;
	int aLen = (int)mCmdLine.size();
	std::string aWord = Tokenize(mCmdLine, ' ', aPos);
	int aCompare = StrICmp(aWord.c_str(), theName);
	while (true)
	{
		if (aCompare == 0)
			return true;
		if (aLen <= aPos)
			break;
		aWord = Tokenize(mCmdLine, ' ', aPos);
		aCompare = StrICmp(aWord.c_str(), theName);
	}
	return false;
}

// 0x42ac60 (mahjongchina), 0x425e90 (mahjongjapan) (the app's strings 0x40 and 0x41 are the product and the title)
App::App(int thePeriod) : AppBase(0x40, 0x41), mPeriod(thePeriod)
{
	gApp = this;
}

// 0x42ace0 (mahjongchina), 0x425f10 (mahjongjapan)
App::~App()
{
	if (gApp == this)
		gApp = nullptr;
}

// 0x42ad50 (mahjongchina), 0x425f80 (mahjongjapan)
bool App::Quit()
{
	if (CanQuit())
	{
		mManager.Shutdown();
		return true;
	}
	return false;
}

// 0x42ae30 (mahjongchina), 0x426060 (mahjongjapan)
// 0x429520 (mahjongchina), 0x4296f0 (mahjongjapan) inlined here: AppBase's own Init, which only returns true (the end
// of this one)
bool App::Init()
{
	mCmdLine = GetHost()->CommandLine();
	SetupNames();
	if (RegGetBool(nullptr, "UseRegistryDefaults", false))
	{
		mUseDefaults = true;
		RegSetBool(nullptr, "UseRegistryDefaults", false);
	}
	gSoundsOn = RegGetBool(nullptr, "SoundsOn", true);
	gSoundsVolume = RegGetInt(nullptr, "SoundsVolume", 100);
	if (gSoundsVolume < 0)
		gSoundsVolume = 0;
	else if (100 < gSoundsVolume)
		gSoundsVolume = 100;
	gMusicOn = RegGetBool(nullptr, "MusicOn", true);
	gMusicVolume = RegGetInt(nullptr, "MusicVolume", 0x32);
	if (gMusicVolume < 0)
		gMusicVolume = 0;
	else if (100 < gMusicVolume)
		gMusicVolume = 100;
	int aScreenWidth;
	int aScreenHeight;
	GetHost()->ScreenSize(aScreenWidth, aScreenHeight);
	gWideAvailable = ((aScreenHeight / 3) * 0x73) / 100 < aScreenWidth / 4;
	gWideScreen = RegGetBool(nullptr, "WideScreen", false);
	bool aFullScreen = RegGetBool(nullptr, "FullScreen", true);
	if (HasCmdFlag("-no3d"))
		gHardwareAccel = false;
	else
		gHardwareAccel = RegGetBool(nullptr, "HardwareAcceleration", true);
	OnInit();
	if (!mManager.Init(aFullScreen, mPeriod))
		return false;
	mManager.SetMusicVolume(gMusicVolume);
	mManager.SetSoundVolume(gSoundsVolume);
	mManager.LoadSound(SOUND_CLICK, false);
	mManager.SwitchTo(CreateFirstLayer());
	return true;
}

// 0x42b490 (mahjongchina), 0x4266c0 (mahjongjapan)
void App::Exit()
{
	RegSetInt(nullptr, "SoundsVolume", gSoundsVolume);
	RegSetInt(nullptr, "MusicVolume", gMusicVolume);
	RegSetBool(nullptr, "FullScreen", gFullScreen);
	RegSetBool(nullptr, "WideScreen", gWideScreen);
	RegSetBool(nullptr, "HardwareAcceleration", gHardwareAccel);
	OnExit();
}

} // namespace jb
