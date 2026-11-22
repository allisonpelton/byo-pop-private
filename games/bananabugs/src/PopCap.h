// PopCap: the opening screen (0x45e8d0..0x45f1a0 in the original). PopCap's logo swells in with a wobble, holds,
// and fades; a distributor's logo (partner.xml's PartnerImage), if there is one, then fades in and out the same way.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

enum PopCapPhase
{
	POPCAP_SHOWING = 0,
	POPCAP_FADING = 2,       // the logo's hold ended
	POPCAP_DONE = 3,
};

class PopCap : public ObjBase
{
public:
	int mPhase;              // PopCapPhase
	int mPartnerTrack;
	bool mHasPartner;        // a partner logo is still to show

	PopCap();
	~PopCap();
	void Notify(ObjBase* theObj) override;

	bool IsDone() { return mPhase == POPCAP_DONE; } // 0x45eb20
	void BuildLogoTracks();
	void BuildPartnerTrack();
};

} // namespace Sexy
