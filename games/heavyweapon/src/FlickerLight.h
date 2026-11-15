#ifndef __HEAVYWEAPON_FLICKERLIGHT_H__
#define __HEAVYWEAPON_FLICKERLIGHT_H__

// FlickerLight: a glow sprite whose brightness flickers at random, with a buzzing sound while it flickers (the main menu's
// and the armory's lamps). Original: no vtable, 0x14 bytes, constructor 0x423d90, destructor body 0x423db0, Draw 0x423dc0,
// Update 0x423e50. Not a widget: its owner calls Update and Draw.

namespace Sexy
{

class Graphics;
class HeavyWeaponApp;
class SoundInstance;
class Sprite;

class FlickerLight
{
public:
	HeavyWeaponApp*	mApp;			// +0x00
	Sprite*			mSprite;		// +0x04 the glow
	SoundInstance*	mSound;			// +0x08 the buzz, while it plays
	int				mFlickerLeft;	// +0x0c updates left of the flicker in progress (0 for none)
	int				mBrightness;	// +0x10 the glow's grey level while it flickers

public:
	FlickerLight(HeavyWeaponApp* theApp, Sprite* theSprite);	// 0x423d90
	~FlickerLight();											// 0x423db0
	void	Draw(Graphics* g);									// 0x423dc0
	void	Update();											// 0x423e50
};

}

#endif
