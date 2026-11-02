// Sexy::ChallengeMenu (0x9e8 bytes; 0x6dd2cd-0x6e665e, menu +0x4a8) and Sexy::ChallengeButton (0x658 bytes; vtable
// 0x7e011c): the planets of the challenge categories, picked from the main menu's play menu.
//
// The original draws its planets in a small 3D world: a camera on curves (ChallengeMenu::Update 0x6e3f41) and a planet
// at each place on a circle (AddedToManager 0x6dfc49). With Direct3D off (Is3DAccelerated() false, as here) a planet is
// a picture made for each frame (0x6dde98): the flat planet image, IMAGE_PLANET, says for each pixel where to look in
// the planet's cloud-and-surface layer and how bright to make it. The sun, the moon and the texts are drawn over the
// planets (0x6e132d); the sphere meshes (0x6e4fad) and the rest of the 3D drawing are left out (NOTES.md).
#pragma once

#include "ButtonListener.h"
#include "ButtonWidget.h"
#include "CurvedVal.h"
#include "EffectsManager.h"
#include "Math3D.h"
#include "TwistButton.h"
#include "TwistDialogListener.h"
#include "TwistWidget.h"

#include <vector>

namespace Sexy {

class AwardTelly;
class TextImage;

class Menu;
class MemoryImage;
class OrbitText;
class WinTwistApp;

class ChallengeMenu;

// One category's planet (0x658 bytes, ctor 0x6e5337); its id is the category's number. It is a TwistButton whose
// picture is IMAGE_CHALLENGE_BUTTON (the size of its cel is its size: the hit test is a circle about the middle).
class ChallengeButton : public TwistButton {
public:
	ChallengeMenu* mMenu = nullptr;
	// +0x168 to +0x4e0: the curves of the planet's moon meter (the stage bar); Update (0x6e3b0e) steps them and
	// UpdateProgress (0x6e0ddb) sets them
	int mStage = 0;                 // +0x164: the challenge the planet starts (from 1)
	CurvedVal mBarFill;             // +0x168: the bar's fill grows by it when a stage is done
	CurvedVal mUnknown1D8;          // +0x1d8: the lock and the new number glow
	CurvedVal mUnknown248;          // +0x248: the old number's glow and the lock's wipe
	CurvedVal mUnknown2B8;          // +0x2b8: the additive glow of the bar
	CurvedVal mDim;                 // +0x328: 1 darkens the planet and its text (the planet is closed or opening)
	CurvedVal mUnknown398;          // +0x398: stepped; nothing here reads it
	CurvedVal mTextFade;            // +0x408: the bar's and the name's fade
	CurvedVal mEclipseFade;         // +0x478: the eclipse title's fade
	float mUnknown4E8 = 1.0f;       // +0x4e8: rises to 1 by 0.005 an update; below 1 it throws sparks
	float mGlow = 0;                // +0x4ec: the name's glow while the mouse is over the planet (to 0.5)
	std::vector<Image*> mLayers;    // +0x4f0: the category's layer pictures (planet_x, cloud_x); the planet is the first
	Image* mNameImage = nullptr;    // +0x500: IMAGE_CHALLENGEUI_TITLES_<category>
	float mNameScale = 1.0f;        // +0x504: 1, or 0.75 on a screen narrower than 16 by 9
	Color mCategoryColor;           // +0x508: the category's color (challenges.xml); the bar and the glow use it
	int mCategory = 0;              // +0x518: the category's number (also the id)
	bool mOpen = false;             // +0x51c: the category is open to the player
	bool mUnknown51D = false;       // +0x51d: when set, Update does nothing (nothing sets it)
	float mUnknown520 = 0;          // +0x520: AddedToManager's start angle round the circle
	float mUnknown524 = 0;          // +0x524: this planet's angle
	float mUnknown528 = 0;          // +0x528: the menu's +0x188
	int mUnknown52C = 0;            // +0x52c: counts the pulses of mUnknown530 still to come
	float mUnknown530 = 0;          // +0x530: runs down by 0.04 an update, and starts again at 1 while mUnknown52C is over 0
	CurvedVal mIdle;                // +0x538: set at construction; Update steps it
	// The planet's place in the menu's 3D world (AddedToManager 0x6dfc49 sets it; the camera flies to it when the
	// planet is picked)
	float mWorldPos[3] = {0, 0, 0};   // +0x5a8
	// Where Draw put the planet: the screen's x and y and its depth, which says how big it is
	float mScreenPos[3] = {0, 0, 0};  // +0x5b4
	float mFade = 0;                  // +0x5c0: the bar and the name come in (0 to 1) while mUnknown5C4 is set
	bool mUnknown5C4 = false;         // +0x5c4: set by the menu's Update (0x6dd2bd) from whether the planet is open
	bool mUnknown5C5 = false;         // +0x5c5: SetState 3 sets it on every planet but the picked one
	bool mEclipse = false;            // +0x5c6: every challenge of the category is done; its night challenge is next
	int mProgress = 0;                // +0x5c8: the category's best (Profile +0x204)
	bool mAdditive = false;           // +0x5cc: the category's "additive" (the 3D draw blends its clouds by it)
	bool mUnknown5CD = false;         // +0x5cd: Update runs UpdateProgress when the curve at +0x1d8 ends while it is set
	float mEclipseFlares[32] = {};       // +0x5d0: the eclipse title's flares (Update moves them; only the 3D draw shows them)
	OrbitText* mTip = nullptr;        // +0x650: the planet's tip (MakeTip), a night challenge's new record
	float mTilt = 0;                  // +0x160: the planet's turn about x (only the 3D meshes show it)

