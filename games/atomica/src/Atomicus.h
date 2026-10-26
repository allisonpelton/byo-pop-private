#ifndef __ATOMICA_ATOMICUS_H__
#define __ATOMICA_ATOMICUS_H__

// Atomicus.cpp: the adviser on the machine's monitor (Atomica.exe 0x40b200-0x40c240, 0x421c40-0x421e10): a head that
// looks about, blinks and speaks a voice clip when something happens. A message is a sound after a delay.
// port: the original keeps lip-sync data (a table of mouth shapes for each clip); it is not ported, the mouth moves
// at random while a clip is due to play.

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
		int		mDelay;			// updates left
		Sound*	mSound;
		int		mTalk;			// updates the mouth moves
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
	bool		mTalking;		// +0x44 a clip plays
	int			mTalkTick;
	std::vector<Message>	mMessages;	// +0x6c

	Atomicus(AtomicaApp* theApp);					// 0x40b200
	~Atomicus();									// 0x40b2a0
	void	Update();								// 0x40b330
	void	Say(int theMessage);					// 0x40b5a0
	void	Comment(int theCombo);					// 0x40c000
	void	Blink();								// 0x40b550
	void	Draw(Graphics* g, int theX, int theY);	// 0x40b9f0
};

}

#endif
