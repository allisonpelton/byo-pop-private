// Sexy::MainMenu (Volume 2: constructor 0x466420; ButtonListener at +0x88, DialogListener at +0x8c, PopAnimListener
// at +0x90): the library hall where Lex waits by his book. Each of
// the menu's things (the adventure door, the arena, the hall of fame, the mini-games, the tome, the options and
// quit signs) lights up under the mouse, with a scroll that unrolls below it saying what it is, and Lex says a word
// about it. The adventure, the arena and the mini-games are animations (images/pam/mm_*.pam, with a label for each
// state); the others are pictures of two cels, the second lit. The player's name is on the plaque; a link under it
// changes the player: the players' dialog (JeffLib::UserSelectionDialog), where one picks, adds or deletes a
// player. (The addresses below are Volume 1's, from which this class was made, unless they say Volume 2.)
#pragma once

#include <map>
#include <string>
#include <vector>

#include "ButtonListener.h"
#include "DialogListener.h"
#include "PopAnim.h"
#include "Rect.h"
#include "Widget.h"

namespace JeffLib {
class UserSelectionDialog;
}

namespace Sexy {

class GameApp;
class Graphics;
class HyperlinkWidget;
class Image;
class MemoryImage;

class MainMenu : public Widget, public ButtonListener, public DialogListener, public PopAnimListener {
public:
	enum Item {
		ITEM_OPTIONS = 0,
		ITEM_QUIT = 1,
		ITEM_ADVENTURE = 2,
		ITEM_ARENA = 3,
		ITEM_HALL_OF_FAME = 4,
		ITEM_MINI_GAMES = 5,
		ITEM_TOME = 6,
		NUM_ITEMS = 7,  // (Volume 1 had a film reel, the 8th)
		REACTION_WELCOME = 7,  // Lex's welcome, when the curtain has opened over the menu
	};

	// The scroll that unrolls under a thing (0x34 bytes, constructor V1:0x478060): IMAGE_MM_SCROLL at its top and
	// its bottom, and between them the parchment with the thing's name and what it is, shown as far as the
	// scroll has unrolled
	class Tooltip {
	public:
		enum State {
			STATE_HIDDEN = -2,
			STATE_CLOSING = -1,
			STATE_OPENING = 1,
			STATE_OPEN = 2,
		};

		int mY;               // +0x00, the top roll's y
		int mBaseY;           // +0x04, where the top roll was put: it drifts 10 pixels down while open
		int mBottomY;         // +0x08, the bottom roll's y when unrolled
		int mX;               // +0x0c
		int mTextX;           // +0x10, the parchment's x in the scroll
		int mHeight;          // +0x14, how much of the parchment shows
		int mFullHeight;      // +0x18
		int mState;           // +0x1c
		int mClosedBottomY;   // +0x20, the bottom roll's y when rolled up
		int mBottomRollY;     // +0x24
		int mTitleY;          // +0x28, the title's offset on the parchment
		bool mShowText;       // +0x2c
		MemoryImage* mImage;  // +0x30, the parchment with its title and text

		Tooltip(Image* theTitle, const std::string& theText, int theTitleY);
		~Tooltip();

		// 0x466280: under the mouse, kept on the screen
		void SetPosition(int theX, int theY);
		void Update();  // 0x466310
		void Draw(Graphics* g);  // V1:0x478410
	};

