#ifndef __ATOMICA_ATOMICUS_H__
#define __ATOMICA_ATOMICUS_H__

// Atomicus.cpp: the adviser on the machine's monitor (Atomica.exe 0x40b200-0x40c240, 0x421c40-0x421e10): a head that
// looks about, blinks and speaks a voice clip when something happens. A message is a sound after a delay, with the
// clip's mouth data (a list of keyframes, read from the executable: GameText.h); the mouth follows it from the update the
// sound starts, and the head talks until the list ends (0x421c40-0x421e10).

#include "Support.h"
#include <vector>

namespace Sexy
{

class AtomicaApp;
class Graphics;
class Sound;

class Atomicus
{
public:
	struct Message
	{
		int		mDelay;			// +0 updates between the message and its sound
		int		mCount;			// +4 updates left
		Sound*	mSound;			// +8
		unsigned int	mLipData;	// +0xc the address of the clip's mouth data in the original
	};

	AtomicaApp*	mApp;			// +4
	bool		mMuted;			// +8 no messages (while a game is loaded)
	int			mOffset;		// +0xc how far the head is slid down when it is hidden
	int			mBrightness;	// +0x14 the monitor flickers between 255 and 251
	int			mArrive;		// +0x18 the head slides in (starts at 220)
	int			mLookRight;		// +0x1c
	int			mLookTick;		// +0x20
	float		mGaze;			// +0x24 0 to 10
	int			mNoise;			// +0x28
	int			mNoiseStep;		// +0x2c
	bool		mBlinking;		// +0x30
	float		mBlinkFrame;	// +0x34
	int			mBlinkCount;	// +0x38
	bool		mOnlyLevelUp;	// +0x3c only the level-up voice is heard
	// the lip-sync (0x421c40, the original's object at +0x40)
	bool		mTalking;		// +0x44 the mouth data is running
	bool		mPending;		// +0x45 a clip is waiting to start (set from the beginning: no looking about until one has played)
	const int*	mLipCursor;		// +0x4c the next keyframe
	float		mLipFrame;		// +0x50 the mouth picture shown
	float		mLipPos;		// +0x54
	float		mLipStep;		// +0x58
	float		mLipTarget;		// +0x5c
	unsigned int	mLipTick;	// +0x60 the clock at the last update
	int			mLipCountdown;	// +0x64 updates until the sound starts
	Sound*		mLipSound;		// +0x68
	std::vector<Message>	mMessages;	// +0x6c

	Atomicus(AtomicaApp* theApp);					// 0x40b200
	~Atomicus();									// 0x40b2a0
	void	Update();								// 0x40b330
	void	Say(int theMessage);					// 0x40b5a0
	void	Comment(int theCombo);					// 0x40c000
	void	Blink();								// 0x40b550
	void	LipStart(Sound* theSound, unsigned int theLipData);	// 0x421e10
	void	LipUpdate();							// 0x421d00
	void	LipNext();								// 0x421c70
	void	Draw(Graphics* g, int theX, int theY);	// 0x40b9f0
};

}

#endif
