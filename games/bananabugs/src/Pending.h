// port: stand-ins for the parts of the original not translated yet, so the screens that are can run. Each piece moves
// to its own class's files as that class is translated, and this file goes once it is empty. Each stand-in names the
// original's address; one that does less than the original says so.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class Anim;
class LevelObj;

// The level being played (0x405770); a stand-in shows nothing and never ends
class Board : public ObjBase
{
public:
	int mLeaveTrack;           // +0x280: the track MainMenu's StartLeaving (0x440a60) starts, also on a board
	LevelObj* mLevel;          // +0x788

	Board();
	~Board();
	void StartLeaving() { StartTrack(mLeaveTrack); } // 0x440a60
};

// What a level ended with (LevelObj::GetResult); the names are to be checked as LevelObj is translated
enum LevelResult
{
	LEVEL_RESULT_NONE = 0,
	LEVEL_RESULT_WON = 1,      // on to the map
	LEVEL_RESULT_QUIT = 2,     // back to the main menu
	LEVEL_RESULT_LOST = 3,     // back to the main menu
};

// The rules of a level (0x439700); the stand-in's never end
class LevelObj
{
public:
	int GetResult() { return LEVEL_RESULT_NONE; } // 0x43a9b0
};

// What the main menu chose (MainMenu::GetChoice)
enum MenuChoice
{
	MENU_CHOICE_NONE = 0,
	MENU_CHOICE_PLAY = 1,
	MENU_CHOICE_QUIT = 5,
};

// The main menu (0x43fc90); the stand-in shows nothing and chooses nothing
class MainMenu : public ObjBase
{
public:
	int mChoice;               // +0x27c: MenuChoice
	int mLeaveTrack;           // +0x280

	MainMenu();
	~MainMenu();
	int GetChoice() { return mChoice; } // 0x440a50
	void StartLeaving() { StartTrack(mLeaveTrack); } // 0x440a60
};

// The map between levels (0x448e10)
class MetaGame : public ObjBase
{
public:
	bool mWantsMenu;           // +0x27c
	bool mLeaving;             // +0x27d

	MetaGame();
	~MetaGame();
	bool TakeLevelChosen();    // 0x449b90
};

// A player's profile (0x459380), also the object that shows the player's state in a level
class Player : public ObjBase
{
public:
	bool mMetaGameStarted;     // +0x3b8

	static Player* sCurrent;   // 0x670198

	Player();
	~Player();
	static void BoardClick(int theButton); // 0x45d8e0
};

// The pause window (0x454ef0)
class PauseWin
{
public:
	static bool sEnabled;      // 0x60e64b
	static void PauseForFocusLoss(); // 0x455920
	static bool IsBlocking();  // 0x455a10
	static void ResumeOnRelease(); // 0x455a40
	static void Enable() { sEnabled = true; } // 0x455a30
};

// The windows (0x4270e0); how many are open (0x60e4dc)
class BBWindow
{
public:
	static int sOpenCount;
};

// GameApp's globals (0x42d330)
void CreateParticleLayer();  // 0x428190
void InitHighScores();       // 0x435aa0
void LoadParticles();        // 0x4346e0
void LoadProfiles();         // 0x45e7f0
void SaveProfiles();         // 0x45e110

// GameWidget's preloads (0x42f3b0): each class's pictures, loaded once and kept
void PreloadNetFly();        // 0x42ad40
void PreloadBananas();       // 0x405730
void PreloadMenuTiles();     // 0x446830
void PreloadWalkThru();      // 0x4756d0
void PreloadSlots();         // 0x46c710
void PreloadFly();           // 0x423980
void PreloadInGameButtons(); // 0x4558f0
void PreloadAnt();           // 0x420650
void PreloadMoth();          // 0x423d10
void PreloadBeetle();        // 0x421750
void PreloadAskWin();        // 0x403f10
void PreloadCentipede();     // 0x422f10
void PreloadWasp();          // 0x420b30
void PreloadCaterpillar();   // 0x422220
void PreloadFlyPaths();      // 0x41aee0
void PreloadMothPaths();     // 0x41c470
void PreloadSkeeter();       // 0x425110
void PreloadBonusPictures(); // 0x406dd0
void InitBonusTable();       // 0x40af90
void PreloadSpinStars();     // 0x41f980
void PreloadRolyPoly();      // 0x424c80
void PreloadMap();           // 0x4463c0
void PreloadFireBall();      // 0x42b3f0
void PreloadPipes();         // 0x459360
void PreloadRope();          // 0x464250
void InitLevelTable();       // 0x43f3e0
void PreloadCockroach();     // 0x4242e0

} // namespace Sexy
