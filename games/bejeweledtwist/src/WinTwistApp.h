// Sexy::WinTwistApp (vtable 0x7f433c, 111 slots; 0x75e900-0x768d00): Bejeweled Twist's app. Its fields are named
// here by their offsets in the original's object; the framework's own fields are SexyAppBase's.
#pragma once

#include "SexyApp.h"

#include <vector>

namespace Sexy {

class CustomResourceManager;
class SoundInstance;
class Widget;

class WinTwistApp : public SexyApp {
public:
	// The three sizes of the game's art (images\1200\, images\768\, images\600\): the height of the screen it was
	// drawn for. 2D mode always uses 600 at 800x600.
	enum { ART_RES_1200 = 1200, ART_RES_768 = 768, ART_RES_600 = 600 };

	int mArtRes = 0;                    // +0xf2c
	float mScale = 0.0f;                // +0xf30: mArtRes / 1200, from the art's 1600x1200 to the screen
	bool mAllowArtRes1200 = true;       // +0xf28: the largest art only when the machine can take it
	bool mUseSexyCache = true;          // +0xf14: PopCap's image cache (SexyCache) had the pictures: no loader screen
	bool mInitialLoadDone = false;      // +0xf8d: Init got past loading the loader screen's resources
	bool mTubeLoaded = false;           // +0xf8c: the Tube group (0x763092)
	bool mLoadingThreadDone = false;    // +0x7f1: LoadingThreadProc loaded everything
	int mLoadMode = 0;                  // +0xf18: 1 skips the Sounds group, 2 loads only it (never set: always 0)
	Widget* mLoaderScreen = nullptr;    // +0x860
	bool mSoundsOff = false;            // +0x928: no sound effects (set elsewhere, not yet read)
	SoundInstance* mUnmanagedSound = nullptr;  // +0x938: a tracked sound whose volume UpdateFrames leaves alone
	double mSfxVolumeScale = 1.0;       // +0x948: applied to the tracked sounds
	struct TrackedSound {
		SoundInstance* mInstance;
		double mVolume;
	};
	std::vector<TrackedSound> mTrackedSounds;  // +0x960: sounds UpdateFrames keeps at mSfxVolumeScale until they end

	WinTwistApp();                      // 0x766ce7
	~WinTwistApp() override;            // 0x768c34

	void Init() override;               // 0x7652cc
	void LoadingThreadProc() override;  // 0x76497d
	void UpdateFrames() override;       // 0x765f8d

	// 0x760659: 2D mode, the art at 600 and the screen at 800x600 in 16 bits
	void Use2DMode();
	// 0x766974: plays a sound unless sounds are off, the app has no focus or the board is paused; theTrack keeps it
	// in mTrackedSounds, theScaleVolume multiplies its volume by mSfxVolumeScale
	SoundInstance* PlaySoundEx(int theSoundId, int thePan, float thePitch, float theVolume, bool theTrack,
		bool theScaleVolume);
	// 0x763092: loads the Tube group (the sidebar's tube) and extracts it
	bool LoadTubeResources();
};

extern WinTwistApp* gApp;  // 0xe0b698

}  // namespace Sexy
