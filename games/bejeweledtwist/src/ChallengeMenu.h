// Sexy::ChallengeMenu (0x9e8 bytes; 0x6dd2cd-0x6e665e, menu +0x4a8) and Sexy::ChallengeButton (0x658 bytes; vtable
// 0x7e011c): the planets of the challenge categories, picked from the main menu's play menu.
//
// port: the original draws its planets in a small 3D world (a camera on curves, planets as spheres with two cloud
// layers, the sun, a moon meter by each planet). The port keeps the menu's logic (which categories are open, how far
// the player has got in each, the picks, the saved-game question, the start of the challenge through
// Board::SetChallenge and the hyperspace) and runs the camera's curves and Transform (SetState 0x6e2318, Update
// 0x6e3f41), the sun's glow particles and the two buttons' slide as the original does. The drawing is still
// temporary: the planets are laid out flat on the screen, and the projection that places each planet (0x6dde98),
// the planet pictures (0x6e4fad's meshes, the software planet), the sun (0x6e132d), the moon meter and the eclipse
// are left out and are listed in NOTES.md.
#pragma once

#include "ButtonListener.h"
#include "ButtonWidget.h"
#include "CurvedVal.h"
#include "EffectsManager.h"
#include "Math3D.h"
#include "TwistDialogListener.h"
#include "TwistWidget.h"

#include <vector>

namespace Sexy {

class Menu;
class TwistButton;
class WinTwistApp;

class ChallengeMenu;

// One category's planet; its id is the category's number
class ChallengeButton : public ButtonWidget {
public:
	ChallengeMenu* mMenu = nullptr;
	int mCategory = 0;        // +0x518
	int mStage = 0;           // +0x164: the challenge the planet starts (from 1)
	bool mOpen = false;       // +0x51c: the category is open to the player
	bool mEclipse = false;    // +0x5c6: every challenge of the category is done; its night challenge is next
	int mProgress = 0;        // +0x5c8: the category's best (Profile +0x204)
	// The planet's place in the menu's 3D world (AddedToManager 0x6dfc49 sets it; the camera flies to it when the
	// planet is picked)
	float mWorldPos[3] = {0, 0, 0};  // +0x5a8
	bool mUnknown5C4 = false;        // +0x5c4: set by the menu's Update (0x6dd2bd) from whether the planet is open
	bool mUnknown5C5 = false;        // +0x5c5: SetState 3 sets it on every planet but the picked one

	ChallengeButton(int theCategory, ChallengeMenu* theMenu);  // 0x6e5337
	void Draw(Graphics* g) override;                           // 0x6dde98 (the 2D planet only as a disc)
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
	float mHoverFade = 0;                     // +0x198: the hovered planet's text
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
	CurvedVal mUnknown928;                    // +0x928: the glow colours' blend (the night mode's unlock sets it, 0x6e32c9)
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

	ChallengeMenu();                          // 0x6e5c3e
	~ChallengeMenu() override;                // 0x6e3f22

	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x6dfc49
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x6dd2cd
	void Update() override;                                             // 0x6e3f41
	void Draw(Graphics* g) override;                                    // 0x6dd322
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
	void Layout();                             // port: the flat layout of the planets
	// 0x6dd42e: the open curve is moving (it has a rate and has not reached its end)
	bool IsOpenCurveRunning() const;
	// port: how much of the temporary planets show: the open curve's value while the menu comes in, its opposite
	// while it goes out
	float PlaceholderAmount();
	// 0x6e5a8d: the camera stack from properties/camerastack.txt
	void LoadCameraStack();
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
