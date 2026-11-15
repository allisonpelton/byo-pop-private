#ifndef __JB_APP_H__
#define __JB_APP_H__

// The engine's application. AppBase (vtable 0x51cec0 (mahjongchina), 0x51e3c4 (mahjongjapan),
// 0x428f40..0x429d80 (mahjongchina), 0x4290c0..0x42a0d0 (mahjongjapan)) holds the names, the registry under
// Software\<company>\<product> and the command line; App (vtable 0x51d0ec (mahjongchina), 0x51e270 (mahjongjapan),
// 0x42ac60..0x42b600 (mahjongchina), 0x425e90..0x426850 (mahjongjapan)) reads the settings, starts the manager and
// pushes the first layer, and writes the settings back at the end. The game's app (vtable 0x51be1c (mahjongchina),
// 0x51df94 (mahjongjapan), the static at 0x5d8cb8 (mahjongjapan)) fills in the four hooks.
//
// Port: the window, its message loop (Run 0x42ad80 (mahjongchina), 0x425fb0 (mahjongjapan)), the single-instance mutex,
// the version info, the Direct3D tests and their "3DSettings" values are the host's or are left out; the host calls the
// manager's frame on its timer, as WM_TIMER did.

#include "Manager.h"

namespace jb
{

class AppBase
{
public:
	std::string mCmdLine;      // +0x0c
	int mProductId;            // +0x20 the exe's string of the product's name (the registry key)
	int mTitleId;              // +0x24 the exe's string of the window's title
	bool mUseDefaults = false; // +0x28 the registry is not read

	// 0x428f40 (mahjongchina), 0x4290c0 (mahjongjapan)
	AppBase(int theProductId, int theTitleId);
	// 0x428fd0 (mahjongchina), 0x429150 (mahjongjapan)
	virtual ~AppBase() {}

	virtual bool Init() = 0;                         // +0x04
	virtual void Exit() {}                           // +0x08 (0x4221c0 (mahjongchina), 0x429700 (mahjongjapan))
	virtual const char* GetCompany() { return "JollyBear"; }  // +0x0c (0x429ab0 (mahjongjapan))

	// 0x429030 (mahjongchina), 0x4291b0 (mahjongjapan): a DWORD of the registry, or theDefault
	int RegGetInt(const char* theSection, const char* theName, int theDefault, bool theMachine = false);
	// 0x4292b0 (mahjongchina), 0x429470 (mahjongjapan): true only for 1
	bool RegGetBool(const char* theSection, const char* theName, bool theDefault, bool theMachine = false);
	// 0x4291c0 (mahjongchina), 0x429360 (mahjongjapan)
	static bool RegSetInt(const char* theSection, const char* theName, int theValue);
	// 0x4292e0 (mahjongchina), 0x4294a0 (mahjongjapan)
	static void RegSetBool(const char* theSection, const char* theName, bool theValue);
	// 0x429610 (mahjongchina), 0x429800 (mahjongjapan): the company, product and title names and the folders
	void SetupNames();
	// 0x429ac0 (mahjongjapan): the product's key, the title's letters, digits and spaces
	void MakeProductKey();
	// 0x429960 (mahjongchina), 0x429ca0 (mahjongjapan)
	bool HasCmdFlag(const char* theName);
};

class App : public AppBase
{
public:
	Manager mManager;          // +0x34
	int mPeriod;               // +0x11c ms per frame

	// 0x42ac60 (mahjongchina), 0x425e90 (mahjongjapan)
	App(int thePeriod);
	// 0x42ace0 (mahjongchina), 0x425f10 (mahjongjapan)
	~App() override;

	bool Init() override;                      // 0x42ae30 (mahjongchina), 0x426060 (mahjongjapan)
	void Exit() override;                      // 0x42b490 (mahjongchina), 0x4266c0 (mahjongjapan)
	virtual bool CanQuit() { return true; }    // +0x14 (0x40f4c0 (mahjongchina), 0x424a50 (mahjongjapan))
	virtual Layer* CreateFirstLayer() = 0;     // +0x18
	virtual void StartGame() = 0;              // +0x1c
	virtual void OnInit() = 0;                 // +0x20 before the manager starts
	virtual void OnExit() = 0;                 // +0x24

	// 0x42ad50 (mahjongchina), 0x425f80 (mahjongjapan): CanQuit, then the manager shuts down (the window is destroyed)
	bool Quit();
	// 0x424ad0 (mahjongjapan): the screen changes to theLayer, with a fade
	void SwitchTo(Layer* theLayer) { mManager.SwitchTo(theLayer); }
};

// 0x5240cc (mahjongchina), 0x527ac0 (mahjongjapan): the app
extern App* gApp;
// The names (0x5d80d0 (mahjongchina), 0x5d8e70 (mahjongjapan) company, 0x5d8e80 (mahjongjapan) product key,
// 0x5d8e90 (mahjongjapan) title, 0x5d80e0 (mahjongchina), 0x5d8e68 (mahjongjapan) exe name, 0x5d80e8 (mahjongchina),
// 0x5d8e78 (mahjongjapan) exe folder, 0x5d80c8 (mahjongchina), 0x5d8e58 (mahjongjapan) user data folder,
// 0x5d80c0 (mahjongchina), 0x5d8e50 (mahjongjapan) common data folder)
extern std::string gCompany, gProductKey, gTitle, gExeName, gExeFolder, gUserFolder, gCommonFolder;
// The settings (SoundsOn 0x526b58 (mahjongchina), 0x527ae0 (mahjongjapan); SoundsVolume 0x5d8150 (mahjongchina),
// 0x5d8e28 (mahjongjapan); MusicOn 0x526b59 (mahjongchina), 0x527ae1 (mahjongjapan); MusicVolume
// 0x5d8154 (mahjongchina), 0x5d8e2c (mahjongjapan); HardwareAcceleration 0x526b5b (mahjongchina),
// 0x527ae2 (mahjongjapan); full screen 0x5d8158 (mahjongchina), 0x5d8e30 (mahjongjapan); WideScreen
// 0x5d8159 (mahjongchina), 0x5d8e31 (mahjongjapan))
extern bool gSoundsOn, gMusicOn, gHardwareAccel, gFullScreen, gWideScreen;
extern int gSoundsVolume, gMusicVolume;
// 0x5d8162 (mahjongchina), 0x5d8e32 (mahjongjapan): the desktop is wide enough for the wide screen (its width over 4
// above its height over 3, plus 15%)
extern bool gWideAvailable;
// 0x5d8224 (mahjongchina), 0x5d8e18 (mahjongjapan): the display runs on Direct3D, when the display's 3D test passed
// (0x432d4f (mahjongjapan)) and HardwareAcceleration is on. The port has no such test: the setting's value, as on a
// machine where the test passes.
inline bool IsAccelerated() { return gHardwareAccel; }

// 0x4249e0 (mahjongjapan): the exe's folder; 0x429ae0 (mahjongchina), 0x429e20 (mahjongjapan), 0x429b00 (mahjongchina),
// 0x429e40 (mahjongjapan): the data folders
inline const std::string& GetExeFolder() { return gExeFolder; }
// 0x4249f0 (mahjongjapan) inlined here: the user folder's getter (it copies the string of 0x429ae0 (mahjongchina),
// 0x429e20 (mahjongjapan))
inline const std::string& GetUserFolder() { return gUserFolder; }
// 0x424a10 (mahjongjapan) inlined here: the shared folder's getter (it copies the string of 0x429b00 (mahjongchina),
// 0x429e40 (mahjongjapan))
inline const std::string& GetCommonFolder() { return gCommonFolder; }

} // namespace jb

#endif
