// Sexy::Logo (0x3c8 bytes, a TwistWidget; 0x6fe61a-0x7003f4): the game's title. It drops in from above (mDrop),
// sparkles appear on its brightest pixels once it is down, and the trademark sign fades in beside it. With Direct3D a
// shine also runs over it now and then (mShine and the two curves linked to it).
#pragma once

#include "CurvedVal.h"
#include "Point.h"
#include "TwistWidget.h"

#include <vector>

namespace Sexy {

class EffectsManager;
class Image;

class Logo : public TwistWidget {
public:
	EffectsManager* mFXManager;          // +0x148: where the sparkles go
	std::vector<Point> mSparklePoints;   // +0x14c: the title's bright, opaque pixels
	float mAlpha = 1.0f;                 // +0x15c
	int mLogoTop = 0;                    // +0x160: the title's top edge where it was last drawn
	float mFlashRise = 0;                  // +0x168: rises to 1 once the title is down, then starts mFlashFall
	float mSettleFade = 0;                  // +0x16c: rises to 1 once the title is down
	float mFlashFall = 0;                  // +0x170: falls from 1 (these three are not read in 2D)
	CurvedVal mTrademarkAlpha;           // +0x178
	float mLogoScale = 1.0f;             // +0x1f0: the title's size with Direct3D
	Image* mLogoImage;                   // +0x1f4
	CurvedVal mDrop;                     // +0x200: 0 in the middle of the screen, 1 in place near the top
	CurvedVal mShine;                    // +0x270
	CurvedVal mShineGreen;               // +0x2e0
	CurvedVal mShineBlue;                // +0x350
	int mShineDelay;                     // +0x3c0: the updates until the next shine

	explicit Logo(EffectsManager* theFXManager);  // 0x70026f
	~Logo() override;                              // 0x7003de

	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x6fe61a
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x6fe635
	void Update() override;                                             // 0x6ffbcc
	void Draw(Graphics* g) override;                                    // 0x6fe63e

	// 0x6ffb4e: starts the drop again; without theShow the title is hidden and down
	void Reset(bool theShow);
};

}  // namespace Sexy
