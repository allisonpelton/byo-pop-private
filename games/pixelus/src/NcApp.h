#ifndef __PIXELUS_NCAPP_H__
#define __PIXELUS_NCAPP_H__

// NcApp.cpp: Nuclide Games' application layer between PopCap's SexyApp and the game (Pixelus.exe 0x4e2c10-0x4eb000,
// linked after the C library; the "Nc" classes). Behaviour spec: docs/GAME_NOTES.md section 1.
//
// Original: class NcApp : public Sexy::SexyApp, vftable 0x527f28 (87 slots: SexyApp's 71, then 16 of its own;
// 71-79 are pure virtuals the game fills in), data from +0x640. It holds the resource tables (images, fonts,
// sounds and songs by index, as the game's Res tables list them), the pause count and the trial bookkeeping,
// and makes the dialogs every Nuclide game has (credits, options, the quit question).
// Offsets below are the original's (code never depends on them).

#include "SexyApp.h"
#include <string>
#include <vector>

namespace Sexy
{

class Image;
class ImageFont;
class Dialog;

class NcApp : public SexyApp
{
public:
	// Resource tables: the file-name tables of Res.cpp. The original keeps pointers to the game's tables
	const char* const*		mImageNames;        // +0x624 (8-byte rows: name, load group)
	const unsigned char*	mImageGroups;       // +0x624 + 4: the load group of each image
	int						mNumImages;         // +0x628
	int						mNumFonts;          // +0x62c
	int						mPauseCount;        // +0x630 NcApp::Pause nests
	Image**					mImages;            // +0x638 GetImage("images\\" + name) of each, a DDImage copy
	ImageFont**				mFonts;             // +0x63c ImageFont("data\\...txt") of each
	bool					mHintsDisabled;     // registry "HintsDisabled" (NcApp 0x4e3aa0 / 0x4e3b20)
	bool					mQuitRequested;      // +0x674 the quit question was answered yes: UpdateFrames (0x4e2f80) then shuts the app down

public:
	NcApp();
	virtual ~NcApp();

	virtual void			ButtonPress(int theId) override;                // 0x4e2e00 slot 4
	virtual void			ButtonDepress(int theId) override;              // 0x4e4920 slot 5
	virtual void			Init() override;                                // 0x4e36c0 slot 17

	// Resource loading (called by the game's loading thread, in its order)
	void					LoadMusicNames(const char* const* theNames, int theCount);      // 0x4e3740
	void					LoadFonts(const char* const* theNames, int theCount);           // 0x4e3820
	void					LoadImages(const char* const* theNames, const unsigned char* theGroups, int theCount); // 0x4e4250
	void					LoadSounds(const char* const* theNames, int theCount);          // 0x4e4410
	bool					LoadImage(int theIndex);                                        // 0x4e40a0
	void					SetCursorImages(int thePointer, int theHand, int theDragging, int theText); // 0x4e2c10
	void					CenterDialog(Dialog* theDialog);                                // 0x4e2d50
	void					Pause(bool thePause);                                           // 0x4e3bb0
	bool					IsPaused() const { return mPauseCount > 0; }                    // 0x4e2e60
	void					SetHintsDisabled(bool theDisabled);                             // 0x4e3aa0
	bool					GetHintsDisabled();                                             // 0x4e3b20
	// The game's own handler of a paused/unpaused change (slot 84 in the original)
	virtual void			HandlePaused(bool theWasPaused, bool theIsPaused, int theCount) {}
	// The loading screen's play link was clicked (slot 79 in the original, which fills it with its first-screen code)
	virtual void			PlayClicked() {}
};

}

#endif