	ChallengeButton(int theCategory, ChallengeMenu* theMenu);  // 0x6e5337
	~ChallengeButton() override;                               // 0x6e0864: the tip goes with it
	// 0x6e0aa5: the tip, "YOUR NEW ECLIPSE RECORD IS" mProgress, shown twice (the board's end of a night challenge)
	void MakeTip();
	void Update() override;                                    // 0x6e3b0e
	void Draw(Graphics* g) override;                           // 0x6dde98 (the software planet)
	void MouseDown(int x, int y, int theClickCount) override;  // 0x6dd224: not the other buttons
	using TwistButton::MouseDown;
	bool IsPointVisible(int x, int y) override;                // 0x6dd237: a circle about the middle
	void PlayMouseOverSound() override;                        // 0x6dd1e9
};

class ChallengeMenu : public TwistWidget, public ButtonListener, public TwistDialogListener {
public:
	// The ids of the two buttons below the planets (a planet's is its category's number)
	enum { BUTTON_PLAY_MENU = 100, BUTTON_MAIN_MENU = 101 };

	std::vector<ChallengeButton*> mButtons;   // +0x164: one for each category
	int mHover = -1;                          // +0x160: the planet the mouse is over
	TwistButton* mPlayMenuButton = nullptr;   // +0x174
	TwistButton* mMainMenuButton = nullptr;   // +0x178
	int mCategory = 0;                        // +0x17c: the planet picked
	int mStageSel = 0;                        // +0x180: and its challenge
	int mStarted = 0;                         // +0x184: set when a challenge starts
	float mUnknown15C = 0;                    // +0x15c: runs down by 0.03 an update
	float mUnknown188 = 0;                    // +0x188: 0.4 of the height; AddedToManager gives it to the planets (+0x528)
	// The hovered planet's pictures (ButtonMouseEnter 0x6e665e makes them, DrawSunAndTexts 0x6e132d draws them)
	MemoryImage* mTitleImage = nullptr;       // +0x18c: the category's name, in its colour (and the eclipse title)
	TextImage* mDescImage = nullptr;          // +0x190: the challenge's description
	TextImage* mDescGlowImage = nullptr;      // +0x194: the same with the font's second layer, flashed by mUnknown15C
	float mHoverFade = 1.0f;                  // +0x198: the hovered planet's text
	SexyString mBestText;                     // +0x19c: "BEST: %d" under an eclipse planet's name, else empty
	EffectsManager* mFXManager1B8 = nullptr;  // +0x1b8: a widget of the menu
	EffectsManager* mFXManager1BC = nullptr;  // +0x1bc: the sun's glow (moved down by the open curve, drawn by Draw)
	int mState = 0;                           // +0x1c4: 0 away, 1 shown by the play menu, 2 leaving for a menu, 3 leaving
	                                          // for a challenge, 4 shown at once (after a game), 5 only shown
	// The curves SetState sets for each state (the strings are the exe's, by address) and Update steps. The ones that
	// turn the camera are in degrees; the offsets are in the menu's 3D world.
	CurvedVal mOpen;                          // +0x1c8: how far the menu has come in (or gone out); the others are linked to it
	CurvedVal mButtonSlide;                   // +0x238: the two buttons rise from below the screen
	float mStartDelay = 0;                    // +0x2a8: Update waits until it has run down by 0.01 an update
	CurvedVal mCameraRotX;                    // +0x2b0: the camera's turn about x
	CurvedVal mCameraRotY;                    // +0x320: about y
	CurvedVal mCameraRotZ;                    // +0x390: about z
	CurvedVal mCameraDistance;                // +0x400: along the camera's third axis, from 16.15 away
	CurvedVal mCameraRight;                   // +0x470: along its first axis
	CurvedVal mCameraUp;                      // +0x4e0: along its second axis
	CurvedVal mCameraShiftX;                  // +0x550: added to the camera's x
	CurvedVal mCameraShiftY;                  // +0x5c0: added to its y
	float mCameraPos[3] = {0, -12.0f, 0};     // +0x630: where the camera starts
	CurvedVal mUnknown640;                    // +0x640
	CurvedVal mUnknown6B0;                    // +0x6b0
	CurvedVal mUnknown720;                    // +0x720
	CurvedVal mCameraTurn;                    // +0x790: how far the hyperspace's camera turns to mCameraTarget
	Mat3 mCameraTarget;                       // +0x800: the camera's turn when a planet is picked (SetState 1)
	float mZoomPos[3] = {0, 0, 0};            // +0x824: the picked planet's place (SetState 2 to 4)
	CurvedVal mZoom;                          // +0x830: the camera flies from its start to the picked planet by it
	CurvedVal mUnknown8A0;                    // +0x8a0
	CurvedVal mUnknown928;                    // +0x928: the glow colours' blend (only 0x6e32c9 sets it, which nothing calls)
	// The camera stack: the camera places of properties/camerastack.txt (0x6e5a8d), two lines of three numbers
	// each. The mouse drag changes the one at mCameraStackIndex; nothing else reads them.
	struct CameraStackEntry {
		float mPos[3];  // the first line
		float mRot[3];  // the second line (the drag turns these)
	};
	std::vector<CameraStackEntry> mCameraStack;  // +0x998
	int mCameraStackIndex = 0;                // +0x9b8
	bool mDragging = false;                   // +0x9bd: the left button is held and the drag has begun
	int mDragStartX = 0;                      // +0x9c0
	int mDragStartY = 0;                      // +0x9c4
	int mLastMouseX = 0;                      // +0x9c8
	int mLastMouseY = 0;                      // +0x9cc
	float mUnknown9D0 = 0;                    // +0x9d0: counts up while no planet is hovered
	float mUnknown9D4 = 0;                    // +0x9d4: then rises to 1
	bool mUnknown9BC = false;                 // +0x9bc
	// +0x9dc: the board's end-of-challenge message (0x6b330d makes it); Update lifts its rank bar's timer while its update
	// count is 250 and deletes it once clicked
	AwardTelly* mAwardTelly = nullptr;
	// +0x9e0: the 512 by 512 picture the software planets are made in (0x6e5c3e; the original never frees it)
	MemoryImage* mPlanetImage = nullptr;

