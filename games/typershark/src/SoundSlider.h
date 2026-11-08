#ifndef __TYPERSHARK_SOUNDSLIDER_H__
#define __TYPERSHARK_SOUNDSLIDER_H__

// SoundSlider.cpp: the Options dialog's volume slider; releasing it plays one sample so the player hears the new
// volume.
// Original: class Sexy::SoundSlider : public Slider, vftable 0x52f10c, sizeof 0x88 (constructor 0x441230,
// destructor 0x441ed0, deleting destructor 0x441ea0). The base is the 2004 Slider (Slider2004).

#include "compat2004/Compat2004.h"

namespace Sexy
{

class SexyAppBase;

class SoundSlider : public Slider2004
{
public:
	int						mSoundNum;		// +0x80 the sample to play (ctor argument 5)
	SexyAppBase*			mApp;			// +0x84 the WinWSApp whose slot 41 (PlaySample) is called (argument 6)

public:
	SoundSlider(Image* theTrackImage, Image* theThumbImage, int theId, SliderListener* theListener,
		int theSoundNum, SexyAppBase* theApp);								// 0x441230
	virtual ~SoundSlider() override;										// 0x441ed0 (deleting 0x441ea0), slot 0

	virtual void			MouseUp(int x, int y) override;					// 0x441270 slot 30
};

}

#endif
