#ifndef __WINDMAPP_H__
#define __WINDMAPP_H__

// WinDMApp.cpp: the application class ("DM" = Diamond Mine, Bejeweled's original name).
// Original: class Sexy::WinDMApp : public SexyApp, sizeof 0x878, a global object (gApp @0x53b570)
// whose static ctor runs WinDMApp(); WinMain calls Init(), Start(), Shutdown() on it non-virtually.
// Vtables: primary 0x507ccc (103 slots, ButtonListener slots 0-6 first), DialogListener 0x507cc0.
// sizeof(SexyApp) is 0x728 (last field mTimedOut at +0x720, 8-byte aligned), so WinDMApp data
// starts at +0x728; Ghidra's WinDMApp_data.offset_N is absolute N+0x724. Offsets below are absolute.
// Fields the original ctor leaves alone (+0x728..+0x787, +0x808, +0x80c, +0x874) rely on the
// static zero-init of the global; the port must initialise them to 0 / NULL / false.
// Behaviour spec: docs/GAME_NOTES.md (WinDMApp section).

#include "SexyApp.h"
#include "Res.h"
#include <list>

namespace Sexy
{

class Board;
class TitleScreen;
class HighScoreWidget;
class Image;
class MemoryImage;
class DDImage;
class ImageFont;
class ButtonWidget;
class Dialog;
class MusicInterface;

// Dialog ids (SexyAppBase dialog map). YES/OK/footer presses arrive in ButtonDepress as 2000+id,
// NO/CANCEL as 3000+id.
enum DialogId
{
	DIALOG_NEW_GAME = 0,
	DIALOG_QUIT = 1,
	DIALOG_OPTIONS = 2,
	DIALOG_CONFIRM_NORMAL_MODE = 3,
	DIALOG_CONFIRM_TIMED_MODE = 4,
	DIALOG_REGISTER = 5,                // RegisterDialog (not ported)
	DIALOG_REGISTRATION_INVALID = 6,    // ShowRegistrationInvalidDialog 0x4d5386 (not ported)
	DIALOG_REGISTER_TO_RECORD = 7,      // ShowRegisterToRecordScoreDialog 0x4d5486 (not ported)
	DIALOG_RECORD_SCORE = 8,
	DIALOG_INSTRUCTIONS = 9,            // non-modal first-move hint
	DIALOG_OPENING_BROWSER = 10,
	DIALOG_URL_FAILED = 11,
	DIALOG_FULL_INSTRUCTIONS = 12,      // also used by ShowColorDepthDialog
	DIALOG_HIGH_SCORES = 13,
	// 14 unused
	DIALOG_REGISTER_NAG = 15,           // ShowRegisterNagDialog 0x4d5cdf (not ported)
	DIALOG_CONFIRM_LOAD = 16,
	DIALOG_CHECK_UPDATES = 17,          // never created; its YES ran DoUpdateCheck 0x4d68da (not ported)
	DIALOG_UPDATE_CHECK = 18,           // UpdateCheckDialog (not ported)
	DIALOG_ALREADY_REGISTERED = 19,
	DIALOG_UPDATE_AVAILABLE = 10018,    // UpdateCheckDialog result (mId+10000, YES_NO)
	DIALOG_UP_TO_DATE = 20018           // UpdateCheckDialog result (mId+20000, FOOTER)
};

// Side-bar ButtonWidget ids (WinDMApp is their ButtonListener).
enum ButtonId
{
	BUTTON_NEW_GAME = 10,
	BUTTON_NORMAL_MODE = 11,
	BUTTON_TIMED_MODE = 12,
	BUTTON_OPTIONS = 13,
	BUTTON_QUIT = 14,
	BUTTON_HINT = 15
};

class WinDMApp : public SexyApp
{
public:
	enum
	{
		NUM_SHRINK_FRAMES = 6,
		NUM_TIMERBAR_IMAGES = 25,       // 24 hues + white
		NUM_LOADING_TASKS = 60          // mNumLoadingThreadTasks: 29 images + 21 sounds + 2 + 7 + 1
	};

