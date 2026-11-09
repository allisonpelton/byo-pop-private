// Sexy::LoaderScreen (0x668 bytes, a Widget and a ButtonListener; 0x71f552-0x7207a5): the screen that shows while
// the loading thread runs. The PopCap logo fades in, then the device with its progress bar, the title (a Logo) and
// the spiral button; when everything is loaded it hands over to the main menu, slides away and finishes.
#pragma once

#include "ButtonListener.h"
#include "CurvedVal.h"
#include "Widget.h"

namespace Sexy {

class Logo;
class PlayButton;
class WinTwistApp;

class LoaderScreen : public Widget, public ButtonListener {
public:
	PlayButton* mPlayButton;         // +0x8c
	Logo* mLogo;                     // +0x90
	WinTwistApp* mApp;               // +0x94
	double mProgress = 0;            // +0x98: the loading thread's progress
	double mLoadStartTime = 0;       // +0xa0: a double, never set, never read
	bool mDone = false;              // +0xa8: loaded, handing over to the main menu
	bool mPlayClicked = false;       // +0xa9
	float mSmoothProgress = 0;       // +0xac: what the bar shows
	CurvedVal mPopCapAlpha;          // +0xb0: the PopCap logo
	CurvedVal mSetOnlyCurve;         // +0x120: set from curve 0x7e6b38 and moved on at the hand-over; never read
	CurvedVal mLogoAlpha;            // +0x190: the title's opacity
	CurvedVal mLoadingAlpha;         // +0x200: the "loading" text
	CurvedVal mFadeIn;               // +0x270: from black
	CurvedVal mSlide;                // +0x2e0: the device's height, then its slide away
	CurvedVal mButtonY;              // +0x350: the spiral button's offset from the middle
	CurvedVal mButtonAlpha;          // +0x3c0: the main menu's button follows it
	CurvedVal mArcAlpha;             // +0x430: the glow arc
	CurvedVal mBarGlow;              // +0x4a0: the four glow bars, one after another
	CurvedVal mArcGlow;              // +0x510: the glow arc again, after the bars
	CurvedVal mTrademarkAlpha;       // +0x580
	bool mFinished = false;          // +0x660: the app removes the screen
	int mCountdown = 0;              // +0x664: the updates from loaded to mFinished

	explicit LoaderScreen(WinTwistApp* theApp);  // 0x72032b
	~LoaderScreen() override;                    // 0x71f552

	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x71febe
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x71fefa
	void Update() override;                                             // 0x71ff54
	void Draw(Graphics* g) override;                                    // 0x71f5c0
	void DrawAll(ModalFlags* theFlags, Graphics* g) override;           // 0x71f65f
	void DrawOverlay(Graphics* g) override;                             // 0x71f65c
	using Widget::DrawOverlay;
	void Resize(int theX, int theY, int theWidth, int theHeight) override;  // 0x7202b4
	using Widget::Resize;
	void MouseDown(int x, int y, int theClickCount) override;          // 0x71ff31
	using Widget::MouseDown;

	void ButtonDepress(int theId) override;  // 0x71ff03
};

}  // namespace Sexy
