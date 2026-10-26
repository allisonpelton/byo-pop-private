#include "SongPlayer.h"

#include "MusicInterface.h"

using namespace JeffLib;

bool SongPlayer::LoadSong(const std::string& theFileName, double theAmplify)
{
	bool aLoaded1 = mMusic->LoadMusic(1, theFileName);
	bool aLoaded2 = mMusic->LoadMusic(2, theFileName);
	if (theAmplify != 0.0) {
		mMusic->SetMusicAmplify(1, theAmplify);
		mMusic->SetMusicAmplify(2, theAmplify);
	}
	return aLoaded1 && aLoaded2;
}

void SongPlayer::PlaySong(int theOrder, float theSpeed, bool theLoop)
{
	mQueuedOrder = -1;
	if (mCurrentSong == -1) {
		mMusic->FadeIn(1, theOrder, theSpeed, !theLoop);
		mCurrentSong = 1;
		mCurrentOrder = theOrder;
		return;
	}
	mLastOrder = mCurrentOrder;
	if (theOrder == mCurrentOrder || theOrder == -1)
		return;
	int anOther = mCurrentSong == 1 ? 2 : 1;
	bool aFade = theSpeed > 1e-05f;
	if (mMusic->IsPlaying(mCurrentSong) && mMusic->IsPlaying(anOther)) {
		mMusic->StopMusic(anOther);
		mMusic->FadeOut(mCurrentSong, true, theSpeed);
	} else if (mMusic->IsPlaying(mCurrentSong)) {
		if (aFade)
			mMusic->FadeOut(mCurrentSong, true, theSpeed);
		else
			mMusic->StopMusic(mCurrentSong);
	}
	if (aFade)
		mMusic->FadeIn(anOther, theOrder, theSpeed, !theLoop);
	else
		mMusic->PlayMusic(anOther, theOrder, !theLoop);
	mCurrentSong = anOther;
	mCurrentOrder = theOrder;
}
