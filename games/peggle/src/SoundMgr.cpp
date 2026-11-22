// SoundMgr.cpp: see SoundMgr.h.

#include "SoundMgr.h"

#include "MusicInterface.h"
#include "NotTranslated.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "ThunderballApp.h"

using namespace Sexy;

static const float MUSIC_FADE_STEP = 0.01f;	  // how far the music's volume moves each update once the fade has begun (ModValFloat, SoundMgr.cpp 1422, 199)
static const float MUSIC_VOLUME_FLOOR = 0.4f; // and the lowest it goes (1423, 204)
static const float SOUND_FADE_STEP = 0.05f;	  // a fading sound loses this much volume each update

// 0x45f1d0
SoundMgr::SoundMgr()
{
	Clear();
	mMusicVolume = 1.0f;
	mMusicPaused = false;
	mMusicVolumeStep = 0;
	mFadeTimer = 0;
}

// 0x442e90 (0x441f30)
void SoundMgr::Clear()
{
	mTime = 0;
	for (std::list<SoundInstance*>::iterator anItr = mFadeSounds.begin(); anItr != mFadeSounds.end(); ++anItr)
		(*anItr)->Release();
	mFadeSounds.clear();
}

// 0x437600
void SoundMgr::CancelFade()
{
	mFadeTimer = 0;
	mMusicVolumeStep = 0;
	mMusicVolume = 1.0f;
}

// 0x452810
void SoundMgr::AddFadeSound(SoundInstance* theSound)
{
	mFadeSounds.push_back(theSound);
}

// 0x437560
void SoundMgr::PauseMusic(bool thePause)
{
	if (mMusicPaused != thePause)
	{
		mMusicPaused = thePause;
		if (thePause)
			gThunderballApp->mMusicInterface->PauseAllMusic();
		else
			gThunderballApp->mMusicInterface->ResumeAllMusic();
	}
}

// 0x4375a0
void SoundMgr::FadeMusic(int theTime)
{
	NOT_TRANSLATED("SoundMgr::FadeMusic");
}

// 0x458f30
void SoundMgr::AddSound(int theId, float thePitch, int thePan, int theDelay, int theMinGap, float theVolume)
{
	if (theVolume == 0)
		return;
	SoundRequest aRequest = {theMinGap, theId, thePan, thePitch, theVolume};
	if (theDelay == 0)
		PlaySound(aRequest);
	else
		mWaiting.insert(std::make_pair(mTime + theDelay, aRequest));
}

// 0x45af70
void SoundMgr::AddSound(int theId, int theMinGap)
{
	AddSound(theId, 0, 0, 0, theMinGap, -1.0f);
}

// 0x45afa0
void SoundMgr::AddSoundPitch(int theId, float thePitch, int theMinGap)
{
	AddSound(theId, thePitch, 0, 0, theMinGap, -1.0f);
}

// 0x458e80
void SoundMgr::PlaySound(const SoundRequest& theRequest)
{
	int& aLastTime = mLastPlayed[std::make_tuple(theRequest.mId, theRequest.mPitch, theRequest.mPan)];
	if (aLastTime == 0 || theRequest.mMinGap <= gThunderballApp->mUpdateCount - aLastTime)
	{
		aLastTime = gThunderballApp->mUpdateCount;
		SoundInstance* anInstance = gThunderballApp->mSoundManager->GetSoundInstance(theRequest.mId);
		if (anInstance != nullptr)
		{
			if (theRequest.mPitch != 0)
				anInstance->AdjustPitch(theRequest.mPitch);
			if (theRequest.mPan != 0)
				anInstance->SetPan(theRequest.mPan);
			if (theRequest.mVolume > 0)
				anInstance->SetVolume(theRequest.mVolume);
			anInstance->Play(false, true);
		}
	}
}

// 0x458fc0
void SoundMgr::Update()
{
	mTime++;
	while (!mWaiting.empty() && mWaiting.begin()->first <= mTime)
	{
		SoundRequest aRequest = mWaiting.begin()->second;
		mWaiting.erase(mWaiting.begin());
		PlaySound(aRequest);
	}
	UpdateSounds();
}

// 0x441d50: the music's fade, and the sounds that fade out lose volume until they are gone
void SoundMgr::UpdateSounds()
{
	if (0 < mFadeTimer && --mFadeTimer == 0)
		mMusicVolumeStep = MUSIC_FADE_STEP;
	if (mMusicVolumeStep != 0)
	{
		float aOldVolume = mMusicVolume;
		mMusicVolume = mMusicVolumeStep + mMusicVolume;
		if (MUSIC_VOLUME_FLOOR <= mMusicVolume)
		{
			if (1.0f < mMusicVolume)
				mMusicVolume = 1.0f;
		}
		else
			mMusicVolume = MUSIC_VOLUME_FLOOR;
		if (aOldVolume != mMusicVolume)
			gThunderballApp->mMusicInterface->SetSongVolume(gThunderballApp->mCurMusic, mMusicVolume);
	}
	for (std::list<SoundInstance*>::iterator anItr = mFadeSounds.begin(); anItr != mFadeSounds.end();)
	{
		SoundInstance* aSound = *anItr;
		float aVolume = 0;
		if (aSound->IsPlaying())
		{
			aVolume = (float)aSound->GetVolume() - SOUND_FADE_STEP;
			if (aVolume > 0)
			{
				aSound->SetVolume(aVolume);
				++anItr;
				continue;
			}
		}
		aSound->Release();
		anItr = mFadeSounds.erase(anItr);
	}
}
