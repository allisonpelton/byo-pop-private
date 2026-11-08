// fwtest: exercises the portable SexyAppFramework (SDL2 / Emscripten) with the real game assets.
//
// Loads images, both font formats, a sound and a tracker module the way Bejeweled does, and draws a
// test screen: the title screen, the seven gems, a rotated, an additive and a colorized gem, an alpha
// FillRect, text in both fonts, a ButtonWidget and a modal Dialog (both play a sound when pressed).
//
// Arguments:
//   -gamedir=<dir>       game data folder (default: the repo's assets/bejeweled; /assets on the web)
//   -frames=N            after N frames drawn once loading has finished...
//   -screenshot=<png>    ...save the screen (DDInterface::ReadScreen) as PNG, then shut down
//   -hotkeytest          the mod menu's F2 (docs/MOD_MENU_DESIGN.md 7.3): SDL key events through HandleEvent, as
//                        real input arrives; then, natively, the window: its minimum size and Alt+Enter's borderless
//                        full screen; prints one line per check and exits with the number that failed
//   -realinput=<file>    the framework's real input (SexyAppBase.h, docs/REAL_INPUT.md), with tools/fwtest/realinput.txt:
//                        its "expect ..." lines check what reached the board and the app, "report" prints the count and
//                        shuts down; the exit code is the number that failed. The test dialog is not shown.
//   -commands=<file>     the framework's live commands (SexyAppBase.h, docs/TEST_COMMANDS.md): the same "expect",
//                        "killdialog" and "report" lines, appended while it runs, each answered OK or ERROR on the log
//                        (tools/fwtest/commands.sh drives it). The test dialog is not shown. "state" and "move" answer
//                        as a port's do, for tools/playbymove (its bot plays a small task through every kind of step).
// Exits with 1 when the game files fail to load.

#include "SexyAppBase.h"
#include "WidgetManager.h"
#include "Widget.h"
#include "ButtonWidget.h"
#include "ButtonListener.h"
#include "Dialog.h"
#include "DialogButton.h"
#include "Graphics.h"
#include "DDImage.h"
#include "DDInterface.h"
#include "ImageFont.h"
#include "SoundManager.h"
#include "SDLSoundManager.h"
#include "MusicInterface.h"
#include "ModMenu.h"
#include "KeyCodes.h"
#include "platform/ModManager.h"
#include "platform/VFS.h"

#include <SDL.h>
#include <stb_image_write.h>
#include <cmath>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include "winstate.h"

#ifndef FWTEST_GAMEDIR
#define FWTEST_GAMEDIR "assets/bejeweled"
#endif
#ifndef FWTEST_USERDIR
#define FWTEST_USERDIR "userdata"
#endif

using namespace Sexy;

namespace
{

const int GEM_SIZE = 52;
const int NUM_GEMS = 7;
const int DIALOG_TEST = 1;
const int BUTTON_DIALOG = 1;

class FwTestApp;

int gHotkeyFailures = 0;

class FwTestBoard : public Widget, public ButtonListener
{
public:
	FwTestApp* mApp;
	ButtonWidget* mDialogButton;
	int mUpdateCnt;
	int mKeyDowns;  // the keys and characters that reached the board (-hotkeytest, -realinput)
	int mKeyUps;
	int mKeyChars;
	std::string mTyped;  // the characters since the last "expect typed" (-realinput)
	int mLastChar;
	int mMouseDowns;  // the mouse events that reached the board (-realinput), in game pixels
	int mMouseUps;
	int mDrags;
	int mDownX, mDownY, mDownButton, mDownClicks;  // the last press: button 0 left, 1 right, 2 middle
	int mUpX, mUpY;
	int mMoveX, mMoveY;  // the last move or drag
	int mWheel;
	int mRightDowns;  // right-button presses (the move loop's bot, "move")

	virtual void KeyDown(KeyCode theKey) { mKeyDowns++; Widget::KeyDown(theKey); }
	virtual void KeyUp(KeyCode theKey) { mKeyUps++; Widget::KeyUp(theKey); }
	virtual void KeyChar(SexyChar theChar)
	{
		mKeyChars++;
		mTyped += (char) theChar;
		mLastChar = (unsigned char) theChar;
		Widget::KeyChar(theChar);
	}

	using Widget::MouseDown;
	using Widget::MouseUp;
	virtual void MouseMove(int x, int y) { mMoveX = x; mMoveY = y; }
	virtual void MouseDrag(int x, int y) { mDrags++; mMoveX = x; mMoveY = y; }
	virtual void MouseDown(int x, int y, int theBtnNum, int theClickCount)
	{
		mMouseDowns++;
		if (theBtnNum == 1)
			mRightDowns++;
		mDownX = x;
		mDownY = y;
		mDownButton = theBtnNum;
		mDownClicks = theClickCount;
	}
	virtual void MouseUp(int x, int y, int theBtnNum, int theClickCount)
	{
		mMouseUps++;
		mUpX = x;
		mUpY = y;
	}
	virtual void MouseWheel(int theDelta) { mWheel += theDelta; }

