// port: the engine's sound over the platform's mixer (see SimpleSoundDX.h).

#include "SimpleSoundDX.h"
#include "Engine.h"
#include "Stream.h"

#include "platform/Audio.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

using namespace simplesounddx;

namespace
{

const float MILLISECONDS_TO_SECONDS = 0.001f;  // 0x566b80 (feedingfrenzy2)
const double VOLUME_SCALE = 2000.0;            // 0x5669a4 (feedingfrenzy2): DirectSound's hundredths of a decibel
const double SILENCE = -10000.0;               // 0x566854 (feedingfrenzy2): DSBVOLUME_MIN
const int TIMER_PERIOD = 100;                  // the window's timer, in milliseconds (WM_TIMER 1)
const int READ_CHUNK = 65536;

// The volume heard as the mixer's gain: the original sets DirectSound's volume to 2000 times the volume's natural
// logarithm (0x435f60 (feedingfrenzy2)), at least DSBVOLUME_MIN; that many hundredths of a decibel are an amplitude
// of 10 to the volume divided by 2000, so the volume to the power ln 10. DSBVOLUME_MIN is silence
float gainOf(float theVolume)
{
	if (!(theVolume > 0.0f))
		return 0.0f;
	double aHundredths = VOLUME_SCALE * std::log((double)theVolume);
	if (!(aHundredths > SILENCE))
		return 0.0f;
	return (float)std::pow(10.0, aHundredths / VOLUME_SCALE);
}

bool readWhole(engine::InputStream* theStream, std::vector<uint8_t>& theBytes)
{
	if (theStream == nullptr)
		return false;
	uint8_t aChunk[READ_CHUNK];
	for (;;)
	{
		int aRead = theStream->read(aChunk, READ_CHUNK);
		if (aRead <= 0)
			break;
		theBytes.insert(theBytes.end(), aChunk, aChunk + aRead);
	}
	theStream->close();
	return !theBytes.empty();
}

} // namespace

SoundHandleDX::SoundHandleDX(SimpleSoundDX* theSystem, SoundSource* theSource)
    : mSystem(theSystem), mSource(theSource), mVoice(0), mCategory(0), mVolume(1.0f), mLooping(false), mPlaying(false),
      mEnded(false), mVolumeHeard(0.0f), mFadeInTime(0.0f), mFadeOutTime(0.0f), mFadeTimeLeft(-1.0f),
      mFadeStep(FADE_NONE)
{
	if (mSource->mStream)
		mSource->mTaken = true;
	mSystem->addHandle(this);
}

SoundHandleDX::~SoundHandleDX()
{
	if (mVoice != 0)
		Platform::Mixer::Get().StopVoice(mVoice);
	mSource->mTaken = false;
	mSystem->removeHandle(this);
}

void SoundHandleDX::play()
{
	if (mEnded)
		stop();
	if (mPlaying)
		return;
	if (mFadeInTime > 0.0f)
	{
		mFadeTimeLeft = mFadeInTime;
		mFadeStep = FADE_IN;
		applyVolume(0.0f);
	}
	else
	{
		applyVolume(mVolume);
		mFadeStep = FADE_NONE;
	}
	// The DirectSound buffer always loops, the stream refilling it; port: the voice loops when the handle does, and
	// starts from where the sound was put back to (its start)
	float aGain = gainOf(mVolumeHeard);
	mVoice = Platform::Mixer::Get().StartVoice(mSource->mData, aGain, aGain, 1.0, mLooping);
	mPlaying = true;
}

void SoundHandleDX::stop()
{
	if (mFadeOutTime > 0.0f)
	{
		mFadeStep = FADE_OUT;
		mFadeTimeLeft = mFadeOutTime;
		return;
	}
	if (mVoice != 0)
		Platform::Mixer::Get().StopVoice(mVoice);
	mVoice = 0;
	mPlaying = false;
	rewind();
	// The listener's slot 0x04 (+0x04): no listener is translated yet
}

void SoundHandleDX::rewind()
{
	if (mPlaying)
	{
		if (mVoice != 0)
			Platform::Mixer::Get().StopVoice(mVoice);
		mVoice = 0;
		mPlaying = false;
	}
	mEnded = false;
}

void SoundHandleDX::setVolume(float theVolume)
{
	mVolume = std::min(1.0f, std::max(0.0f, theVolume));
	mVolumeHeard = -1.0f;
}

void SoundHandleDX::setFadeInTime(float theSeconds)
{
	mFadeInTime = theSeconds > 0.0f ? theSeconds : 0.0f;
}

void SoundHandleDX::setFadeOutTime(float theSeconds)
{
	mFadeOutTime = theSeconds > 0.0f ? theSeconds : 0.0f;
}

void SoundHandleDX::update(int theMilliseconds)
{
	if (!mPlaying)
		return;
	checkEnd();
	if (!mPlaying)
		return;
	updateFade(theMilliseconds);
}

void SoundHandleDX::checkEnd()
{
	// (a voice the mixer had none free for counts as ended)
	if (mEnded || (mVoice != 0 && Platform::Mixer::Get().IsVoicePlaying(mVoice)))
		return;
	mEnded = true;
	stop();
}

