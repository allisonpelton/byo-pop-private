// port: test hook for -audiocapture=<file.wav> (AudioCapture.h; Bejeweled's, games/bejeweled/src/testhooks, copied). Not part of the original game.

#include "AudioCapture.h"

#include "platform/Audio.h"

#include <SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

using namespace Sexy;

namespace
{

void PutU32(uint8_t* theOut, uint32_t theValue)
{
	for (int i = 0; i < 4; i++)
		theOut[i] = (uint8_t) (theValue >> (8 * i));
}

void PutU16(uint8_t* theOut, uint16_t theValue)
{
	theOut[0] = (uint8_t) theValue;
	theOut[1] = (uint8_t) (theValue >> 8);
}

// The 44-byte header of a PCM WAV: 2 channels, 16 bits
void WriteHeader(FILE* theFile, int theRate, long long theFrames)
{
	uint32_t aDataSize = (uint32_t) (theFrames * 4);
	uint8_t aHeader[44] = {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ', 16, 0, 0, 0, 1, 0, 2, 0};
	PutU32(aHeader + 4, 36 + aDataSize);
	PutU32(aHeader + 24, (uint32_t) theRate);
	PutU32(aHeader + 28, (uint32_t) theRate * 4);
	PutU16(aHeader + 32, 4);
	PutU16(aHeader + 34, 16);
	aHeader[36] = 'd';
	aHeader[37] = 'a';
	aHeader[38] = 't';
	aHeader[39] = 'a';
	PutU32(aHeader + 40, aDataSize);
	fseek(theFile, 0, SEEK_SET);
	fwrite(aHeader, 1, sizeof(aHeader), theFile);
}

}

AudioCapture::AudioCapture(const std::string& theFileName) :
	mFile(fopen(theFileName.c_str(), "wb")), mLocked(false), mRate(Platform::Mixer::Get().GetRate()), mUpdates(0), mFrames(0)
{
	if (mFile == NULL)
	{
		SDL_Log("audiocapture: cannot write %s", theFileName.c_str());
		return;
	}
	WriteHeader(mFile, mRate, 0);
	Platform::Mixer::Get().Lock();
	mLocked = Platform::Mixer::Get().IsInitialized();
	SDL_Log("audiocapture: %s, %d Hz%s", theFileName.c_str(), mRate, mLocked ? "" : " (no audio device: silence)");
}

AudioCapture::~AudioCapture()
{
	Finish();
}

void AudioCapture::Update()
{
	if (mFile == NULL)
		return;
	// Exactly mRate frames every 100 updates, whatever the rate
	mUpdates++;
	int aFrames = (int) (mUpdates * mRate / 100 - mFrames);
	mMix.assign((size_t) aFrames * 2, 0.0f);
	if (mLocked)
		Platform::Mixer::Get().RenderOffline(mMix.data(), aFrames);
	mPcm.resize(mMix.size());
	for (size_t i = 0; i < mMix.size(); i++)
		mPcm[i] = (short) std::lround(std::max(-1.0f, std::min(1.0f, mMix[i])) * 32767.0f);
	fwrite(mPcm.data(), sizeof(short), mPcm.size(), mFile);
	mFrames += aFrames;
}

void AudioCapture::Finish()
{
	if (mFile != NULL)
	{
		WriteHeader(mFile, mRate, mFrames);
		fclose(mFile);
		mFile = NULL;
		SDL_Log("audiocapture: %lld frames (%.2f s)", mFrames, (double) mFrames / mRate);
	}
	if (mLocked)
	{
		Platform::Mixer::Get().Unlock();
		mLocked = false;
	}
}
