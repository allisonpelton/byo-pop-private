// Help: the help screen of the main menu (0x434700..0x4351b0 in the original). Three pictures slide in, the two top
// halves from above and the bottom from below, and a click anywhere slides them out; the screen deletes itself as the
// bottom one leaves.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class Help : public ObjBase
{
public:
	ObjBase* mTopLeft;       // +0x27c
	ObjBase* mTopRight;      // +0x280
	ObjBase* mBottom;        // +0x284
	bool mClosing;           // +0x288
	float mHoverMinZBefore;  // +0x28c: the engine's mouse floor, put back as the screen goes

	Help();
	~Help();
	void Notify(ObjBase* theObj) override;
	bool MouseUp(ObjBase* theObj) override;

	void Start();
	bool IsDone() { return mClosing; } // 0x4348b0

private:
	void BuildTopLeft();
	void BuildTopRight();
	void BuildBottom();
};

} // namespace Sexy
