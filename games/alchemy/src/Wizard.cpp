// The wizard's head (see Wizard.h).

#include "Wizard.h"
#include "AlchemyApp.h"
#include "Graphics.h"

#include <cmath>

using namespace Sexy;

// 0x510340: {mouth picture (0 closed, 1..4 wizmouth2..5), updates to hold it}; the track ends with {0, 0}
static const int NUM_TRACK_STEPS = 100;
static const int gLipSync[4][NUM_TRACK_STEPS][2] =
{
	{{0,1},{3,3},{4,7},{4,3},{3,8},{2,6},{0,7},{4,5},{4,8},{1,5},{0,7},{2,8},{1,8},{2,6},{0,9},{2,11},{0,0}},
	{{0,1},{3,4},{1,5},{2,9},{0,5},{2,7},{0,6},{4,4},{3,6},{2,4},{0,7},{2,4},{3,6},{3,7},{4,4},{2,4},{0,0}},
	{{0,1},{4,2},{1,5},{2,3},{0,10},{2,7},{3,5},{4,11},{2,4},{3,10},{2,7},{0,0}},
	{{0,1},{2,2},{3,5},{4,2},{2,4},{0,4},{1,2},{3,2},{4,1},{2,7},{1,4},{0,0}}
};

// 0x414887
WizardBox::WizardBox(AlchemyApp* theApp)
{
	mApp = theApp;
	mAlpha = 0;
	mFadeDir = 1;
	mBobX = 0;
	mBobY = 0;
	mTalking = false;
	mTrack = 0;
	mStep = 0;
	mTimer = 0;
	mHasTransparencies = true;
}

// 0x4148ce
void WizardBox::Talk(int theTrack)
{
	mStep = 0;
	mTrack = theTrack;
	mTalking = true;
	mTimer = gLipSync[theTrack][0][1];
	MarkDirty();
}

// 0x414903
void WizardBox::StopTalking()
{
	mTalking = false;
	MarkDirty();
}

// 0x414912
void WizardBox::Update()
{
	Widget::Update();

	if (mTalking)
	{
		mTimer--;
		if (mTimer <= 0)
		{
			mStep++;
			mTimer = gLipSync[mTrack][mStep][1];
			if (mTimer == 0)
				StopTalking();
			MarkDirty();
		}
	}

	if (mUpdateCnt % 6 == 0)
	{
		mBobX += 0.08;
		mBobY += 0.2;
		while (mBobX > 6.28318)
			mBobX -= 6.28318;
		while (mBobY > 6.28318)
			mBobY -= 6.28318;
		MarkDirty();
	}

	if (mFadeDir > 0)
	{
		if (mAlpha != 1.0)
		{
			mAlpha += 0.01;
			if (mAlpha >= 1.0)
				mAlpha = 1.0;
			MarkDirty();
		}
	}
	else if (mFadeDir < 0)
	{
		if (mAlpha > 0.0)
		{
			mAlpha -= 0.01;
			if (mAlpha <= 0.0)
				mAlpha = 0.0;
			MarkDirty();
		}
	}
}

// 0x414a7d
void WizardBox::Draw(Graphics* g)
{
	if (mAlpha < 0.001)
		return;

	if (mAlpha < 1.0)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, (int) (mAlpha * 255.0)));
	}

	int aX = (int) ((sin(mBobX) + 1.0) * 4.0);
	int aY = (int) ((sin(mBobY) + 1.0) * 8.0);

	int aMouth = mTalking ? gLipSync[mTrack][mStep][0] : 0;
	Image* aHead = mApp->mImages[IMAGE_WIZARDHEAD];
	if (aMouth == 0)
		g->DrawImage(aHead, aX, aY);
	else
	{
		// the mouth picture replaces the bottom right corner of the head
		Image* aMouthImage = mApp->mImages[IMAGE_WIZARDHEAD + aMouth];
		int aDX = aHead->GetWidth() - aMouthImage->GetWidth();
		int aDY = aHead->GetHeight() - aMouthImage->GetHeight();
		g->DrawImage(aHead, aX, aY, Rect(0, 0, aHead->GetWidth(), aDY));
		g->DrawImage(aHead, aX, aY + 0x6b, Rect(0, aDY, aDX, aHead->GetHeight() - aDY));
		g->DrawImage(aMouthImage, aX + aDX, aY + aDY);
	}

	g->SetColorizeImages(false);
}
