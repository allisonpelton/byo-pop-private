#ifndef __ASTROPOP_ASTROPOPAPP_H__
#define __ASTROPOP_ASTROPOPAPP_H__

// AstroPopApp: the application class of AstroPop Deluxe (original constructor 0x427ee0, vftable 0x5629a0 with 68
// slots like BookWorm's, scalar deleting destructor 0x42dc30 in slot 14, sizeof SexyApp 0x600 so the game's own data
// starts at +0x600). Offsets in the comments are the original's (the port's base classes have other sizes).
// The 2004 SexyApp keeps its trial counters at +0x514 / +0x518; registration is out of the port (the game always
// runs as a registered copy), see docs/GAME_NOTES.md "Registration and outbound code left out".

#include "Color.h"
#include "Res.h"
#include "SexyApp.h"
#include "UserProfile.h"
#include <string>
#include <vector>

namespace Sexy
{

class LoaderScreen;
class MainMenu;
class XcfFile;
class Widget;
class MemoryImage;
class SoundInstance;
class Font;
class Image;
class Dialog;
class MusicInterface;
class Board;

class AstroPopApp;
extern AstroPopApp* gApp;   // 0x5dc220 (set by WinMain; the port: by the constructor)

// Sin and cos of whole degrees, filled by the constructor (the original keeps them in .bss at 0x5a42e8 / 0x5a4888)
extern float gSinTable[360];
extern float gCosTable[360];

// An entry of the hall of fame (0x2c bytes): kind 1 is a result of a game by its score (the second number is the level
// reached), kind 2 a result of a timed game by the time played in seconds (the second number)
struct HallEntry
{
	std::string	mName;      // +0x00
	int			mScore;     // +0x1c
	int			mSecond;    // +0x20
	int			mMode;      // +0x24
	int			mKind;      // +0x28
};

class AstroPopApp : public SexyApp
{
public:
	enum
	{
		NUM_BONUS_LEVELS = 30,                      // data\bonus<n>.xcf
		NUM_MUSIC_SLOTS = 2,                        // music.mo3 is loaded twice: the music crossfades between two slots
		NUM_LOOP_SOUNDS = 8                         // sounds 0x16a .. 0x171 (SOUND_CLASSIC_GAME .. SOUND_KLAXON)
	};

	std::vector<std::string>	mGoodbyeMessages;   // +0x608 the quit dialog's ten lines (constructor)
	std::vector<SoundInstance*>	mLoopSounds;       // +0x618 eight entries, set when the loading thread ends
	std::vector<int>			mPausedSongs;       // +0x630 music slots paused by LostFocus, resumed by GotFocus
	Board*						mBoard;             // +0x628 the running game (class Board, 0x1c0c bytes), NULL for none
	MainMenu*					mMainMenu;          // +0x644 the game selector (ShowMainMenu)
	LoaderScreen*				mLoaderScreen;      // +0x640 Init .. the title (the original ctor leaves it uninitialised)
	Widget*						mScreen64c;         // +0x64c the screen shown over the selector (pilot choice, hall of fame ...); ShowMainMenu removes it
	bool						mFlag658;           // +0x658 ValidateUser: profile setting 0x11 (1 when missing) is not 0
	bool						mOldSavesDeleted;   // +0x659 Init found a profile of an older version and deleted it
	bool						mOldScoresDeleted;  // +0x65a Init found a high-score file of an older version
	bool						mFlag65b = false;   // +0x65b set by the Board once a game has lasted 6000 updates
	int							mTimer664;          // +0x664 counted down by UpdateFrames
	int							mGameUpdates;       // +0x668 counted by the Board's Update while a game runs
	Font*						mDialogFont;        // +0x66c FONT_DIALOG_BOX, set when the loading thread ends
	MemoryImage*				mGradient[3];       // +0x674 +0x678 +0x67c 1 x 480 white, grey and dark ramps
	std::vector<std::string>	mUserNames;         // +0x684 the names under the registry key Users
	UserProfile*				mUser;              // +0x690 the current player (NULL for none)
	SoundInstance*				mEngineLoop;        // +0x694 SOUND_ENGINE_LOOP
	SoundInstance*				mHyperspaceLoop;    // +0x698 SOUND_HYPERSPACE_LOOP
	MemoryImage*				mLightningImage;    // +0x6a0 IMAGE_WARP_FX_LIGHTENING, flipped copy
	MemoryImage*				mLightningImage2;   // +0x6a4 the same, drawn as it is
	XcfFile*					mDescriptor;        // +0x6a8 data\descriptor.xcf: the level settings (LoadDescriptor)
	std::vector<HallEntry>		mHall;              // +0x6c4 the hall of fame (0x2c bytes an entry), kept in users\datahs.usr
	bool						mFlag6f9;           // +0x6f9 ValidateUser: profile setting 0x13 (1 when missing) is not 0
	bool						mFlag6fa;           // +0x6fa ValidateUser: profile setting 0x72 is 1
	bool						mFlag6fb;           // +0x6fb ValidateUser: profile setting 0x73 (1 when missing) is 1
	bool						mFlag6fc;           // +0x6fc (set by the help screen; read by the Board's Update)
	Widget*						mHelpScreen;        // +0x69c the full-screen help (ShowFullScreenHelp), NULL for none
	int							mMusicSlot;         // +0x6d8 the slot that plays (0 or 1)
	int							mMusicOrder;        // +0x6dc the song position (order) that plays, -1 for none

public:
	AstroPopApp();                                              // 0x427ee0
	virtual ~AstroPopApp();                                     // 0x42dc30 slot 14

