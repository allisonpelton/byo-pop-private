// Sexy::ReplayWidget (0x170 bytes, a TwistWidget and ButtonListener; 0x6a34c7-0x6a3796, 0x6a99fe-0x6a9b84, 0x6ca983):
// the pod with the replay button that rises from the bottom of the board after a good move (Board::ShowReplayPod,
// the board's +0x2624). It bobs while it is up; its button rewinds the move and plays it again (Board::StartReplay).
#pragma once

#include "ButtonListener.h"
#include "TwistWidget.h"

namespace Sexy {

class Image;
class TwistButton;

class ReplayWidget : public TwistWidget, public ButtonListener {
public:
	enum { STATE_RISING = 0, STATE_SETTLING = 1, STATE_UP = 2, STATE_GOING = 3 };

	bool mPopped = false;          // +0x14c: up (or on its way up) and bobbing
	float mUnknown150 = 0;         // +0x150: counts down to the next round of +0x154 (nothing draws them)
	float mUnknown154 = 0;         // +0x154
	int mUnknown158 = 0;           // +0x158: 0 or 1, flips at each round
	int mTargetY = 0;              // +0x15c
	int mState = STATE_GOING;      // +0x160
	Image* mPod;                   // +0x164: IMAGE_INGAMEUI_REPLAYPOD
	TwistButton* mButton;          // +0x168: IMAGE_INGAMEUI_REPLAYPODBUTTON, id 0x2a

	ReplayWidget();                // 0x6a99fe (its two arguments, 5 and the board's +0x148, are not used)
	~ReplayWidget() override;      // 0x6a9acc

	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x6a34c7
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x6a35dd
	void Update() override;                                             // 0x6a35fd
	void Draw(Graphics* g) override;                                    // 0x6a3796
	void DrawAll(ModalFlags* theFlags, Graphics* g) override;           // 0x6a3709

	void ButtonDepress(int theId) override;                             // 0x6ca983

	void Show();  // 0x6a9aeb: it rises under the board's middle, with its sound
};

}  // namespace Sexy
