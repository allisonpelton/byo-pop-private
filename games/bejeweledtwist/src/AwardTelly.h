// Sexy::AwardTelly (0x898 bytes, a TwistWidget and ButtonListener; 0x6a1a23-0x6a2dfd): the award screen over the rank
// bar. For a new rank (ShowRank) the old badge turns into the new one under "RANK UP" with the ranks' names, and a
// Continue button sends it away. The trophy screen (0x6a2318: "challenge complete", with a rank bar of its own) shares
// it; it comes with the challenges.
#pragma once

#include "ButtonListener.h"
#include "CurvedVal.h"
#include "TwistWidget.h"

namespace Sexy {

class EffectsManager;
class RankBonusWidget;
class TwistButton;

class AwardTelly : public TwistWidget, public ButtonListener {
public:
	int mState = 0;                  // +0x158: 1 comes in, 2 waits for Continue, 3 goes, 4 and 5 the trophy's end
	int mKind = 0;                   // +0x15c: 1 a trophy, 3 a rank
	int mCounter = 0;                // +0x160: updates in state 2 (the button blinks after 300)
	int mRank = 0;                   // +0x164
	CurvedVal mShow;                 // +0x168: in, and back out (its rate turned round)
	CurvedVal mUnknown1D8;           // +0x1d8
	CurvedVal mAlpha;                // +0x248: of everything
	float mUnknown2B8 = 0;           // +0x2b8: up to 1 while it comes in
	EffectsManager* mFXManager;      // +0x2bc: drawn under it
	EffectsManager* mFXManager2;     // +0x2c0: a child widget
	void* mTrophy = nullptr;         // +0x2c4
	RankBonusWidget* mRankBonus = nullptr;  // +0x2c8: the trophy's rank bar
	bool mUnknown2CC = false;        // +0x2cc
	int mBadge = 0;                  // +0x2d0: the new rank's badge (rank / 5)
	int mOldBadge = 0;               // +0x2d4
	int mMark = 0;                   // +0x2d8: and its mark (rank % 10)
	int mOldMark = 0;                // +0x2dc
	CurvedVal mChange;               // +0x2e0: drives the change from the old badge to the new; the old mark's alpha
	CurvedVal mOldMarkScale;         // +0x350
	CurvedVal mOldMarkDrop;          // +0x3c0
	CurvedVal mNewMarkAlpha;         // +0x430
	CurvedVal mNewMarkScale;         // +0x4a0
	CurvedVal mOldBadgeAlpha;        // +0x510
	CurvedVal mNewBadgeAlpha;        // +0x580
	CurvedVal mBadgeGlow;            // +0x5f0
	CurvedVal mTrophyIn;             // +0x660
	CurvedVal mTrophyScale;          // +0x6d0
	CurvedVal mRankUpAlpha;          // +0x740: "RANK UP"
	CurvedVal mOldTextAlpha;         // +0x7b0: the old rank's name
	CurvedVal mNewTextAlpha;         // +0x820: the new one's; the trophy sound at 0.4
	TwistButton* mButton = nullptr;  // +0x890: Continue
	bool mPressed = false;           // +0x894

	AwardTelly();            // 0x6a216e
	~AwardTelly() override;  // 0x6a19c2

	void Resize(int theX, int theY, int theWidth, int theHeight) override;  // 0x6a1a23: always the whole screen
	using Widget::Resize;
	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x6a1a88
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x6a1b85
	void Update() override;                                             // 0x6a2730
	void Draw(Graphics* g) override;                                    // 0x6a1f63
	void MouseDown(int x, int y, int theClickCount) override;           // 0x6a2154
	using TwistWidget::MouseDown;
	void ButtonPress(int theId) override;                               // 0x6a2133
	void ButtonDepress(int theId) override;                             // 0x6a214a

	void ShowRank(int theRank);   // 0x6a256e
	void DrawBadges(Graphics* g);  // 0x6a1c08 (slot 70): the old badge and mark into the new
	void DrawRankText(Graphics* g, int theRank, float theAlpha);  // 0x6a2ca1 (slot 71): "Rank N:" and its name
};

}  // namespace Sexy
