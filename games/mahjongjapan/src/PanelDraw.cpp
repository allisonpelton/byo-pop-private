#include "PlayLayer.h"

#include "Edition.h"
#include "Font.h"
#include "JBBase.h"
#include "Res.h"

namespace mahjong
{

// 0x409510 (mahjongjapan)
void Panel::Draw()
{
	int aX;
	int aY;
	int anImage;
	if (mMode == 0)
	{
		DrawImage(0, 0, IMAGE_PANEL_TOP, nullptr, true, 100, 100, 0xff);
		DrawImage(0, 0x41, IMAGE_PANEL_TOP_SHADOW, nullptr, true, 100, 100, 0xff);
		DrawImage(0, 0x1ef, IMAGE_PANEL_BOTTOM_EDGE, nullptr, true, 100, 100, 0xff);
		DrawImage(0, 0x1ff, IMAGE_PANEL_BOTTOM, nullptr, true, 100, 100, 0xff);
		DrawImage(0, 0x4d, IMAGE_PANEL_LEFT_EDGE, nullptr, true, 100, 100, 0xff);
		anImage = IMAGE_PANEL_RIGHT_EDGE;
		aY = 0x4d;
		aX = 0x316;
	}
	else
	{
		DrawImage(0, 0, IMAGE_PANEL_TOP_WIDE, nullptr, true, 100, 100, 0xff);
		anImage = IMAGE_PANEL_BOTTOM_WIDE;
		aY = 0x1ff;
		aX = 0;
	}
	DrawImage(aX, aY, anImage, nullptr, true, 100, 100, 0xff);
	mTitleFont->DrawRight(this, 0x40, 0x18, mLabelTime.c_str(), 100, 100, 0xff);
	mTitleFont->DrawRight(this, 0x2a8, 0x18, mLabelScore.c_str(), 100, 100, 0xff);
	mLabelFont->DrawRight(this, 0x40, 0x2e, mLabelBest.c_str(), 100, 100, 0xff);
	mLabelFont->DrawRight(this, 0x2a8, 0x2e, mLabelBest.c_str(), 100, 100, 0xff);
	mLabelFont->DrawCentred(this, 0x3b, 0x21b, mLabelPairs.c_str(), 0x2a, 100, 100, 0xff);
	mLabelFont->DrawCentred(this, 0x3b, 0x23e, mLabelLeft.c_str(), 0x2a, 100, 100, 0xff);
	mTitleFont->DrawCentred(this, 0, mMode == 1 ? 0x1e : 0x18, mTitle.c_str(), jb::SCREEN_WIDTH, 100, 100, 0xff);
	if (!mStat1.empty())
		mSmallFont->DrawRight(this, 0xf2, 0x2d, mStat1.c_str(), 100, 100, 0xff);
	if (!mStat2.empty())
		mSmallFont->DrawLeft(this, 0x22b, 0x2d, mStat2.c_str(), -1, 100, 100, 0xff);
	if (!mBestTime.empty())
		mSmallFont->DrawCentred(this, 0x55, 0x2d, mBestTime.c_str(), 0x52, 100, 100, 0xff);
	if (!mBestScore.empty())
		mSmallFont->DrawCentred(this, 0x2b7, 0x2d, mBestScore.c_str(), 0x4c, 100, 100, 0xff);
	mClockFont->DrawLeft(this, 0x5f, 0x16, mTime.c_str(), -1, 100, 100, 0xff);
	mClockFont->DrawCentred(this, 0x2b7, 0x16, mScore.c_str(), 0x4c, 100, 100, 0xff);
	mSmallFont->DrawCentred(this, 0x3a, 0x22a, mPairs.c_str(), 0x28, 100, 100, 0xff);
	mSmallFont->DrawCentred(this, 0x3a, 0x24e, mLeft.c_str(), 0x28, 100, 100, 0xff);
	if (0 < mSteps)
	{
		int aWidth = 0x124 / mSteps;
		jb::Rect aRect = {0, 0, mStep * aWidth + 6, 0xe};
		if (0 < mStep)
			DrawImage(0xfe, 0x22, IMAGE_PROGRESS_BAR, &aRect, true, 100, 100, 0xff);
		int i = 1;
		if (1 < mSteps)
		{
			int anX = aWidth + 0xfc;
			do
			{
				DrawImage(anX, 0x26, IMAGE_PROGRESS_TICK, nullptr, true, 100, 100, 0xff);
				i++;
				anX += aWidth;
			} while (i < mSteps);
		}
		if (0 < mStep)
			DrawImage(mStep * aWidth + 0xfd, 0x20, IMAGE_PROGRESS_MARKER, nullptr, true, 100, 100, 0xff);
	}
}

} // namespace mahjong
