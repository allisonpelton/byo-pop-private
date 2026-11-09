#include "App.h"

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

// 0x4290c0
AppBase::AppBase(int theProductId, int theTitleId) : mProductId(theProductId), mTitleId(theTitleId)
{
}

// 0x4291b0
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

// 0x429230
std::string AppBase::RegGetString(const char* theSection, const char* theName, const char* theDefault,
	bool theMachine)
{
	if (!mUseDefaults && !gCompany.empty())
	{
		std::string aValue;
		if (GetHost()->RegReadString(theSection, theName, theMachine, aValue))
			return aValue;
	}
	return theDefault;
}

// 0x429470
bool AppBase::RegGetBool(const char* theSection, const char* theName, bool theDefault, bool theMachine)
{
	return RegGetInt(theSection, theName, theDefault ? 1 : 0, theMachine) == 1;
}

// 0x429360
bool AppBase::RegSetInt(const char* theSection, const char* theName, int theValue)
{
	if (gCompany.empty())
		return false;
	return GetHost()->RegWriteInt(theSection, theName, theValue);
}

// 0x4293c0
bool AppBase::RegSetString(const char* theSection, const char* theName, const char* theValue)
{
	if (gCompany.empty())
		return false;
	if (theName == nullptr)
		return GetHost()->RegDeleteKey(theSection);
	if (theValue == nullptr)
		return GetHost()->RegDeleteValue(theSection, theName);
	return GetHost()->RegWriteString(theSection, theName, theValue);
}

// 0x4294a0
void AppBase::RegSetBool(const char* theSection, const char* theName, bool theValue)
{
	RegSetInt(theSection, theName, theValue ? 1 : 0);
}

// 0x429800
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
	if (gExeName.empty())
		gExeName = "MahjongEscapeAJ";
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

// 0x429ac0
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

// 0x429b90
int AppBase::GetCmdInt(const char* theName)
{
	int aPos = 0;
	bool aFound = false;
	int aLen = (int)mCmdLine.size();
	std::string aWord = Tokenize(mCmdLine, ' ', aPos);
	while (true)
	{
		if (StrICmp(aWord.c_str(), theName) == 0)
			aFound = true;
		if (aLen <= aPos)
			break;
		aWord = Tokenize(mCmdLine, ' ', aPos);
		if (aFound)
		{
			char* anEnd;
			return (int)strtol(aWord.c_str(), &anEnd, 10);
		}
	}
	return 0;
}

// 0x429ca0
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

// 0x425e90 (the app's strings 0x40 and 0x41 are the product and the title)
App::App(int thePeriod) : AppBase(0x40, 0x41), mPeriod(thePeriod)
{
	gApp = this;
}

// 0x425f10
App::~App()
{
	if (gApp == this)
		gApp = nullptr;
}

// 0x425f80
bool App::Quit()
{
	if (CanQuit())
	{
		mManager.Shutdown();
		return true;
	}
	return false;
}

// 0x426060
// 0x4296f0 inlined here: AppBase's own Init, which only returns true (the end of this one)
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
	mManager.LoadSound(0x69, false);
	mManager.SwitchTo(CreateFirstLayer());
	return true;
}

// 0x4266c0
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
