// fwtest: exercises the portable SexyAppFramework (SDL2 / Emscripten) with the real game assets.
//
// Loads images, both font formats, a sound and a tracker module the way Bejeweled does, and draws a
// test screen: the title screen, the seven gems, a rotated, an additive and a colorized gem, an alpha
// FillRect, text in both fonts, a ButtonWidget and a modal Dialog (both play a sound when pressed).
//
// Arguments:
//   -gamedir=<dir>       game data folder (default: the repo's assets folder; /assets on the web)
//   -frames=N            after N frames drawn once loading has finished...
//   -screenshot=<png>    ...save the screen (DDInterface::ReadScreen) as PNG, then shut down

#include "SexyAppFramework/SexyAppBase.h"
#include "SexyAppFramework/WidgetManager.h"
#include "SexyAppFramework/Widget.h"
#include "SexyAppFramework/ButtonWidget.h"
#include "SexyAppFramework/ButtonListener.h"
#include "SexyAppFramework/Dialog.h"
#include "SexyAppFramework/Graphics.h"
#include "SexyAppFramework/DDImage.h"
#include "SexyAppFramework/DDInterface.h"
#include "SexyAppFramework/ImageFont.h"
#include "SexyAppFramework/SoundManager.h"
#include "SexyAppFramework/MusicInterface.h"
#include "platform/VFS.h"

#include <SDL.h>
#include <stb_image_write.h>
#include <cstring>
#include <vector>

#ifndef FWTEST_GAMEDIR
#define FWTEST_GAMEDIR "assets"
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

class FwTestBoard : public Widget, public ButtonListener
{
public:
	FwTestApp* mApp;
	ButtonWidget* mDialogButton;
	int mUpdateCnt;

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

	virtual bool DrawDirtyStuff()
	{
		bool drawn = SexyAppBase::DrawDirtyStuff();

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
	SDL_Log("fwtest: game data %s, user data %s", aGameDir.c_str(), FWTEST_USERDIR);

	static FwTestApp* anApp = new FwTestApp();
	anApp->SetCmdLine(argc, argv);
	anApp->Init();
	anApp->Start();
#ifndef __EMSCRIPTEN__
	// (on the web Start() hands the loop to the browser and doesn't return)
	delete anApp;
#endif
	return 0;
}
