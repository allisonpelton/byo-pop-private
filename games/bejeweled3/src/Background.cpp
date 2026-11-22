// Sexy::Background (bejeweled3, 0x77dbb0 to 0x77f8e0): the animated picture behind the game screen. See Background.h.
#include "Background.h"

#include "Bej3App.h"
#include <SDL.h>
#include "Color.h"
#include "Graphics.h"
#include "PopAnim.h"

namespace Sexy {

namespace {
const int ART_RES_BASE = 1200;   // the size the animation files were drawn for
const int WIDGET_WIDTH = 1920;   // 0x77f8e0: the animation's size in 1200ths of the art height, 1920 by 1200
const int WIDGET_HEIGHT = 1200;
}  // namespace

// 0x77dbb0 (bejeweled3): the name is that of the animation ("Lion_tower_cascade.pam"). port: the file is loaded here, as the exe's
// worker thread would (0x77f090: the PopAnim made with both scales the art size over 1200 and told the file by its path)
Background::Background(const std::string& theFileName)
{
	mZOrder = -1;   // 0x749c20 sets it (+0x60) after the constructor
	mSpeedCurve.SetConstant(1.0);   // 0x77dbb0: FUN_00437760(1.0)
	// the name without ".pam" is the folder as well
	std::string aBase = theFileName;
	size_t aDot = aBase.find('.');
	if (aDot != std::string::npos)
		aBase.erase(aDot);
	std::string aPath = "images\\1200\\backgrounds\\" + aBase + "\\" + aBase + ".pam";
	PopAnim* anAnim = new PopAnim(0, nullptr);
	anAnim->mImageScale = (float)gApp->mArtRes / (float)ART_RES_BASE;
	anAnim->mDrawScale = (float)gApp->mArtRes / (float)ART_RES_BASE;
	if (!anAnim->LoadAnimation(aPath) || !anAnim->GetError().empty()) {
		SDL_Log("Background: %s: %s", aPath.c_str(), anAnim->GetError().c_str());
		delete anAnim;
		return;
	}
	// LoadAnimation clears the flag, so the generator is seeded after it
	// port: the exe's animation seeds its own generator from the app's Rand() at the first particle effect it makes (PopAnim +0x1e4),
	// on whichever update its loader thread has finished by: not a draw the logic can count on. The port seeds it with a constant
	// so that the app's Rand() sequence (the hints) is the board's alone
	anAnim->mRand.SRand(0);
	anAnim->mRandSeeded = true;
	mAnim = anAnim;
}

Background::~Background()
{
	delete mAnim;
}

// 0x77f8e0 (bejeweled3). The first update after the load starts the animation: PlayFrame(0, reset), sized to 1920 by 1200 (in art
// pixels), and the update that runs it is made at once (the sum starts at 1). It is run each update by UpdateF(speed) and, each time
// the sum of the speed goes above 1, by Update().
// port: left out: the exe starts it only while the app's "AnimateBackground" (+0x9e0) is on and the app's flag at +0x687 is set (which
// the options screen sets), and stops it and lets it go when that option is turned off; the fade between two backgrounds (the app's
// transition at 0x46a4e0, +0x133 and +0x134) and the sounds' thread
void Background::Update()
{
	Widget::Update();
	if (mAnim == nullptr)
		return;
	bool aJustStarted = false;
	if (!mStarted) {
		aJustStarted = true;
		mAnim->PlayFrame(0, true);
		mAnim->Resize(0, 0, gApp->mArtRes * WIDGET_WIDTH / ART_RES_BASE, gApp->mArtRes * WIDGET_HEIGHT / ART_RES_BASE);
		mStarted = true;
	}
	float aSpeed = (float)mSpeedCurve.GetOutVal();
	mAnim->UpdateF(aSpeed);
	mFrameFraction += aSpeed;
	if (mFrameFraction < 1.0f && aJustStarted)
		mFrameFraction = 1.0f;
	while (mFrameFraction > 1.0f) {
		mAnim->Update();
		mFrameFraction -= 1.0f;
	}
}

// 0x77eb50 (bejeweled3): the animation when it is loaded and running; black otherwise (the fades with the transition are not translated)
void Background::Draw(Graphics* g)
{
	if (mAnim != nullptr && mAnim->mLoaded && mStarted) {
		mAnim->Draw(g);
		return;
	}
	g->SetColor(Color(0, 0, 0));
	g->FillRect(0, 0, mWidth, mHeight);
}

}  // namespace Sexy
