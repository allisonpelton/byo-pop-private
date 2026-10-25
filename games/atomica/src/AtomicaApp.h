#ifndef __ATOMICA_ATOMICAAPP_H__
#define __ATOMICA_ATOMICAAPP_H__

// AtomicaApp.cpp: the application class of Atomica Deluxe 2.52 (Atomica.exe 0x406260-0x40b1e0 and the helpers in
// 0x405ad0-0x406240). Behaviour spec: docs/GAME_NOTES.md section 3.
//
// Original: a class derived from the 2003 SexyApp (SexyApp : ButtonListener, 55 virtual slots) with its vftable
// at 0x4ae638; the object is 0x4478 bytes. It overrides slots 5, 11 (LoadingThreadProc, 0x406f90), 13 (the
// destructor), 24 (WriteToRegistry, 0x406ec0), 25 (ReadFromRegistry, 0x406dc0), 28 and 30 (Init, 0x4067b0).
// Offsets in the comments are the original's.

#include "SexyApp.h"
#include "Support.h"

namespace Sexy
{

class Board;
class TitleScreen;
class ImageFont;

class AtomicaApp : public SexyApp
{
public:
	enum
	{
		GAME_WIDTH = 640,			// ctor: mWidth 0x280, mHeight 0x1e0
		GAME_HEIGHT = 480
	};

	// The fonts (data\*.txt)
	ImageFont*		mAtomicFont;			// +0x468 atomicfont
	ImageFont*		mLevelFont;				// +0x46c levelfont
	ImageFont*		mScoreFont;				// +0x470 scorefont
	ImageFont*		mStencilFont;			// +0x474 stencilfont
	ImageFont*		mMonitorFont;			// +0x478 monitorfont
	ImageFont*		mMonitorFontMedium;		// +0x47c monitorfont(medium)
	ImageFont*		mMonitorFontSmall;		// +0x480 monitorfont(small)
	ImageFont*		mFloatyFont;			// +0x484 floatyfont
	ImageFont*		mAnnounceFont;			// +0x488 announcefont
	ImageFont*		mCommentFont;			// +0x48c commentfont
	ImageFont*		mMouseoverFont;			// +0x490 mouseoverfont

	Board*			mBoard;					// +0x494 the game in progress
	Widget*			mMenu;					// +0x498 the main menu
	TitleScreen*	mTitleScreen;			// +0x49c until the first click
	SpriteMgr*		mSprites;				// +0x4a0

	std::string		mLoadingText;			// the line the title screen shows under its bar (title screen +0xc4)

	GameRand		mRand;					// +0x3da4 the game's random numbers

	// the pictures of the title screen (loaded by the TitleScreen's constructor in the original)
public:
	AtomicaApp();
	virtual ~AtomicaApp();

	virtual void		Init() override;
	virtual void		LoadingThreadProc() override;
	virtual void		LoadingThreadCompleted() override;
	virtual void		WriteToRegistry() override;
	virtual void		ReadFromRegistry() override;
	virtual void		HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) override;

	void				TitleScreenDone();					// the title screen's play link was clicked
};

extern AtomicaApp* gApp;

}

#endif