	FwTestBoard(FwTestApp* theApp);
	virtual ~FwTestBoard();

	virtual void Update();
	virtual void Draw(Graphics* g);
	virtual void AddedToManager(WidgetManager* theWidgetManager);
	virtual void RemovedFromManager(WidgetManager* theWidgetManager);
	virtual void ButtonDepress(int theId);
};

class FwTestApp : public SexyAppBase
{
public:
	DDImage* mGemSheetImage;
	DDImage* mTitleScreenImage;
	DDImage* mScoreFontImage;
	ImageFont* mMainFont;
	ImageFont* mScoreFont;
	int mSelectSound;
	FwTestBoard* mBoard;

	int mScreenshotFrames;
	std::string mScreenshotPath;
	int mFramesSinceLoaded;
	bool mHotkeyTest;
	const char* mCheckName;  // starts each check's line: "hotkey" or "realinput"
	int mChecks;
	int mDialogsClosed;  // the test dialog closed by its button ("state", "move")

	FwTestApp()
	{
		mProdName = "fwtest";
		mRegKey = "PopCap\\fwtest";
		mTitle = _S("fwtest");
		mWidth = 640;
		mHeight = 480;

		mGemSheetImage = NULL;
		mTitleScreenImage = NULL;
		mScoreFontImage = NULL;
		mMainFont = NULL;
		mScoreFont = NULL;
		mSelectSound = -1;
		mBoard = NULL;
		mScreenshotFrames = 0;
		mFramesSinceLoaded = 0;
		mHotkeyTest = false;
		mCheckName = "hotkey";
		mChecks = 0;
		mDialogsClosed = 0;
	}

	virtual ~FwTestApp()
	{
		if (mBoard != NULL)
			mWidgetManager->RemoveWidget(mBoard);
		delete mBoard;
		KillDialog(DIALOG_TEST);
		ProcessSafeDeleteList();

		delete mMainFont;
		delete mScoreFont;
		delete mScoreFontImage;
		delete mGemSheetImage;
		delete mTitleScreenImage;
	}

	virtual void HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
	{
		if (theParamName == "-frames")
			mScreenshotFrames = atoi(theParamValue.c_str());
		else if (theParamName == "-screenshot")
			mScreenshotPath = theParamValue;
		else if (theParamName == "-hotkeytest")
			mHotkeyTest = true;
		else if (theParamName != "-gamedir") // handled by main()
			SexyAppBase::HandleCmdLineParam(theParamName, theParamValue);
	}

	virtual void LoadingThreadProc()
	{
		mGemSheetImage = GetImage("images\\gemsheet6");
		mTitleScreenImage = GetImage("images\\titlescreen6");
		mScoreFontImage = GetImage("images\\scorefont1");
		if ((mGemSheetImage == NULL) || (mTitleScreenImage == NULL) || (mScoreFontImage == NULL))
		{
			Popup("fwtest: failed to load images from " + Platform::VFS::Get().GetBaseDir());
			mLoadingFailed = true;
			return;
		}

		mMainFont = new ImageFont(this, "data\\mainfont1.txt");
		mScoreFont = new ImageFont(mScoreFontImage, "data\\scorefont1.txt");
		if ((mMainFont->mFontData == NULL) || (!mMainFont->mFontData->mInitialized) ||
			(mScoreFont->mFontData == NULL) || (!mScoreFont->mFontData->mInitialized))
		{
			Popup("fwtest: failed to load the fonts");
			mLoadingFailed = true;
			return;
		}

		mSelectSound = mSoundManager->LoadSound("sounds\\select");
		if (mSelectSound < 0)
			SDL_Log("fwtest: failed to load sounds\\select");

		if (!mMusicInterface->LoadMusic(0, "music\\1980-autonomus.s3m"))
			SDL_Log("fwtest: failed to load music\\1980-autonomus.s3m");
	}

	virtual void LoadingThreadCompleted()
	{
		if (mLoadingFailed)
			return;

		mBoard = new FwTestBoard(this);
		mBoard->Resize(0, 0, mWidth, mHeight);
		mWidgetManager->AddWidget(mBoard);
		mWidgetManager->SetFocus(mBoard);

		mMusicInterface->PlayMusic(0);

		// -realinput's and -commands' clicks reach the board: no modal dialog over it
		if ((mRealInputFileName.empty()) && (mCommandsFileName.empty()))
			ShowTestDialog();
	}

