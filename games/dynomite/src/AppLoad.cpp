// The loading of FossilizedApp (InitHook, slot 15, 0x419c30): every picture, cell, font and sound of the game, in the
// original's order, with its loading steps. Drafted by tools/genres.py from the folded decompilation, the loops and
// the fonts, masks and fossils translated by hand. The field of each resource has the original's offset in
// FossilizedApp.h.
#include "FossilizedApp.h"
#include "SoundInstance.h"
#include "SoundManager.h"

using namespace Sexy;

namespace
{

// port: the three looping sounds the boards start and stop every update (the grind, the clock's tick and the footfall's
// rumble) are instances made once while loading. Without a sound device (the "could not initialize" message, a page
// with no Web Audio) the sound manager makes none, and every board's first update would call through a null pointer;
// those fields get one shared instance that plays nothing instead.
class SilentSoundInstance : public SoundInstance
{
public:
	virtual void			Release() override {}
	virtual void			SetBaseVolume(double) override {}
	virtual void			SetBasePan(int) override {}
	virtual void			AdjustPitch(double) override {}
	virtual void			SetVolume(double) override {}
	virtual void			SetPan(int) override {}
	virtual bool			Play(bool, bool) override { return false; }
	virtual void			Stop() override {}
	virtual bool			IsPlaying() override { return false; }
	virtual bool			IsReleased() override { return false; }
	virtual double			GetVolume() override { return 0.0; }
};

SoundInstance* LoopInstance(SoundManager* theManager, unsigned int theId)
{
	SoundInstance* anInstance = theManager->GetSoundInstance(theId);
	static SilentSoundInstance sSilent;
	if (anInstance == NULL)
		anInstance = &sSilent;
	return anInstance;
}

}

