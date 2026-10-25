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
//                        real input arrives; prints one line per check and exits with the number that failed
// Exits with 1 when the game files fail to load.

#include "SexyAppBase.h"
#include "WidgetManager.h"
#include "Widget.h"
#include "ButtonWidget.h"
#include "ButtonListener.h"
#include "Dialog.h"
#include "Graphics.h"
#include "DDImage.h"
#include "DDInterface.h"
#include "ImageFont.h"
#include "SoundManager.h"
#include "MusicInterface.h"
#include "ModMenu.h"
#include "platform/ModManager.h"
#include "platform/VFS.h"

#include <SDL.h>
#include <stb_image_write.h>
#include <cstring>
#include <vector>

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
	int mKeyDowns;  // the keys and characters that reached the board (-hotkeytest)
	int mKeyUps;
	int mKeyChars;

	virtual void KeyDown(KeyCode theKey) { mKeyDowns++; Widget::KeyDown(theKey); }
	virtual void KeyUp(KeyCode theKey) { mKeyUps++; Widget::KeyUp(theKey); }
	virtual void KeyChar(SexyChar theChar) { mKeyChars++; Widget::KeyChar(theChar); }

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
		SDL_Log("hotkey: %s: %s", isOk ? "ok" : "FAILED", theWhat);
		if (!isOk)
			gHotkeyFailures++;
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

		SDL_Log("hotkey: %d failed", gHotkeyFailures);
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
