#ifndef __ASTROPOP_UPGRADEDIALOG_H__
#define __ASTROPOP_UPGRADEDIALOG_H__

// UpgradeDialog: the box that tells the player on the warp screen that the pilot's SUPA weapon has been upgraded (id 0x76;
// original class of 0x1ec bytes, constructor 0x48a110 on the AstroDialog constructor 0x489e20, destructor 0x48a670,
// vftable 0x56b7b8). The title is a picture, below it comes a line with the name of the upgrade and a picture of the
// weapon's gauges. Each gauge is a bar that fills a little every update, from where it was before the upgrade to where it
// stands now, and a sound plays after a second. The weapons of the first and the fourth pilot have three gauges, the
// other two have two. A weapon that has every upgrade says so instead. Offsets in the comments are the original's.

#include "AstroDialog.h"
#include <string>

namespace Sexy
{

class Image;

class UpgradeDialog : public AstroDialog
{
public:
	enum
	{
		NUM_BARS = 3
	};

	std::string		mText;                  // +0x194 the announcement, with the name of the upgrade at its end
	Image*			mOverlay;               // +0x1b0 the picture of the gauges (the first and fourth pilot's, or the others')
	float			mBarNow[NUM_BARS];      // +0x1b4 how full each bar is drawn now (0 .. 1)
	float			mBarGoal[NUM_BARS];     // +0x1c0 how full each bar will be: the level of the upgrade over its number of levels
	struct BarSpot
	{
		int		mX;
		int		mY;
	};
	BarSpot			mBarSpot[NUM_BARS];     // +0x1cc where each bar is drawn from the picture's top left (the x of the third is -1
											// when there is no third bar)
	bool			mSoundPlayed;           // +0x1e4
	int				mSoundDelay;            // +0x1e8 updates left until the sound (starts at 100)

public:
	// theMode is the pilot, theSetting the profile setting of the upgrade that was just awarded and theName its name
	UpgradeDialog(AstroPopApp* theApp, int theMode, int theSetting, const std::string& theName);   // 0x48a110
	virtual ~UpgradeDialog();                                                                        // 0x48a670

	virtual void	Draw(Graphics* g) override;     // 0x487f00
	virtual void	Update() override;              // 0x484530
};

}

#endif
