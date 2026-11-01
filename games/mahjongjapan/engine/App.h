#ifndef __MAHJONGJAPAN_APP_H__
#define __MAHJONGJAPAN_APP_H__

// The engine's application. AppBase (vtable 0x51e3c4, 0x4290c0..0x42a0d0) holds the names, the registry under
// Software\<company>\<product> and the command line; App (vtable 0x51e270, 0x425e90..0x426850) reads the settings,
// starts the manager and pushes the first layer, and writes the settings back at the end. The game's app
// (vtable 0x51df94, the static at 0x5d8cb8) fills in the four hooks.
//
// Port: the window, its message loop (Run 0x425fb0), the single-instance mutex, the version info, the Direct3D
// tests and their "3DSettings" values are the host's or are left out; the host calls the manager's frame on its
// timer, as WM_TIMER did.

#include "Manager.h"

namespace jb
{

class AppBase
{
public:
	std::string mCmdLine;      // +0x0c
	std::string mClassName;    // +0x18
	int mProductId;            // +0x20 the exe's string of the product's name (the registry key)
	int mTitleId;              // +0x24 the exe's string of the window's title
	bool mUseDefaults = false; // +0x28 the registry is not read

	// 0x4290c0
	AppBase(int theProductId, int theTitleId);
	// 0x429150
	virtual ~AppBase() {}

	virtual bool Init() = 0;                         // +0x04
	virtual void Exit() {}                           // +0x08 (0x429700)
	virtual const char* GetCompany() { return "JollyBear"; }  // +0x0c (0x429ab0)

	// 0x4291b0: a DWORD of the registry, or theDefault
	int RegGetInt(const char* theSection, const char* theName, int theDefault, bool theMachine = false);
	// 0x429230
	std::string RegGetString(const char* theSection, const char* theName, const char* theDefault,
		bool theMachine = false);
	// 0x429470: true only for 1
	bool RegGetBool(const char* theSection, const char* theName, bool theDefault, bool theMachine = false);
	// 0x429360
	static bool RegSetInt(const char* theSection, const char* theName, int theValue);
	// 0x4293c0: without a name the section is deleted, without a value the name
	static bool RegSetString(const char* theSection, const char* theName, const char* theValue);
	// 0x4294a0
	static void RegSetBool(const char* theSection, const char* theName, bool theValue);
	// 0x429800: the company, product and title names and the folders
	void SetupNames();
	// 0x429ac0: the product's key, the title's letters, digits and spaces
	void MakeProductKey();
	// 0x429b90: the number after a command line word (0 when there is none)
	int GetCmdInt(const char* theName);
	// 0x429ca0
	bool HasCmdFlag(const char* theName);
};

class App : public AppBase
{
public:
	Manager mManager;          // +0x34
	int mPeriod;               // +0x11c ms per frame

	// 0x425e90
	App(int thePeriod);
	// 0x425f10
	~App() override;

	bool Init() override;                      // 0x426060
	void Exit() override;                      // 0x4266c0
	virtual bool CanQuit() { return true; }    // +0x14 (0x424a50)
	virtual Layer* CreateFirstLayer() = 0;     // +0x18
	virtual void StartGame() = 0;              // +0x1c
	virtual void OnInit() = 0;                 // +0x20 before the manager starts
	virtual void OnExit() = 0;                 // +0x24

	// 0x425f80: CanQuit, then the manager shuts down (the window is destroyed)
	bool Quit();
	// 0x424ad0: the screen changes to theLayer, with a fade
	void SwitchTo(Layer* theLayer) { mManager.SwitchTo(theLayer); }
};

// 0x527ac0: the app
extern App* gApp;
// The names (0x5d8e70 company, 0x5d8e80 product key, 0x5d8e90 title, 0x5d8e68 exe name, 0x5d8e78 exe folder,
// 0x5d8e58 user data folder, 0x5d8e50 common data folder)
extern std::string gCompany, gProductKey, gTitle, gExeName, gExeFolder, gUserFolder, gCommonFolder;
// The settings (0x527ae0 SoundsOn, 0x5d8e28 SoundsVolume, 0x527ae1 MusicOn, 0x5d8e2c MusicVolume,
// 0x527ae2 HardwareAcceleration, 0x5d8e30 full screen, 0x5d8e31 WideScreen)
extern bool gSoundsOn, gMusicOn, gHardwareAccel, gFullScreen, gWideScreen;
extern int gSoundsVolume, gMusicVolume;
// 0x5d8e32: the desktop is wide enough for the wide screen (its width over 4 above its height over 3, plus 15%)
extern bool gWideAvailable;
// 0x5d8e18: the display runs on Direct3D, when the display's 3D test passed (0x432d4f) and HardwareAcceleration is
// on. The port has no such test: the setting's value, as on a machine where the test passes.
inline bool IsAccelerated() { return gHardwareAccel; }

// 0x4249e0: the exe's folder; 0x429e20, 0x429e40: the data folders
inline const std::string& GetExeFolder() { return gExeFolder; }
inline const std::string& GetUserFolder() { return gUserFolder; }
inline const std::string& GetCommonFolder() { return gCommonFolder; }

} // namespace jb

#endif
