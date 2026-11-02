// Sexy::RankBonusWidget (0x4f0 bytes, a TwistWidget; 0x7360b8-0x738fae): the rank bar after a level or a game. The bar
// rises from the bottom of the screen with a slot for each point the rank needs; the points earned fly into it as stars,
// one every 15 updates. A full bar raises the rank (AwardTelly shows the new badge); a click or the end of its time
// sends it off the bottom again.
#pragma once

#include "CurvedVal.h"
#include "TwistWidget.h"

#include <string>
#include <vector>

namespace Sexy {

class AwardTelly;
class EffectsManager;
class Image;

// 0x7ea230 (and 0x7dbb50): the badge for every five ranks; 0x7ea208 (and 0x7dbb24): the mark for the ranks between
extern const int kRankBadges[21];
extern const int kRankBadgeMarks[10];

class RankBonusWidget : public TwistWidget {
public:
	// A star on its way into the bar (0x20 bytes)
	struct Star {
		int mX;           // +0x00: where it starts
		int mY;           // +0x04
		int mToX;         // +0x08: its slot, set when it starts to fly
		int mToY;         // +0x0c
		int mCel;         // +0x10: shrinks as it flies
		int mStartCel;    // +0x14
		float mProgress;  // +0x18: 0 waiting, then to 1 in its slot
		float mAlpha;     // +0x1c
	};

	float mFadeIn = 0;           // +0x148
	float mFadeIn2 = 1.0f;       // +0x154: a copy of it
	int mUnknown158 = 0;         // +0x158
	int mState = 0;              // +0x15c: 0 rises, 1 fills, 2 waits, 3 the award, 4 its end, 5 goes
	float mTimer = 0;            // +0x160
	std::string mUnknown164;     // +0x164
	std::string mUnknown180;     // +0x180
	bool mUnknown1A0 = false;    // +0x1a0: set while it waits (state 2)
	int mCenterX = 0;            // +0x1a4
	int mCenterY = 0;            // +0x1a8
	float mBarY = 0;                // +0x1ac: the bar's top
	float mVel = 0;              // +0x1b0
	float mUnknown1B4 = 1.0f;    // +0x1b4
	Image* mBar;                 // +0x1b8: IMAGE_RANKBAR_LEVELUP
	int mTotal;                  // +0x1bc: the rank's points with the bonus
	int mPoints;                 // +0x1c0: before it
	int mUnknown1C4 = 0;         // +0x1c4
	int mBonus;                  // +0x1c8: the stars
	int mRank;                   // +0x1cc: the player's rank when it came up
	CurvedVal mFlash;            // +0x1d0: the button's flash (DrawOverlay)
	CurvedVal mPillars;          // +0x240: the pillars and the screen rise
	CurvedVal mSlide;            // +0x2b0: the pillars close in; at 0.85 of it the rank goes up
	CurvedVal mGlow;             // +0x320: the "hot n sexy" glow's colour
	CurvedVal mUnknown390;       // +0x390
	CurvedVal mFade;             // +0x400: the stars fade
	float mUnknown470 = 0;       // +0x470
	float mUnknown474 = 0;       // +0x474
	EffectsManager* mFXManager;  // +0x478
	int mStep = 1;               // +0x47c: a twentieth of the bonus, at least 1
	int mSlots;                  // +0x480: the points between this rank and the next
	int mFilled;                 // +0x484: the slots filled
	int mFocus;                  // +0x488: the slot the stars fly to; the slots round it are wider
	int mTarget;                 // +0x48c: mFilled with the bonus
	float mScale = 0;            // +0x490: the zoom it comes in with (the Direct3D draw's)
	Image* mStarImage;           // +0x498: IMAGE_RANKBAR_STAR
	int mStarWidth;              // +0x49c: its size, 1.1 times
	int mUnknown4A0[4] = {};     // +0x4a0
	int mStarHeight;             // +0x4b0
	int mBarWidth;               // +0x4b4: of the slots, 0.475 of the bar's image
	int mLastFilled = -1;        // +0x4b8: the slot that flashes
	float mFilledFlash = 0;      // +0x4bc: its flash, from 1 down
	float mBlend = 1.0f;         // +0x4c0: from the slots' old widths to their new ones
	float* mWantWidths;          // +0x4c4
	float* mWidths;              // +0x4c8
	float* mOldWidths;           // +0x4cc
	AwardTelly* mTelly = nullptr;  // +0x4d0: the new rank's badge
	std::vector<Star> mStars;    // +0x4d4
	int mFocusX = 0;             // +0x4e4: the left of the focused slot
	int mStarsX;                      // +0x4e8: where the stars wait
	int mStarsY;                 // +0x4ec

	RankBonusWidget(int thePoints, int theBonus, int theX, int theY);  // 0x738824
	~RankBonusWidget() override;                                       // 0x73702b

	void Resize(int theX, int theY, int theWidth, int theHeight) override;  // 0x7360b8: always the whole screen
	using Widget::Resize;
	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x736168
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x73618f
	void Update() override;                                             // 0x737242
	void Draw(Graphics* g) override;                                    // 0x737985
	void DrawOverlay(Graphics* g) override;                             // 0x736260
	using TwistWidget::DrawOverlay;
	void MouseDown(int x, int y, int theClickCount) override;           // 0x736366
	using TwistWidget::MouseDown;

	void Reset();          // 0x73610b
	void RankUp();         // 0x7361b6: the next rank, saved, and its AwardTelly
	void Layout();         // 0x73668f: the slots' widths round mFocus
	void BlendWidths();    // 0x736398
	void UpdateFades();    // 0x7363fe
	void DrawBar(Graphics* g);  // 0x73687c: the slots, gold to mFilled and blue after
	// 0x738d19: the badge of theRank at theX, theY (its middle), scaled
	static void DrawBadge(Graphics* g, int theRank, int theX, int theY, float theScale, float theAlpha);
};

}  // namespace Sexy