void SoundHandleDX::updateFade(int theMilliseconds)
{
	float aVolume = mVolume;
	float aStep = (float)theMilliseconds * MILLISECONDS_TO_SECONDS;
	if (mFadeStep == FADE_IN)
	{
		mFadeTimeLeft = mFadeTimeLeft - aStep;
		if (!(mFadeTimeLeft >= 0.0f))
		{
			mFadeTimeLeft = 0.0f;
			mFadeStep = FADE_NONE;
		}
		aVolume = (float)(((double)mFadeInTime - mFadeTimeLeft) / mFadeInTime * mVolume);
	}
	else if (mFadeStep == FADE_NONE)
	{
		// A sound that does not loop fades out once no more than the fade-out's time of it is left (slot 0x74, the
		// fade-out's bytes, against the bytes played): the port's mixer gives no such count, and no translated
		// caller sets a fade-out on a sound that plays to its end, so this is not yet translated
	}
	else if (mFadeStep == FADE_OUT || mFadeStep == FADE_OUT_AGAIN)
	{
		mFadeTimeLeft = mFadeTimeLeft - aStep;
		if (!(mFadeTimeLeft >= 0.0f))
			mFadeTimeLeft = 0.0f;
		aVolume = (float)((1.0 - ((double)mFadeOutTime - mFadeTimeLeft) / mFadeOutTime) * mVolume);
	}
	applyVolume(aVolume);
	if (!(aVolume > 0.0f) && mFadeStep == FADE_OUT)
		endFadeOut();
}

void SoundHandleDX::endFadeOut()
{
	if (mVoice == 0)
		return;
	Platform::Mixer::Get().StopVoice(mVoice);
	mVoice = 0;
	mPlaying = false;
	// The listener's slot 0x04 (+0x04): no listener is translated yet
	rewind();
}

void SoundHandleDX::applyVolume(float theVolume)
{
	float aCategoryVolume = mSystem->getCategoryVolume(mCategory);
	float aHeard = (float)((double)mSystem->getMasterVolume() * aCategoryVolume * theVolume);
	if (aCategoryVolume == 0.0f || mVolumeHeard != aHeard)
	{
		mVolumeHeard = aHeard;
		if (mVoice != 0)
		{
			float aGain = gainOf(aHeard);
			Platform::Mixer::Get().SetVoiceGain(mVoice, aGain, aGain);
		}
	}
}

SimpleSoundDX::SimpleSoundDX() : mMasterVolume(0.0f), mTimerLeft(0.0)
{
}

SimpleSoundDX::~SimpleSoundDX()
{
	if (simplesound::getSimpleSound() == this)
		simplesound::setSimpleSound(nullptr);
}

bool SimpleSoundDX::isRegistered(const std::string& theName)
{
	return mSources.find(theName) != mSources.end();
}

bool SimpleSoundDX::registerSound(const std::string& theName, engine::InputStream* theStream)
{
	return registerSource(theName, theStream, false);
}

bool SimpleSoundDX::registerStream(const std::string& theName, engine::InputStream* theStream)
{
	return registerSource(theName, theStream, true);
}

bool SimpleSoundDX::registerSource(const std::string& theName, engine::InputStream* theStream, bool theDecodedAsPlayed)
{
	if (isRegistered(theName))
		return true;
	std::vector<uint8_t> aBytes;
	if (!readWhole(theStream, aBytes))
		return false;
	Platform::Mixer& aMixer = Platform::Mixer::Get();
	std::shared_ptr<Platform::SoundData> aData =
	    theDecodedAsPlayed ? aMixer.LoadStreamedSoundData(aBytes, theName) : aMixer.LoadSoundData(aBytes, theName);
	if (!aData)
		return false;
	SoundSource& aSource = mSources[theName];
	aSource.mData = aData;
	aSource.mStream = theDecodedAsPlayed;
	return true;
}

simplesound::SoundHandle* SimpleSoundDX::getHandle(const std::string& theName, int theSetupValue, int theCategory)
{
	std::map<std::string, SoundSource>::iterator anEntry = mSources.find(theName);
	if (anEntry == mSources.end())
		return nullptr;
	SoundSource* aSource = &anEntry->second;
	if (aSource->mStream && aSource->mTaken)
		return nullptr;
	SoundHandleDX* aHandle = new SoundHandleDX(this, aSource);
	aHandle->setCategory(theCategory);
	return aHandle;
}

void SimpleSoundDX::setMasterVolume(float theVolume)
{
	mMasterVolume = std::min(1.0f, std::max(0.0f, theVolume));
	applyVolumes();
}

void SimpleSoundDX::stopAll()
{
	for (SoundHandleDX* aHandle : mHandles)
		aHandle->stop();
}

void SimpleSoundDX::update(int theMilliseconds)
{
	// A copy, as an update may end a handle's sound and so let a caller delete another
	std::vector<SoundHandleDX*> aHandles = mHandles;
	for (SoundHandleDX* aHandle : aHandles)
	{
		if (std::find(mHandles.begin(), mHandles.end(), aHandle) != mHandles.end())
			aHandle->update(theMilliseconds);
	}
}

void SimpleSoundDX::applyVolumes()
{
	// Each handle's volume set again (so that it is applied anew) and its update run for no time
	std::vector<SoundHandleDX*> aHandles = mHandles;
	for (SoundHandleDX* aHandle : aHandles)
	{
		aHandle->setVolume(aHandle->getVolume());
		aHandle->update(0);
	}
}

void SimpleSoundDX::advanceTimer(double theMilliseconds)
{
	mTimerLeft += theMilliseconds;
	while (mTimerLeft >= TIMER_PERIOD)
	{
		mTimerLeft -= TIMER_PERIOD;
		update(TIMER_PERIOD);
	}
}

void SimpleSoundDX::removeHandle(SoundHandleDX* theHandle)
{
	mHandles.erase(std::remove(mHandles.begin(), mHandles.end(), theHandle), mHandles.end());
}
