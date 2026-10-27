// Sexy::ChapterLoadScreen (0xb4 bytes, constructor V1:0x42e780): the box over the closed curtain while a chapter's
// resources load (GameApp::StartLoadChapterResources), with one of the game's hints, a new one every half
// second, and a red bar for the resources loaded so far (the app's mChapterLoadCount of mChapterLoadTotal).
#pragma once

#include <string>
#include <vector>

#include "Widget.h"

namespace Sexy {

class GameApp;

class ChapterLoadScreen : public Widget {
public:
	GameApp* mApp;                       // +0x88
	std::vector<std::string> mMessages;  // +0x8c: the hints
	std::vector<int> mShown;             // +0x9c: the hints shown so far, by index
	int mCurrent;                        // +0xac: the hint shown
	int mTimer = 50;                     // +0xb0: updates until the next hint

	explicit ChapterLoadScreen(GameApp* theApp);

	void Update() override;           // 0x4270d0
	void Draw(Graphics* g) override;  // 0x4272a0
};

}  // namespace Sexy
