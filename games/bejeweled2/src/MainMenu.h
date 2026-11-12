#ifndef __MAINMENU_H__
#define __MAINMENU_H__

// MainMenu.h: the game selector (Sexy::MainMenu, 0x5c80e1-0x5c9dcd; the object is 0x1a0 bytes): four game modes on big
// round buttons, the options, high scores and quit buttons along the bottom, the secret button that swaps the four modes
// for the four unlocked ones, and the link that changes the user. The menu slides in from the top and out again. The
// original's "more games" button (id 12) is not ported.

#include "Widget.h"
#include "ButtonListener.h"
#include "CurvedVal.h"

namespace Sexy
{

class WinDMApp;
class MemoryImage;
class PodButton;
class HyperlinkWidget;
class SoundInstance;
class Font;

class MainMenu : public Widget, public ButtonListener
{
public:
	enum State
	{
		STATE_SLIDING_IN = 0,   // the menu comes down (Show)
		STATE_READY = 1,        // the buttons answer
		STATE_SLIDING_OUT = 2,  // Hide
		STATE_HIDDEN = 3,
		STATE_STARTING = 4,     // StartGame: a game is being set up
		STATE_NO_BUTTONS = 5    // the buttons do not answer (a game is being set up, or the menu closes)
	};

public:
	WinDMApp*		mApp;               // +0x8c (the ButtonListener subobject is at +0x88)
	bool			mBackdropClear;     // +0x90 the backdrop image is not drawn (the title screen's picture shows through)
	MemoryImage*	mBackdropImage;     // +0x94 the picture behind the menu (the one the board used)
	MemoryImage*	mTextImage;         // +0x98 the user's name in the glow font, 0xf9 by 0x40
	int				mStartTimer;        // +0x9c updates until StartGame (when the display is not accelerated)
	bool			mStarting;          // +0xa0 a game is starting: the buttons do nothing
	float			mModeGlow[4];       // +0xa4 how lit each of the four mode buttons is, 0 to 1 (the mouse is over it)
	float			mModeGlow2[4];      // +0xb4 the glow of a button that stays lit
	PodButton*		mActionButton;      // +0xc4 id 1 (Hyper with the secret button on)
	PodButton*		mClassicButton;     // +0xc8 id 0 (Twilight)
	PodButton*		mPuzzleButton;      // +0xcc id 2 (Cognito)
	PodButton*		mEndlessButton;     // +0xd0 id 3 (Finity)
	PodButton*		mSecretButton;      // +0xd4 id 4: swaps the mode buttons for the unlocked modes
	bool			mSecretShown;       // +0xd8 the secret button shows (a mode is unlocked)
	PodButton*		mOptionsButton;     // +0xdc id 10
	PodButton*		mScoresButton;      // +0xe0 id 11
	PodButton*		mQuitButton;        // +0xe8 id 13 (the "more games" button, id 12 at +0xe4, is not ported)
	HyperlinkWidget* mUserLink;         // +0xec id 0x14 "click to change user"
	int				mSlideDelay;        // +0xf0 updates before the menu slides
	CurvedVal		mSlide;             // +0x128 the vertical offset of the whole menu: 700 (above) to 0
	int				mSelectedMode;      // +0x158 0 to 3, 4 to 7 with the secret button, 8 the hidden mode
	int				mState;             // +0x15c State
	int				mHoverCircle;       // +0x160 the button the mouse was over last (0 to 3), -1 for none
	int				mHoverCircleCount;  // +0x164 consecutive moves from a button to the next one: 32 start the hidden mode
	int				mShutdownTimer;     // +0x168 updates until the app shuts down (after the quit dialog's yes)
	float			mLightFade;         // +0x16c the three light pairs fade in with the mouse over a mode button
	float			mLightGlow[3];      // +0x170 how bright each pair of lights is
	float			mLightPhase;        // +0x17c the lights run one after the other as this counts up
	float			mHoverFade;         // +0x180 the volume of the lights' sound
	float			mCenterGlow;        // +0x184 the glow behind the middle of the gadgets
	SoundInstance*	mSound;             // +0x188 the lights' sound
	SoundInstance*	mPrevSound;         // +0x18c the one before
	int				mUserDialogTimer;   // +0x190 updates until the "create user" dialog opens
	int				mWelcomeTimer;      // +0x194 updates until the welcome message opens
	Font*			mPrevFont;          // +0x198 (never set)
	Font*			mSmallFont;         // +0x19c a smaller copy of the name's font, for a long name

public:
	MainMenu(WinDMApp* theApp);                                     // 0x5c9dcd
	virtual ~MainMenu();                                            // 0x5c7ffa

	void			Show();                                         // 0x5c8aa9
	void			Hide();                                         // 0x5c80e1
	void			SetBackground(MemoryImage* theImage);           // 0x5c8128 takes over the picture
	void			SelectMode(int theMode);                        // 0x5c8148
	void			StartGame();                                    // 0x5c81e7
	void			StartShutdownTimer();                           // 0x5c824d
	void			CancelNewGame();                                // 0x5c828a
	void			ResizeComponents();                             // 0x5c8409
	void			RehupGameButtons();                             // 0x5c8800 the pictures of the four mode buttons
	void			DrawMouseover(Graphics* g, int theX, int theY, int theIndex, float theGlow, float theGlow2); // 0x5c8afd
	void			DrawTopText(Graphics* g);                       // 0x5c9572 the welcome text
	void			CreateFontGlowImages();                         // 0x5c9783
	void			SyncProfile();                                  // 0x5c981c

	virtual void	AddedToManager(WidgetManager* theManager) override;     // 0x5c82d0
	virtual void	RemovedFromManager(WidgetManager* theManager) override; // 0x5c8371
	virtual void	ButtonPress(int theId) override;                // 0x5c88fe
	virtual void	ButtonDepress(int theId) override;              // 0x5c8933
	virtual void	Update() override;                              // 0x5c8d7f
	virtual void	Draw(Graphics* g) override;                     // 0x5c98f5
};

}

#endif