	ChallengeMenu();                          // 0x6e5c3e
	~ChallengeMenu() override;                // 0x6e3f22

	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x6dfc49
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x6dd2cd
	void Update() override;                                             // 0x6e3f41
	void Draw(Graphics* g) override;                                    // 0x6dd322
	void DrawAll(ModalFlags* theFlags, Graphics* g) override;          // 0x6e4a25
	void MouseWheel(int theDelta) override;                             // 0x6dfe2a (the camera's zoom: left out)

	void DialogButtonDepress(int theDialogId, int theButtonId) override;  // 0x6e58f7
	void ButtonDepress(int theId) override;                               // 0x6e54c3
	void ButtonMouseEnter(int theId) override;                            // 0x6e665e
	void ButtonMouseLeave(int theId) override;                            // 0x6dd418

	void Show(int theMode, bool theNow);       // 0x6e30d5
	void Close(int theState);                  // 0x6e3142
	void SetState(int theState, bool theNow);  // 0x6e2318
	// 0x6e0ddb: each planet's open/closed state and stage from the player's progress
	int UpdateProgress(bool theAnimate, bool theClearTips);
	void Start(bool theNewGame);               // 0x6e4ab6: the picked challenge
	// 0x6dd42e: the open curve is moving (it has a rate and has not reached its end)
	bool IsOpenCurveRunning() const;
	// 0x6e5a8d: the camera stack from properties/camerastack.txt
	void LoadCameraStack();
	// 0x6e132d: what DrawAll draws over the planets: the sun, the moon and its ring, the dancing glows, the hovered
	// planet's texts, the trial's picture and the menu's title
	void DrawSunAndTexts(Graphics* g);
};

// The globals the menu's Update (0x6e3f41) and SetState (0x6e2318) keep for the drawing (the planets 0x6dde98, the sun
// 0x6e132d)
extern CurvedVal gSunCurve;      // 0x85c558: the sun's glow (SetState links it to mOpen)
extern Transform3D gMenuCamera;  // 0x85c51c: the camera: its place and the turn of its three axes
extern float gUnknown85C508;     // 0x85c508: 1 - mUnknown6B0, but not below 0.25
extern Color gUnknown85C50C;     // 0x85c50c: mUnknown928 from (128, 32, 128) to (0, 0, 128)
extern Color gSunGlowColor;      // 0x85c5c8: mUnknown928 from white to (255, 128, 255)
extern float gAspect;            // 0x84ebf8: the screen's width over its height
extern float gAspect45;          // 0x84ebfc: 45 times that

}  // namespace Sexy
