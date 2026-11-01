#include "Splash.h"

#include "App.h"
#include "Font.h"
#include "Manager.h"

namespace jb
{

// 0x5d8e10 (made empty by 0x425d60)
std::string gVersion;

// 0x42f650
Logo::Logo(bool theSplashLogo) : mSplashLogo(theSplashLogo)
{
}

// 0x42f6b0: on black, the SplashLogo image or Jolly Bear's logo with the version at the bottom right
void Logo::Draw()
{
	if (mSplashLogo)
	{
		jb::FillRect(nullptr, 0);
		DrawImage(mX, mY, 0xff01, nullptr, true, 100, 100, 0xff);
		return;
	}
	jb::FillRect(nullptr, 0);
	DrawImage(0x115, 0x6d, 0x4f, nullptr, true, 100, 100, 0xff);
	DrawImage(0xe0, 0x92, 0x4d, nullptr, true, 100, 100, 0xff);
	DrawImage(0x169, 0x19a, 0x50, nullptr, true, 100, 100, 0xff);
	DrawImage(0x14d, 0x1aa, 0x37, nullptr, true, 100, 100, 0xff);
	DrawImage(0x1d2, 0x1aa, 0x51, nullptr, true, 100, 100, 0xff);
	DrawImage(0xe2, 0x235, 0x4e, nullptr, true, 100, 100, 0xff);
	if (!gVersion.empty())
		mFont->DrawRight(this, 0x316, 0x253, gVersion.c_str(), 100, 100, 0xff);
}

// 0x42f7f0: the logo's images come from the exe. The SplashLogo image is centred across and a third of the way down,
// and with it the links stay off.
void Logo::Init()
{
	mManager->PreloadExeImage(0x4f);
	mManager->PreloadExeImage(0x4d);
	mManager->PreloadExeImage(0x50);
	mManager->PreloadExeImage(0x37);
	mManager->PreloadExeImage(0x51);
	mManager->PreloadExeImage(0x4e);
	mManager->PreloadExeImage(0x3a);
	if (mSplashLogo)
	{
		gActivateLinks = false;
		ImageInfo anInfo;
		GetImageInfo(0xff01, anInfo);
		mX = (800 - anInfo.mWidth) / 2;
		mY = (600 - anInfo.mHeight) / 3;
	}
	RegUpdate(true);
	mFont = GetExeFont(0x3a, 0x39, -1);
}

// 0x42f8c0: two seconds, then Jolly Bear's logo after the SplashLogo, or the game
bool Logo::Update()
{
	if (0 < mTime)
	{
		if (mTime == 0xffff)
			mTime = 2000;
		else
			mTime = mTime - GetDelta();
		if (mTime < 1)
		{
			mTime = 0;
			if (mSplashLogo)
			{
				mManager->SwitchTo(new Splash(false));
				return false;
			}
			gApp->StartGame();
		}
	}
	return false;
}

// 0x425320
Splash::Splash(bool theSplashLogo) : Layer(0, 0), mSplashLogo(theSplashLogo)
{
}

// 0x425370: SplashLogo is a file in the exe's folder (image 0xff01)
void Splash::Load()
{
	if (mSplashLogo)
		mSplashLogo = mManager->LoadFileImage(0xff01, (GetExeFolder() + "SplashLogo").c_str());
	Add(new Logo(mSplashLogo), 0, 0);
}

} // namespace jb
