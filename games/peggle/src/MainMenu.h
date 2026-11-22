// MainMenu.h: the main menu (the original's MainMenu.cpp). The mascot's picture with the four game buttons as parts of the
// artwork lit when the mouse is over them, four brown and purple buttons along the bottom, the mascot's speech bubble that says
// what the button under the mouse is for, and the trophy that shows what the player has earned.

#ifndef __MAINMENU_H__
#define __MAINMENU_H__

#include "ButtonListener.h"
#include "Widget.h"

#include <string>

namespace Sexy
{

class Graphics;
class HyperlinkWidget;
class ThunderButton;
class ThunderballApp;

// the ids of the menu's buttons (what ButtonDepress and ButtonMouseEnter get)
enum MainMenuButton
{
	MAIN_MENU_BUTTON_ADVENTURE = 0,
	MAIN_MENU_BUTTON_SOLO = 1,		   // the second game mode (the game calls it Quick Play)
	MAIN_MENU_BUTTON_DUEL = 2,
	MAIN_MENU_BUTTON_CHALLENGE = 3,
	MAIN_MENU_BUTTON_REPLAYS = 4,
	MAIN_MENU_BUTTON_OPTIONS = 5,
	MAIN_MENU_BUTTON_DEMO = 6,
	MAIN_MENU_BUTTON_QUIT = 7,
	MAIN_MENU_BUTTON_NOT_YOU = 8		   // the link "If this is not you, click here." that opens the list of players
};

// what the mascot's bubble says: greeting the player, one of the buttons, or (the last three) the trophy
enum MainMenuBubble
{
	BUBBLE_GREETING = 0,
	BUBBLE_ADVENTURE = 1,
	BUBBLE_SOLO = 2,
	BUBBLE_DUEL = 3,
	BUBBLE_CHALLENGE = 4,
	BUBBLE_DEMO = 5,
	BUBBLE_REPLAYS = 6,
	BUBBLE_OPTIONS = 7,
	BUBBLE_QUIT = 8,		  // also makes the mascot cry
	BUBBLE_TROPHY_ADVENTURE = 9,	// the trophy of the rank 1, 2 and 3 (BUBBLE_TROPHY_ADVENTURE - 1 + the rank)
	BUBBLE_TROPHY_CHALLENGES = 10,
	BUBBLE_TROPHY_LEVELS = 11
};

class MainMenu : public Widget, public ButtonListener
{
public:
	explicit MainMenu(ThunderballApp* theApp); // 0x4a79e0
	~MainMenu() override;

	void Resize(int theX, int theY, int theWidth, int theHeight) override; // 0x494a00
	void MouseMove(int x, int y) override;								   // 0x494aa0
	void Update() override;												   // 0x4af680
	void Draw(Graphics* g) override;									   // 0x4a8c00

	void ButtonMouseEnter(int theId) override;	// 0x494ad0: the hidden sequence of buttons opens the story
	void ButtonDepress(int theId) override;		// 0x4afd20

	// Reads what the profile in use has done: where the adventure goes on, the rank of the trophy (and starts its animation
	// when it went up).
	void SyncPlayerInfo(); // 0x499c20

	void PlaceButtons();												// the place of every button (set again in each update)
	void SetCurBubble(int theBubble, int theDelay);						// 0x494a40: changes the bubble after the mouse has stayed
	void StartTrophyAnimation();										// 0x494b10
	void DrawSpeechBubble(Graphics* g, int theBubble, int theAlpha, bool theDrawBubble); // 0x4a8470
	void DrawTrophyEffect(Graphics* g);									// 0x494b50
	void DoNotYet(const std::string& theText);							// 0x4a90a0: the dialog of a mode that is locked
	void StartGame();													// 0x4a9190: from the mode and the profile, the screen that follows

	using Widget::MouseMove;

	ThunderballApp* mApp;				// +0x8c
	HyperlinkWidget* mNotYouLink;		// +0x90
	ThunderButton* mAdventureButton;	// +0x94
	ThunderButton* mSoloButton;			// +0x98
	ThunderButton* mDuelButton;			// +0x9c
	ThunderButton* mChallengeButton;	// +0xa0
	ThunderButton* mDemoButton;			// +0xa4
	ThunderButton* mReplaysButton;		// +0xa8
	ThunderButton* mOptionsButton;		// +0xac
	ThunderButton* mQuitButton;			// +0xb0
	// (+0xb4 is the "Buy" button of the trial, which this port does not have)
	int mStartStage;					// +0xb8: where the adventure goes on (the profile's, never below 0)
	int mStartLevel;					// +0xbc
	bool mRandomLevel;					// +0xc0: the second or third button was pressed with the right mouse button
	bool mMouseMoved;					// +0xc1: the mouse has moved since the menu came up (the bubble waits for it)
	int mMouseIgnoreCountdown;			// +0xc4: updates left in which a mouse move does not count (the menu has just come in)
	int mTrophyRank;					// +0xc8: 0 for no trophy, up to 3
	bool mMascotCrying;					// +0xcc: the bubble that was drawn last is the one of the Quit button
	int mEasterEggProgress;				// +0xd0: how many buttons of the hidden sequence the mouse has passed over
	int mCurBubble;						// +0xd4: a MainMenuBubble
	int mBubbleFade;					// +0xd8: counts down from BUBBLE_FADE_UPDATES while the bubble changes
	int mPrevBubble;					// +0xdc: what the bubble said before it changed
	int mBubbleHoverCount;				// +0xe0: updates the mouse has been over what the next bubble is for
	int mFrame;							// +0xe4: counts the updates (it turns the sun's glow)
	int mTrophyAnimTime;				// +0xe8: counts up to mTrophyAnimLength in the trophy's rise
	int mTrophyAnimLength;				// +0xec: 0 when there is no animation
	int mQuitDarken;					// +0xf0: grows while the mascot cries (the picture goes darker), shrinks when it stops
};

} // namespace Sexy

#endif
