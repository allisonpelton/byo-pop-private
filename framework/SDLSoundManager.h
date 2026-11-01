#ifndef __SDLSOUNDMANAGER_H__
#define __SDLSOUNDMANAGER_H__

// SoundManager / SoundInstance on the port's software mixer (replaces DSoundManager/DSoundInstance).
// Volume and pan follow DirectSound semantics as used by the original framework:
//   volume 0..1 -> VolumeToDB() curve; pan -10000..10000 = hundredths of a dB of attenuation.

#include "SoundManager.h"
#include "SoundInstance.h"
#include <memory>

namespace Platform { struct SoundData; }

namespace Sexy
{

class SDLSoundManager;

// port: a test log of every sound and music event, for comparing with the original's (games/bookwormadv/tools/pilot/
// soundcmp.py). Each line is the update count and the event. Off unless BYOPOP_SOUNDLOG names a file, which is opened
// on the first event.
void SoundLog(const char* theFormat, ...);
bool SoundLogOn();

class SDLSoundInstance : public SoundInstance
{
	friend class SDLSoundManager;

protected:
	SDLSoundManager* mManager;
	std::shared_ptr<Platform::SoundData> mData;
	int mVoice = 0;
	double mBaseVolume;
	int mBasePan;
	double mVolume = 1.0;
	int mPan = 0;
	double mPitch = 1.0;
	bool mReleased = false;
	bool mAutoRelease = false;
	bool mHasPlayed = false;
	unsigned int mSfxID = 0;     // port: for the sound log
	int mLogNum = 0;             // port: the instance's number in the sound log

	void RehupVolume();
	int LogVolume() const;

public:
	SDLSoundInstance(SDLSoundManager* theManager, std::shared_ptr<Platform::SoundData> theData, double theBaseVolume, int theBasePan);
	virtual ~SDLSoundInstance();

	virtual void Release();
	virtual void SetBaseVolume(double theBaseVolume);
	virtual void SetBasePan(int theBasePan);
	virtual void AdjustPitch(double theNumSteps);
	virtual void SetVolume(double theVolume);
	virtual void SetPan(int thePosition);
	virtual bool Play(bool looping, bool autoRelease);
	virtual void Stop();
	virtual bool IsPlaying();
	virtual bool IsReleased();
	virtual double GetVolume();
	double GetPitch() const { return mPitch; }   // port: the playback rate AdjustPitch set (1.0 unshifted)
};

class SDLSoundManager : public SoundManager
{
	friend class SDLSoundInstance;

protected:
	std::shared_ptr<Platform::SoundData> mSounds[MAX_SOURCE_SOUNDS];
	double mBaseVolumes[MAX_SOURCE_SOUNDS];
	int mBasePans[MAX_SOURCE_SOUNDS];
	SDLSoundInstance* mInstances[MAX_CHANNELS];
	double mMasterVolume = 1.0;

	int FindFreeChannel();
	void ReleaseFinishedChannels();

public:
	SDLSoundManager();
	virtual ~SDLSoundManager();

	// DirectSound volume curve from DSoundManager::VolumeToDB, converted to linear amplitude.
	static double VolumeToGain(double theVolume);

	virtual bool Initialized();
	virtual bool LoadSound(unsigned int theSfxID, const std::string& theFilename);
	virtual int LoadSound(const std::string& theFilename);
	virtual void ReleaseSound(unsigned int theSfxID);
	virtual void SetVolume(double theVolume);
	virtual bool SetBaseVolume(unsigned int theSfxID, double theBaseVolume);
	virtual bool SetBasePan(unsigned int theSfxID, int theBasePan);
	virtual SoundInstance* GetSoundInstance(unsigned int theSfxID);
	virtual void ReleaseSounds();
	virtual void ReleaseChannels();
	virtual double GetMasterVolume();
	virtual void SetMasterVolume(double theVolume);
	virtual void Flush();
	virtual void SetCooperativeWindow(HWND theHWnd, bool isWindowed);
	virtual void StopAllSounds();
	virtual int GetFreeSoundId();
	virtual int GetNumSounds();
};

}

#endif