	virtual Dialog* NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
	{
		// The port has no system font, so every dialog needs its fonts set (Bejeweled does the same)
		Dialog* aDialog = new Dialog(NULL, NULL, theDialogId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);
		aDialog->SetHeaderFont(mMainFont);
		aDialog->SetLinesFont(mMainFont);
		aDialog->SetButtonFont(mMainFont);
		return aDialog;
	}

	void ShowTestDialog()
	{
		Dialog* aDialog = DoDialog(DIALOG_TEST, true, _S("FWTEST DIALOG"), _S("Press OK to play sounds\\select and close this dialog."), _S("OK"), Dialog::BUTTONS_FOOTER);
		int aWidth = 260;
		aDialog->Resize(mWidth - aWidth - 16, 24, aWidth, aDialog->GetPreferredHeight(aWidth));
	}

	bool MoveTaskDone()
	{
		return (mBoard != NULL) && (mBoard->mTyped.find("Hi") != std::string::npos) && (mBoard->mRightDowns > 0) &&
			(mBoard->mDrags > 0) && (mDialogsClosed >= 2) && (GetDialog(DIALOG_TEST) == NULL);
	}

	void PlaySelectSound()
	{
		if (mSelectSound >= 0)
			PlaySample(mSelectSound);
	}

	virtual void ButtonDepress(int theId)
	{
		// Dialog buttons arrive as 2000 + dialog id (yes / OK / footer) and 3000 + dialog id (no / cancel)
		if ((theId == 2000 + DIALOG_TEST) || (theId == 3000 + DIALOG_TEST))
		{
			mDialogsClosed++;
			PlaySelectSound();
			KillDialog(DIALOG_TEST);
		}
	}

	void SendKey(Uint32 theType, SDL_Keycode theKey, Uint16 theMod, Uint8 theRepeat)
	{
		SDL_Event anEvent;
		SDL_zero(anEvent);
		anEvent.type = theType;
		anEvent.key.state = (theType == SDL_KEYDOWN) ? SDL_PRESSED : SDL_RELEASED;
		anEvent.key.repeat = theRepeat;
		anEvent.key.keysym.sym = theKey;
		anEvent.key.keysym.mod = theMod;
		HandleEvent(anEvent);
	}

	void SendText(const char* theText)
	{
		SDL_Event anEvent;
		SDL_zero(anEvent);
		anEvent.type = SDL_TEXTINPUT;
		SDL_strlcpy(anEvent.text.text, theText, sizeof(anEvent.text.text));
		HandleEvent(anEvent);
	}

	void Check(const char* theWhat, bool isOk)
	{
		mChecks++;
		SDL_Log("%s: %s: %s", mCheckName, isOk ? "ok" : "FAILED", theWhat);
		if (!isOk)
			gHotkeyFailures++;
	}

