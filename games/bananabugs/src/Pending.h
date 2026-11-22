// port: stand-ins for the parts of the original not translated yet, so the screens that are can run. Each piece moves
// to its own class's files as that class is translated, and this file goes once it is empty. Each stand-in names the
// original's address; one that does less than the original says so.
#pragma once

#include "AskWin.h"
#include "BBWindow.h"
#include "MenuAnim.h"
#include "ObjBase.h"

namespace Sexy
{

class Anim;
class BBFont;
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

// The options window (0x453e30): the stand-in has no controls and reports itself closed at once
class Options : public BBWindow
{
public:
	Options();
};

// The high score window (0x4351b0): the stand-in lists no scores and reports itself closed at once
class HighScore : public BBWindow
{
public:
	HighScore();
};

// The window that asks who is playing (0x4721e0): the stand-in lists no players and reports itself closed at once
class UserSelect : public BBWindow
{
public:
	UserSelect();
};

// The window that asks for a new player's name (0x4504d0): the stand-in takes no name and reports itself closed at once
class NewUser : public BBWindow
{
public:
	NewUser(bool theFlag);
	void Open(ObjBase* theParent); // 0x4505e0
};

// The help screen (0x434700): the stand-in shows nothing and is done at once
class Help : public ObjBase
{
public:
	Help();
	void Start();                  // 0x434770
	bool IsDone() { return true; } // 0x4348b0
};

// The map between levels (0x448e10)
class MetaGame : public ObjBase
{
public:
	bool mWantsMenu;           // +0x27c
	bool mLeaving;             // +0x27d

	static bool sFirstVisit;   // 0x5ff082: set as a meta game starts, cleared by the main menu when it continues one

	MetaGame();
	~MetaGame();
	bool TakeLevelChosen();    // 0x449b90
	static void ClearFirstVisit() { sFirstVisit = false; } // 0x44f440
};

// A player's profile (0x459380), also the object that shows the player's state in a level
class Player : public ObjBase
{
public:
	int mSavedProgress;        // +0x2b4: above 0 once a game is in progress; the main menu then asks about continuing it
	int mMapLevel;             // +0x2bc: the level the map moves to
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

// Whether the level being played takes the mouse (the byte at +0x8ffd of the LevelObj sLevel points to)
bool LevelAcceptsMouse();

// The profiles the main menu reads and changes (0x459380)
const char* GetProfileName(); // 0x45dde0: the current profile's name, or nullptr when no profile is in use
void ResetProfile();         // 0x45e7d0: starts the current profile's game afresh

// GameApp's globals (0x42d330)
void InitHighScores();       // 0x435aa0
void LoadProfiles();         // 0x45e7f0
void SaveProfiles();         // 0x45e110

// GameWidget's preloads (0x42f3b0): each class's pictures, loaded once and kept
void PreloadNetFly();        // 0x42ad40
void PreloadBananas();       // 0x405730
void PreloadWalkThru();      // 0x4756d0
void PreloadSlots();         // 0x46c710
void PreloadFly();           // 0x423980
void PreloadInGameButtons(); // 0x4558f0
void PreloadAnt();           // 0x420650
void PreloadMoth();          // 0x423d10
void PreloadBeetle();        // 0x421750
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
void PreloadFireBall();      // 0x42b3f0
void PreloadPipes();         // 0x459360
void PreloadRope();          // 0x464250
void InitLevelTable();       // 0x43f3e0
void PreloadCockroach();     // 0x4242e0

} // namespace Sexy