	// +0x721..+0x727 SexyApp tail padding
	// data\diff.txt, read in this order by LoadingThreadProc (operator>>, no error check):
	int				mNormalPointsBase;          // +0x728 500  normal mLevelPoints = (int)((PerLevel*L + Base) * mult)
	// +0x72c (padding)
	double			mNormalPointsPerLevel;      // +0x730 150
	double			mNormalMultBase;            // +0x738 1.0  normal mPointMultiplier = PerLevel*L + Base
	double			mNormalMultPerLevel;        // +0x740 0.5
	int				mTimedPointsBase;           // +0x748 250  timed mLevelPoints
	// +0x74c (padding)
	double			mTimedPointsPerLevel;       // +0x750 10
	double			mTimedMultBase;             // +0x758 2.0  timed mPointMultiplier
	double			mTimedMultPerLevel;         // +0x760 0.5
	double			mTimeDrainBase;             // +0x768 1.0  Board::mTimeDrainRate = (PerLevel*L + Base) * mult
	double			mTimeDrainPerLevel;         // +0x770 2.0
	double			mTimeDrainAccelBase;        // +0x778 0.01 Board::mTimeDrainAccel = (PerLevel*L + Base) * mult
	double			mTimeDrainAccelPerLevel;    // +0x780 0.005
	Board*			mBoard;                     // +0x788 current board widget (NewGame); kept while the HighScoreWidget shows
	TitleScreen*	mTitleScreen;               // +0x78c title screen until TitleScreenIsFinished
	Image**			mImages;                    // +0x790 new Image*[NUM_IMAGES] indexed by ImageId (DDImages from GetImage)
	MemoryImage***	mGemShrinkImages;           // +0x794 [7 colors][6 frames] 49x49 shrinking-gem images (new [7] of new [6])
	HighScoreWidget* mHighScoreWidget;          // +0x798 game-over screen (ShowHighScoreWidget), removed by NewGame
	SexyString		mLastScoreName;             // +0x79c registry LastScoreName; prefills RecordScoreDialog
	std::list<ButtonWidget*> mButtonList;       // +0x7b8 side-bar buttons made by MakeButton (pad, _Myhead +0x7bc, _Mysize +0x7c0)
	ImageFont*		mAlphaDigitsFont;           // +0x7c4 alphadigits4 (FONT_ALPHADIGITS)
	ImageFont*		mScoreFont;                 // +0x7c8 scorefont1 (FONT_SCOREFONT); never deleted in the original
	ImageFont*		mHeaderFont;                // +0x7cc afont1 (FONT_HEADER)
	ImageFont*		mMainFont;                  // +0x7d0 mainfont1 (FONT_MAIN)
	ImageFont*		mOverFont;                  // +0x7d4 overfont2 (FONT_OVERFONT)
	Image*			mUnusedImagesA[6];          // +0x7d8 zeroed by the ctor, deleted by the dtor, otherwise unused
	Image*			mUnusedImagesB[6];          // +0x7f0 same
	ButtonWidget*	mNormalModeButton;          // +0x808 button 11; mButtonImage = IMAGE_OVER_NORMAL_SELECTED while normal mode
	ButtonWidget*	mTimedModeButton;           // +0x80c button 12; mButtonImage = IMAGE_OVER_TIMETRIAL_SELECTED while timed mode
	DDImage*		mTimerBarImages[NUM_TIMERBAR_IMAGES]; // +0x810 colorized timerbar: [k] hue HSL(85*k/23,255,128), [24] white
	bool			mTimedMode;                 // +0x874 current mode (Board ctor arg; Pause/Unpause 'user' flag on focus/modal changes)
	bool			mShowInstructions;          // +0x875 ctor true; first scoring swap / LoadGame clear it; gates the tick-150 instructions dialog
	bool			mNoPopLink;                 // +0x876 registry NoPopLink, read only, never used
	bool			mIsDiamondMine;             // +0x877 property IsDiamondMine (PreDisplayHook): Diamond Mine strings

public:
	WinDMApp();                                                     // 0x4d4d82
	virtual ~WinDMApp();                                            // 0x4d4b08 (scalar deleting dtor 0x4d4d0e)

