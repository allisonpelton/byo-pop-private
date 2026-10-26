// Sexy::CutScene (0xd4 bytes, constructor 0x4415d0, vtable 0x7ac29c): a story cut scene, a Flash movie
// (images/cutscenes/*.swf) shown 640x480 in the middle of the screen between two bars, with its caption at the
// top and a hint that a click skips it. The app plays one with PlayCutScene behind the curtain; the movie's
// "done" (or a skip) ends it, the curtain closes over it and the scripts' function the app was given goes on.
#pragma once

#include <string>

#include "FlashWidget.h"
#include "Widget.h"

namespace Sexy {

class GameApp;

class CutScene : public Widget, public FlashListener {
public:
	FlashWidget* mFlash;          // +0x90 (a MyFlashWidget)
	GameApp* mApp;                // +0x94
	std::string mCallback;        // +0x98: the scripts' function called (no arguments) when it ends
	std::string mCaption;         // +0xb4
	bool mWaitingForCurtain = false;  // +0xd0: paused until the curtain has opened over it
	bool mEnded = false;          // +0xd1
	bool mSkipped = false;        // +0xd2
	bool mCanSkip = true;         // +0xd3

	explicit CutScene(GameApp* theApp);
	~CutScene() override;  // 0x441780

	// 0x441b50: starts the movie and keeps what to call when it ends and the caption
	void Load(const std::string& theFileName, const std::string& theCallback, const std::string& theCaption);
	// 0x4418a0: the movie is over: the curtain closes over it (when the app is waiting for that) and the
	// scripts' function is called
	void End();
	// 0x441d90: the player chose to skip it
	void Skip();

	void Update() override;  // 0x441b10
	void Draw(Graphics* g) override;  // 0x441970
	// 0x441c70: a click (its button going up) asks whether to skip the movie
	void MouseUp(int x, int y, int theBtnNum, int theClickCount) override;

	void FlashAnimEnded(int theId) override;  // 0x441850
	void FlashCommand(int theId, const std::string& theCommand, const std::string& theParam) override;  // 0x441860
};

}  // namespace Sexy