	// A thing of the menu (Volume 2: 0x80 bytes). The adventure, the arena and the mini-games are animations
	// (mPopAnim, the mouse is over them within mPopRect); the others pictures (mImage, two cels: the second is lit),
	// the quit sign and the hall of fame's within the opaque pixels of a mask.
	struct MenuItem {
		PopAnim* mPopAnim = nullptr;    // +0x00, owned
		Rect mPopRect;                  // +0x04, where the mouse is over the animation
		Image* mImage = nullptr;        // +0x18, the picture (its size sets the sizes below, also of the animations')
		Image* mMask = nullptr;         // +0x1c, the picture's mask: a pixel counts when it is 0xffffffff
		Rect mRect;                     // +0x20, where the picture goes
		Rect mSrcRect;                  // +0x30, all of one cel
		Rect mHitRect;                  // +0x40, where the mouse is over it (within the mask)
		Tooltip* mTooltip = nullptr;    // +0x50
		int mReserved54 = 0;            // +0x54
		int mReserved58 = 0;            // +0x58
		int mFade = 0;                  // +0x5c, a glow nothing draws in Volume 2
		int mFadeDelta = 0;             // +0x60
		int mFadeTarget = 0;            // +0x64
		int mPulse = 0;                 // +0x68, 1 or -1 while it glows on its own (the adventure, when it can go on)
		bool mPressed = false;          // +0x6c
		bool mDisabled = false;         // +0x6d
		bool mHovered = false;          // +0x6e
		bool mVisible = true;           // +0x6f
		bool mHasMask = false;          // +0x70
		const unsigned long* mBits = nullptr;  // +0x74, the mask's pixels, got again at every update
		int mWidth = 0;                 // +0x78, the mask's size
		int mHeight = 0;                // +0x7c

		// 0x4663e0: whether a point is within the mask's white (true for a thing with no mask)
		bool HitTest(int theX, int theY) const;
		// the mask's pixels and size again (at every update)
		void UpdateMask();
	};

	// What Lex says about a thing: a sample and a label of his animation (the map's values at +0xe8)
	struct Reaction {
		int mSound;
		std::string mLabel;
	};

	GameApp* mApp;                            // +0x94
	HyperlinkWidget* mChangeUserLink;         // +0x98
	std::string mTypedKeys;                   // +0xb8, the letters typed, for the secret words
	std::vector<MenuItem*> mItems;            // +0xd8
	JeffLib::UserSelectionDialog* mUserDialog = nullptr;  // +0xe4: the players' dialog while it is up
	std::map<int, Reaction> mReactions;       // +0xe8, by item (and REACTION_WELCOME)
	PopAnim* mLex;                            // +0xf4, images/pam/mmlex.pam
	int mHoverUpdate[NUM_ITEMS];              // +0xf8 (Volume 2: 7 of them), the update a thing was last hovered at (nothing reads it)
	int mLogoX;                               // +0x118
	int mLogoY;                               // +0x11c, slides up from where the title screen left it
	int mLogoTimer;                           // +0x120, counts down once the logo is up (nothing reads it)
	int mLogoTimerDelta;                      // +0x124
	bool mFromTitle;                          // +0x128, shown from the title screen: the logo is drawn over all
	bool mAdventureAvailable;                 // +0x129, there are books left to play
	bool mArenaUnlocked;                      // +0x12a
	bool mMiniGamesUnlocked;                  // +0x12b
	bool mTomeUnlocked;                       // +0x12c, the profile's UnlockedBestiary
	bool mReallyQuit;                         // +0x12d, asking whether to quit (Lex cries)
	bool mExpansionAvailable;                 // +0x12e, books beyond the first three (an expansion's; nothing draws it now)

	MainMenu(GameApp* theApp, bool theFromTitle);
	~MainMenu() override;  // 0x469710

