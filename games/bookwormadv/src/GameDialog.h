// Sexy::GameDialog (0x168 bytes, constructor 0x4bed30, vtable 0x7b4b6c): the game's dialog, which the app makes
// for every DoDialog (NewDialog, 0x443f00). A parchment (IMAGE_DIALOG) with its header and lines, an optional
// banner picture over its top, and its own buttons (FadeButtons on IMAGE_DIALOG_BUTTON, stretched) straddling
// its bottom edge: yes and no, ok and cancel, or one with the footer's text. Escape presses yes (KeyDown).
#pragma once

#include "Dialog.h"

namespace Sexy {

class FadeButton;
class GameApp;

class GameDialog : public Dialog {
public:
	GameApp* mApp;                 // +0x150
	Image* mBanner = nullptr;      // +0x154
	FadeButton* mButton1 = nullptr;  // +0x158: yes, ok or the footer's (ID_YES)
	FadeButton* mButton2 = nullptr;  // +0x15c: no or cancel (ID_NO)
	int mBannerY = 0;              // +0x160
	bool mAllowDrag = true;        // +0x164

	GameDialog(int theId, const SexyString& theHeader, const SexyString& theLines, const SexyString& theFooter,
	           int theButtonMode);
	~GameDialog() override;

	// 0x4be9c0: the fonts and colours, and the size: at least theMinWidth wide and as wide as the header,
	// centred, a little below the first dialog open (or near the top)
	void Layout(int theMinWidth);

	// slot 81, 0x4bf0d0: the buttons' labels (an empty one is left as it is)
	virtual void SetButtonText(const SexyString& theYes, const SexyString& theNo);
	// slot 82, 0x4bf520
	virtual void SetBanner(Image* theImage, int theY);
	// slot 83, 0x401170
	virtual void SetAllowDrag(bool allowDrag) { mAllowDrag = allowDrag; }

	// Widget
	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x4bf2d0
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x4bf310
	void OrderInManagerChanged() override;                              // 0x4bf290
	void Resize(int theX, int theY, int theWidth, int theHeight) override;  // 0x4bf350
	void Draw(Graphics* g) override;                                    // 0x4bf4d0
	void KeyDown(KeyCode theKey) override;                              // 0x4bf540
	void MouseDrag(int x, int y) override;                              // 0x4bf5a0
};

}  // namespace Sexy
