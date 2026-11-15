#ifndef __HEAVYWEAPON_SUPPLYCOPTER_H__
#define __HEAVYWEAPON_SUPPLYCOPTER_H__

// SupplyCopter.cpp: the white helicopter that flies across and drops a power-up for the player. It is the player's
// unit (friendly): shooting it costs points, in survival time, and it stays through the level's end. It is not in
// craft.xml's table, so it holds the level end back by entry 0's index.
// Original: 0x7c bytes, constructor 0x437450; vtable 0x5305a8: 0 destructor 0x437680 (deleting 0x437910), 1 Draw
// 0x437710, 2 Update 0x437940, 3 UpdateF 0x4376f0, 4 Killed 0x437c50, 6 SilenceSounds 0x437890, 7 RestoreSounds
// 0x4378b0.

#include "Enemy.h"

namespace Sexy
{

class SoundInstance;

class SupplyCopter : public Enemy
{
public:
	enum
	{
		FLASH_FADE = 5					// the "don't shoot" sign fades this much an update
	};

	int				mPowerUp;			// +0x70 the power-up it drops (HeavyWeaponApp::POWERUP_, or a weapon)
	bool			mHasDropped;		// +0x74 the power-up has been dropped
	double			mRotorFrame;		// +0x78 the rotors' cel, 0..6
	int				mDropOffset;		// +0x80 the drop is made when it is within 5 pixels of this x, -100..99
	int				mFlash;				// +0x84 the "don't shoot" sign's alpha, 255 when hit, down 5 an update
	int				mDamageStage;		// +0x88 the damage sounds played, 0..3
	SoundInstance*	mSound;				// +0x8c its looping rotor sound, or NULL
	bool			mTipShown;			// +0x90 the "don't shoot" tip has been considered for this copter

	SupplyCopter(Board* theBoard, int thePowerUp);			// 0x437450
	virtual ~SupplyCopter();								// 0x437680
	virtual void	Draw(Graphics* g) override;				// 0x437710 slot 1
	virtual void	Update() override;						// 0x437940 slot 2
	virtual void	UpdateF(float theFrac) override;		// 0x4376f0 slot 3
	virtual void	Killed() override;						// 0x437c50 slot 4
	virtual void	SilenceSounds() override;				// 0x437890 slot 6
	virtual void	RestoreSounds() override;				// 0x4378b0 slot 7

private:
	// port: its rotor sound's volume and pan follow where it is (the original writes this twice, in Update and
	// RestoreSounds)
	void			SetSoundVolume();
};

}

#endif
