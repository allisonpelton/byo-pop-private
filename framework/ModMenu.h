#ifndef __MODMENU_H__
#define __MODMENU_H__

// port: the mod menu (docs/MOD_MENU_DESIGN.md 5), the same in every game, native and web. It lists the installed
// mods in load order with their on/off switches, order, badges and messages, and shows one page of settings per
// mod (platform/ModSettings.h). Every change is written at once (platform/ModManager.h, the desired state);
// what still waits for a restart or the next game is said in the footer, and "Restart game" restarts
// (SexyAppBase::RequestRestart).
//
// One look for every game: drawn with rectangles and the built-in font (BuiltinFont.h), so it works before a
// game has loaded any art. It is a single full-screen widget (no child widgets, not a Dialog, not in the dialog
// map) that SexyAppBase::OpenModMenu adds as a base modal above everything, and that pauses the game through
// ModalOpen()/ModalClose() as a modal dialog does. It never draws a random number: its handlers, Update and Draw
// run inside MTAutoDisallowRand.
//
// Keys: Up/Down rows; Left/Right change a value (Shift: 10 steps); Home/End, PgUp/PgDn; Space toggles; Enter opens
// or activates; Alt+Up/Down moves a mod; Delete resets a setting; Tab/Shift+Tab next/previous control; Esc backs
// out (closes on the list page); F2 closes. Enter, Esc, Tab, Backspace and Space act on the character they type,
// the last event of their key press, so a key that closes the menu sends nothing on to the game (Bejeweled's
// board pauses on any typed character). Mouse: click, wheel, drag a slider or the list.
//
// On the web the list page also has "Add or remove mods", which opens the page's Mods panel (web/modpanel.js) over
// the menu; the page calls byopop_menu and byopop_restart (end of ModMenu.cpp) for its Mods button and Restart.

#include "Widget.h"
#include "../platform/Json.h"
#include "../platform/ModSettings.h"

#include <string>
#include <vector>

namespace Sexy
{

class SexyAppBase;
class BuiltinFont;
struct ScriptCommand;

class ModMenu : public Widget
{
public:
	// What the menu shows of one setting and one mod (from ModManager::GetStateJson, the same model the web panel
	// and -dumpmods see)
	struct SettingView
	{
		Platform::ModSettingDef mDef;  // a copy: the desired state may be scanned again
		Platform::JsonValue mValue;     // as the files say now
		std::string mLabel;             // Latin-1
		std::string mHelp;
		bool mEnabled = true;           // its enabledIf holds
		std::string mPending;           // "", "nextGame" or "restart"
	};
	struct ModView
	{
		std::string mId;
		std::string mName;              // Latin-1 from here on
		std::string mVersion;
		std::string mAuthor;
		std::string mDescription;
		bool mEnabled = true;
		bool mScores = false;
		bool mFeatures = false;
		bool mHasSettings = false;
		bool mDuplicate = false;        // another folder has the same id: it cannot be changed here
		bool mCanUp = false;
		bool mCanDown = false;
		std::string mPending;
		std::vector<std::string> mErrors;
		std::vector<std::string> mWarnings;
		std::vector<SettingView> mSettings;
	};

	enum
	{
		T_NONE,
		T_ROW,        // list: a mod's row (its switch and name)
		T_UP,
		T_DOWN,
		T_OPEN,       // list: "Settings", or "About" for a mod without settings
		T_BACK,
		T_RESTART,
		T_CLOSE,
		T_RESET,
		T_YES,        // a confirmation's buttons
		T_NO,
		T_VALUE,      // mod page: a setting's control
		T_MINUS,      // ... and its parts (mouse only)
		T_PLUS,
		T_SLIDER,
		T_PREV,
		T_NEXT,
		T_FIELD,
		T_LABEL,      // a setting's label: a click focuses it (and switches a bool, as a label does its checkbox)
		T_SCROLL,     // the scroll bar (mouse only)
		T_FILES       // web: "Add or remove mods", the page's Mods panel (Module.onModPanel)
	};

	struct Target
	{
		int mKind;
		int mItem;      // mod index (list) or setting index (mod page); -1 for the footer and header
		Rect mRect;
		bool mDisabled;
		bool mInContent;
	};

	struct Item
	{
		int mKind;      // see ModMenu.cpp
		int mIndex;
		int mY;         // in the content, before scrolling
		int mHeight;
		std::vector<std::string> mLines;
		int mColor;
	};

public:
	ModMenu(SexyAppBase* theApp);
	virtual ~ModMenu();

	virtual void			Update();
	virtual void			Draw(Graphics* g);
	virtual void			KeyDown(KeyCode theKey);
	virtual void			KeyChar(SexyChar theChar);
	virtual void			MouseMove(int x, int y);
	virtual void			MouseDown(int x, int y, int theClickCount);
	virtual void			MouseUp(int x, int y, int theClickCount);
	virtual void			MouseDrag(int x, int y);
	virtual void			MouseWheel(int theDelta);
	virtual void			MouseLeave();
	virtual bool			WantsFocus();

