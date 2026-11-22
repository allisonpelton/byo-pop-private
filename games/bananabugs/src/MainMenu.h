// MainMenu: the title screen (0x43fc90..0x4449c0 in the original). A jungle scene of swaying vines, plants, drifting
// smoke and light rays, a tree that carries five buttons (play, options, high scores, help, exit) and a sign with the
// player's name on it. The buttons swell for the mouse; a press opens the window or screen it stands for, one at a
// time, and the menu's state says which is open. GameWidget asks GetChoice each frame for what to do next.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class AskWin;
class Help;
class HighScore;
class Map;
class NewUser;
class ObjString;
class Options;
class UserSelect;
struct EmitterSet;

// What the menu chose (MainMenu::mChoice); the screens the menu opens set it too
enum MenuChoice
{
	MENU_CHOICE_NONE = 0,
	MENU_CHOICE_PLAY = 1,        // the map has finished: GameWidget starts the game
	MENU_CHOICE_OPTIONS = 2,
	MENU_CHOICE_HIGH_SCORES = 3,
	MENU_CHOICE_HELP = 4,
	MENU_CHOICE_QUIT = 5,
	MENU_CHOICE_USER_SELECT = 6,
};

// Which window or screen the menu waits on (MainMenu::mMenuState)
enum MenuState
{
	MENU_STATE_IDLE = 0,
	MENU_STATE_CONTINUE_ASK = 1,     // "continue last game?" is open
	MENU_STATE_CONTINUE_CLOSING = 2, // it has been answered and is closing
	MENU_STATE_START_MAP = 3,        // the map opens at the next update
	MENU_STATE_MAP = 4,
	MENU_STATE_OPTIONS = 5,
	MENU_STATE_HIGH_SCORES = 6,
	MENU_STATE_HELP = 7,
	MENU_STATE_NEW_USER = 8,
	MENU_STATE_USER_SELECT = 9,
	MENU_STATE_EXIT_ASK = 10,        // "exit game?" is open
	MENU_STATE_EXIT_NO = 11,         // answered no, closing
	MENU_STATE_EXIT_YES = 12,        // answered yes, closing
	MENU_STATE_LEFT = 13,            // the map is done; the menu waits for the doors
};

const int MENU_BUTTONS = 5;
const int MENU_SMOKES = 3;
const int MENU_RAYS = 3;
const int MENU_VINES_01 = 3;
const int MENU_VINES_02 = 2;
const int MENU_FOGS = 2;
const int MENU_PLANTS_03 = 4;

class MainMenu : public ObjBase
{
public:
	int mChoice;             // +0x27c: MenuChoice
	int mLeaveTrack;         // +0x280: the track GameWidget starts as the doors close (StartLeaving)
	int mFadeTrack;          // +0x284: a track that fades the menu's values in; nothing in the menu starts it
	int mHoldTrack;          // +0x288: a track that holds the menu's values at 0; nothing starts it
	int mStartDelay;         // +0x28c: frames after the doors open before the menu takes the mouse
	bool mReady;             // +0x290: the mouse and the buttons work this frame
	int mMapTimer;           // +0x294: set to 2000 when the map opens; nothing reads it
	Map* mMap;               // +0x298
	AskWin* mAskWin;         // +0x29c: "continue last game?" or "exit game?"
	Options* mOptions;       // +0x2a0
	NewUser* mNewUser;       // +0x2a4
	UserSelect* mUserSelect; // +0x2a8
	HighScore* mHighScore;   // +0x2ac
	Help* mHelp;             // +0x2b0
	int mMenuState;          // +0x2b4: MenuState
	int mButtonsArrived;     // +0x2b8: how many of the buttons have landed (at 5 the sign shows)

