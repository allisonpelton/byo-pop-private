// Sexy::Bej3App (vtable 0x8647b4 (bejeweled3), 105 slots): Bejeweled 3's app. Its fields' comments start with their
// offsets in the original's object; the framework's own fields are SexyAppBase's. docs/NOTES.md ("Boot") lists what
// the original's Init and loading thread do, and which of it is translated yet.
#pragma once

#include "SexyApp.h"

namespace Sexy {

class PrimeResourceManager;

class Bej3App : public SexyApp {
public:
	// The three sizes of the game's art (images\1200\, images\768\, images\600\): the height of the screen it was
	// drawn for. The 2D mode always uses 600 at 800x600.
	enum { ART_RES_1200 = 1200, ART_RES_768 = 768, ART_RES_600 = 600 };

	// +0x7f0: the original's SexyAppBase::mResourceManager, which in this framework generation is the manifest-2 manager
	// (engines/prime); the framework's own mResourceManager is left unused
	PrimeResourceManager* mPrimeResourceManager = nullptr;
	int mArtRes = ART_RES_600;              // +0x9e8: "ArtRes"
	bool mArtResFromCmdLine = false;        // +0x9dc: -600, -768 or -1200 chose the art size: "ArtRes" is not read
	bool mArtResChanged = false;            // +0x9db: the options changed the art size: "ArtRes" is written
	double mVoiceVolume = 0.85;             // +0x990: "VoiceVolume", in percent in the registry
	double mZenAmbientVolume = 0.5;         // +0x998: "ZenAmbientVolume"
	double mZenAmbientMusicVolume = 0.0;    // +0x9a0: "ZenAmbientMusicVolume"
	double mZenBinauralVolume = 0.25;       // +0x9a8: "ZenBinauralVolume"
	double mZenBreathVolume = 0.5;          // +0x9b0: "ZenBreathVolume"
	bool mAnimateBackground = true;         // +0x9e0: "AnimateBackground"
	int mTipIndex = 0;                      // +0xd78: "TipIdx", the loading screen's next tip in properties\tips.txt
	// port: the 3D mode comes later (docs/PORT_PLAN.md, "Rendering"): until then every run is the original's 2D mode
	bool mUse3D = false;

	Bej3App();                              // 0x76d9e0 (bejeweled3)
	~Bej3App() override;

	void Init() override;                   // 0x7748e0 (bejeweled3)
	void LoadingThreadProc() override;      // 0x772740 (bejeweled3)
	void WriteToRegistry() override;        // 0x76d450 (bejeweled3)
	void ReadFromRegistry() override;       // 0x76fe40 (bejeweled3)
	void HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) override;  // 0x76f9d0 (bejeweled3)
	void MakeWindow() override;
	void PostDDInterfaceInitHook() override;
	SharedImageRef GetSharedImage(const std::string& theFileName, const std::string& theVariant = "",
		bool* isNew = nullptr) override;    // 0x46ff40 (bejeweled3)

	// 0x76b020 (bejeweled3): without 3D the game is 800x600 with the 600 art
	void Use2DMode();

	// The framework's two, on this generation's manager
	void LoadResourceManifest();            // 0x46e0e0 (bejeweled3)
	void ShowResourceError(bool doExit = false);  // 0x46e190 (bejeweled3)
};

extern Bej3App* gApp;

}  // namespace Sexy