	virtual void	ButtonPress(int theId) override;            // 0x408e90 slot 3 (plays SOUND_BUTTON_DOWN)
	virtual void	ButtonDepress(int theId) override;          // 0x431ac0 slot 4
	virtual void	UpdateFrames() override;                    // 0x408b20 slot 7
	virtual void	LoadingThreadCompleted() override;          // 0x4242d0 slot 13
	virtual void	Init() override;                            // 0x42f170 slot 16
	virtual void	LoadingThreadProc() override;               // 0x42cda0 slot 17
	virtual void	Shutdown() override;                        // 0x4290d0 slot 33
	virtual void	HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) override; // 0x40e060 slot 35
	virtual void	SwitchScreenMode(bool wantWindowed) override; // 0x4090d0 slot 53
	virtual Dialog*	DoDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
						const SexyString& theDialogLines, const SexyString& theDialogFooter,
						int theButtonMode) override;            // 0x412360 slot 55
	virtual void	GotFocus() override;                        // 0x4253e0 slot 62
	virtual void	LostFocus() override;                       // 0x425380 slot 63
	virtual void	SetMusicVolumeFloat(float theVolume);       // 0x408b70 slot 67 (a new virtual of the game)

	// port: the 2004 SexyApp makes a BassMusicInterface itself (slot 15, 0x408e30); here an SDLMusicInterface
	// with BASS 2.0's music gain (docs/FRAMEWORK_2004.md section 6)
	virtual MusicInterface* CreateMusicInterface() override;
	// port: the game's texts are read from the player's own exe (GameText.h) before anything shows text
	virtual void	InitPropertiesHook() override;

	void			PlaySong(int theOrder);                     // 0x408b90
	void			PlaySampleAt(int theSound, int theColumn);  // 0x408cc0 a sample panned by the column it happens in
	void			PauseMusicForFocus(bool lost);              // 0x4252b0
	void			SetLoopSoundsMuted(bool muted);             // 0x408db0
	std::string		GetRankName(int theScore, int* theIndex);   // 0x40f110 the name of the rank of a score (and its number)
	int				PointsToNextRank(int theScore);             // 0x409210 -1 at the highest rank
	MemoryImage*	MakeRankImage(int theRank, int thePercent); // 0x40c000 the picture of a rank, scaled to thePercent (above 0)
	Color			GetDialogColor(int theSet);                 // 0x409110 the colour of a dialog set (0 .. 5): the tint of its pictures
	void			MakeDialogImages(const Color& theInsetColor, const Color& theButtonColor, MemoryImage** theInsetImage,
						MemoryImage** theButtonImage, bool theHelp);  // 0x408f00 the dialog's inset and button pictures in two colours
	void			ListUsers();                                // 0x4254b0 mUserNames from the registry key Users
	void			LoadHighScores();                           // 0x42a0b0 users\datahs.usr

	void			OnClickToPlay();                            // 0x4319b0 the title's button: the loader goes, the profile comes
	void			LoadDescriptor();                           // 0x42c150
	void			SelectStartUser();                          // 0x4300c0 the last player, or the first, or the new-player dialog
	std::string		GetLastUserName();                          // 0x411880
	void			ResetFailedUserLoad();                      // 0x4298c0 a profile that could not be read starts again
	void			ValidateUser();                             // 0x4285d0 the profile's settings checked and copied to the flags
	void			ShowMainMenu();                             // 0x411490
	void			CloseBoard();                               // 0x412320 the game goes (the Board is deleted when it is safe), the selector stays
	void			ShowFullScreenHelp();                       // 0x422db0 (stub)
	// port: the original keeps the left button's state in the application (+0x4e1), set by its mouse dispatcher; the
	// widget manager's record of the buttons held for a widget is the same while the Board takes the clicks
	bool			IsLeftButtonDown();
	void			ShowNewUserDialog();                        // 0x40bc00 (stub)
	void			ShowMessageDialog(const std::string& theHeader, const std::string& theText); // 0x411ba0 (stub): an OK box
	Dialog*			MakeDialog(int theId, bool theFlag, int theWidth, int theX, int theY, int theButtons, const std::string& theHeader, const std::string& theText, const std::string& theYes, const std::string& theNo, bool theModal);   // 0x411ba0 (stub)
	// The screens the selector opens. Not translated yet: each one logs "not translated" and does nothing.
	void			StartGame(bool theContinue);                // 0x428c70 (the Board)
	void			ShowCharSelector(int theMode);              // 0x411660 (pilot choice)
	void			ShowHallOfFame();                           // 0x4288a0
	bool			AddHallOfFame(const std::string& theName, int theScore, int theSecond, int theMode, int theKind); // 0x429a40
	void			ShowQuitDialog();                           // 0x422c00
	void			ShowUserSelect();                           // 0x422870
	void			ShowOptions(bool theFromGame);              // 0x40bd70
	void			ShowContinueDialog(const std::string& thePilot, int theLevel); // 0x411ba0 with the CONTINUE? texts

	// port: the standard test commands (docs/TEST_COMMANDS.md), src/testhooks/TestHooks.cpp
	virtual bool	HandleScriptCommand(const ScriptCommand& theCommand) override;

	using SexyApp::SwitchScreenMode;

private:
	bool			LoadBonusLevels();                          // 0x42c910
};

}

#endif
