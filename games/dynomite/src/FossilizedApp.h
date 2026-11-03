#ifndef __FOSSILIZEDAPP_H__
#define __FOSSILIZEDAPP_H__

// The application of Dynomite Deluxe (FossilizedApp, vtable 0x4fa410; docs/GAME_NOTES.md section 3). The original's
// SexyApp is one class of 61 slots (docs/FRAMEWORK_2003.md); the port's derives from the framework's SexyApp. Every picture,
// cell, font and sound is a field, with the original's offset in its comment (a translated function that reads
// app+0x1298 finds its field by that offset).

#include "FibRand.h"
#include "MTRand.h"
#include "SexyApp.h"
#include "Sprite.h"

namespace Sexy
{

class DDImage;
class ImageFont;
class SoundInstance;
class Dynomite;
class HighScores;
class AngleTable;
class IntroScreen;
class SideBar;
class TitleScreen;
class Tutorial;

class FossilizedApp : public SexyApp
{
public:
	DDImage*     mImgSidebar;                             // +0x10d4
	Pic*         mPicLargedbutton;                        // +0x1228
	Pic*         mPicMinidialog;                          // +0x1250
	Pic*         mPicScrollbuttons;                       // +0x1278
	Cell*        mPicScrollbuttons_c0;                    // +0x127c
	Cell*        mPicScrollbuttons_c1;                    // +0x1280
	Pic*         mPicIntroscreen;                         // +0x1284
	Pic*         mPicWizardgraphics;                      // +0x128c
	Cell*        mPicWizardgraphics_r1290;                // +0x1290
	Cell*        mPicWizardgraphics_c8;                   // +0x1294
	Cell*        mPicWizardgraphics_c7;                   // +0x1298
	Cell*        mPicWizardgraphics_c0;                   // +0x129c
	Cell*        mPicWizardgraphics_c1;                   // +0x12a0
	Cell*        mPicWizardgraphics_c2;                   // +0x12a4
	Cell*        mPicWizardgraphics_c3;                   // +0x12a8
	Cell*        mPicWizardgraphics_c4;                   // +0x12ac
	Cell*        mPicWizardgraphics_c5;                   // +0x12b0
	Cell*        mPicWizardgraphics_c6;                   // +0x12b4
	Cell*        mPicWizardgraphics_r12b8;                // +0x12b8
	Cell*        mPicWizardgraphics_r12bc;                // +0x12bc
	Cell*        mPicWizardgraphics_r12c0;                // +0x12c0
	Cell*        mPicWizardgraphics_r12c4;                // +0x12c4
	Cell*        mPicWizardgraphics_r12c8;                // +0x12c8
	Cell*        mPicWizardgraphics_r12cc;                // +0x12cc
	Cell*        mPicWizardgraphics_r12d0;                // +0x12d0
	Pic*         mPicInchylarge;                          // +0x12d4
	Cell*        mInchyLargeCells[2];                        // +0x12d8 inchylarge_c0..c1
	Pic*         mPicInchylargemouth;                     // +0x12e0
	Cell*        mInchyLargeMouthCells[4];                   // +0x12e4 inchylargemouth_c0..c3
	Pic*         mPicWelcomescreen;                       // +0x12f4
	Cell*        mPicWelcomescreen_r12f8;                 // +0x12f8
	Cell*        mPicWelcomescreen_r12fc;                 // +0x12fc
	Cell*        mPicWelcomescreen_r1300;                 // +0x1300
	Cell*        mPicWelcomescreen_r1304;                 // +0x1304
	Pic*         mPicCheckboxunchecked;                   // +0x1308
	Pic*         mPicCheckboxchecked;                     // +0x130c
	Pic*         mPicSlider;                              // +0x1310
	Pic*         mPicThumb;                               // +0x1314
	Pic*         mPicInchysmallsheet;                     // +0x1318
	Pic*         mPicInchysmallblink;                     // +0x131c
	Cell*        mInchyCells[3];                             // +0x1320 inchysmallsheet_c0..c2
	Pic*         mPicInchysmallmouthsheet;                // +0x132c
	Cell*        mInchyMouthCells[4];                        // +0x1330 inchysmallmouthsheet_c0..c3
	Pic*         mPicSidebarbuttons;                      // +0x1340
	Cell*        mPicSidebarbuttons_r1344;                // +0x1344
	Cell*        mPicSidebarbuttons_r1348;                // +0x1348
	Cell*        mPicSidebarbuttons_r134c;                // +0x134c
	Cell*        mPicSidebarbuttons_r1350;                // +0x1350
	Cell*        mPicSidebarbuttons_r1354;                // +0x1354
	Cell*        mPicSidebarbuttons_r1358;                // +0x1358
	Pic*         mPicSkillsheet;                          // +0x135c
	Cell*        mSkillCells[7];                             // +0x1360 skillsheet_c0..c6
	Pic*         mPicPausebuttons;                        // +0x137c
	Cell*        mPauseCells[6];                             // +0x1380 pausebuttons_c0..c5
	Cell*        mPicPausebuttons_r1398;                  // +0x1398
	Cell*        mPicPausebuttons_r139c;                  // +0x139c
	Pic*         mPicLeftwall;                            // +0x13a0
	Pic*         mPicRightwall;                           // +0x13a4
	Pic*         mPicMenuoverlap;                         // +0x13a8
	Pic*         mPicJungleback;                          // +0x13b0
	Pic*         mPicRowstogo;                            // +0x13b4
	Pic*         mPicEggsheet;                            // +0x13b8
	Pic*         mPicBomb;                                // +0x13bc
	Cell*        mPicBomb_c0;                             // +0x13c0
	Pic*         mPicKroneysheet;                         // +0x1574
	Pic*         mPicKroneyfacesheet;                     // +0x1578
	Pic*         mPicKroneyhourglasssheet;                // +0x157c
	Pic*         mPicKroneyflipsheet;                     // +0x1580
	Pic*         mPicKroneysandtopsheet;                  // +0x1584
	Pic*         mPicKroneysandbottomsheet;               // +0x1588
	Pic*         mPicEggpilesheet;                        // +0x1684
	Cell*        mPicEggpilesheet_c0;                     // +0x1688
	Cell*        mPicEggpilesheet_c1;                     // +0x168c
	Pic*         mPicRope;                                // +0x1690
	Pic*         mPicSlingshot;                           // +0x1694
	Pic*         mPicRubberband;                          // +0x1698
	Pic*         mPicYolksheet;                           // +0x169c
	Cell*        mYolkCells[2];                              // +0x16a0 yolksheet_c0..c1
	Pic*         mPicEggwhite;                            // +0x16a8
	Pic*         mPicBoomsheet;                           // +0x16ac
	Pic*         mPicSparksheet;                          // +0x16ec
	Pic*         mPicDynamitesheet;                       // +0x16fc
	Pic*         mPicSteggy;                              // +0x1718
	Pic*         mPicTanksheet;                           // +0x171c
	Cell*        mPicTanksheet_c0;                        // +0x1720
	Cell*        mPicTanksheet_c1;                        // +0x1724
	Cell*        mPicTanksheet_c2;                        // +0x1728
	Pic*         mPicSteggyterror;                        // +0x172c
	Pic*         mPicTankterror;                          // +0x1730
	Pic*         mPicTanksweat;                           // +0x1734
	Pic*         mPicTankeyesheet;                        // +0x1738
	Pic*         mPicTankmouthsheet;                      // +0x174c
	Pic*         mPicWhirleysheet;                        // +0x1760
	Pic*         mPicHandsheet;                           // +0x17c4
	Pic*         mPicScorefont;                           // +0x17ec
	Pic*         mPicBabysheet;                           // +0x181c
	Cell*        mBabyCells[3];                              // +0x1820 babysheet_c0..c2
	Pic*         mPicBonuseggsheet;                       // +0x182c
	Pic*         mPicBonusviewsheet;                      // +0x185c
	Pic*         mPicWowstarsheet;                        // +0x188c
	Pic*         mPicAimx;                                // +0x18a4
	Pic*         mPicFoot;                                // +0x18a8
	Pic*         mPicCeiling;                             // +0x18ac
	Pic*         mPicGameover;                            // +0x18b0
	Pic*         mPicCleared;                             // +0x18b4
	Pic*         mPicTextballoons;                        // +0x18b8
	Cell*        mPicTextballoons_c0;                     // +0x18bc
	Cell*        mPicTextballoons_c1;                     // +0x18c0
	Cell*        mPicTextballoons_c2;                     // +0x18c4
	Pic*         mPicTextwords;                           // +0x18c8
	Cell*        mPicTextwords_c0;                        // +0x18cc
	Cell*        mPicTextwords_c5;                        // +0x18d0
	Cell*        mPicTextwords_c1;                        // +0x18d4
	Cell*        mPicTextwords_c2;                        // +0x18d8
	Cell*        mPicTextwords_c3;                        // +0x18dc
	Cell*        mPicTextwords_c4;                        // +0x18e0
	Pic*         mPicCombotext;                           // +0x18e4
	Cell*        mComboTextCells[8];                         // +0x18e8 combotext_c0..c7
	Pic*         mPicAlert;                               // +0x1908
	Pic*         mPicBolt;                                // +0x190c
	Pic*         mPicWhirleyalarm;                        // +0x1910
	Pic*         mPicMamaportrait;                        // +0x1914
	Pic*         mPicMamameter;                           // +0x1918
	Cell*        mPicMamameter_c0;                        // +0x191c
	Cell*        mPicMamameter_c1;                        // +0x1920
	Pic*         mPicThermoglow;                          // +0x1924
	GameSound*   mSndBreakegg;                            // +0x19f0
	GameSound*   mSndExplosion;                           // +0x19f4
	GameSound*   mSndAooga;                               // +0x19fc
	GameSound*   mSndBlastit;                             // +0x1a00
	GameSound*   mSndCharge;                              // +0x1a04
	GameSound*   mSndKaboom;                              // +0x1a08
	GameSound*   mSndFireinthehole;                       // +0x1a0c
	GameSound*   mSndRubbershot;                          // +0x1a10
	GameSound*   mSndWhirleyhit;                          // +0x1a14
	GameSound*   mSndMama1;                               // +0x1a18
	GameSound*   mSndMama1_1a1c;                          // +0x1a1c
	GameSound*   mSndPopbonus;                            // +0x1a20
	GameSound*   mSndGrind;                               // +0x1a24
	GameSound*   mSndXwhirleyscoming1;                    // +0x1a2c
	GameSound*   mSndXwhirleyscoming2;                    // +0x1a30
	GameSound*   mSndXtimesrunningout;                    // +0x1a34
	GameSound*   mSndScoremultiplier;                     // +0x1a38
	GameSound*   mSndFootlands;                           // +0x1a3c
	GameSound*   mSndXwelcometodynomite;                  // +0x1a44
	GameSound*   mSndXgameover;                           // +0x1a48
	GameSound*   mSndInchypop;                            // +0x1a4c
	GameSound*   mSndXcombo;                              // +0x1a54
	GameSound*   mSndXsupercombo;                         // +0x1a58
	GameSound*   mSndXcambriancombo;                      // +0x1a5c
	GameSound*   mSndXtriassiccombo;                      // +0x1a60
	GameSound*   mSndXjurassiccombo;                      // +0x1a64
	GameSound*   mSndXcretaceouscombo;                    // +0x1a68
	GameSound*   mSndXdynamitecombo;                      // +0x1a6c
	GameSound*   mSndThunderbolt;                         // +0x1a78
	GameSound*   mSndAlert;                               // +0x1a7c
	GameSound*   mSndReward;                              // +0x1a80
	GameSound*   mSndWhirleysiren;                        // +0x1a84
	GameSound*   mSndWarning;                             // +0x1a88
	GameSound*   mSndFootfallrumble;                      // +0x1a8c
	GameSound*   mSndFootfall;                            // +0x1a94
	GameSound*   mSndChime;                               // +0x1a98
	GameSound*   mSndTicktock;                            // +0x1a9c
	GameSound*   mSndFossil;                              // +0x1aa4
	GameSound*   mSndSpeedup;                             // +0x1aa8
	// arrays of cells made by the loader's loops (offset: first element)
	Cell*        mWhirleyCells[24];                       // +0x1764 whirleysheet 60x60 (two loops: i and i + 12)
	Cell*        mDialogCells[9];                         // +0x122c dialog 48x48
	Cell*        mMiniDialogCells[9];                     // +0x1254 minidialog 10x10
	Cell*        mEggCells[20];                           // +0x13c4 EggSheet 35x39
	Pic*         mEggSheetCracked[4];                     // +0x1414 EggSheet with crackmask1..4 multiplied in
	Cell*        mCrackedEggCells[20][4];                 // +0x1434 (stride 0x10) the cracked sheets' cells
	Cell*        mKroneyCells[8];                         // +0x158c kroneysheet 82x130
	Cell*        mKroneyFaceCells[16];                    // +0x15ac kroneyfacesheet 29x43 (two loops: i and i + 8)
	Cell*        mKroneyHourglassCells[8];                // +0x15ec kroneyhourglasssheet 42x66
	Cell*        mKroneyFlipCells[14];                    // +0x160c kroneyflipsheet 154x196
	Cell*        mKroneySandTopCells[8];                  // +0x1644 kroneysandtopsheet 42x31
	Cell*        mKroneySandBottomCells[8];               // +0x1664 kroneysandBottomsheet 42x36
	Cell*        mBoomCells[15];                          // +0x16b0 boomsheet 88x123
	Cell*        mSparkCells[3];                          // +0x16f0 sparksheet 11x11
	Cell*        mDynamiteCells[3];                       // +0x1700 dynamitesheet 48x48
	Cell*        mTankEyeCells[4];                        // +0x173c tankeyesheet 14x13
	Cell*        mTankMouthCells[4];                      // +0x1750 tankmouthsheet 17x19
	Cell*        mHandCells[9];                           // +0x17c8 handsheet 81x118
	Cell*        mScoreDigitCells[11];                    // +0x17f0 scorefont 17x39
	Cell*        mBonusEggCells[10];                      // +0x1834 bonus(egg)sheet 26x29
	Cell*        mBonusViewCells[10];                     // +0x1864 bonus(view)sheet 33x34
	Cell*        mWowStarCells[5];                        // +0x1890 wowstarsheet 12x11
	PicFont*     mWizardFont;                             // +0x1288 images\WizardFont, A-Z, space 5
	PicFont*     mAnnounceFont;                           // +0x192c images\AnnounceFont, advances -4, space 5
	PicFont*     mTinyFont;                               // +0x1930 images\TinyFont, space 2
	MaskedPic*   mFossils[20];                            // +0x1934 images\fossils\<name> (15 used)
	MaskedPic*   mFossilCopies[20];                       // +0x1984 copies of the fossils
	ImageFont*   mWhiteFont;                              // +0x19d4 data\whitefont.txt
	ImageFont*   mWhiteFontSmall;                         // +0x19d8 data\whitefontsmall.txt
	ImageFont*   mWhiteFont2;                             // +0x19dc data\WhiteFont2.txt
	TitleScreen* mTitleScreen;                            // +0x19e0
	bool         mSoundInitialized;                       // +0x10b8
	SoundInstance* mGrindInstance;                         // +0x1a28
	SoundInstance* mSound1a50;                             // +0x1a50
	SoundInstance* mFootfallRumbleInstance;                // +0x1a90
	SoundInstance* mTicktockInstance;                      // +0x1aa0
	PicFont*     mAnnounceFont1928;                       // +0x1928 the same font as +0x192c

