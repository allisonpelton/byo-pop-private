// Sexy::CutScene (0xdc bytes, constructor 0x436560, vtables 0x6fa21c, 0x6fa338 (its FlashListener part at +0x88) and
// 0x6fa344 (ButtonListener, +0x8c); code 0x436560-0x436e60): a story cut scene, a Flash movie
// (images/cutscenes/*.swf) shown 640x480 in the middle of the screen between two bars, with its caption at the
// top. The app plays one with PlayCutScene behind the curtain; the movie's "done" (or a skip) ends it, the curtain
// closes over it and the scripts' function the app was given goes on. A click anywhere jumps the movie to the end of
// the scene it is in (the movies label each scene "Nstart" and "Nend"); the hint and the skip button of Volume 1's
// are not shown (the button is made, hidden, and enabled once the curtain is open).
#pragma once

#include <string>

#include "ButtonListener.h"
#include "FlashWidget.h"
#include "Widget.h"

namespace Sexy {

class GameApp;

class DialogButton;

class CutScene : public Widget, public FlashListener, public ButtonListener {
public:
	FlashWidget* mFlash;          // +0x90 (a MyFlashWidget)
	GameApp* mApp;                // +0x94
	std::string mCallback;        // +0x98: the scripts' function called (no arguments) when it ends
	std::string mCaption;         // +0xb4
	DialogButton* mSkipButton;    // +0xd0: hidden, disabled until the curtain is open
	bool mWaitingForCurtain = false;  // +0xd4: paused until the curtain has opened over it
	bool mEnded = false;          // +0xd5
	bool mSkipped = false;        // +0xd6
	bool mSkipButtonEnabled = false;  // +0xd7
	bool mCanSkip = true;         // +0xd8

	explicit CutScene(GameApp* theApp);
	~CutScene() override;  // 0x4367e0

	// 0x436b80: starts the movie and keeps what to call when it ends and the caption
	void Load(const std::string& theFileName, const std::string& theCallback, const std::string& theCaption);
	// 0x436900: the movie is over: the curtain closes over it (when the app is waiting for that) and the
	// scripts' function is called
	void End();
	// 0x436e00: the player chose to skip it
	void Skip();

	void Update() override;  // 0x436af0
	void Draw(Graphics* g) override;  // 0x4369d0
	// 0x436c60: a click (its button going up) jumps the movie to the end of its scene: to the label that is the
	// scene's number (the first character of the label it is at) and "end"
	void MouseUp(int x, int y, int theBtnNum, int theClickCount) override;

	void FlashAnimEnded(int theId) override;  // 0x4368b0
	void FlashCommand(int theId, const std::string& theCommand, const std::string& theParam) override;  // 0x4368c0
	// 0x436e20: the skip button
	void ButtonDepress(int theId) override;
};

}  // namespace Sexy