	Anim* mHut01Anim;        // +0x2c0
	Anim* mHut02Anim;        // +0x2c4
	Anim* mTreeAnim;         // +0x2c8
	Anim* mButtonsAnim;      // +0x2cc: five cels, one a button
	Anim* mLogoAnim;         // +0x2d0
	Anim* mPlant01Anim;      // +0x2d4
	Anim* mPlant02Anim;      // +0x2d8
	Anim* mPlant03Anim;      // +0x2dc
	Anim* mPlantsAnim;       // +0x2e0
	Anim* mRaysAnim;         // +0x2e4
	Anim* mVines01Anim;      // +0x2e8
	Anim* mVines02Anim;      // +0x2ec
	Anim* mFogAnim;          // +0x2f0
	Anim* mSmokeAnim;        // +0x2f4: three cels, one a puff of the chimney's smoke

	ObjBase* mHut01;         // +0x2f8
	ObjBase* mHut02;         // +0x2fc
	ObjBase* mSmoke[MENU_SMOKES];    // +0x300
	ObjBase* mTree;          // +0x30c: the buttons hang from it, and it shakes as each lands
	ObjBase* mRays[MENU_RAYS];       // +0x310
	ObjBase* mVines01[MENU_VINES_01]; // +0x31c: the third is never set (see BuildVines01Right)
	ObjBase* mVines02[MENU_VINES_02]; // +0x328
	ObjBase* mFog[MENU_FOGS];        // +0x330
	ObjBase* mButtons[MENU_BUTTONS]; // +0x33c: the objects the mouse is over; the id of each is 100 and its place
	ObjBase* mLogo;          // +0x350
	ObjBase* mPlant01;       // +0x354
	ObjBase* mPlant02;       // +0x358
	ObjBase* mPlant03[MENU_PLANTS_03]; // +0x35c
	ObjBase* mPlantsStrip;   // +0x36c
	ObjBase* mButtonGroups[MENU_BUTTONS]; // +0x370: each button's parent, which swells and lands
	ObjBase* mSign;          // +0x384: shows the player's name
	ObjString* mSignText;    // +0x388


	static bool sShowHighScores; // 0x60e648: the next menu opens on the high scores (a new score was entered)

	MainMenu();              // 0x43fc90
	~MainMenu();             // 0x440000
	void Notify(ObjBase* theObj) override;      // 0x440030
	bool Update() override;                     // 0x440240
	bool MouseUp(ObjBase* theObj) override;     // 0x440720

	int GetChoice() { return mChoice; }        // 0x440a50
	void StartLeaving() { StartTrack(mLeaveTrack); } // 0x440a60
	static void ShowHighScoresNext() { sShowHighScores = true; } // 0x4449b0

private:
	void BuildHuts();            // 0x440b70
	void BuildSmoke1();          // 0x440db0
	void BuildSmoke2();          // 0x441180
	void BuildSmoke3();          // 0x441560
	void BuildTree();            // 0x441940
	void BuildPlantsStrip();     // 0x441c80
	void BuildPlant01();         // 0x441e00
	void BuildPlant02();         // 0x441ff0
	void BuildPlant03First();        // 0x4421e0
	void BuildPlant03Second();        // 0x4423d0
	void BuildPlant03Third();        // 0x4425c0
	void BuildPlant03Fourth();        // 0x4427b0
	void BuildFogUpper();        // 0x4429a0
	void BuildFogLower();        // 0x442bf0
	void BuildRayRight();        // 0x442e00
	void BuildRayLeft();         // 0x442fe0
	void BuildRayCentre();       // 0x4431c0
	void BuildVines02Left();     // 0x443390
	void BuildVines02Right();    // 0x443580
	void BuildVines01Left();     // 0x443770
	void BuildVines01Middle();   // 0x443960
	void BuildVines01Right();    // 0x443b60
	void BuildLogo();            // 0x443d60
	void BuildButtons();         // 0x444090
	void BuildSign();            // 0x4446d0
	void BuildHoldTrack();       // 0x440a70
	void ShowSignName();
};

// The two tracks the menu shares with the level (0x405770 builds its own copies): a track that shrinks the object away
// as a screen is left, and one that fades one in. Each returns the track's index.
int BuildLeaveTrack(ObjBase* theObj); // 0x4059c0
int BuildFadeTrack(ObjBase* theObj);  // 0x405c80

} // namespace Sexy