	// -realinput's and -commands' own lines (tools/fwtest/realinput.txt, tools/fwtest/commands.sh): "expect <what>
	// <values>" checks the state the events left, "killdialog" closes the test dialog, "report" prints the count and
	// shuts down. Each answers on the log as a game's test command does (docs/TEST_COMMANDS.md): OK or ERROR.
	virtual bool HandleScriptCommand(const ScriptCommand& theCommand)
	{
		// the standard test commands' two for a driver (docs/TEST_COMMANDS.md), for tools/playbymove's check: "state"
		// and the move a bot makes next towards a small task that takes every kind of step (keys, a drag with a wait,
		// a right click, a hover and a click, twice the dialog opened and closed); "over" once it is done
		if (theCommand.mCommand == "state")
		{
			std::string aTyped;
			if (mBoard != NULL)
			{
				for (char c : mBoard->mTyped)
				{
					if ((c == '"') || (c == '\\'))
						aTyped += '\\';
					if ((unsigned char) c >= 32)
						aTyped += c;
				}
			}
			SDL_Log("STATE {\"screen\":\"%s\",\"typed\":\"%s\",\"rightdowns\":%d,\"drags\":%d,\"dialogs\":%d,\"over\":%s}",
				(mBoard == NULL) ? "loading" : (GetDialog(DIALOG_TEST) != NULL) ? "dialog" : "board", aTyped.c_str(),
				(mBoard != NULL) ? mBoard->mRightDowns : 0, (mBoard != NULL) ? mBoard->mDrags : 0, mDialogsClosed,
				MoveTaskDone() ? "true" : "false");
			return true;
		}
		if (theCommand.mCommand == "move")
		{
			Dialog* aDialog = GetDialog(DIALOG_TEST);
			if (mBoard == NULL)
				SDL_Log("MOVE {\"kind\":\"wait\",\"input\":[{\"wait\":10}]}");
			else if (MoveTaskDone())
				SDL_Log("MOVE null");
			else if ((aDialog != NULL) && (aDialog->mYesButton != NULL))
			{
				// a dialog's buttons are the widget manager's own, at the screen's coordinates (Dialog::AddedToManager)
				int aX = aDialog->mYesButton->mX + aDialog->mYesButton->mWidth / 2;
				int aY = aDialog->mYesButton->mY + aDialog->mYesButton->mHeight / 2;
				SDL_Log("MOVE {\"kind\":\"ok\",\"input\":[{\"move\":[%d,%d]},{\"click\":[%d,%d]}],\"updates\":2}", aX, aY, aX, aY);
			}
			else if (mBoard->mTyped.find("Hi") == std::string::npos)
				SDL_Log("MOVE {\"kind\":\"type\",\"input\":[{\"key\":\"H\"},{\"key\":\"i\"},{\"key\":\"Enter\"}]}");
			// the drag before any click: every release is a drag at its point (HandleEvent moves the mouse there
			// first, as 1.3's window procedure does)
			else if (mBoard->mDrags == 0)
				SDL_Log("MOVE {\"kind\":\"drag\",\"input\":[{\"press\":[100,150]},{\"move\":[130,160]},{\"wait\":2},{\"move\":[160,170]},{\"release\":[160,170]}]}");
			else if (mBoard->mRightDowns == 0)
				SDL_Log("MOVE {\"kind\":\"look\",\"input\":[{\"click\":[300,200],\"button\":\"right\"}]}");
			else
				SDL_Log("MOVE {\"kind\":\"button\",\"input\":[{\"move\":[548,420]},{\"click\":[548,420]}]}");
			return true;
		}
		if (theCommand.mCommand == "killdialog")
		{
			KillDialog(DIALOG_TEST);
			SDL_Log("OK killdialog");
			return true;
		}
		if (theCommand.mCommand == "report")
		{
			SDL_Log("%s: %d checks, %d failed", mCheckName, mChecks, gHotkeyFailures);
			SDL_Log("OK report");
			Shutdown();
			return true;
		}
		if (theCommand.mCommand != "expect")
			return false;

		mCheckName = mCommandsFileName.empty() ? "realinput" : "commands";
		std::istringstream anArgs(theCommand.mArgs);
		std::string aWhat;
		anArgs >> aWhat;
		int a = 0, b = 0, c = 0, d = 0;
		std::string aWord;
		bool isOk = false;
		if (aWhat == "board")
			isOk = mBoard != NULL;
		else if (mBoard == NULL)
			isOk = false;
		else if ((aWhat == "move") && (anArgs >> a >> b))
			isOk = (mBoard->mMoveX == a) && (mBoard->mMoveY == b);
		else if ((aWhat == "drags") && (anArgs >> a))
			isOk = mBoard->mDrags == a;
		else if ((aWhat == "down") && (anArgs >> a >> b >> aWord >> d))
		{
			c = (aWord == "left") ? 0 : (aWord == "right") ? 1 : (aWord == "middle") ? 2 : -1;
			isOk = (mBoard->mDownX == a) && (mBoard->mDownY == b) && (mBoard->mDownButton == c) && (mBoard->mDownClicks == d);
		}
		else if ((aWhat == "up") && (anArgs >> a >> b))
			isOk = (mBoard->mUpX == a) && (mBoard->mUpY == b);
		else if ((aWhat == "buttons") && (anArgs >> a >> b))
			isOk = (mBoard->mMouseDowns == a) && (mBoard->mMouseUps == b);
		else if ((aWhat == "wheel") && (anArgs >> a))
			isOk = mBoard->mWheel == a;
		else if ((aWhat == "keys") && (anArgs >> a >> b))
			isOk = (mBoard->mKeyDowns == a) && (mBoard->mKeyUps == b);
		else if (aWhat == "typed")
		{
			std::string aText;
			std::getline(anArgs, aText);
			size_t aStart = aText.find('"');
			size_t anEnd = aText.rfind('"');
			aText = ((aStart != std::string::npos) && (anEnd > aStart)) ? aText.substr(aStart + 1, anEnd - aStart - 1) : "";
			isOk = mBoard->mTyped == aText;
			if (!isOk)
				SDL_Log("realinput: typed \"%s\"", mBoard->mTyped.c_str());
			mBoard->mTyped.clear();
		}
		else if ((aWhat == "lastchar") && (anArgs >> a))
			isOk = mBoard->mLastChar == a;
		else if ((aWhat == "held") && (anArgs >> aWord >> a))
		{
			KeyCode aKey = GetKeyCodeFromName(aWord);
			isOk = (aKey > KEYCODE_UNKNOWN) && (aKey < 0xFF) && (mWidgetManager->mKeyDown[aKey] == (a != 0));
		}
		else if ((aWhat == "active") && (anArgs >> a))
			isOk = mActive == (a != 0);
		else if ((aWhat == "mousein") && (anArgs >> a))
			isOk = mMouseIn == (a != 0);
		else if ((aWhat == "windowed") && (anArgs >> a))
			isOk = mIsWindowed == (a != 0);
		else if ((aWhat == "minimized") && (anArgs >> a))
			isOk = mMinimized == (a != 0);
		else if ((aWhat == "dialog") && (anArgs >> a))
			isOk = (GetDialog(DIALOG_TEST) != NULL) == (a != 0);
		else if ((aWhat == "modmenu") && (anArgs >> a))
			isOk = IsModMenuOpen() == (a != 0);
		else if ((aWhat == "windowsize") && (anArgs >> a >> b))
		{
			int aWidth = 0, aHeight = 0;
			SDL_GetWindowSize(mHWnd, &aWidth, &aHeight);
			isOk = (aWidth == a) && (aHeight == b);
			if (!isOk)
				SDL_Log("realinput: the window is %dx%d", aWidth, aHeight);
		}
		else
			SDL_Log("realinput: line %d: unknown expect '%s'", theCommand.mLine, theCommand.mArgs.c_str());

		std::string aLine = StrFormat("line %d (update %d): expect %s", theCommand.mLine, mUpdateCount, theCommand.mArgs.c_str());
		Check(aLine.c_str(), isOk);
		if (isOk)
			SDL_Log("OK expect %s", theCommand.mArgs.c_str());
		else
			SDL_Log("ERROR expect %s: not so", theCommand.mArgs.c_str());
		return true;
	}

