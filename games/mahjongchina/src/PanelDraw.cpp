#include "PlayLayer.h"

#include "Edition.h"
#include "Font.h"
#include "JBBase.h"
#include "Res.h"

namespace mahjong
{

// 0x409350 (mahjongchina)
void Panel::Draw()
{
	int aX;
	int aY;
	int anImage;
	if (mMode == 0)
	{
		DrawImage(0, 0, IMAGE_PANEL_TOP, nullptr, true, 100, 100, 0xff);
		DrawImage(0, 0x3c, IMAGE_PANEL_TOP_SHADOW, nullptr, true, 100, 100, 0xff);
		DrawImage(0, 0x1f8, IMAGE_PANEL_BOTTOM_EDGE, nullptr, true, 100, 100, 0xff);
		anImage = IMAGE_PANEL_BOTTOM;
		aY = 0x208;
		aX = 0;
	}
	else
	{
		DrawImage(0, 0, IMAGE_PANEL_TOP_WIDE, nullptr, true, 100, 100, 0xff);
		DrawImage(0x36, 0x3c, IMAGE_PANEL_BAMBOO_TOP, nullptr, true, 100, 100, 0xff);
		DrawImage(0x36, 0x1fa, IMAGE_PANEL_BAMBOO_BOTTOM, nullptr, true, 100, 100, 0xff);
		DrawImage(0, 0x208, IMAGE_PANEL_BOTTOM_WIDE, nullptr, true, 100, 100, 0xff);
		DrawImage(0, 0x72, IMAGE_PANEL_BAMBOO_LEFT, nullptr, true, 100, 100, 0xff);
		DrawImage(0x310, 0x72, IMAGE_PANEL_BAMBOO_RIGHT, nullptr, true, 100, 100, 0xff);
		DrawImage(0, 0x3c, IMAGE_PANEL_CORNER_TOP_LEFT, nullptr, true, 100, 100, 0xff);
		DrawImage(0x2ea, 0x3c, IMAGE_PANEL_CORNER_TOP_RIGHT, nullptr, true, 100, 100, 0xff);
		DrawImage(0, 0x1d2, IMAGE_PANEL_CORNER_BOTTOM_LEFT, nullptr, true, 100, 100, 0xff);
		anImage = IMAGE_PANEL_CORNER_BOTTOM_RIGHT;
		aY = 0x1d2;
		aX = 0x2ea;
	}
	DrawImage(aX, aY, anImage, nullptr, true, 100, 100, 0xff);
	mTitleFont->DrawRight(this, 0x40, 0x19, mLabelTime.c_str(), 100, 100, 0xff);
	mTitleFont->DrawRight(this, 700, 0x19, mLabelScore.c_str(), 100, 100, 0xff);
	mLabelFont->DrawRight(this, 0x40, 0x2e, mLabelBest.c_str(), 100, 100, 0xff);
	mLabelFont->DrawRight(this, 700, 0x2e, mLabelBest.c_str(), 100, 100, 0xff);
	mLabelFont->DrawCentred(this, 0x37, 0x21d, mLabelPairs.c_str(), 0x2a, 100, 100, 0xff);
	mLabelFont->DrawCentred(this, 0x37, 0x240, mLabelLeft.c_str(), 0x2a, 100, 100, 0xff);
	(mMode == 1 ? mEscapeTitleFont : mTitleFont)->DrawCentred(this, 0, 0x1b, mTitle.c_str(), jb::SCREEN_WIDTH, 100, 100,
		0xff);
	if (!mStat1.empty())
		mSmallFont->DrawRight(this, 0x108, 0x32, mStat1.c_str(), 100, 100, 0xff);
	if (!mStat2.empty())
		mSmallFont->DrawLeft(this, 0x22b, 0x32, mStat2.c_str(), -1, 100, 100, 0xff);
	if (!mBestTime.empty())
		mSmallFont->DrawCentred(this, 0x46, 0x2e, mBestTime.c_str(), 0x52, 100, 100, 0xff);
	if (!mBestScore.empty())
		mSmallFont->DrawCentred(this, 0x2c2, 0x2e, mBestScore.c_str(), 0x4c, 100, 100, 0xff);
	mClockFont->DrawLeft(this, 0x4c, 0x16, mTime.c_str(), -1, 100, 100, 0xff);
	mClockFont->DrawCentred(this, 0x2c2, 0x16, mScore.c_str(), 0x4c, 100, 100, 0xff);
	mSmallFont->DrawCentred(this, 0x38, 0x22f, mPairs.c_str(), 0x28, 100, 100, 0xff);
	mSmallFont->DrawCentred(this, 0x38, 0x251, mLeft.c_str(), 0x28, 100, 100, 0xff);
	if (0 < mSteps)
	{
		int aWidth = 0x113 / mSteps;
		jb::Rect aRect = {0, 0, mStep * aWidth + 6, 0xf};
		DrawImage(0x10c, 0x27, IMAGE_PROGRESS_BAR, &aRect, true, 100, 100, 0xff);
		int i = 1;
		if (1 < mSteps)
		{
			int anX = aWidth + 0x10a;
			do
			{
				DrawImage(anX, 0x2c, IMAGE_PROGRESS_TICK, nullptr, true, 100, 100, 0xff);
				i++;
				anX += aWidth;
			} while (i < mSteps);
		}
		DrawImage(mStep * aWidth + 0x109, 0x21, IMAGE_PROGRESS_MARKER, nullptr, true, 100, 100, 0xff);
	}
}

} // namespace mahjong