	void					ShowList();
	bool					ShowMod(const std::string& theModId);
	const std::string&		GetPageMod() const { return mPageMod; }
	// The Restart button: asks first when a game is in progress (isConfirmed: the answer was yes)
	void					PressRestart(bool isConfirmed);
	// Commits a text being edited (F2 does)
	void					CommitEdit();
	// SexyAppBase::CloseModMenu, before it removes the menu: commits a text being edited, puts the cursor back
	void					Closing();
	// An edit through the menu (ModManager's Edit* calls): the footer shows its error, or the note that a gameplay
	// change moves the game in progress to other high scores
	void					DoEdit(const std::string& theOp, const std::string& theModId, const std::string& theSettingId, const Platform::JsonValue& theValue);

	// The script command "modmenu ..." (SexyAppBase::RunScript; syntax in SexyAppBase.h)
	static void				RunScriptCommand(SexyAppBase* theApp, const ScriptCommand& theCommand);
	// The app, before its display goes away: the fonts and the glyph image
	static void				FreeResources();

private:
	void					Refresh();
	void					Layout();
	void					AddTarget(int theKind, int theItem, const Rect& theRect, bool isDisabled, bool isInContent);
	const Target*			FindTarget(int theKind, int theItem) const;
	const Target*			TargetAt(int x, int y) const;
	bool					IsFocusable(const Target& theTarget) const;
	void					SetFocusTarget(int theKind, int theItem);
	void					MoveFocusRow(int theDelta);
	void					MoveFocusTab(int theDelta);
	void					ScrollToFocus();
	void					ScrollBy(int theDelta);
	void					Activate(int theKind, int theItem, bool isSpace);
	void					ChangeValue(int theItem, int theSteps);
	void					SetValue(int theItem, const Platform::JsonValue& theValue);
	void					StartEdit(int theItem);
	void					CancelEdit();
	void					Back();
	void					Close();
	void					AskConfirm(int theWhat, const std::string& theText);
	void					AnnounceFocus();
	int						ContentHeight() const;
	Rect					ContentRect() const;
	Rect					SliderTrack(const Target& theTarget) const;
	Platform::JsonValue		SliderValue(int theItem, int x) const;
	ModView*				PageModView();
	SettingView*			PageSetting(int theItem);
	std::string				OpenLabel(int theMod) const;
	std::string				FooterHelp();
	std::string				PendingText() const;
	std::vector<std::pair<std::string, int> > PageMessages();  // the page's messages and their TEXT_ colours (the slot)

	void					DrawButton(Graphics* g, const Rect& theRect, const std::string& theLabel, int theKind, int theItem, bool isDisabled);
	void					DrawBadge(Graphics* g, int x, int y, const std::string& theText, const Color& theColor, int* theWidth);
	void					DrawMessages(Graphics* g);
	void					DrawListRow(Graphics* g, const Item& theItem);
	void					DrawSettingRow(Graphics* g, const Item& theItem);

public:
	SexyAppBase*			mApp;

private:
	BuiltinFont*			mFont;
	BuiltinFont*			mBold;
	Rect					mPanel;

	// The model
	unsigned				mSerial;
	bool					mHasModel;
	std::vector<ModView>	mMods;
	bool					mNoMods;
	std::string				mSettingsFile;
	std::string				mScoreKey;
	std::string				mRestartScoreKey;
	std::vector<std::string> mPendingRestart;
	std::vector<std::string> mPendingNextGame;
	std::vector<std::string> mGameMessages;  // the game's messages about every mod (GetModMessages, mod "")

	// The page and the layout
	std::string				mPageMod;       // "" for the list
	std::vector<Item>		mItems;
	std::vector<Target>		mTargets;
	int						mScroll;
	bool					mLayoutDirty;

	// Focus and pointer
	int						mFocusKind;
	int						mFocusItem;
	int						mHoverKind;
	int						mHoverItem;
	int						mPressKind;
	int						mPressItem;
	int						mPressX;
	int						mPressY;
	int						mPressScroll;
	bool					mPressInContent;
	bool					mDragScroll;
	bool					mDragThumb;
	int						mDragSlider;    // setting index while its slider is dragged, else -1
	Platform::JsonValue		mDragValue;
	std::string				mAnnounced;

	// Text editing (a text or colour setting)
	int						mEditItem;      // -1 when not editing
	std::string				mEditText;      // Latin-1
	int						mEditCaret;

	// The footer
	int						mConfirm;       // 0, or what a confirmation asks (see ModMenu.cpp)
	std::string				mConfirmText;
	std::string				mNote;
	int						mRestartAction; // 0, or 1/2: the Restart button was pressed (unconfirmed/confirmed),
											// done at the next Update outside the menu's no-random scope
	int						mOldCursor;
	bool					mHasModPanel;   // web: the page has a Mods panel that adds and removes mods
};

}

#endif
