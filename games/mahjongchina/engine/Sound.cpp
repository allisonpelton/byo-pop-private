#include "Sound.h"

#include "Host.h"

#include "platform/Audio.h"
#include "platform/openmpt/OpenMPTPlayer.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace jb
{

static void SoundLog(const char* theFormat, ...);

// The music as BASS played it: libopenmpt's player, with the channel's volume (and its slides) and the MUSICPOS
// sync of SetMusicLoop applied as it renders. Changed only between Mixer::Lock and Unlock.
class MusicPlayer : public Platform::ModulePlayer
{
public:
	std::unique_ptr<Platform::ModulePlayer> mInner;
	int mRate;
	float mGain = 1.0f;
	float mSlideTo = 1.0f;
	float mSlideStep = 0.0f;   // per frame
	int mSlideLeft = 0;        // frames
	bool mSyncSet = false;     // 0x5d9368
	int mSyncRowStep = 1;
	int mSyncOrder = 0;
	int mLastOrder = -1;
	int mLastRow = -1;

	MusicPlayer(std::unique_ptr<Platform::ModulePlayer> theInner, int theRate)
		: mInner(std::move(theInner)), mRate(theRate)
	{
	}

	void SetPosition(int theOrder, int theRow) override
	{
		mInner->SetPosition(theOrder, theRow);
		mLastOrder = -1;
		mLastRow = -1;
	}

	bool GetPosition(int& theOrder, int& theRow) override
	{
		return mInner->GetPosition(theOrder, theRow);
	}

	// BASS_ChannelSetAttributes
	void SetGain(float theGain)
	{
		mGain = theGain;
		mSlideLeft = 0;
	}

	// BASS_ChannelSlideAttributes
	void Slide(float theGain, int theMs)
	{
		int aFrames = (int)((int64_t)theMs * mRate / 1000);
		if (aFrames <= 0)
		{
			SetGain(theGain);
			return;
		}
		mSlideTo = theGain;
		mSlideLeft = aFrames;
		mSlideStep = (theGain - mGain) / aFrames;
	}

	bool Render(float* theOut, int theFrames, bool theLoop) override
	{
		// small pieces, so that a row change is seen within about a millisecond of it
		const int kPiece = 64;
		bool aPlaying = true;
		int aDone = 0;
		while (aDone < theFrames)
		{
			int aCount = std::min(kPiece, theFrames - aDone);
			float* anOut = theOut + aDone * 2;
			if (!mInner->Render(anOut, aCount, theLoop))
				aPlaying = false;
			for (int i = 0; i < aCount; i++)
			{
				if (0 < mSlideLeft)
				{
					mGain += mSlideStep;
					mSlideLeft--;
					if (mSlideLeft == 0)
						mGain = mSlideTo;
				}
				anOut[i * 2] *= mGain;
				anOut[i * 2 + 1] *= mGain;
			}
			aDone += aCount;
			int anOrder, aRow;
			if (mInner->GetPosition(anOrder, aRow) && (anOrder != mLastOrder || aRow != mLastRow))
			{
				mLastOrder = anOrder;
				mLastRow = aRow;
				// 0x4372b0: the sync's row a multiple of the step jumps to the order, and the sync is removed
				if (mSyncSet && aRow % mSyncRowStep == 0)
				{
					SoundLog("music position %d", mSyncOrder);
					mSyncSet = false;
					SetPosition(mSyncOrder, 0);
				}
			}
			if (!aPlaying)
			{
				if (aDone < theFrames)
					memset(theOut + aDone * 2, 0, sizeof(float) * 2 * (size_t)(theFrames - aDone));
				break;
			}
		}
		return aPlaying;
	}
};

static float Gain(int theVolume)
{
	return theVolume / 100.0f;
}

// A record of what the sound plays, for comparing with the original's BASS calls (tools/soundlog.py): written only
// when the environment's MJ_SOUNDLOG names a file
static void SoundLog(const char* theFormat, ...)
{
	static FILE* aFile = nullptr;
	static bool isOpen = false;
	if (!isOpen)
	{
		isOpen = true;
		const char* aPath = getenv("MJ_SOUNDLOG");
		if (aPath != nullptr && aPath[0] != 0)
			aFile = fopen(aPath, "w");
	}
	if (aFile == nullptr)
		return;
	fprintf(aFile, "%u ", GetHost()->TickCount());
	va_list anArgs;
	va_start(anArgs, theFormat);
	vfprintf(aFile, theFormat, anArgs);
	va_end(anArgs);
	fputc('\n', aFile);
	fflush(aFile);
}

static uint32_t Crc32(const uint8_t* theData, uint32_t theSize)
{
	uint32_t aCrc = 0xffffffff;
	for (uint32_t i = 0; i < theSize; i++)
	{
		aCrc ^= theData[i];
		for (int k = 0; k < 8; k++)
			aCrc = (aCrc >> 1) ^ (0xedb88320 & (0 - (aCrc & 1)));
	}
	return ~aCrc;
}

// 0x436860 (BASS_SampleFree, BASS_MusicFree)
Sound::~Sound()
{
	Shutdown();
	for (auto& anEntry : mSamples)
		delete anEntry.second;
	mSamples.clear();
	for (auto& anEntry : mMusics)
	{
		Platform::Mixer::Get().UnloadSong(anEntry.second->mSong);
		delete anEntry.second;
	}
	mMusics.clear();
}

// 0x436b20
void Sound::Init()
{
	if (!mInitialized)
	{
		Platform::RegisterOpenMPTModules();
		mInitialized = Platform::Mixer::Get().IsInitialized() || Platform::Mixer::Get().Init();
	}
}

// 0x436b50
void Sound::Shutdown()
{
	if (mInitialized)
	{
		mInitialized = false;
		Platform::Mixer& aMixer = Platform::Mixer::Get();
		for (auto& anEntry : mSamples)
		{
			for (int aVoice : anEntry.second->mVoices)
				aMixer.StopVoice(aVoice);
			anEntry.second->mVoices.clear();
			anEntry.second->mLast = 0;
		}
		for (auto& anEntry : mMusics)
			aMixer.StopSong(anEntry.second->mSong);
		mMusic = nullptr;
	}
}

// 0x436b70, 0x436c50
void Sound::Load(int theId, bool theLoop)
{
	if (!mInitialized || mSamples.find(theId) != mSamples.end())
		return;
	uint32_t aSize = 0;
	const uint8_t* aData = FindResourceData(Module::Dll, "WAVE", theId & 0xffff, &aSize);
	if (aData == nullptr)
		return;
	std::vector<uint8_t> aFile(aData, aData + aSize);
	std::shared_ptr<Platform::SoundData> aSound = Platform::Mixer::Get().LoadSoundData(aFile, "WAVE");
	if (aSound == nullptr)
		return;
	SoundLog("load %d %08x %d", theId, Crc32(aData, aSize), theLoop ? 1 : 0);
	Sample* aSample = new Sample();
	aSample->mData = aSound;
	aSample->mLoop = theLoop;
	aSample->mMax = theLoop ? 1 : 10;
	mSamples[theId] = aSample;
}

// 0x436d50: BASS_SampleGetChannel takes a free channel, or with BASS_SAMPLE_OVER_POS the one that has played longest
void Sound::Play(int theId)
{
	if (!mInitialized || mInactive)
		return;
	auto anIt = mSamples.find(theId);
	if (anIt == mSamples.end())
		return;
	Sample* aSample = anIt->second;
	Platform::Mixer& aMixer = Platform::Mixer::Get();
	std::vector<int>& aVoices = aSample->mVoices;
	aVoices.erase(std::remove_if(aVoices.begin(), aVoices.end(),
		[&](int theVoice) { return !aMixer.IsVoicePlaying(theVoice); }), aVoices.end());
	if ((int)aVoices.size() >= aSample->mMax)
	{
		aMixer.StopVoice(aVoices.front());
		aVoices.erase(aVoices.begin());
	}
	SoundLog("play %d %d", theId, mVolume);
	int aVoice = aMixer.StartVoice(aSample->mData, Gain(mVolume), Gain(mVolume), 1.0, aSample->mLoop);
	if (aVoice > 0)
	{
		aVoices.push_back(aVoice);
		aSample->mLast = aVoice;
		aSample->mLastPaused = false;
	}
}

// 0x436de0: BASS_SampleStop stops all of the sample's channels
void Sound::Stop(int theId)
{
	if (!mInitialized)
		return;
	auto anIt = mSamples.find(theId);
	if (anIt == mSamples.end())
		return;
	Sample* aSample = anIt->second;
	SoundLog("stop %d", theId);
	for (int aVoice : aSample->mVoices)
		Platform::Mixer::Get().StopVoice(aVoice);
	aSample->mVoices.clear();
	aSample->mLastPaused = false;
	aSample->mPaused = false;
}

// 0x436e50
void Sound::StopAllExcept(int theId)
{
	if (!mInitialized)
		return;
	for (auto& anEntry : mSamples)
	{
		if (anEntry.first != theId)
			Stop(anEntry.first);
	}
}

int Sound::LastState(Sample* theSample)
{
	if (theSample->mLast == 0 || !Platform::Mixer::Get().IsVoicePlaying(theSample->mLast))
		return 0;
	return theSample->mLastPaused ? 3 : 1;
}

void Sound::ApplyVolume(Sample* theSample, int theVoice)
{
	Platform::Mixer::Get().SetVoiceGain(theVoice, Gain(mVolume), Gain(mVolume));
	(void)theSample;
}

// A paused channel holds its place: no step and no output
void Sound::PauseVoice(int theVoice, bool thePause)
{
	Platform::Mixer& aMixer = Platform::Mixer::Get();
	if (thePause)
	{
		aMixer.SetVoicePitch(theVoice, 0.0);
		aMixer.SetVoiceGain(theVoice, 0.0f, 0.0f);
	}
	else
	{
		aMixer.SetVoicePitch(theVoice, 1.0);
		aMixer.SetVoiceGain(theVoice, Gain(mVolume), Gain(mVolume));
	}
}

// 0x436ec0
void Sound::SetVolume(int theVolume)
{
	if (!mInitialized)
		return;
	mVolume = theVolume;
	SoundLog("volume %d", theVolume);
	for (auto& anEntry : mSamples)
	{
		Sample* aSample = anEntry.second;
		int aState = LastState(aSample);
		if (aState == 1)
			ApplyVolume(aSample, aSample->mLast);
		// a paused channel takes the volume when it resumes (PauseVoice)
	}
}

// 0x436f60
void Sound::Pause(bool thePause, bool theLoopingOnly, int theExcept)
{
	if (!mInitialized)
		return;
	for (auto& anEntry : mSamples)
	{
		Sample* aSample = anEntry.second;
		if ((!theLoopingOnly || aSample->mLoop) && theExcept != anEntry.first)
		{
			if (!thePause)
			{
				if (aSample->mLast != 0 && LastState(aSample) == 3)
				{
					SoundLog("resume %d", anEntry.first);
					PauseVoice(aSample->mLast, false);
					aSample->mLastPaused = false;
					aSample->mPaused = false;
				}
			}
			else if (aSample->mLast != 0 && LastState(aSample) == 1)
			{
				SoundLog("pause %d", anEntry.first);
				PauseVoice(aSample->mLast, true);
				aSample->mLastPaused = true;
				aSample->mPaused = true;
			}
		}
	}
}

// 0x437520: the samples Pause has not paused
void Sound::PauseSamples(bool thePause)
{
	for (auto& anEntry : mSamples)
	{
		Sample* aSample = anEntry.second;
		if (aSample->mPaused || aSample->mLast == 0)
			continue;
		if (thePause)
		{
			if (LastState(aSample) == 1)
			{
				SoundLog("pause %d", anEntry.first);
				PauseVoice(aSample->mLast, true);
				aSample->mLastPaused = true;
			}
		}
		else if (LastState(aSample) == 3)
		{
			SoundLog("resume %d", anEntry.first);
			PauseVoice(aSample->mLast, false);
			aSample->mLastPaused = false;
		}
	}
}

// 0x437010 (BASS_MusicLoad with BASS_MUSIC_LOOP | BASS_MUSIC_POSRESET)
void Sound::LoadMusic(int theId)
{
	if (!mInitialized || mMusics.find(theId) != mMusics.end())
		return;
	uint32_t aSize = 0;
	const uint8_t* aData = FindResourceData(Module::Dll, "WAVE", theId & 0xffff, &aSize);
	if (aData == nullptr)
		return;
	Platform::Mixer& aMixer = Platform::Mixer::Get();
	std::vector<uint8_t> aFile(aData, aData + aSize);
	std::unique_ptr<Platform::ModulePlayer> anInner = Platform::CreateOpenMPTModule(aFile, aMixer.GetRate());
	if (anInner == nullptr)
		return;
	SoundLog("music load %d %08x", theId, Crc32(aData, aSize < 0x8000 ? aSize : 0x8000));
	MusicPlayer* aPlayer = new MusicPlayer(std::move(anInner), aMixer.GetRate());
	aPlayer->SetGain(Gain(mMusicVolume));
	int aSong = aMixer.AddSong(std::unique_ptr<Platform::ModulePlayer>(aPlayer));
	if (aSong == 0)
		return;
	Music* aMusic = new Music();
	aMusic->mSong = aSong;
	aMusic->mPlayer = aPlayer;
	mMusics[theId] = aMusic;
}

// 0x4371a0
void Sound::PlayMusic(int theId, bool theRestart, int theFadeMs, int theOrder)
{
	if (!mInitialized)
		return;
	Music* aMusic = nullptr;
	auto anIt = mMusics.find(theId);
	if (anIt != mMusics.end())
		aMusic = anIt->second;
	if (!theRestart && mMusic == aMusic)
	{
		if (mMusicPaused)
			PauseMusic(false);
		return;
	}
	StopMusic();
	if (aMusic == nullptr)
		return;
	Platform::Mixer& aMixer = Platform::Mixer::Get();
	SoundLog("music play %d fade %d order %d", theId, theFadeMs, theOrder);
	mMusic = aMusic;
	aMixer.Lock();
	aMusic->mPlayer->SetGain(theFadeMs == 0 ? Gain(mMusicVolume) : 0.0f);
	aMixer.Unlock();
	aMixer.PlaySong(aMusic->mSong, 0, 0, true, true);
	aMixer.Lock();
	if (theOrder != 0)
		aMusic->mPlayer->SetPosition(theOrder & 0xffff, 0);
	if (theFadeMs != 0)
		aMusic->mPlayer->Slide(Gain(mMusicVolume), theFadeMs);
	aMixer.Unlock();
	mMusicPaused = false;
	if (mInactive)
		aMixer.PauseSong(aMusic->mSong, true);
}

// 0x437300
void Sound::SetMusicLoop(int theRowStep, int theOrder)
{
	if (mMusic == nullptr)
		return;
	Platform::Mixer& aMixer = Platform::Mixer::Get();
	SoundLog("music loop %d %d", theRowStep, theOrder);
	aMixer.Lock();
	mMusic->mPlayer->mSyncSet = true;
	mMusic->mPlayer->mSyncRowStep = (theRowStep & 0xffff) == 0 ? 1 : (theRowStep & 0xffff);
	mMusic->mPlayer->mSyncOrder = theOrder & 0xffff;
	aMixer.Unlock();
}

// 0x437360
void Sound::StopMusic()
{
	if (!mInitialized || mMusic == nullptr)
		return;
	Platform::Mixer& aMixer = Platform::Mixer::Get();
	SoundLog("music stop");
	aMixer.Lock();
	mMusic->mPlayer->mSyncSet = false;
	aMixer.Unlock();
	aMixer.StopSong(mMusic->mSong);
	mMusic = nullptr;
}

// 0x4373a0
void Sound::PauseMusic(bool thePause)
{
	if (!mInitialized || mMusic == nullptr || mMusicPaused == thePause)
		return;
	Platform::Mixer& aMixer = Platform::Mixer::Get();
	if (!thePause)
	{
		if (!aMixer.IsSongPlaying(mMusic->mSong))
		{
			SoundLog("music resume");
			aMixer.PauseSong(mMusic->mSong, false);
			mMusicPaused = false;
		}
	}
	else if (aMixer.IsSongPlaying(mMusic->mSong))
	{
		SoundLog("music pause");
		aMixer.PauseSong(mMusic->mSong, true);
		mMusicPaused = true;
	}
}

// 0x437400
void Sound::SetMusicVolume(int theVolume)
{
	if (!mInitialized)
		return;
	mMusicVolume = theVolume;
	SoundLog("music volume %d", theVolume);
	Platform::Mixer& aMixer = Platform::Mixer::Get();
	aMixer.Lock();
	for (auto& anEntry : mMusics)
		anEntry.second->mPlayer->SetGain(Gain(theVolume));
	aMixer.Unlock();
}

// 0x437470
void Sound::SlideMusic(int thePercent, int theMs)
{
	if (!mInitialized || mMusic == nullptr)
		return;
	int aVolume = MulDiv(mMusicVolume, thePercent, 100);
	SoundLog("music slide %d %d", aVolume, theMs);
	Platform::Mixer& aMixer = Platform::Mixer::Get();
	aMixer.Lock();
	mMusic->mPlayer->Slide(Gain(aVolume), theMs);
	aMixer.Unlock();
}

// 0x4374b0
void Sound::Activate(bool theActive)
{
	if (!mInitialized)
		return;
	Platform::Mixer& aMixer = Platform::Mixer::Get();
	if (!theActive)
	{
		if (!mInactive)
		{
			if (mMusic != nullptr && !mMusicPaused)
			{
				SoundLog("music pause");
				aMixer.PauseSong(mMusic->mSong, true);
			}
			PauseSamples(true);
			mInactive = true;
		}
	}
	else if (mInactive)
	{
		if (mMusic != nullptr && !mMusicPaused)
		{
			SoundLog("music resume");
			aMixer.PauseSong(mMusic->mSong, false);
		}
		PauseSamples(false);
		mInactive = false;
	}
}

} // namespace jb
