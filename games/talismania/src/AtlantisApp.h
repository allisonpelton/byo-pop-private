#ifndef __TALISMANIA_ATLANTISAPP_H__
#define __TALISMANIA_ATLANTISAPP_H__

// The game's application (AtlantisApp, 0x1d8 bytes, constructed at 0x401580; the game's internal name is "Atlantis"),
// which the engine keeps and asks for its configuration (slot 1, 0x402aa0) and then to start (slot 2, 0x402d60). The
// engine's run gets it from the game's boot, 0x401100, which found the archive and loaded the base data set first.
// Translated so far: the boot's archive and the application's construction.

#include "AtlantisData.h"
#include "Object.h"
#include "sigslot.h"

class AtlantisApp : public engine::Object, public sigslot::HasSlots
{
public:
	AtlantisApp();
	~AtlantisApp() override;

	// 0x401100: loose files on, then the archive Talismania.saf instead of them when it is there; the application
	// returned with one reference held by the caller
	static AtlantisApp* boot();

	// 0x402aa0 (not yet translated)
	void configure();
	// 0x402d60 (not yet translated)
	void start();

	AtlantisData mData;
};

// 0x813aa2: set by the boot when the archive is used. The loading screen reads its few loose files with loose files
// switched back on (0x4060a0)
extern bool gUsingArchive;

#endif
