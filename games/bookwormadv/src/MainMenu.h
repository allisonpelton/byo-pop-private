// Sexy::MainMenu (0x130 bytes, constructor 0x4785a0, vtable 0x7aeefc; its ButtonListener part at +0x88,
// DialogListener at +0x8c, PopAnimListener at +0x90): the library hall where Lex waits by his book. Each of the
// menu's things (the adventure door, the arena, the hall of fame, the mini-games, the tome, the film reel, the
// options and quit signs) lights up under the mouse, with a scroll that unrolls below it saying what it is, and
// Lex says a word about it. The player's name is on the plaque; a link under it changes the player: the players'
// dialog (JeffLib::UserSelectionDialog), where one picks, adds or deletes a player.
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
		ITEM_FILM_REEL = 7,  // the cut scenes and songs, once the player has the media player
		NUM_ITEMS = 8,
		REACTION_WELCOME = 8,  // Lex's welcome, when the curtain has opened over the menu
	};

	// The scroll that unrolls under a thing (0x34 bytes, constructor 0x478060): IMAGE_MM_SCROLL at its top and
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

		// 0x478280: under the mouse, kept on the screen
		void SetPosition(int theX, int theY);
		void Update();  // 0x478310
		void Draw(Graphics* g);  // 0x478410
	};

	// A thing of the menu (0x64 bytes): its picture lit up, drawn over the hall where it is
	struct MenuItem {
		Image* mOverImage = nullptr;    // +0x00
		Image* mNormalImage = nullptr;  // +0x04, its name, drawn on it while lit (some have none)
		Rect mRect;                     // +0x08, where the lit picture goes
		Rect mSrcRect;                  // +0x18, all of it
		Rect mHitRect;                  // +0x28, where the mouse is over it (within the picture's opaque pixels)
		Tooltip* mTooltip = nullptr;    // +0x38
		Image* mReelImage = nullptr;    // +0x3c, the film reel's, drawn under it
		Image* mReelDownImage = nullptr;  // +0x40 (nothing draws it)
		int mFade = 0;                  // +0x44, the lit picture's alpha
		int mFadeDelta = 0;             // +0x48
		int mFadeTarget = 0;            // +0x4c
		int mPulse = 0;                 // +0x50, 1 or -1 while it glows on its own (the adventure, when it can go on)
		bool mPressed = false;          // +0x54
		bool mDisabled = false;         // +0x55
		bool mHovered = false;          // +0x56
		bool mVisible = true;           // +0x57
		const unsigned long* mBits = nullptr;  // +0x58, the lit picture's pixels, for the hit test
		int mWidth = 0;                 // +0x5c
		int mHeight = 0;                // +0x60

		// 0x478560: whether a point is on the lit picture's opaque pixels
		bool HitTest(int theX, int theY) const;
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
	int mHoverUpdate[NUM_ITEMS];              // +0xf8, the update a thing was last hovered at (nothing reads it)
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
	bool mExpansionAvailable;                 // +0x12e, books beyond the first three (an expansion's)

	MainMenu(GameApp* theApp, bool theFromTitle);
	~MainMenu() override;  // 0x47a760

	// 0x479e80: the unlocked things and the scrolls again, from the profile
	void RefreshUnlocks();
	// 0x479ce0: Lex's word about a thing (or his welcome, for -1 or REACTION_WELCOME), unless he is busy or a
	// sample of his is still playing
	void PlayReaction(int theItem);
	// 0x479c80: rolls the scrolls up
	void CloseTooltips();
	// 0x47d670: whether to quit, Lex in tears behind the question (the app's DIALOG_REALLY_QUIT: its yes quits)
	void AskReallyQuit();
	// 0x47c290: the arena's door, once it is open: the scripts' timed mode begins, and a new run goes to its book
	// (ResetTimedMode first) unless the profile has one in progress (TM_InGame), which the player is asked whether
	// to go on with (dialog DIALOG_CONTINUE_GAME, answered in GameApp::ButtonDepress). The things of the menu are
	// off meanwhile.
	void StartArena();
	// 0x47c170: the answer to that question. theContinue: the saved run (the book under the menu, then the curtain;
	// the new game's curtain when the run was at its checkpoint, which that one takes to the treasure screen);
	// else a new run (ResetTimedMode, then the new game's curtain)
	void ContinueArena(bool theContinue);

	// 0x47c770: the players' dialog over the menu (MaxProfiles of the properties, 10, at most), filled
	// by FillUserList
	void ShowUserDialog();
	// 0x47ca20: the dialog's players, one for each users/*.bwa (the first MaxProfiles of them), and the player
	// now playing first and chosen
	void FillUserList();
	// 0x47d200: whether theName is not any player's yet (capitals and small letters count the same); no for a
	// blank one
	bool IsUserNameFree(const std::string& theName);
	// 0x47d390 (called by GameApp::NewUserDialogDone with the name asked from the dialog's NEW): false and a message
	// when the name is taken, so the name dialog stays; else the dialog's list is made again, the players' dialog
	// goes and a player who has not played begins the intro. A blank name (CANCEL) is nothing.
	bool TakeNewUser(const std::string& theName);
	// 0x47d940 (the app's LostFocus calls it): the menu is redrawn, and Lex goes idle unless he is doing one of the
	// secret words' animations (his current label is one of the words, whatever the letters' case)
	void AppLostFocus();

	// Widget
	void Update() override;                       // 0x47c5b0
	void UpdateF(float theFrac) override;         // 0x47c560
	void Draw(Graphics* g) override;              // 0x47aa70
	void DrawOverlay(Graphics* g) override;       // 0x47af50
	void KeyChar(SexyChar theChar) override;      // 0x47da70
	void MouseMove(int x, int y) override;        // 0x47afe0
	void MouseDown(int x, int y, int theClickCount) override;  // 0x47b350
	void MouseUp(int x, int y, int theClickCount) override;    // 0x47b470

	// ButtonListener: the link to change the player (0x47c720)
	void ButtonDepress(int theId) override;

	// DialogListener: the players' dialog (OK takes the chosen player, CANCEL none, NEW asks a name, DELETE asks
	// whether to delete) and the confirmation of a delete (0x47cd70)
	void DialogButtonDepress(int theDialogId, int theButtonId) override;

	// PopAnimListener: Lex goes back to idling when a label is done (0x47a9e0), and plays the samples his
	// animation asks for (0x47da10)
	void PopAnimStopped(int theId) override;
	void PopAnimCommand(int theId, const std::string& theCommand, const std::string& theParam) override;

private:
	void MakeTooltips(const std::string& theTomeText);
	// 0x47d510: a player who has not played yet starts the intro (and the menu is no longer from the title)
	bool StartIntroIfNew();
};

}  // namespace Sexy
