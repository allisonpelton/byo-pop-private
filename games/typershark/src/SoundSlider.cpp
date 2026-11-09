#include "SoundSlider.h"
#include "SexyAppBase.h"

using namespace Sexy;

// 0x441230
SoundSlider::SoundSlider(Image* theTrackImage, Image* theThumbImage, int theId, SliderListener* theListener,
	int theSoundNum, SexyAppBase* theApp) :
	Slider2004(theTrackImage, theThumbImage, theId, theListener)
{
	mSoundNum = theSoundNum;
	mApp = theApp;
}

// 0x441ed0
SoundSlider::~SoundSlider()
{
}

// 0x441270 slot 30: the slider's release (value reported to the listener), then the sample
void SoundSlider::MouseUp(int x, int y)
{
	Slider2004::MouseUp(x, y);
	mApp->PlaySample(mSoundNum);
}