	// F2 opens and closes the mod menu, only as a press without modifiers that is not a repeat, never in a run
	// with scripted input; neither it nor its release reaches the game; Esc closes the menu and types nothing into
	// the game
	void RunHotkeyTest()
	{
		KillDialog(DIALOG_TEST);
		ProcessSafeDeleteList();
		Check("the board has the keyboard", mWidgetManager->mFocusWidget == mBoard);

		SendKey(SDL_KEYDOWN, SDLK_F2, KMOD_NONE, 0);
		Check("F2 opens the menu", IsModMenuOpen());
		Check("the menu has the keyboard", mWidgetManager->mFocusWidget == mModMenu);
		SendKey(SDL_KEYDOWN, SDLK_F2, KMOD_NONE, 1);
		Check("a repeat of F2 leaves it open", IsModMenuOpen());
		SendKey(SDL_KEYUP, SDLK_F2, KMOD_NONE, 0);
		SendKey(SDL_KEYDOWN, SDLK_F2, KMOD_NUM, 0);
		Check("F2 (Num Lock on) closes it", !IsModMenuOpen());
		SendKey(SDL_KEYUP, SDLK_F2, KMOD_NUM, 0);
		ProcessSafeDeleteList();
		Check("the board has the keyboard again", mWidgetManager->mFocusWidget == mBoard);
		Check("the board saw no key of it", (mBoard->mKeyDowns == 0) && (mBoard->mKeyUps == 0) && (mBoard->mKeyChars == 0));

		const Uint16 aMods[] = { KMOD_LSHIFT, KMOD_RCTRL, KMOD_LALT, KMOD_LGUI };
		const char* aNames[] = { "Shift+F2", "Ctrl+F2", "Alt+F2", "Win+F2" };
		for (int i = 0; i < 4; i++)
		{
			SendKey(SDL_KEYDOWN, SDLK_F2, aMods[i], 0);
			std::string aWhat = std::string(aNames[i]) + " does not open it";
			Check(aWhat.c_str(), !IsModMenuOpen());
			SendKey(SDL_KEYUP, SDLK_F2, aMods[i], 0);
			CloseModMenu();
		}
		Check("... and goes to the game as a key", mBoard->mKeyDowns == 4);

		mScriptedInput = true;
		SendKey(SDL_KEYDOWN, SDLK_F2, KMOD_NONE, 0);
		Check("with scripted input F2 does nothing", !IsModMenuOpen());
		SendKey(SDL_KEYUP, SDLK_F2, KMOD_NONE, 0);
		mScriptedInput = false;

		// Esc on the list closes the menu, on its character: the game gets the key's release only
		SendKey(SDL_KEYDOWN, SDLK_F2, KMOD_NONE, 0);
		SendKey(SDL_KEYUP, SDLK_F2, KMOD_NONE, 0);
		int aChars = mBoard->mKeyChars;
		int aDowns = mBoard->mKeyDowns;
		int anUps = mBoard->mKeyUps;
		SendKey(SDL_KEYDOWN, SDLK_ESCAPE, KMOD_NONE, 0);
		Check("Esc closes the menu", !IsModMenuOpen());
		Check("the game gets neither Esc's key nor its character", (mBoard->mKeyChars == aChars) && (mBoard->mKeyDowns == aDowns));
		// Esc held: its repeats go nowhere (Bejeweled's board would pause on their characters), its release goes on
		for (int i = 0; i < 3; i++)
			SendKey(SDL_KEYDOWN, SDLK_ESCAPE, KMOD_NONE, 1);
		Check("the game gets none of the repeats of the Esc that closed it", (mBoard->mKeyChars == aChars) && (mBoard->mKeyDowns == aDowns));
		SendKey(SDL_KEYUP, SDLK_ESCAPE, KMOD_NONE, 0);
		Check("... and gets its release", mBoard->mKeyUps == anUps + 1);
		SendKey(SDL_KEYDOWN, SDLK_ESCAPE, KMOD_NONE, 0);
		Check("the next Esc press goes to the game", (mBoard->mKeyChars == aChars + 1) && (mBoard->mKeyDowns == aDowns + 1));
		SendKey(SDL_KEYUP, SDLK_ESCAPE, KMOD_NONE, 0);
		ProcessSafeDeleteList();

		// Enter on the menu's Close button (Tab from the empty list goes round to it) closes it the same way
		SendKey(SDL_KEYDOWN, SDLK_F2, KMOD_NONE, 0);
		SendKey(SDL_KEYUP, SDLK_F2, KMOD_NONE, 0);
		aChars = mBoard->mKeyChars;
		aDowns = mBoard->mKeyDowns;
		SendKey(SDL_KEYDOWN, SDLK_RETURN, KMOD_NONE, 0);
		Check("Enter on Close closes the menu", !IsModMenuOpen());
		Check("the game gets no character of Enter", mBoard->mKeyChars == aChars);
		SendKey(SDL_KEYDOWN, SDLK_RETURN, KMOD_NONE, 1);
		SendKey(SDL_KEYDOWN, SDLK_RETURN, KMOD_NONE, 1);
		Check("... nor of its repeats", (mBoard->mKeyChars == aChars) && (mBoard->mKeyDowns == aDowns));
		SendKey(SDL_KEYUP, SDLK_RETURN, KMOD_NONE, 0);
		ProcessSafeDeleteList();

		// Space on Close: it closes the menu on the text it types; the repeats and their text go nowhere, another
		// key pressed meanwhile still types
		SendKey(SDL_KEYDOWN, SDLK_F2, KMOD_NONE, 0);
		SendKey(SDL_KEYUP, SDLK_F2, KMOD_NONE, 0);
		aChars = mBoard->mKeyChars;
		aDowns = mBoard->mKeyDowns;
		SendKey(SDL_KEYDOWN, SDLK_SPACE, KMOD_NONE, 0);
		SendText(" ");
		Check("Space on Close closes the menu", !IsModMenuOpen());
		SendKey(SDL_KEYDOWN, SDLK_SPACE, KMOD_NONE, 1);
		SendText(" ");
		SendKey(SDL_KEYDOWN, SDLK_SPACE, KMOD_NONE, 1);
		SendText(" ");
		Check("the game gets nothing of Space or its repeats", (mBoard->mKeyChars == aChars) && (mBoard->mKeyDowns == aDowns));
		SendKey(SDL_KEYDOWN, SDLK_a, KMOD_NONE, 0);
		SendText("a");
		Check("a key pressed while Space is held still reaches the game", (mBoard->mKeyChars == aChars + 1) && (mBoard->mKeyDowns == aDowns + 1));
		SendKey(SDL_KEYUP, SDLK_a, KMOD_NONE, 0);
		SendKey(SDL_KEYUP, SDLK_SPACE, KMOD_NONE, 0);
		ProcessSafeDeleteList();

		// Not the hotkey, but run here with it: SoundManager::SetDefaultPitch reaches every instance GetSoundInstance
		// hands out (Bejeweled's look.soundPitch), and 0 leaves them alone
		Check("the select sound is loaded for the pitch check", mSelectSound >= 0);
		if (mSelectSound >= 0)
		{
			mSoundManager->SetDefaultPitch(-2.0);
			SDLSoundInstance* aShifted = static_cast<SDLSoundInstance*>(mSoundManager->GetSoundInstance(mSelectSound));
			mSoundManager->SetDefaultPitch(0.0);
			SDLSoundInstance* aPlain = static_cast<SDLSoundInstance*>(mSoundManager->GetSoundInstance(mSelectSound));
			Check("a default pitch of -2 shifts a new instance 2 semitones down",
				(aShifted != NULL) && (std::fabs(aShifted->GetPitch() - std::pow(2.0, -2.0 / 12.0)) < 1e-12));
			Check("a default pitch of 0 leaves it unshifted", (aPlain != NULL) && (aPlain->GetPitch() == 1.0));
			if (aShifted != NULL)
				aShifted->Release();
			if (aPlain != NULL)
				aPlain->Release();
		}

		RunWindowTest();
		SDL_Log("hotkey: %d failed", gHotkeyFailures);
	}