	AngleTable*  mAngles;                                 // +0x5d4
	MTRand       mRand;                                   // +0x5d8 seeded with the tick count
	FibRand      mPicker;                                 // +0xf9c (the board seeds it)
	int          mSkill;                                  // +0x1084 1 normal, 2 hard, 3 expert
	int          mFossilKind;                             // +0x1088 the fossil challenge: 0 strategic, 1 panic
	int          mGameMode;                               // +0x108c 1 stomped, 2 endless, 3 fossil, 4 time trial
	int          mField1090;                              // +0x1090 the puzzle list's chosen pack
	bool         mInTutorial;                             // +0x1094 (the menu button goes straight to the main menu)
	int          mField4d8;                               // +0x4d8 the framework's: games that lasted a minute (the trial's screens compare it)
	bool         mLockCursor;                             // +0x1095 registry LockCursor: the board keeps the cursor in a window
	std::string  mLastScoreName;                          // +0x1098 registry LastScoreName: the name last given a high score
	Tutorial*    mTutor;                                  // +0x10bc the tutorial (StartTutorial), taken down by the main menu
	Pic*         mSavePic;                                // +0x10ec SaveGame's picture of the board, 60 x 60
	char         mSaveMode[0x14];                         // +0x10f0 the saved game's mode (GetSaveNameDialog)
	char         mSaveName[0x14];                         // +0x1104 and its name (SAVED_GAME, or the slot's)
	char         mPackFile[0x100];                        // +0x1118 the Stomped Puzzle pack (DEFAULT.EGG, STOMPED\...)
	unsigned int mPuzzleSkills;                           // +0x1218 the skills the pack has (bits 1, 2, 4), -1 all
	SideBar*     mSideBar;                                // +0x19e4
	IntroScreen* mIntroScreen;                            // +0x19e8 the main menu
	Dynomite*    mBoard;                                  // +0x19ec

