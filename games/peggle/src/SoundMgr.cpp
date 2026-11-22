// SoundMgr.cpp: see SoundMgr.h.

#include "SoundMgr.h"

#include "MusicInterface.h"
#include "NotTranslated.h"
#include "SoundInstance.h"
#include "ThunderballApp.h"

using namespace Sexy;

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