	// The window, natively (the user, 2026-11-07): it can't be made smaller than the game's screen, and Alt+Enter's full
	// screen is a borderless window over the whole monitor at the desktop's own mode, not topmost, with the cursor free
	void RunWindowTest()
	{
#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
		int aMinWidth = 0, aMinHeight = 0;
		SDL_GetWindowMinimumSize(mHWnd, &aMinWidth, &aMinHeight);
		Check("window: its minimum size is the game's", (aMinWidth == mWidth) && (aMinHeight == mHeight));
		// a resize below it, as a drag of the frame asks for (DefWindowProc checks it against WM_GETMINMAXINFO)
		FwResizeWindow(mHWnd, 100, 80);
		int aWidth = 0, aHeight = 0;
		SDL_GetWindowSize(mHWnd, &aWidth, &aHeight);
		Check("window: a smaller size keeps the game's", (aWidth == mWidth) && (aHeight == mHeight));

		SDL_DisplayMode aDesktop, aMode;
		const int aDisplay = SDL_GetWindowDisplayIndex(mHWnd);
		SDL_GetDesktopDisplayMode(aDisplay, &aDesktop);
		SendKey(SDL_KEYDOWN, SDLK_RETURN, KMOD_LALT, 0);
		SendKey(SDL_KEYUP, SDLK_RETURN, KMOD_LALT, 0);
		Check("window: Alt+Enter goes full screen at the desktop's size",
			(!mIsWindowed) && ((SDL_GetWindowFlags(mHWnd) & SDL_WINDOW_FULLSCREEN_DESKTOP) == SDL_WINDOW_FULLSCREEN_DESKTOP));
		SDL_GetCurrentDisplayMode(aDisplay, &aMode);
		Check("window: ... without changing the display's mode",
			(aMode.w == aDesktop.w) && (aMode.h == aDesktop.h) && (aMode.refresh_rate == aDesktop.refresh_rate));
		FwWindowState aState;
		const bool haveState = FwGetWindowState(mHWnd, &aState);
		Check("window: ... borderless (no caption, no frame)", haveState && aState.mBorderless);
		Check("window: ... and not topmost", haveState && !aState.mTopmost);
		Check("window: ... over the whole monitor", haveState && aState.mCoversMonitor);
		Check("window: ... with the cursor free to leave it", haveState && aState.mCursorFree);
		Check("window: a focus loss does not minimize it", SDL_GetHintBoolean(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, SDL_TRUE) == SDL_FALSE);
		SendKey(SDL_KEYDOWN, SDLK_RETURN, KMOD_LALT, 0);
		SendKey(SDL_KEYUP, SDLK_RETURN, KMOD_LALT, 0);
		SDL_GetWindowSize(mHWnd, &aWidth, &aHeight);
		Check("window: Alt+Enter again gives the window back",
			(mIsWindowed) && ((SDL_GetWindowFlags(mHWnd) & SDL_WINDOW_FULLSCREEN) == 0) && (aWidth >= mWidth) && (aHeight >= mHeight));
#endif
	}

