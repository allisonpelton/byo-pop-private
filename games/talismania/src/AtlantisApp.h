#ifndef __TALISMANIA_ATLANTISAPP_H__
#define __TALISMANIA_ATLANTISAPP_H__

// The game's application (AtlantisApp, 0x1d8 bytes, constructed at 0x401580; the game's internal name is "Atlantis"),
// which the engine keeps and asks for its configuration (slot 1, 0x402aa0) and then to start (slot 2, 0x402d60). The
// engine's run gets it from the game's boot, 0x401100, which found the archive and loaded the base data set first.
// Translated so far: the boot's archive and the application's construction.

#include "AtlantisData.h"
#include "AtlantisScene.h"
#include "AtlantisTheme.h"
#include "LoadingState.h"
#include "Object.h"
#include "State.h"
#include "sigslot.h"

class AtlantisApp : public engine::Object, public sigslot::HasSlots
{
public:
	AtlantisApp();
	~AtlantisApp() override;

	// 0x401100: loose files on, then the archive Talismania.saf instead of them when it is there; the application
	// returned with one reference held by the caller
	static AtlantisApp* boot();

	// 0x402aa0 (not yet translated: it makes the persisted options, AtlantisOptions, over the vendor and the key, and
	// sets the window's size and rate, which the port's host sets)
	void configure();
	// 0x402d60: the start is logged and the scene made from the options' scene class and enabled; the sound engine, the
	// popup menu stacks, the signal connections and the loading state are not yet translated
	void start();

	AtlantisData mData;
	engine::Ref<AtlantisTheme> mTheme;  // the one theme every data set's theme files are added to
	AppClass mOptions;  // the application options "std", read by the boot
	engine::Ref<AtlantisScene> mScene;  // +0x38
	engine::StateMgr<AppState> mStateMgr;  // +0x17c

	// 0x40ecc0: each frame, the current state's update
	void updateSlot(engine::UpdateContext* theContext) { mStateMgr.update(theContext); }
};

// 0x813aa2: set by the boot when the archive is used. The loading screen reads its few loose files with loose files
// switched back on (0x4060a0)
extern bool gUsingArchive;

#endif
