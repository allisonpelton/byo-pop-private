// Sexy::Hyperspace (Hyperspace.h).
#include "Hyperspace.h"

#include "Graphics.h"
#include "Res.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "X86Math.h"

using namespace Sexy;

Hyperspace::Hyperspace()
{
	// The level comes from the board (+0x158 - 1) when there is one: none yet. The ctor also sets up the curves of the
	// level's world and the camera (+0x798 to +0x1008), which only the Direct3D drawing and the jumps read.
	mIs3D = gApp->Is3DAccelerated();
	RemoveAllWidgets(true, false);
}

Hyperspace::~Hyperspace() = default;

void Hyperspace::Update()
{
	if (mState == 0 || mState == 6) {
		Widget::Update();
		double aWarmUp = mWarmUp + 0.0010000000474974513;
		if (1.0 <= aWarmUp)
			aWarmUp = 1.0;
		mWarmUp = aWarmUp;
	}
	// The other states (the jumps, +0x12b0's curve), the camera and the board's thrust particles (the only Rand() of
	// Update, run only while the board's +0x200c is this Hyperspace) come with the board
}

void Hyperspace::Draw(Graphics* g)
{
	DeferOverlay(0);
	// With Direct3D: the level's world in 3D (0x71042e and on); comes with Direct3D, which the port does not have
	if (mTransition == 12 || mTransition == 13) {
		// The end of a jump: comes with the board (0x713ee3)
	}
	DrawImageScaledCentered(g, IMAGE_STATIC_SPACE, (float)(gApp->mWidth / 2), (float)(gApp->mHeight / 2), 3, 1.0f,
		1.0f);
	if (mTransition == 8 || mTransition == 14 || mTransition == 9 || mTransition == 10) {
		// The tunnel of a jump (0x711b65): comes with the board
	}
	if (mFlash.GetOutVal() == 0.0)
		return;
	g->SetColor(Color(255, 255, 255, Ftol(mFlash.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
	g->FillRect(0, 0, mWidth, mHeight);
}

void Hyperspace::DrawOverlay(Graphics* g)
{
	// Direct3D only (+0x156 and +0x157): the planets, the saucer and the tube
	(void)g;
}

void Hyperspace::SetTransition(int theTransition)
{
	mTransition = theTransition;
	// Transitions 8, 9, 10, 12 and 14 set up a jump (and with Direct3D, 8 the next level's world): come with the board
	Update();
}
