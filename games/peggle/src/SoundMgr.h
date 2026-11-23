// SoundMgr.h: the board's sounds (the original's Sexy::SoundMgr): the sounds that wait for their time (a ball's add sound a
// few updates after the last), the time each sound last played (so a sound does not play twice in a row), the music's fade, and
// the sounds that fade out. Only the making, the emptying and the fade's bookkeeping are translated so far; the two tables
// (the waiting sounds and the last times) are declared with AddSound's translation, and they are empty until then.

#ifndef __SOUNDMGR_H__
#define __SOUNDMGR_H__

#include <list>
#include <map>
#include <tuple>

namespace Sexy
{

class SoundInstance;

// A sound that is asked for: which one, how it is played, and how soon after its last play it may play again
struct SoundRequest
{
	int mMinGap;	// the updates that must have passed since the same sound (same id, pitch and pan) last played
	int mId;
	int mPan;		// 0: not set
	float mPitch;	// in steps; 0: not set
	float mVolume;	// 0 or less: not set
};

class SoundMgr
{
public:
	SoundMgr();			  // 0x45f1d0
	virtual ~SoundMgr() {} // 0x437560's neighbour in the vtable; the original's deleting destructor

	void Clear();					   // 0x442e90
	void Update();		// 0x458fc0: the sounds that wait come due, then UpdateSounds
	// 0x458f30: plays the sound now (theDelay 0) or theDelay updates from now; a volume of 0 plays nothing
	void AddSound(int theId, float thePitch, int thePan, int theDelay, int theMinGap, float theVolume);
	void AddSound(int theId, int theMinGap);	// 0x45af70
	void AddSoundPitch(int theId, float thePitch, int theMinGap); // 0x45afa0
	void PlaySound(const SoundRequest& theRequest); // 0x458e80
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
	std::multimap<int, SoundRequest> mWaiting; // the sounds that wait, by the update they are due at
	std::map<std::tuple<int, float, int>, int> mLastPlayed; // +0x24: the update each sound (id, pitch, pan) last played at
	std::list<SoundInstance*> mFadeSounds; // +0x34: the sounds that are fading out
};

} // namespace Sexy

#endif
