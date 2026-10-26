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

void SongPlayer::PlaySongWhenDone(int theOrder, float theSpeed, int theAtOrder, int theAtRow)
{
	mQueuedSpeed = theSpeed;
	mQueuedAtOrder = theAtOrder;
	mQueuedAtRow = theAtRow;
	mQueuedOrder = -1;
	if (!mMusic->IsPlaying(mCurrentSong)) {
		PlaySong(theOrder, 0.005f, true);
		return;
	}
	mQueuedOrder = theOrder;
}

void SongPlayer::Update()
{
	// port: the original checked this in its loop over the songs, for the copy playing, with BASS's channel state;
	// a fade out to silence that stopped that copy also ended it. The framework stops a song a fade out silenced,
	// so a stopped copy covers both when no order is waited for.
	if (mQueuedOrder == -1 || mCurrentSong == -1)
		return;
	bool aDone = false;
	bool aPlaying = mMusic->IsPlaying(mCurrentSong);
	if (mQueuedAtOrder == -1 && !aPlaying) {
		aDone = true;
	} else if (aPlaying && mQueuedAtOrder != -1) {
		int aPosition = mMusic->GetMusicPosition(mCurrentSong);
		if (aPosition != -1 && mQueuedAtOrder == (aPosition & 0xffff) &&
		    (mQueuedAtRow == -1 || mQueuedAtRow == (int)((unsigned int)aPosition >> 16)))
			aDone = true;
	}
	if (aDone)
		PlaySong(mQueuedOrder, mQueuedSpeed, true);
}