	virtual bool DrawDirtyStuff()
	{
		bool drawn = SexyAppBase::DrawDirtyStuff();

		if ((drawn) && (mLoaded) && (mHotkeyTest) && (mBoard != NULL) && (++mFramesSinceLoaded == 3))
		{
			RunHotkeyTest();
			Shutdown();
		}

		if ((drawn) && (mLoaded) && (mScreenshotFrames > 0) && (++mFramesSinceLoaded == mScreenshotFrames))
		{
			if (!mScreenshotPath.empty())
				SaveScreenshot(mScreenshotPath);
			Shutdown();
		}

		return drawn;
	}

	void SaveScreenshot(const std::string& thePath)
	{
		std::vector<ulong> aBits(mWidth * mHeight);
		if (!mDDInterface->ReadScreen(&aBits[0]))
		{
			SDL_Log("fwtest: ReadScreen failed");
			return;
		}

		std::vector<unsigned char> aRGBA(mWidth * mHeight * 4);
		for (int i = 0; i < mWidth * mHeight; i++)
		{
			ulong aPixel = aBits[i];
			aRGBA[i*4 + 0] = (unsigned char) ((aPixel >> 16) & 0xFF);
			aRGBA[i*4 + 1] = (unsigned char) ((aPixel >> 8) & 0xFF);
			aRGBA[i*4 + 2] = (unsigned char) (aPixel & 0xFF);
			aRGBA[i*4 + 3] = (unsigned char) ((aPixel >> 24) & 0xFF);
		}

		if (stbi_write_png(thePath.c_str(), mWidth, mHeight, 4, &aRGBA[0], mWidth * 4))
			SDL_Log("fwtest: saved %s after %d frames", thePath.c_str(), mFramesSinceLoaded);
		else
			SDL_Log("fwtest: failed to write %s", thePath.c_str());
	}
};

FwTestBoard::FwTestBoard(FwTestApp* theApp)
{
	mApp = theApp;
	mUpdateCnt = 0;
	mKeyDowns = 0;
	mKeyUps = 0;
	mKeyChars = 0;
	mLastChar = 0;
	mMouseDowns = 0;
	mMouseUps = 0;
	mDrags = 0;
	mDownX = mDownY = mDownButton = mDownClicks = -1;
	mUpX = mUpY = -1;
	mMoveX = mMoveY = -1;
	mWheel = 0;
	mRightDowns = 0;

	mDialogButton = new ButtonWidget(BUTTON_DIALOG, this);
	mDialogButton->mLabel = _S("DIALOG");
	mDialogButton->SetFont(theApp->mMainFont);
	mDialogButton->Resize(480, 404, 136, 32);
}

FwTestBoard::~FwTestBoard()
{
	delete mDialogButton;
}

void FwTestBoard::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	AddWidget(mDialogButton);
}