	// V1:0x479e80: the unlocked things and the scrolls again, from the profile
	void RefreshUnlocks();
	// port: test hook (the test command goto): the click on an item of the menu, at a point where the click path takes
	// it (MouseUp is the click path itself). False when the item cannot be clicked now
	bool ActivateItem(int theItem);
	// V1:0x479ce0: Lex's word about a thing (or his welcome, for -1 or REACTION_WELCOME), unless he is busy or a
	// sample of his is still playing
	void PlayReaction(int theItem);
	// V1:0x479c80: rolls the scrolls up
	void CloseTooltips();
	// V1:0x47d670: whether to quit, Lex in tears behind the question (the app's DIALOG_REALLY_QUIT: its yes quits)
	void AskReallyQuit();
	// V1:0x47c290: the arena's door, once it is open: the scripts' timed mode begins, and a new run goes to its book
	// (ResetTimedMode first) unless the profile has one in progress (TM_InGame), which the player is asked whether
	// to go on with (dialog DIALOG_CONTINUE_GAME, answered in GameApp::ButtonDepress). The things of the menu are
	// off meanwhile.
	void StartArena();
	// V1:0x47c170: the answer to that question. theContinue: the saved run (the book under the menu, then the curtain;
	// the new game's curtain when the run was at its checkpoint, which that one takes to the treasure screen);
	// else a new run (ResetTimedMode, then the new game's curtain)
	void ContinueArena(bool theContinue);

	// V1:0x47c770: the players' dialog over the menu (MaxProfiles of the properties, 10, at most), filled
	// by FillUserList
	void ShowUserDialog();
	// V1:0x47ca20: the dialog's players, one for each users/*.bwa (the first MaxProfiles of them), and the player
	// now playing first and chosen
	void FillUserList();
	// V1:0x47d200: whether theName is not any player's yet (capitals and small letters count the same); no for a
	// blank one
	bool IsUserNameFree(const std::string& theName);
	// V1:0x47d390 (called by GameApp::NewUserDialogDone with the name asked from the dialog's NEW): false and a message
	// when the name is taken, so the name dialog stays; else the dialog's list is made again, the players' dialog
	// goes and a player who has not played begins the intro. A blank name (CANCEL) is nothing.
	bool TakeNewUser(const std::string& theName);
	// V1:0x47d940 (the app's LostFocus calls it): the menu is redrawn, and Lex goes idle unless he is doing one of the
	// secret words' animations (his current label is one of the words, whatever the letters' case)
	void AppLostFocus();

	// Widget
	void Update() override;                       // 0x46c950
	void UpdateF(float theFrac) override;         // 0x46c8a0
	void Draw(Graphics* g) override;              // 0x46a260
	void DrawOverlay(Graphics* g) override;       // 0x46a570
	void KeyChar(SexyChar theChar) override;      // V1:0x47da70
	void MouseMove(int x, int y) override;        // V1:0x47afe0
	void MouseDown(int x, int y, int theClickCount) override;  // 0x46b240
	void MouseUp(int x, int y, int theClickCount) override;    // 0x46b3b0

	// ButtonListener: the link to change the player (0x46cae0)
	void ButtonDepress(int theId) override;

	// DialogListener: the players' dialog (OK takes the chosen player, CANCEL none, NEW asks a name, DELETE asks
	// whether to delete) and the confirmation of a delete (0x46d130)
	void DialogButtonDepress(int theDialogId, int theButtonId) override;

	// PopAnimListener: Lex goes back to idling when a label is done (V1:0x47a9e0), and plays the samples his
	// animation asks for (0x46de50)
	void PopAnimStopped(int theId) override;
	void PopAnimCommand(int theId, const std::string& theCommand, const std::string& theParam) override;

private:
	void MakeTooltips(const std::string& theTomeText);
	// Volume 2 (the end of the constructor, and 0x468f23 in RefreshUnlocks): the three animations, the adventure's
	// the replay's once the adventure is completed, with the label each one starts on
	void LoadPopAnims(bool theFromConstructor);
	// the label of the adventure's animation for the book now played ("book4", "book5" or "book6" and theSuffix,
	// " idle" or " mouseover")
	void PlayAdventureLabel(const char* theSuffix);
	// the click on the adventure (in MouseUp); theDisableAll: all the things go dark
	void ClickAdventure(bool& theDisableAll);
	// V1:0x47d510: a player who has not played yet starts the intro (and the menu is no longer from the title)
	bool StartIntroIfNew();
};

}  // namespace Sexy