	// set in Init (0x41f820)
	DDImage*     mImgTitle;                               // +0x10c0 images\Title
	DDImage*     mImgGreenbar;                            // +0x10c4 images\greenbar
	Pic*         mPicCursor1;                             // +0x10d8
	Pic*         mPicPointer2;                            // +0x10dc
	Pic*         mPicOverhand;                            // +0x10e0
	Pic*         mPicTyper;                               // +0x10e4
	Pic*         mPicUnregistered;                        // +0x10e8
	Pic*         mPicDialog;                              // +0x121c
	Pic*         mPicDbutton;                             // +0x1220
	Pic*         mPicWaitbar;                             // +0x1224
	GameSound*   mSndEgglands;                            // +0x19f8
	GameSound*   mSndClick;                               // +0x1a40

	FossilizedApp();											// 0x4198b0
	virtual ~FossilizedApp();									// 0x419320

	virtual void	Init() override;							// 0x41f820 (slot 14)
	virtual void	LoadingThreadProc() override;				// slot 15 runs LoadGame
	virtual void	ReadFromRegistry() override;				// 0x41e250
	virtual void	WriteToRegistry() override;					// 0x41e420
	// 0x4201b0 (slot 2): the app's own buttons (5 plays again, 100 and 101 the high-score name's OK and Cancel, 500-701
	// the save dialog's) and the dialogs' yes (2000 + id) and no (3000 + id)
	virtual void	ButtonDepress(int theId) override;
	// 0x418e90 (slot 1): a save slot (600-699) is chosen, a saved game's slot on the main menu (900-999) too and a second
	// press on it is OK; any other press clicks
	virtual void	ButtonPress(int theId) override;
	virtual void	ModalOpen() override;						// 0x418b60 (slot 56): a modal dialog pauses the side bar
	virtual void	ModalClose() override;					// 0x418b80 (slot 57): and resumes it