void FwTestBoard::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	RemoveWidget(mDialogButton);
}

void FwTestBoard::Update()
{
	Widget::Update();
	mUpdateCnt++;
	MarkDirty();
}

void FwTestBoard::ButtonDepress(int theId)
{
	if (theId == BUTTON_DIALOG)
	{
		mApp->PlaySelectSound();
		mApp->ShowTestDialog();
	}
}

void FwTestBoard::Draw(Graphics* g)
{
	g->DrawImage(mApp->mTitleScreenImage, 0, 0);

	// Translucent panel behind the gems and the text
	g->SetColor(Color(0, 0, 0, 128));
	g->FillRect(12, 306, 616, 162);

	// The seven gems: gem g's resting frame is column 0 of row g*2 (row g*2+1 is its shimmer)
	for (int i = 0; i < NUM_GEMS; i++)
		g->DrawImage(mApp->mGemSheetImage, 24 + i*56, 316, Rect(0, i*2*GEM_SIZE, GEM_SIZE, GEM_SIZE));

	Rect aGemRect(0, 4*2*GEM_SIZE, GEM_SIZE, GEM_SIZE); // purple
	int aX = 24 + NUM_GEMS*56 + 8;

	// Rotated (radians, around the cell centre)
	g->DrawImageRotated(mApp->mGemSheetImage, aX, 316, mUpdateCnt * 0.02, &aGemRect);

	// Additive
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->DrawImage(mApp->mGemSheetImage, aX + 56, 316, aGemRect);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);

	// Colorized: tinted and half transparent
	g->SetColorizeImages(true);
	g->SetColor(Color(128, 255, 128, 128));
	g->DrawImage(mApp->mGemSheetImage, aX + 112, 316, aGemRect);
	g->SetColorizeImages(false);

	g->SetFont(mApp->mMainFont);
	g->SetColor(Color(255, 255, 255));
	const char* aLabels[] = { "ROT", "ADD", "COLOR" };
	for (int i = 0; i < 3; i++)
	{
		SexyString aLabel = aLabels[i];
		g->DrawString(aLabel, aX + i*56 + (GEM_SIZE - g->StringWidth(aLabel)) / 2, 384);
	}

	g->SetColor(Color(255, 255, 0));
	g->DrawString(_S("SexyAppFramework port test - ImageFont (script): mainfont1"), 24, 416);

	g->SetFont(mApp->mScoreFont);
	g->SetColor(Color(255, 255, 255));
	g->DrawString(StrFormat(_S("%d"), 1234567890), 24, 452);
}

}

int main(int argc, char* argv[])
{
	std::string aGameDir = FWTEST_GAMEDIR;
	for (int i = 1; i < argc; i++)
	{
		if (strncmp(argv[i], "-gamedir=", 9) == 0)
			aGameDir = argv[i] + 9;
	}

	Platform::MakeDirs(FWTEST_USERDIR);
	Platform::VFS::Get().SetBaseDir(aGameDir);
	Platform::VFS::Get().SetUserDir(FWTEST_USERDIR);
	// the mod menu (-hotkeytest) shows the mods of a folder that does not exist: none
	Platform::ModManager::Get().SetModsDir(FWTEST_USERDIR "/fwtest-no-mods");
	SDL_Log("fwtest: game data %s, user data %s", aGameDir.c_str(), FWTEST_USERDIR);

	static FwTestApp* anApp = new FwTestApp();
	anApp->SetCmdLine(argc, argv);
	anApp->Init();
	anApp->Start();
	// (on the web Start() hands the loop to the browser and doesn't return)
	int aResult = anApp->mLoadingFailed ? 1 : gHotkeyFailures;
#ifndef __EMSCRIPTEN__
	delete anApp;
#endif
	return aResult;
}