	virtual void	Shutdown();                                     // 0x4d43f9 slot 42
	void			NewGame();                                      // 0x4d4455
	virtual void	LoadingThreadCompleted();                       // 0x4d4591 slot 16 (empty)
	virtual void	URLOpenSucceeded(const std::string& theURL);    // 0x4d4592 slot 36
	virtual MusicInterface* CreateMusicInterface();                 // 0x4d45b1 slot 18 (original takes the HWND; always BassMusicInterface, port: SDLMusicInterface)
	void			ShowRecordScoreDialog(const SexyString& theLines); // 0x4d45e3
	virtual Dialog*	NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode); // 0x4d4653 slot 25
	virtual void	ModalOpen();                                    // 0x4d46fc slot 79 (same body as LostFocus)
	virtual void	LostFocus();                                    // 0x4d46fc slot 82
	virtual void	ModalClose();                                   // 0x4d4716 slot 80 (same body as GotFocus)
	virtual void	GotFocus();                                     // 0x4d4716 slot 81
	void			SetTimedMode(bool isTimed, bool startNewGame);  // 0x4d4730
	void			ShowHighScoreWidget();                          // 0x4d47b1
	void			ShowOptionsDialog();                            // 0x4d483e
	virtual void	ButtonPress(int theId);                         // 0x4d48a9 slot 1 (PlaySample(SOUND_MENUCLICK))
	void			ShowHighScoreDialog();                          // 0x4d48b6
	virtual void	UpdateFrames();                                 // 0x4d4921 slot 8 (jmp SexyApp::UpdateFrames)
	virtual void	SetMusicVolume(double theVolume);               // 0x4d4926 slot 60 (forwards to the base)
	virtual bool	KillDialog(int theDialogId);                    // 0x4d4a0e slot 76
	virtual std::string GetGameSEHInfo();                           // 0x4d4ad9 slot 40 (forwards to the base)
	virtual void	URLOpenFailed(const std::string& theURL);       // 0x4d4e97 slot 35
	virtual bool	OpenURL(const std::string& theURL, bool shutdownOnOpen = false); // 0x4d5077 slot 37
	virtual void	WriteToRegistry();                              // 0x4d5174 slot 23
	virtual void	ReadFromRegistry();                             // 0x4d51bc slot 24
	void			ShowRegisterDialog();                           // 0x4d5272 (registration: port shows only the 'already registered' branch)
	void			ConfirmLoadGame();                              // 0x4d550d
	void			ConfirmNewGame();                               // 0x4d5610
	void			ConfirmNormalMode();                            // 0x4d56ec
	void			ConfirmTimedMode();                             // 0x4d57cd
	void			ConfirmQuit();                                  // 0x4d58af
	void			ShowInstructionsDialog();                       // 0x4d59cc
	void			ShowFullInstructionsDialog();                   // 0x4d5ac1
	void			ShowColorDepthDialog();                         // 0x4d5bef
	bool			LoadAppImage(int theImageId);                   // 0x4d62b8
	virtual void	PreDisplayHook();                               // 0x4d6399 slot 26 (does not call SexyApp::PreDisplayHook)
	virtual void	Init();                                         // 0x4d649e slot 49
	virtual void	ButtonDepress(int theId);                       // 0x4d6a40 slot 2
	ButtonWidget*	MakeButton(int theId, const Rect& theRect, int theOverImageId, int theDownImageId); // 0x4d6e71
	void			TitleScreenIsFinished();                        // 0x4d6f28
	virtual void	LoadingThreadProc();                            // 0x4d82d2 slot 22

	// port (web): the page can be closed or crash without Shutdown(); see SexyAppBase::UpdateUserDataBackup
	virtual bool	GetShutdownSave(std::string* theFileName, Buffer* theBuffer);
	virtual void	WriteShutdownRegistry();

	using SexyApp::ButtonPress;
	using SexyApp::KillDialog;
};

}

#endif
