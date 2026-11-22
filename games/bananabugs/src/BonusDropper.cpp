#include "BonusDropper.h"

#include "GameApp.h"
#include "LevelObj.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"

using namespace Sexy;

// 0x40db20: held only (state 2): its sound, and it is put down
bool BonusDropper::Place()
{
	if (mState != BONUS_HELD)
		return false;
	sApp->mSoundQueue->Request(SOUND_COCOLETGO, 0, false);
	return PutDown(mDropTrack);
}

// 0x40c860
void BonusFadingDropper::End()
{
	StartTrack(mEndTrack);
}