static const char* gFullCharset =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789~`!@#$%^&*()-_=+[{]}\\|;:'\",<.>/?";

// 0x419c30
void FossilizedApp::LoadGame()
{
	mNumLoadingThreadTasks = 394;
	mCompletedLoadingThreadTasks = 0;
	mImgSidebar = GetGameImage("images\\sidebar");
	mCompletedLoadingThreadTasks++;
	mPicIntroscreen = new Pic(this, "images\\introscreen", false);
	mCompletedLoadingThreadTasks++;
	mPicWizardgraphics = new Pic(this, "images\\wizardgraphics", false);
	mCompletedLoadingThreadTasks++;
	mPicWizardgraphics_r1290 = new Cell(mPicWizardgraphics, 390, 66, 454, 131);
	mCompletedLoadingThreadTasks++;
	mPicWizardgraphics_c7 = new Cell(mPicWizardgraphics, 65, 65, 7);
	mCompletedLoadingThreadTasks++;
	mPicWizardgraphics_c8 = new Cell(mPicWizardgraphics, 65, 65, 8);
	mCompletedLoadingThreadTasks++;
	mPicWizardgraphics_c0 = new Cell(mPicWizardgraphics, 65, 65, 0);
	mCompletedLoadingThreadTasks++;
	mPicWizardgraphics_c1 = new Cell(mPicWizardgraphics, 65, 65, 1);
	mCompletedLoadingThreadTasks++;
	mPicWizardgraphics_c2 = new Cell(mPicWizardgraphics, 65, 65, 2);
	mCompletedLoadingThreadTasks++;
	mPicWizardgraphics_c3 = new Cell(mPicWizardgraphics, 65, 65, 3);
	mCompletedLoadingThreadTasks++;
	mPicWizardgraphics_c4 = new Cell(mPicWizardgraphics, 65, 65, 4);
	mCompletedLoadingThreadTasks++;
	mPicWizardgraphics_c5 = new Cell(mPicWizardgraphics, 65, 65, 5);
	mCompletedLoadingThreadTasks++;
	mPicWizardgraphics_c6 = new Cell(mPicWizardgraphics, 65, 65, 6);
	mCompletedLoadingThreadTasks++;
	mPicWizardgraphics_r12b8 = new Cell(mPicWizardgraphics, 455, 66, 519, 131);
	mCompletedLoadingThreadTasks++;
	mPicWizardgraphics_r12bc = new Cell(mPicWizardgraphics, 76, 66, 177, 97);
	mCompletedLoadingThreadTasks++;
	mPicWizardgraphics_r12c0 = new Cell(mPicWizardgraphics, 76, 98, 177, 129);
	mCompletedLoadingThreadTasks++;
	mPicWizardgraphics_r12c4 = new Cell(mPicWizardgraphics, 178, 66, 279, 97);
	mCompletedLoadingThreadTasks++;
	mPicWizardgraphics_r12c8 = new Cell(mPicWizardgraphics, 178, 98, 279, 129);
	mCompletedLoadingThreadTasks++;
	mPicWizardgraphics_r12cc = new Cell(mPicWizardgraphics, 280, 66, 381, 97);
	mCompletedLoadingThreadTasks++;
	mPicWizardgraphics_r12d0 = new Cell(mPicWizardgraphics, 280, 99, 381, 129);
	mCompletedLoadingThreadTasks++;
	mWizardFont = new PicFont(this, "images\\WizardFont", "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 5, true);
	mCompletedLoadingThreadTasks++;
	mPicPausebuttons = new Pic(this, "images\\pausebuttons", false);
	mCompletedLoadingThreadTasks++;
	mPauseCells[0] = new Cell(mPicPausebuttons, 25, 24, 0);
	mPauseCells[1] = new Cell(mPicPausebuttons, 25, 24, 1);
	mPauseCells[2] = new Cell(mPicPausebuttons, 25, 24, 2);
	mPauseCells[3] = new Cell(mPicPausebuttons, 25, 24, 3);
	mPauseCells[4] = new Cell(mPicPausebuttons, 25, 24, 4);
	mPauseCells[5] = new Cell(mPicPausebuttons, 25, 24, 5);
	mPicPausebuttons_r1398 = new Cell(mPicPausebuttons, 150, 0, 175, 22);
	mPicPausebuttons_r139c = new Cell(mPicPausebuttons, 176, 0, 201, 22);
	mPicInchylarge = new Pic(this, "images\\inchy-large", false);
	mCompletedLoadingThreadTasks++;
	mInchyLargeCells[0] = new Cell(mPicInchylarge, 162, 158, 0);
	mCompletedLoadingThreadTasks++;
	mInchyLargeCells[1] = new Cell(mPicInchylarge, 162, 158, 1);
	mCompletedLoadingThreadTasks++;
	mPicInchylargemouth = new Pic(this, "images\\inchy-large-mouth", false);
	mCompletedLoadingThreadTasks++;
	mInchyLargeMouthCells[0] = new Cell(mPicInchylargemouth, 23, 22, 0);
	mCompletedLoadingThreadTasks++;
	mInchyLargeMouthCells[1] = new Cell(mPicInchylargemouth, 23, 22, 1);
	mCompletedLoadingThreadTasks++;
	mInchyLargeMouthCells[2] = new Cell(mPicInchylargemouth, 23, 22, 2);
	mCompletedLoadingThreadTasks++;
	mInchyLargeMouthCells[3] = new Cell(mPicInchylargemouth, 23, 22, 3);
	mCompletedLoadingThreadTasks++;
	mPicWelcomescreen = new Pic(this, "images\\welcomescreen", false);
	mCompletedLoadingThreadTasks++;
	mPicWelcomescreen_r12f8 = new Cell(mPicWelcomescreen, 1, 1, 267, 43);
	mCompletedLoadingThreadTasks++;
	mPicWelcomescreen_r12fc = new Cell(mPicWelcomescreen, 4, 46, 271, 70);
	mCompletedLoadingThreadTasks++;
	mPicWelcomescreen_r1304 = new Cell(mPicWelcomescreen, 2, 72, 269, 112);
	mCompletedLoadingThreadTasks++;
	mPicWelcomescreen_r1300 = new Cell(mPicWelcomescreen, 2, 113, 220, 133);
	mCompletedLoadingThreadTasks++;
	mPicSidebarbuttons = new Pic(this, "images\\sidebarbuttons", false);
	mCompletedLoadingThreadTasks++;
	mPicSidebarbuttons_r1344 = new Cell(mPicSidebarbuttons, 0, 0, 102, 52);
	mCompletedLoadingThreadTasks++;
	mPicSidebarbuttons_r1348 = new Cell(mPicSidebarbuttons, 103, 0, 205, 52);
	mCompletedLoadingThreadTasks++;
	mPicSidebarbuttons_r134c = new Cell(mPicSidebarbuttons, 207, 0, 296, 32);
	mCompletedLoadingThreadTasks++;
	mPicSidebarbuttons_r1350 = new Cell(mPicSidebarbuttons, 207, 34, 296, 67);
	mCompletedLoadingThreadTasks++;
	mPicSidebarbuttons_r1354 = new Cell(mPicSidebarbuttons, 0, 53, 97, 82);
	mCompletedLoadingThreadTasks++;
	mPicSidebarbuttons_r1358 = new Cell(mPicSidebarbuttons, 98, 53, 195, 82);
	mCompletedLoadingThreadTasks++;
	mPicWhirleysheet = new Pic(this, "images\\whirleysheet", true);
	mCompletedLoadingThreadTasks++;
	{
		int i = 0;
		do
		{
			mWhirleyCells[i] = new Cell(mPicWhirleysheet, 60, 60, i);
			mWhirleyCells[i + 12] = new Cell(mPicWhirleysheet, 60, 60, i + 12);
			i++;
			mCompletedLoadingThreadTasks++;
		} while (i < 12);
	}
	mPicLargedbutton = new Pic(this, "images\\largedbutton", false);
	mCompletedLoadingThreadTasks++;
	{
		int i = 0;
		do
		{
			mDialogCells[i] = new Cell(mPicDialog, 48, 48, i);
			i++;
		} while (i < 9);
	}
	mPicMinidialog = new Pic(this, "images\\minidialog", false);
	mCompletedLoadingThreadTasks++;
	{
		int i = 0;
		do
		{
			mMiniDialogCells[i] = new Cell(mPicMinidialog, 10, 10, i);
			i++;
		} while (i < 9);
	}
	mPicScrollbuttons = new Pic(this, "images\\scrollbuttons", true);
	mCompletedLoadingThreadTasks++;
	mPicScrollbuttons_c0 = new Cell(mPicScrollbuttons, 49, 49, 0);
	mCompletedLoadingThreadTasks++;
	mPicScrollbuttons_c1 = new Cell(mPicScrollbuttons, 49, 49, 1);
	mCompletedLoadingThreadTasks++;
	mPicSkillsheet = new Pic(this, "images\\skillsheet", true);
	mCompletedLoadingThreadTasks++;
	mSkillCells[0] = new Cell(mPicSkillsheet, 81, 20, 0);
	mCompletedLoadingThreadTasks++;
	mSkillCells[1] = new Cell(mPicSkillsheet, 81, 20, 1);
	mCompletedLoadingThreadTasks++;
	mSkillCells[2] = new Cell(mPicSkillsheet, 81, 20, 2);
	mCompletedLoadingThreadTasks++;
	mSkillCells[3] = new Cell(mPicSkillsheet, 81, 20, 3);
	mCompletedLoadingThreadTasks++;
	mSkillCells[4] = new Cell(mPicSkillsheet, 81, 20, 4);
	mCompletedLoadingThreadTasks++;
	mSkillCells[5] = new Cell(mPicSkillsheet, 81, 20, 5);
	mCompletedLoadingThreadTasks++;
	mSkillCells[6] = new Cell(mPicSkillsheet, 81, 20, 6);
	mCompletedLoadingThreadTasks++;
	mPicCheckboxunchecked = new Pic(this, "images\\checkbox(unchecked)", true);
	mCompletedLoadingThreadTasks++;
	mPicCheckboxchecked = new Pic(this, "images\\checkbox(checked)", true);
	mCompletedLoadingThreadTasks++;
	mPicSlider = new Pic(this, "images\\slider", true);
	mCompletedLoadingThreadTasks++;
	mPicThumb = new Pic(this, "images\\thumb", true);
	mCompletedLoadingThreadTasks++;
	mPicLeftwall = new Pic(this, "images\\leftwall", true);
	mCompletedLoadingThreadTasks++;
	mPicMenuoverlap = new Pic(this, "images\\menuoverlap", true);
	mCompletedLoadingThreadTasks++;
	mPicRightwall = new Pic(this, "images\\rightwall", true);
	mCompletedLoadingThreadTasks++;
	mPicSlingshot = new Pic(this, "images\\slingshot", true);
	mCompletedLoadingThreadTasks++;
	mPicRubberband = new Pic(this, "images\\rubberband", true);
	mCompletedLoadingThreadTasks++;
	// The crack masks are the original's only temporaries; it also takes them out of its list of loaded images
	// (0x430fe0 on +0x5bc), which the port does not keep
	Pic* aCrackMask1 = new Pic(this, "images\\crackmask1", false);
	Pic* aCrackMask2 = new Pic(this, "images\\crackmask2", false);
	Pic* aCrackMask3 = new Pic(this, "images\\crackmask3", false);
	Pic* aCrackMask4 = new Pic(this, "images\\crackmask4", false);
	mPicEggsheet = new Pic(this, "images\\EggSheet", true);
	mCompletedLoadingThreadTasks++;
	mPicBomb = new Pic(this, "images\\Bomb", true);
	mCompletedLoadingThreadTasks++;
	mPicBomb_c0 = new Cell(mPicBomb, 79, 101, 0);
	for (int i = 0; i < 4; i++)		// four separate loads in the original, each counted
	{
		mEggSheetCracked[i] = new Pic(this, "images\\EggSheet", true);
		mCompletedLoadingThreadTasks++;
	}
	{
		int x = 0;
		do
		{
			mEggSheetCracked[0]->MultiplyAlpha(aCrackMask1, x, 0);
			mEggSheetCracked[1]->MultiplyAlpha(aCrackMask2, x, 0);
			mEggSheetCracked[2]->MultiplyAlpha(aCrackMask3, x, 0);
			mEggSheetCracked[3]->MultiplyAlpha(aCrackMask4, x, 0);
			x += 35;
			mCompletedLoadingThreadTasks++;
		} while (x < 700);
	}
	{
		int i = 0;
		do
		{
			mEggCells[i] = new Cell(mPicEggsheet, 35, 39, i);
			mCrackedEggCells[i][0] = new Cell(mEggSheetCracked[0], 35, 39, i);
			mCrackedEggCells[i][1] = new Cell(mEggSheetCracked[1], 35, 39, i);
			mCrackedEggCells[i][2] = new Cell(mEggSheetCracked[2], 35, 39, i);
			mCrackedEggCells[i][3] = new Cell(mEggSheetCracked[3], 35, 39, i);
			i++;
			mCompletedLoadingThreadTasks++;
		} while (i < 20);
	}
	delete aCrackMask1;
	delete aCrackMask2;
	delete aCrackMask3;
	delete aCrackMask4;
	mAnnounceFont = new PicFont(this, "images\\AnnounceFont", gFullCharset, 5, true);
	mCompletedLoadingThreadTasks++;
	mAnnounceFont1928 = mAnnounceFont;
	mAnnounceFont->AdjustAdvances(-4);
	mTinyFont = new PicFont(this, "images\\TinyFont", gFullCharset, 2, true);
	mCompletedLoadingThreadTasks++;
	mPicKroneysheet = new Pic(this, "images\\kroneysheet", true);
	mCompletedLoadingThreadTasks++;
	{
		int i = 0;
		do
		{
			mKroneyCells[i] = new Cell(mPicKroneysheet, 82, 130, i);
			i++;
			mCompletedLoadingThreadTasks++;
		} while (i < 8);
	}
	mPicKroneyhourglasssheet = new Pic(this, "images\\kroneyhourglasssheet", true);
	mCompletedLoadingThreadTasks++;
	{
		int i = 0;
		do
		{
			mKroneyHourglassCells[i] = new Cell(mPicKroneyhourglasssheet, 42, 66, i);
			i++;
			mCompletedLoadingThreadTasks++;
		} while (i < 8);
	}
	mPicKroneysandtopsheet = new Pic(this, "images\\kroneysandtopsheet", true);
	mCompletedLoadingThreadTasks++;
	{
		int i = 0;
		do
		{
			mKroneySandTopCells[i] = new Cell(mPicKroneysandtopsheet, 42, 31, i);
			i++;
			mCompletedLoadingThreadTasks++;
		} while (i < 8);
	}
	mPicKroneysandbottomsheet = new Pic(this, "images\\kroneysandBottomsheet", true);
	mCompletedLoadingThreadTasks++;
	{
		int i = 0;
		do
		{
			mKroneySandBottomCells[i] = new Cell(mPicKroneysandbottomsheet, 42, 36, i);
			i++;
			mCompletedLoadingThreadTasks++;
		} while (i < 8);
	}
	mPicKroneyfacesheet = new Pic(this, "images\\kroneyfacesheet", true);
	mCompletedLoadingThreadTasks++;
	{
		int i = 0;
		do
		{
			mKroneyFaceCells[i] = new Cell(mPicKroneyfacesheet, 29, 43, i);
			i++;
			mCompletedLoadingThreadTasks++;
		} while (i < 8);
	}
	{
		int i = 0;
		do
		{
			mKroneyFaceCells[i + 8] = new Cell(mPicKroneyfacesheet, 29, 43, i + 8);
			i++;
			mCompletedLoadingThreadTasks++;
		} while (i < 8);
	}
	mPicKroneyflipsheet = new Pic(this, "images\\kroneyflipsheet", true);
	mCompletedLoadingThreadTasks++;
	{
		int i = 0;
		do
		{
			mKroneyFlipCells[i] = new Cell(mPicKroneyflipsheet, 154, 196, i);
			i++;
			mCompletedLoadingThreadTasks++;
		} while (i < 14);
	}
	mPicEggpilesheet = new Pic(this, "images\\eggpilesheet", true);
	mCompletedLoadingThreadTasks++;
	mPicEggpilesheet_c0 = new Cell(mPicEggpilesheet, 217, 95, 0);
	mCompletedLoadingThreadTasks++;
	mPicEggpilesheet_c1 = new Cell(mPicEggpilesheet, 217, 95, 1);
	mCompletedLoadingThreadTasks++;
	mPicRope = new Pic(this, "images\\rope", true);
	mCompletedLoadingThreadTasks++;
	mPicYolksheet = new Pic(this, "images\\yolksheet", true);
	mCompletedLoadingThreadTasks++;
	mYolkCells[0] = new Cell(mPicYolksheet, 37, 60, 0);
	mCompletedLoadingThreadTasks++;
	mYolkCells[1] = new Cell(mPicYolksheet, 37, 60, 1);
	mCompletedLoadingThreadTasks++;
	mPicEggwhite = new Pic(this, "images\\eggwhite", true);
	mCompletedLoadingThreadTasks++;
	mPicBoomsheet = new Pic(this, "images\\boomsheet", true);
	mCompletedLoadingThreadTasks++;
	{
		int i = 0;
		do
		{
			mBoomCells[i] = new Cell(mPicBoomsheet, 88, 123, i);
			i++;
			mCompletedLoadingThreadTasks++;
		} while (i < 15);
	}
	mPicDynamitesheet = new Pic(this, "images\\dynamitesheet", true);
	mCompletedLoadingThreadTasks++;
	{
		int i = 0;
		do
		{
			mDynamiteCells[i] = new Cell(mPicDynamitesheet, 48, 48, i);
			i++;
			mCompletedLoadingThreadTasks++;
		} while (i < 3);
	}
	mPicSparksheet = new Pic(this, "images\\sparksheet", true);
	mCompletedLoadingThreadTasks++;
	{
		int i = 0;
		do
		{
			mSparkCells[i] = new Cell(mPicSparksheet, 11, 11, i);
			i++;
			mCompletedLoadingThreadTasks++;
		} while (i < 3);
	}
	mPicJungleback = new Pic(this, "images\\jungleback", true);
	mCompletedLoadingThreadTasks++;
	mPicSteggy = new Pic(this, "images\\steggy", true);
	mCompletedLoadingThreadTasks++;
	mPicTanksheet = new Pic(this, "images\\tanksheet", true);
	mCompletedLoadingThreadTasks++;
	mPicTanksheet_c0 = new Cell(mPicTanksheet, 108, 122, 0);
	mCompletedLoadingThreadTasks++;
	mPicTanksheet_c1 = new Cell(mPicTanksheet, 108, 122, 1);
	mCompletedLoadingThreadTasks++;
	mPicTanksheet_c2 = new Cell(mPicTanksheet, 108, 122, 2);
	mCompletedLoadingThreadTasks++;
	mPicSteggyterror = new Pic(this, "images\\steggyterror", true);
	mCompletedLoadingThreadTasks++;
	mPicTankterror = new Pic(this, "images\\tankterror", true);
	mCompletedLoadingThreadTasks++;
	mPicTanksweat = new Pic(this, "images\\tanksweat", true);
	mCompletedLoadingThreadTasks++;
	mPicTankeyesheet = new Pic(this, "images\\tankeyesheet", true);
	mCompletedLoadingThreadTasks++;
	{
		int i = 0;
		do
		{
			mTankEyeCells[i] = new Cell(mPicTankeyesheet, 14, 13, i);
			i++;
			mCompletedLoadingThreadTasks++;
		} while (i < 4);
	}
	mPicTankmouthsheet = new Pic(this, "images\\tankmouthsheet", true);
	mCompletedLoadingThreadTasks++;
	{
		int i = 0;
		do
		{
			mTankMouthCells[i] = new Cell(mPicTankmouthsheet, 17, 19, i);
			i++;
			mCompletedLoadingThreadTasks++;
		} while (i < 4);
	}
	mPicHandsheet = new Pic(this, "images\\handsheet", true);
	mCompletedLoadingThreadTasks++;
	{
		int i = 0;
		do
		{
			mHandCells[i] = new Cell(mPicHandsheet, 81, 118, i);
			i++;
			mCompletedLoadingThreadTasks++;
		} while (i < 9);
	}
	mPicScorefont = new Pic(this, "images\\scorefont", true);
	mCompletedLoadingThreadTasks++;
	{
		int i = 0;
		do
		{
			mScoreDigitCells[i] = new Cell(mPicScorefont, 17, 39, i);
			i++;
			mCompletedLoadingThreadTasks++;
		} while (i < 11);
	}
	mPicBabysheet = new Pic(this, "images\\babysheet", true);
	mCompletedLoadingThreadTasks++;
	mBabyCells[0] = new Cell(mPicBabysheet, 64, 64, 0);
	mCompletedLoadingThreadTasks++;
	mBabyCells[1] = new Cell(mPicBabysheet, 64, 64, 1);
	mCompletedLoadingThreadTasks++;
	mBabyCells[2] = new Cell(mPicBabysheet, 64, 64, 2);
	mCompletedLoadingThreadTasks++;
	mPicBonuseggsheet = new Pic(this, "images\\bonus(egg)sheet", true);
	mCompletedLoadingThreadTasks++;
	{
		int i = 0;
		do
		{
			mBonusEggCells[i] = new Cell(mPicBonuseggsheet, 26, 29, i);
			i++;
			mCompletedLoadingThreadTasks++;
		} while (i < 10);
	}
	mPicBonusviewsheet = new Pic(this, "images\\bonus(view)sheet", true);
	mCompletedLoadingThreadTasks++;
	{
		int i = 0;
		do
		{
			mBonusViewCells[i] = new Cell(mPicBonusviewsheet, 33, 34, i);
			i++;
			mCompletedLoadingThreadTasks++;
		} while (i < 10);
	}
	mPicWowstarsheet = new Pic(this, "images\\wowstarsheet", true);
	mCompletedLoadingThreadTasks++;
	{
		int i = 0;
		do
		{
			mWowStarCells[i] = new Cell(mPicWowstarsheet, 12, 11, i);
			i++;
			mCompletedLoadingThreadTasks++;
		} while (i < 5);
	}
	mPicAimx = new Pic(this, "images\\aimx", true);
	mCompletedLoadingThreadTasks++;
	mPicFoot = new Pic(this, "images\\foot", true);
	mCompletedLoadingThreadTasks++;
	mPicCeiling = new Pic(this, "images\\ceiling", true);
	mCompletedLoadingThreadTasks++;
	mPicGameover = new Pic(this, "images\\GameOver", true);
	mCompletedLoadingThreadTasks++;
	mPicCleared = new Pic(this, "images\\Cleared", true);
	mCompletedLoadingThreadTasks++;
	mPicRowstogo = new Pic(this, "images\\RowsToGo", true);
	mCompletedLoadingThreadTasks++;
	mPicInchysmallsheet = new Pic(this, "images\\inchysmallsheet", true);
	mCompletedLoadingThreadTasks++;
	mInchyCells[0] = new Cell(mPicInchysmallsheet, 73, 73, 0);
	mCompletedLoadingThreadTasks++;
	mInchyCells[1] = new Cell(mPicInchysmallsheet, 73, 73, 1);
	mCompletedLoadingThreadTasks++;
	mInchyCells[2] = new Cell(mPicInchysmallsheet, 73, 73, 2);
	mCompletedLoadingThreadTasks++;
	mPicInchysmallmouthsheet = new Pic(this, "images\\inchysmallmouthsheet", true);
	mCompletedLoadingThreadTasks++;
	mInchyMouthCells[0] = new Cell(mPicInchysmallmouthsheet, 10, 14, 0);
	mCompletedLoadingThreadTasks++;
	mInchyMouthCells[1] = new Cell(mPicInchysmallmouthsheet, 10, 14, 1);
	mCompletedLoadingThreadTasks++;
	mInchyMouthCells[2] = new Cell(mPicInchysmallmouthsheet, 10, 14, 2);
	mCompletedLoadingThreadTasks++;
	mInchyMouthCells[3] = new Cell(mPicInchysmallmouthsheet, 10, 14, 3);
	mCompletedLoadingThreadTasks++;
	mPicInchysmallblink = new Pic(this, "images\\inchysmallblink", true);
	mCompletedLoadingThreadTasks++;
	mPicTextballoons = new Pic(this, "images\\textballoons", true);
	mCompletedLoadingThreadTasks++;
	mPicTextballoons_c0 = new Cell(mPicTextballoons, 120, 71, 0);
	mCompletedLoadingThreadTasks++;
	mPicTextballoons_c1 = new Cell(mPicTextballoons, 120, 71, 1);
	mCompletedLoadingThreadTasks++;
	mPicTextballoons_c2 = new Cell(mPicTextballoons, 120, 71, 2);
	mCompletedLoadingThreadTasks++;
	mPicTextwords = new Pic(this, "images\\textwords", true);
	mCompletedLoadingThreadTasks++;
	mPicTextwords_c0 = new Cell(mPicTextwords, 83, 32, 0);
	mCompletedLoadingThreadTasks++;
	mPicTextwords_c5 = new Cell(mPicTextwords, 83, 32, 5);
	mCompletedLoadingThreadTasks++;
	mPicTextwords_c1 = new Cell(mPicTextwords, 83, 32, 1);
	mCompletedLoadingThreadTasks++;
	mPicTextwords_c2 = new Cell(mPicTextwords, 83, 32, 2);
	mCompletedLoadingThreadTasks++;
	mPicTextwords_c3 = new Cell(mPicTextwords, 83, 32, 3);
	mCompletedLoadingThreadTasks++;
	mPicTextwords_c4 = new Cell(mPicTextwords, 83, 32, 4);
	mCompletedLoadingThreadTasks++;
	mPicCombotext = new Pic(this, "images\\combotext", true);
	mCompletedLoadingThreadTasks++;
	mComboTextCells[0] = new Cell(mPicCombotext, 71, 24, 0);
	mCompletedLoadingThreadTasks++;
	mComboTextCells[1] = new Cell(mPicCombotext, 71, 24, 1);
	mCompletedLoadingThreadTasks++;
	mComboTextCells[2] = new Cell(mPicCombotext, 71, 24, 2);
	mCompletedLoadingThreadTasks++;
	mComboTextCells[3] = new Cell(mPicCombotext, 71, 24, 3);
	mCompletedLoadingThreadTasks++;
	mComboTextCells[4] = new Cell(mPicCombotext, 71, 24, 4);
	mCompletedLoadingThreadTasks++;
	mComboTextCells[5] = new Cell(mPicCombotext, 71, 24, 5);
	mCompletedLoadingThreadTasks++;
	mComboTextCells[6] = new Cell(mPicCombotext, 71, 24, 6);
	mCompletedLoadingThreadTasks++;
	mComboTextCells[7] = new Cell(mPicCombotext, 71, 24, 7);
	mCompletedLoadingThreadTasks++;
	mPicBolt = new Pic(this, "images\\bolt", true);
	mCompletedLoadingThreadTasks++;
	mPicAlert = new Pic(this, "images\\alert", true);
	mCompletedLoadingThreadTasks++;
	mPicWhirleyalarm = new Pic(this, "images\\whirleyalarm", true);
	mCompletedLoadingThreadTasks++;
	mPicMamaportrait = new Pic(this, "images\\mamaportrait", true);
	mCompletedLoadingThreadTasks++;
	mPicMamameter = new Pic(this, "images\\mamameter", true);
	mCompletedLoadingThreadTasks++;
	mPicMamameter_c0 = new Cell(mPicMamameter, 39, 185, 0);
	mPicMamameter_c1 = new Cell(mPicMamameter, 39, 185, 1);
	mPicThermoglow = new Pic(this, "images\\thermoglow", true);
	mCompletedLoadingThreadTasks++;
	{
		int i = 0;
		int aCount = 20;
		do
		{
			mFossils[i] = NULL;
			mFossilCopies[i] = NULL;
			i++;
			aCount--;
		} while (aCount != 0);
	}
	mFossils[0] = new MaskedPic(this, "images\\fossils\\trilobyte");
	mCompletedLoadingThreadTasks++;
	mFossils[1] = new MaskedPic(this, "images\\fossils\\brachiopod");
	mCompletedLoadingThreadTasks++;
	mFossils[2] = new MaskedPic(this, "images\\fossils\\ammonite");
	mCompletedLoadingThreadTasks++;
	mFossils[3] = new MaskedPic(this, "images\\fossils\\crinoid");
	mCompletedLoadingThreadTasks++;
	mFossils[4] = new MaskedPic(this, "images\\fossils\\coelacanth");
	mCompletedLoadingThreadTasks++;
	mFossils[5] = new MaskedPic(this, "images\\fossils\\dimetrodon");
	mCompletedLoadingThreadTasks++;
	mFossils[6] = new MaskedPic(this, "images\\fossils\\plateosaurus");
	mCompletedLoadingThreadTasks++;
	mFossils[7] = new MaskedPic(this, "images\\fossils\\archaeopteryx");
	mCompletedLoadingThreadTasks++;
	mFossils[8] = new MaskedPic(this, "images\\fossils\\parasaurolophus");
	mCompletedLoadingThreadTasks++;
	mFossils[9] = new MaskedPic(this, "images\\fossils\\brontosaurus");
	mCompletedLoadingThreadTasks++;
	mFossils[10] = new MaskedPic(this, "images\\fossils\\ankylosaurus");
	mCompletedLoadingThreadTasks++;
	mFossils[11] = new MaskedPic(this, "images\\fossils\\pteranodon");
	mCompletedLoadingThreadTasks++;
	mFossils[12] = new MaskedPic(this, "images\\fossils\\stegosaurus");
	mCompletedLoadingThreadTasks++;
	mFossils[13] = new MaskedPic(this, "images\\fossils\\triceratops");
	mCompletedLoadingThreadTasks++;
	mFossils[14] = new MaskedPic(this, "images\\fossils\\tyrannosaurus");
	mCompletedLoadingThreadTasks++;
	{
		int i = 0;
		do
		{
			if (mFossils[i] == NULL)
				break;
			mFossilCopies[i] = new MaskedPic(*mFossils[i]);
			i++;
		} while (i < 20);
	}
	mSndBreakegg = new GameSound(this, "sounds\\breakegg");
	mCompletedLoadingThreadTasks++;
	mSndExplosion = new GameSound(this, "sounds\\explosion");
	mCompletedLoadingThreadTasks++;
	mSndAooga = new GameSound(this, "sounds\\aooga");
	mCompletedLoadingThreadTasks++;
	mSndRubbershot = new GameSound(this, "sounds\\rubbershot");
	mCompletedLoadingThreadTasks++;
	mSndBlastit = new GameSound(this, "sounds\\blastit");
	mCompletedLoadingThreadTasks++;
	mSndCharge = new GameSound(this, "sounds\\charge");
	mCompletedLoadingThreadTasks++;
	mSndKaboom = new GameSound(this, "sounds\\kaboom");
	mCompletedLoadingThreadTasks++;
	mSndFireinthehole = new GameSound(this, "sounds\\fireinthehole");
	mCompletedLoadingThreadTasks++;
	mSndWhirleyhit = new GameSound(this, "sounds\\whirleyhit");
	mCompletedLoadingThreadTasks++;
	mSndMama1 = new GameSound(this, "sounds\\mama1");
	mCompletedLoadingThreadTasks++;
	mSndMama1_1a1c = new GameSound(this, "sounds\\mama1");
	mCompletedLoadingThreadTasks++;
	mSndPopbonus = new GameSound(this, "sounds\\popbonus");
	mCompletedLoadingThreadTasks++;
	mSndGrind = new GameSound(this, "sounds\\grind");
	mCompletedLoadingThreadTasks++;
	mGrindInstance = LoopInstance(mSoundManager, mSndGrind->mId);
	mSndXwhirleyscoming1 = new GameSound(this, "sounds\\xwhirleyscoming1");
	mCompletedLoadingThreadTasks++;
	mSndXwhirleyscoming2 = new GameSound(this, "sounds\\xwhirleyscoming2");
	mCompletedLoadingThreadTasks++;
	mSndXtimesrunningout = new GameSound(this, "sounds\\xtimesrunningout");
	mCompletedLoadingThreadTasks++;
	mSndScoremultiplier = new GameSound(this, "sounds\\scoremultiplier");
	mCompletedLoadingThreadTasks++;
	mSndFootlands = new GameSound(this, "sounds\\footlands");
	mCompletedLoadingThreadTasks++;
	mSndXwelcometodynomite = new GameSound(this, "sounds\\xwelcometodynomite");
	mCompletedLoadingThreadTasks++;
	mSndXgameover = new GameSound(this, "sounds\\xgameover");
	mCompletedLoadingThreadTasks++;
	mSndInchypop = new GameSound(this, "sounds\\inchypop");
	mCompletedLoadingThreadTasks++;
	mSndTicktock = new GameSound(this, "sounds\\ticktock");
	mCompletedLoadingThreadTasks++;
	mTicktockInstance = LoopInstance(mSoundManager, mSndTicktock->mId);
	mSndSpeedup = new GameSound(this, "sounds\\speedup");
	mCompletedLoadingThreadTasks++;
	mSound1a50 = NULL;
	mSndXcombo = new GameSound(this, "sounds\\xcombo");
	mCompletedLoadingThreadTasks++;
	mSndXsupercombo = new GameSound(this, "sounds\\xsupercombo");
	mCompletedLoadingThreadTasks++;
	mSndXcambriancombo = new GameSound(this, "sounds\\xcambriancombo");
	mCompletedLoadingThreadTasks++;
	mSndXtriassiccombo = new GameSound(this, "sounds\\xtriassiccombo");
	mCompletedLoadingThreadTasks++;
	mSndXjurassiccombo = new GameSound(this, "sounds\\xjurassiccombo");
	mCompletedLoadingThreadTasks++;
	mSndXcretaceouscombo = new GameSound(this, "sounds\\xcretaceouscombo");
	mCompletedLoadingThreadTasks++;
	mSndXdynamitecombo = new GameSound(this, "sounds\\xdynamitecombo");
	mCompletedLoadingThreadTasks++;
	mSndThunderbolt = new GameSound(this, "sounds\\thunderbolt");
	mCompletedLoadingThreadTasks++;
	mSndAlert = new GameSound(this, "sounds\\alert");
	mCompletedLoadingThreadTasks++;
	mSndReward = new GameSound(this, "sounds\\reward");
	mCompletedLoadingThreadTasks++;
	mSndWhirleysiren = new GameSound(this, "sounds\\whirleysiren");
	mCompletedLoadingThreadTasks++;
	mSndWarning = new GameSound(this, "sounds\\warning");
	mCompletedLoadingThreadTasks++;
	mSndFootfallrumble = new GameSound(this, "sounds\\footfallrumble");
	mCompletedLoadingThreadTasks++;
	mFootfallRumbleInstance = LoopInstance(mSoundManager, mSndFootfallrumble->mId);
	mSndFootfall = new GameSound(this, "sounds\\footfall");
	mCompletedLoadingThreadTasks++;
	mSndFossil = new GameSound(this, "sounds\\fossil");
	mCompletedLoadingThreadTasks++;
	mSndChime = new GameSound(this, "sounds\\chime");
	mCompletedLoadingThreadTasks++;
}
