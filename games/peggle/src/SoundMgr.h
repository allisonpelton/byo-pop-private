// SoundMgr.h: the board's sounds (the original's Sexy::SoundMgr): the sounds that wait for their time (a ball's add sound a
// few updates after the last), the time each sound last played (so a sound does not play twice in a row), the music's fade, and
// the sounds that fade out. Only the making, the emptying and the fade's bookkeeping are translated so far; the two tables
// (the waiting sounds and the last times) are declared with AddSound's translation, and they are empty until then.

#ifndef __SOUNDMGR_H__
#define __SOUNDMGR_H__

#include <list>

namespace Sexy
{

class SoundInstance;

class SoundMgr
{
public:
	SoundMgr();			  // 0x45f1d0
	virtual ~SoundMgr() {} // 0x437560's neighbour in the vtable; the original's deleting destructor

	void Clear();					   // 0x442e90
	void Update();		// 0x458fc0
	void UpdateSounds();		// 0x441d50
	void CancelFade();				   // 0x437600: the music's volume goes back to full
	void AddFadeSound(SoundInstance* theSound); // 0x452810: the sound fades out and is let go of when it is silent
	void PauseMusic(bool thePause);	   // 0x437560
	void FadeMusic(int theTime);	   // 0x4375a0

	int mTime;				   // +0x10: updates since the last Clear; the waiting sounds are due by it
	float mMusicVolume;		   // +0x14: 1 is full
	float mMusicVolumeStep;	   // +0x18: how much the volume moves by in an update (0 when it is still)
	bool mMusicPaused;		   // +0x1c
	int mFadeTimer;			   // +0x20: updates until the fade starts. Not set by the original's constructor (it is 0 here)
	std::list<SoundInstance*> mFadeSounds; // +0x34: the sounds that are fading out
};

} // namespace Sexy

#endif