	DDImage*		GetGameImage(const std::string& thePath);	// 0x419ad0
	void			LoadGame();									// 0x419c30 (InitHook, slot 15)
	virtual Dialog*	NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
		const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode) override;	// 0x418a00

	void			ShowMainMenu();								// 0x4186d0
	void			StartGame();								// 0x418810
	void			ShowBuyDialog();							// 0x418470: dialog 21 (BUY_AND_UNLOCK_DYNOMITE)
	void			ShowRegisterDialog(const char* theLines);	// 0x4185a0: dialog 14 (REGISTRATION)
	void			ShowHighScoreDialog(HighScores* theHighScores);	// 0x418df0: dialog 6, the name for a high score
	void			MenuButton();								// 0x41e770: the side bar's menu button
	void			ShowOptions();								// 0x418ba0: dialog 2
	void			ShowHighScores();							// 0x418cd0: dialog 4, the options' high scores
	void			ShowCredits();								// 0x418d60: dialog 22, the options' credits
	void			ShowNoWindowed();							// 0x41e940: dialog 3, NO_WINDOWED
	void			ConfirmExit();								// 0x41e590
	void			SaveGame();									// 0x419130
	void			ShowSaveNameDialog();						// 0x418f40: dialog 8, the saved game's name
	void			StartTutorial();							// 0x419040
	void			LoadSavedGame(const char* theFile);			// 0x418fd0
	void			TitleScreenDone();							// 0x419250: the play link
	void			PumpLoading();								// port: the loading runs in the main thread
	virtual bool	HandleScriptCommand(const ScriptCommand& theCommand) override;	// port: TestHooks.cpp

	// port: test hooks (TestHooks.cpp). -statelog=<file> writes "<update> <name> <value>" for each value that decides
	// play when it changes, after each update and its draw, under the names games/dynomite/tools/identical/case.sh gives the
	// original's addresses. The original does not override these two.
	virtual void	HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) override;
	virtual void	UpdateFrames() override;
	void			LogState();
	FILE*			mStateLog = NULL;
	std::vector<std::string> mStateValues;

	using SexyAppBase::ButtonPress;
};

}

#endif
