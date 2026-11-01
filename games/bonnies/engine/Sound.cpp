// The sound system (see Sound.h). FMOD's calls go to the host; their arguments are the original's.

#include "Sound.h"
#include "File.h"
#include "Host.h"
#include "Platform.h"
#include "Resources.h"

namespace bonnies
{

SoundSystem* gSound = nullptr;

namespace
{
Sample* gStreamSample = nullptr;   // 0x47c754
int gStreamChannel = -1;           // 0x473e9c
void* gStreamSong = nullptr;       // 0x47c758
void* gMusic = nullptr;            // 0x47c75c
bool gMusicPlaying = false;        // 0x47c760
bool gInitialized = false;         // 0x47c768
int gLastOrder = 0;                // 0x473ea0
uint32_t gLastOrderTime = 0;       // 0x473ea4
}

// 0x41e760 (FSOUND_SetOutput(DirectSound) before Vista, FSOUND_Init(44100, 64, 0))
SoundSystem::SoundSystem()
{
	mMusicStart = 0;
	mMusicLength = 0;
	mStopOrder = 0;
	mPaused = 0;
	mDelayed = nullptr;
	mFocusPaused = 0;
	mUnused4 = -1;
	Str aPath = soundDir(true) + "music.xm";
	if (!fileExists(aPath))
		aPath = soundDir(true) + "music.mod";
	if (!fileExists(aPath))
		aPath = soundDir(true) + "music.oxm";
	if (fileExists(aPath))
		gMusic = host()->loadSong(aPath);
	gInitialized = true;
}

// 0x41e9c0
int SoundSystem::play(Sample* theSample, float theVolume, int theUnused, int theDelay, float thePitch)
{
	if (mPaused < 1 && mFocusPaused == 0 && gInitialized && theSample != nullptr && theSample->mHandle != nullptr)
	{
		if (0 < theDelay)
		{
			Delayed* aDelayed = new Delayed();
			aDelayed->mSample = theSample;
			aDelayed->mUnused = theUnused;
			aDelayed->mVolume = theVolume;
			aDelayed->mDue = host()->getTicks() + theDelay;
			aDelayed->mNext = mDelayed;
			mDelayed = aDelayed;
			return -1;
		}
		host()->trace("PSa 1");
		int aChannel = host()->playSample(theSample->mHandle);
		if (-1 < aChannel)
		{
			if (thePitch < 0.99f || 1.01f <= thePitch)
			{
				int aFrequency = host()->getFrequency(aChannel);
				if (aFrequency != 0)
					host()->setFrequency(aChannel, ftol((float)aFrequency * thePitch));
			}
			host()->setVolume(aChannel, ftol(gPlatform->getSoundVolume() * theVolume * 255.0f));
			host()->setPaused(aChannel, false);
		}
		host()->trace("PSa 2");
		return aChannel;
	}
	return 0;
}

// 0x41eb40
int SoundSystem::play(const Str& theName, float theVolume, int theUnused, int theDelay, float thePitch)
{
	if (mPaused < 1 && mFocusPaused == 0 && gInitialized)
	{
		host()->trace("PSb 1");
		Sample* aSample = gResources->getSound(theName);
		if (aSample != nullptr && aSample->mHandle != nullptr)
		{
			int aChannel = play(aSample, theVolume, theUnused, theDelay, thePitch);
			host()->trace("PSb 2");
			return aChannel;
		}
	}
	return 0;
}

// 0x41ec60
void SoundSystem::setMusicVolume(float theFactor)
{
	if (gInitialized)
	{
		if (gStreamChannel != -1)
		{
			host()->setVolume(gStreamChannel, ftol(gPlatform->getMusicVolume() * theFactor * 127.5f));
			return;
		}
		void* aSong = gMusicPlaying ? gMusic : gStreamSong;
		if (aSong != nullptr)
			host()->setSongVolume(aSong, ftol(gPlatform->getMusicVolume() * theFactor * 128.0f));
	}
}

// 0x41ece0
bool SoundSystem::isMusicPlaying()
{
	if (gInitialized)
	{
		if (gStreamChannel != -1)
			return host()->isPlaying(gStreamChannel);
		void* aSong = gMusic;
		if (gMusicPlaying || (aSong = gStreamSong, gStreamSong != nullptr))
			return host()->isSongPlaying(aSong);
	}
	return false;
}

// 0x41ed30
void SoundSystem::stopMusic()
{
	if (gInitialized)
	{
		if (gStreamChannel != -1)
		{
			host()->stopSound(gStreamChannel);
			host()->freeSample(gStreamSample->mHandle);
			delete gStreamSample;
			gStreamChannel = -1;
		}
		if (gMusicPlaying)
		{
			host()->stopSong(gMusic);
			gMusicPlaying = false;
		}
		if (gStreamSong != nullptr)
		{
			host()->stopSong(gStreamSong);
			host()->freeSong(gStreamSong);
			gStreamSong = nullptr;
		}
	}
}

// 0x41edc0
void SoundSystem::pause()
{
	setMusicVolume(0.0f);
	if (gMusicPlaying)
		host()->setSongPaused(gMusic, true);
	mFocusPaused = 1;
}

// 0x41edf0
void SoundSystem::pauseMusicOnly()
{
	if (gMusicPlaying)
		host()->setSongPaused(gMusic, true);
}

// 0x41ee10
void SoundSystem::resume()
{
	mFocusPaused = 0;
	if (gMusicPlaying)
		host()->setSongPaused(gMusic, false);
	setMusicVolume(1.0f);
}

// 0x41ee40
void SoundSystem::shutdown()
{
	stopMusic();
	if (gInitialized)
	{
		if (gMusic != nullptr)
			host()->freeSong(gMusic);
		gMusic = nullptr;
		gInitialized = false;
	}
}

// 0x41ee80 (FSOUND_Sample_Load(FSOUND_FREE, path, 0x2031, 0, 0))
Sample* SoundSystem::loadSample(const Str& theName)
{
	if (!gInitialized)
		return nullptr;
	Str aPath = soundDir(true) + theName + ".ogg";
	Sample* aSample = new Sample();
	aSample->mHandle = host()->loadSample(aPath);
	return aSample;
}

// 0x41e990
void SoundSystem::freeSample(Sample* theSample)
{
	if (gInitialized)
	{
		host()->freeSample(theSample->mHandle);
		delete theSample;
	}
}

// 0x41ef90
void SoundSystem::playMusic(int theBegin, int theEnd, int theStartOrder, int theStopOrder, float theFactor)
{
	if (gInitialized)
	{
		mMusicStart = 0;
		mMusicLength = 0;
		mStopOrder = 0;
		stopMusic();
		if (gMusic != nullptr)
		{
			int aVolume = ftol(gPlatform->getMusicVolume() * theFactor * 128.0f);
			host()->setSongVolume(gMusic, 0);
			host()->playSong(gMusic);
			host()->setSongOrder(gMusic, theStartOrder);
			host()->setSongVolume(gMusic, aVolume);
			mMusicStart = host()->getTicks();
			mMusicLength = (theEnd - theBegin) * 1000 + 500;
			mStopOrder = theStopOrder;
			gMusicPlaying = true;
		}
	}
}

// 0x41f060
void SoundSystem::playStream(const Str& theName, float theFactor)
{
	if (!gInitialized)
		return;
	Str aPath = soundDir(true) + theName;
	mMusicStart = host()->getTicks();
	mMusicLength = 0;
	mStopOrder = 0;
	stopMusic();
	if (containsNoCase(aPath, ".ogg") || containsNoCase(aPath, ".mp3") || containsNoCase(aPath, ".wav"))
	{
		gStreamSample = new Sample();
		gStreamSample->mHandle = host()->loadSample(aPath);
		gStreamChannel = host()->playSample(gStreamSample->mHandle);
		if (-1 < gStreamChannel)
		{
			host()->setVolume(gStreamChannel, ftol(gPlatform->getMusicVolume() * theFactor * 127.5f));
			host()->setPaused(gStreamChannel, false);
		}
	}
	else
	{
		gStreamSong = host()->loadSong(aPath);
		if (gStreamSong != nullptr)
		{
			host()->setSongVolume(gStreamSong, ftol(gPlatform->getMusicVolume() * theFactor * 128.0f));
			host()->playSong(gStreamSong);
		}
	}
}

// 0x41f2b0
void SoundSystem::update()
{
	if (!gInitialized)
		return;
	if (gStreamChannel != -1 && !host()->isPlaying(gStreamChannel))
		stopMusic();
	if (mStopOrder != 0 && gMusicPlaying && gMusic != nullptr && isMusicPlaying() &&
		(int)(mMusicStart + 10000) < (int)host()->getTicks())
	{
		int anOrder = host()->getSongOrder(gMusic);
		if (anOrder != gLastOrder)
		{
			gLastOrder = anOrder;
			gLastOrderTime = host()->getTicks();
		}
		if (mStopOrder < anOrder)
			stopMusic();
	}
	Delayed* aDelayed;
	while ((aDelayed = mDelayed) != nullptr)
	{
		while ((int)host()->getTicks() < (int)aDelayed->mDue)
		{
			aDelayed = aDelayed->mNext;
			if (aDelayed == nullptr)
				return;
		}
		// as the original: the list then starts after the entry played, so the entries before it are dropped unplayed
		play(aDelayed->mSample, aDelayed->mVolume, aDelayed->mUnused, 0, 1.0f);
		mDelayed = aDelayed->mNext;
		delete aDelayed;
	}
}

} // namespace bonnies
