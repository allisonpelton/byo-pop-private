#include "SongPlayer.h"

#include <cmath>
#include <cstring>

#include "MusicInterface.h"

using namespace JeffLib;

SongPlayer::~SongPlayer()
{
	delete[] mSpectrum;
}

void SongPlayer::UpdateSpectrum(int theUpdateCnt, int theMode)
{
	if (mSpectrum == nullptr || theUpdateCnt % 4 != 0)
		return;
	// BASS_DATA_FFT2048 of the copy playing: 1024 magnitudes. port: the platform's mixer gives them
	// (MusicInterface::GetMusicFFT), at its own rate where BASS's were at 44.1 kHz
	float anFFT[1024];
	if (!mMusic->GetMusicFFT(mCurrentSong, anFFT, 2048))
		return;
	// a magnitude that is not a number counts as 0
	for (int i = 0; i < 1024; i++)
		if (anFFT[i] != anFFT[i])
			anFFT[i] = 0.0f;
	memset(mSpectrum, 0, mSpectrumWidth * mSpectrumHeight);
	if (theMode == 0) {
		// the square root of each bin from the second on, every other column, the one between them at the height
		// halfway between theirs
		int aLastY = 0;
		for (int x = 0; x < mSpectrumWidth / 2; x++) {
			float aLevel = (float)sqrt((double)anFFT[x + 1]);
			int y = (int)(aLevel * 3.0 * mSpectrumHeight - 4.0);
			if (y > mSpectrumHeight)
				y = mSpectrumHeight;
			int aMid;
			if (x != 0 && (aMid = (aLastY + y) / 2) != 0)
				while (--aMid >= 0)
					mSpectrum[mSpectrumWidth * aMid + x * 2 - 1] = (unsigned char)(aMid + 1);
			aLastY = y;
			while (--y >= 0)
				mSpectrum[mSpectrumWidth * y + x * 2] = (unsigned char)(y + 1);
		}
	} else if (theMode == 1) {
		// bar n sums the bins up to 2^(n * 10 / 27) (at least one), over the log of their count plus 10
		int aBin = 0;
		for (int aBar = 0; aBar < 28; aBar++) {
			float aSum = 0.0f;
			int anEnd = (int)pow(2.0, aBar * 10.0 / 27.0);
			if (anEnd > 1023)
				anEnd = 1023;
			if (anEnd <= aBin)
				anEnd = aBin + 1;
			int aCount = anEnd - aBin + 10;
			for (; aBin < anEnd; aBin++)
				aSum += anFFT[aBin + 1];
			float aLog = (float)log10((double)aCount);
			float aLevel = (float)sqrt((double)(float)(aSum / (double)aLog));
			int y = (int)(aLevel * 1.7 * mSpectrumHeight - 4.0);
			if (y > mSpectrumHeight)
				y = mSpectrumHeight;
			int aBarWidth = mSpectrumWidth / 28;
			while (--y >= 0)
				memset(mSpectrum + mSpectrumWidth * y + aBarWidth * aBar, y + 1, aBarWidth - 2);
		}
	}
}

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
	if (!aPlaying) {
		// port: a song waited on for an order that stopped before this saw the order also ends the wait. The
		// original's loop never stopped, so it always saw the order (the victory and treasure songs reach theirs
		// before they end); the port's can (a browser tab hidden or minimized stops the main loop while the music
		// plays on), and the queued song then never started: no music after a boss, until the next song
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
