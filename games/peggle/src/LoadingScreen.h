// LoadingScreen.h: the screen the game shows while its resources load (the original's LoadingScreen.cpp). It plays the PopCap
// logo, then a sun rises behind a hill as the bar (the row of lights) fills, the Peggle logo drops in, and when everything is
// loaded a ball falls onto the logo, bounces off it in a spray of sparks, and the Play button appears.

#ifndef __LOADINGSCREEN_H__
#define __LOADINGSCREEN_H__

#include "ButtonListener.h"
#include "Widget.h"

#include <vector>

namespace Sexy
{

class Graphics;
class ThunderballApp;
class ThunderButton;

// The share of the whole load (0 to 1) done when the loading thread has finished the given number of its tasks (0x4936b0).
// The screen's bar follows this, not the real time the load takes.
float GetLoadTime(int theCompletedTasks);

class LoadingScreen : public Widget, public ButtonListener
{
public:
	// one spark of the ball's trail
	struct Particle
	{
		float mX;
		float mY;
		int mLife;	  // 255 at the start; falls by 6 an update, and the particle is gone when it is 0 or less
		float mVX;
		float mVY;
		float mAngle; // the direction the ball was travelling in when it was made
	};

	explicit LoadingScreen(ThunderballApp* theApp); // 0x4b1fe0
	~LoadingScreen() override;						// 0x4af650 (0x4a6e70)

	void DoBallBounce();							// 0x494370: drops the ball (the B key does it too)
	void DrawLight(Graphics* g, int theLight);		// 0x494420
	void DrawParticles(Graphics* g);				// 0x4998c0
	void UpdateParticles();							// 0x49f2d0
	void SpawnParticles();							// 0x4b3b90
	void UpdateBall();								// 0x4b40b0
	void SetStartFromSelectedStage();				// 0x494850

	void Update() override;						 // 0x4b4270
	void Draw(Graphics* g) override;			 // 0x4a7250
	void DrawOverlay(Graphics* g) override;		 // 0x494510
	void KeyChar(SexyChar theChar) override;	 // 0x494890
	void ButtonPress(int theId) override;		 // 0x4948b0
	void ButtonDepress(int theId) override;		 // 0x4948d0

	using Widget::DrawOverlay;
	using ButtonListener::ButtonPress;

	ThunderballApp* mApp;		 // +0x8c
	ThunderButton* mPlayButton;	 // +0x90: shown when the load is done
	float mLoadProgress;		 // +0x94: how far the bar has come (0 to 1)
	float mSunAngle;			 // +0x98: the sun's rotation in degrees; it spins once the load is done
	int mSunSpinCount;			 // +0x9c: how many updates the sun has spun
	int mPlayFlashCount;		 // +0xa0: counts up from 1 once the Play button appears; the button flashes during the first 50
	float mDrawnProgress;		 // +0xa4: the progress the picture shows (the sun's height, the shadow)
	int mStartAdventureCountdown; // +0xa8: set to 20 when a new player has been stamped on the certificate; the adventure starts when it runs out
	int mBallState;				 // +0xac: 0 no ball, 1 falling, 2 bounced off the logo, 3 gone
	int mBallHitCount;			 // +0xb0: updates since the ball hit the logo (the logo bounces and flashes), 0 when it has not
	float mBallX;				 // +0xb4
	float mBallY;				 // +0xb8
	float mBallVX;				 // +0xbc
	float mBallVY;				 // +0xc0
	std::vector<Particle> mParticles; // +0xc8
	bool mLoaded;				 // +0xd8: the bar is full, so the Deluxe name and the TM mark are drawn
	int mFrame;					 // +0xe4: counts the updates since the bar started (the logo drops by it)
	int mNumLights;				 // +0xec: how many of the 25 lights are lit
};

} // namespace Sexy

#endif
