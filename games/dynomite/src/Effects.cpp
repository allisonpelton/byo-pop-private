#include "Effects.h"
#include "AngleTable.h"
#include "BinFile.h"
#include "Dynomite.h"
#include "FossilizedApp.h"
#include "Graphics.h"
#include "Sprite.h"
#include <cstdlib>
#include <cstring>

using namespace Sexy;

// 0x4034b0
Effect::Effect()
{
	mType = 0;
}

// 0x4034c0
Effect::~Effect()
{
}

// 0x4b1930
void Effect::Draw()
{
}

// 0x4034d0
bool Effect::Update()
{
	return false;
}

// 0x403540
BabyEffect::BabyEffect()
{
	mType = 7;
}

// 0x403560: the reward sound as it leaves, and the board's +0x108 to 20
BabyEffect::~BabyEffect()
{
	mApp->mSndReward->Play();
	mApp->mBoard->mField108 = 0x14;
}

// 0x4035d0
void BabyEffect::Init(int x, int y, int theBaby, FossilizedApp* theApp)
{
	mApp = theApp;
	mX = (float) x;
	mBaby = theBaby;
	mVY = 0;
	mDelay = 0x32;
	mY = (float) y;
	mGravity = (float) y * 0.015151516f * 0.1f + 1.0f;
}

// 0x403620
bool BabyEffect::Update()
{
	if (mDelay < 1)
	{
		mY = mVY * 0.16666667f + mY;
		mVY = mGravity + mVY;
		if (650.0f < mY)
			return false;
	}
	else
		mDelay--;
	return true;
}

// 0x403700
void BabyEffect::Draw()
{
	Cell* aCell = mApp->mBabyCells[mBaby];
	aCell->Draw((int) mX - (aCell->mWidth >> 1), (int) mY - (aCell->mHeight >> 1));
}

// 0x415360
ConfettiEffect::ConfettiEffect()
{
	mType = 9;
}

// 0x415380
ConfettiEffect::~ConfettiEffect()
{
}

// 0x415390: a random colour, and a random push of up to 7 pixels in a random direction
void ConfettiEffect::Init(int x, int y, FossilizedApp* theApp)
{
	mX = (float) x;
	mApp = theApp;
	mY = (float) y;
	mColor = theApp->mRand.NextNoAssert() % 5;
	unsigned int anAngle = mApp->mRand.NextNoAssert() % 0x168;
	float aDist = (float) (mApp->mRand.NextNoAssert() & 7);
	mX = aDist * mApp->mAngles->mDX[anAngle] + mX;
	mY = aDist * mApp->mAngles->mDY[anAngle] + mY;
	mVX = aDist * 0.5f * mApp->mAngles->mDX[anAngle];
	mVY = aDist * 0.5f * mApp->mAngles->mDY[anAngle];
}

// 0x415450
bool ConfettiEffect::Update()
{
	mX = mVX + mX;
	mY = mVY + mY;
	mVY = mVY + 0.1f;
	if (610.0f < mY)
		return false;
	return true;
}

// 0x415510: where it was a frame ago, then where it is
void ConfettiEffect::Draw()
{
	float x = mX;
	float y = mY;
	Cell* aCell = mApp->mWowStarCells[mColor];
	aCell->Draw((int) (mX - mVX) - (aCell->mWidth >> 1), (int) (mY - mVY) - (aCell->mHeight >> 1));
	aCell = mApp->mWowStarCells[mColor];
	aCell->Draw((int) x - (aCell->mWidth >> 1), (int) y - (aCell->mHeight >> 1));
}

// 0x442320
EggWhiteEffect::EggWhiteEffect()
{
	mType = 3;
}

// 0x442340
EggWhiteEffect::~EggWhiteEffect()
{
}

// 0x442350: three pixels a frame along theAngle, starting two steps out
void EggWhiteEffect::Init(int x, int y, int theAngle, FossilizedApp* theApp)
{
	mX = (float) x;
	mY = (float) y;
	mPic = theApp->mPicEggwhite;
	mVX = theApp->mAngles->mDX[theAngle] * 3.0f;
	mVY = theApp->mAngles->mDY[theAngle] * -3.0f;
	mX = mVX + mVX + (float) x;
	mY = mVY + (float) y + mVY;
}

// 0x4423c0
bool EggWhiteEffect::Update()
{
	mX = mVX + mX;
	mY = mVY + mY;
	mVY = mVY + 0.2f;
	if (650.0f < mY)
		return false;
	return true;
}

// 0x442460
void EggWhiteEffect::Draw()
{
	float x = mX;
	float y = mY;
	mPic->Draw((int) x - (mPic->mWidth >> 1), (int) y - (mPic->mHeight >> 1));
}

// 0x4424b0
YolkEffect::YolkEffect()
{
	mType = 2;
}

// 0x4424d0
YolkEffect::~YolkEffect()
{
}

// 0x4424e0
void YolkEffect::Init(int x, int y, bool isYolk, int theWhich, FossilizedApp* theApp)
{
	mApp = theApp;
	mX = (float) x;
	mYolk = isYolk;
	mWhich = theWhich;
	mVY = 0;
	mY = (float) y;
	mGravity = (float) y * 0.015151516f * 0.1f + 1.0f;
}

// 0x442530
bool YolkEffect::Update()
{
	mY = mVY * 0.16666667f + mY;
	mVY = mGravity + mVY;
	if (650.0f < mY)
		return false;
	return true;
}

// 0x442610
void YolkEffect::Draw()
{
	float x = mX;
	float y = mY;
	Cell* aCell;
	if (mYolk == true)
		aCell = mApp->mYolkCells[mWhich];
	else
		aCell = mApp->mDynamiteCells[mWhich];
	aCell->Draw((int) x - (aCell->mWidth >> 1), (int) y - (aCell->mHeight >> 1));
}

// 0x434b90
ShellEffect::ShellEffect()
{
	mType = 5;
}

// 0x434bb0
ShellEffect::~ShellEffect()
{
}

// 0x434cc0
void ShellEffect::Init(int x, int y, int theAngle, int theColor, int theQuarter, FossilizedApp* theApp)
{
	mColor = theColor;
	mX = (float) x;
	mQuarter = theQuarter;
	mY = (float) y;
	mApp = theApp;
	if (0x167 < theAngle)
		theAngle = theAngle - 0x168;
	if (theAngle < 0)
		theAngle = theAngle + 0x168;
	mVX = theApp->mAngles->mDX[theAngle] * 3.0f;
	mVY = theApp->mAngles->mDY[theAngle] * 10.0f;
	float aGravity = (float) y * 0.015151516f * 0.1f + 1.0f;
	mGravity = aGravity;
	if (mVY < 0.0f)
		mGravity = aGravity * 0.9f;
}

// 0x434bc0
bool ShellEffect::Update()
{
	mX = mVX + mX;
	mY = mVY * 0.16666667f + mY;
	mVY = mGravity + mVY;
	mVX = mVX * 0.99f;
	if (650.0f < mY)
		return false;
	return true;
}

// 0x434d60
void ShellEffect::Draw()
{
	// port: the original's app +0x1424 + (mQuarter + mColor * 4) * 4 is the cracked sheets' cell for colour
	// mColor - 1
	Cell* aCell = mApp->mCrackedEggCells[mColor - 1][mQuarter];
	aCell->Draw((int) mX - (aCell->mWidth >> 1), (int) mY - (aCell->mHeight >> 1));
}

// 0x403c30
BonusItemEffect::BonusItemEffect()
{
	mType = 8;
}

// 0x403c50
BonusItemEffect::~BonusItemEffect()
{
}

// 0x403c60
void BonusItemEffect::Init(int x, int y, int theKind, int theTargetX, int theTargetY, Dynomite* theBoard)
{
	mKind = theKind;
	mX = (float) x;
	mBoard = theBoard;
	mFrames = 0x14;
	mY = (float) y;
	mTargetX = (float) theTargetX;
	mTargetY = (float) theTargetY;
	mVX = ((float) theTargetX - (float) x) * 0.05f;
	mVY = ((float) theTargetY - (float) y) * 0.05f;
}

// 0x403cc0
bool BonusItemEffect::Update()
{
	mFrames--;
	if (mFrames < 1)
	{
		mBoard->AddPowerUp(mKind);
		return false;
	}
	mX = mVX + mX;
	mY = mVY + mY;
	return true;
}

// 0x403dc0
void BonusItemEffect::Draw()
{
	// port: the original's app +0x1860 + mKind * 4 is the bonus view sheet's cell mKind - 1
	Cell* aCell = mBoard->mApp->mBonusViewCells[mKind - 1];
	aCell->Draw((int) mX - (aCell->mWidth >> 1), (int) mY - (aCell->mHeight >> 1));
}

// 0x4346a0
ScorePopup::ScorePopup()
{
	mText = NULL;
	mSpeed = 0.3f;
	mType = 6;
}

// 0x4346d0
ScorePopup::~ScorePopup()
{
	if (mText != NULL)
		free(mText);
}

// 0x434700: centred on x, y
void ScorePopup::Init(int x, int y, const char* theText, FossilizedApp* theApp)
{
	mX = (float) x;
	mY = (float) y;
	char* aText = (char*) malloc(strlen(theText) + 1);
	mApp = theApp;
	mText = aText;
	mBlink = false;
	mBlinkCount = 0x19;
	mLife = 0x96;
	mLength = 0;
	strcpy(aText, theText);
	mLength = (int) strlen(mText);
	mX = mX - (float) (mApp->mAnnounceFont1928->Width(mText) / 2);
	mY = mY - (float) (mApp->mAnnounceFont1928->mHeight / 2);
}

// 0x4347d0: up and off, blinking every 25 frames; 150 frames at most
bool ScorePopup::Update()
{
	float y = mY - mSpeed;
	mBlinkCount--;
	mY = y;
	if (mBlinkCount < 1)
	{
		mBlinkCount = 0x19;
		mBlink = mBlink != true;
	}
	if (y < -50.0f)
		return false;
	mLife--;
	return 0 < mLife;
}

// 0x434900: a black shadow 3 pixels down and right, then light blue, or yellow while it blinks
void ScorePopup::Draw()
{
	Graphics* g = GetDrawGraphics();
	g->SetColorizeImages(true);
	g->SetColor(Color::Black);
	mApp->mAnnounceFont1928->Draw(mText, (int) (mX + 3.0f), (int) (mY + 3.0f));
	if (mBlink == false)
		g->SetColor(Color(0x96, 0x96, 0xff, 0xff));
	else
		g->SetColor(Color(0xff, 0xff, 0, 0xff));
	mApp->mAnnounceFont1928->Draw(mText, (int) mX, (int) mY);
	g->SetColorizeImages(false);
}

// 0x403750
PowerUp::PowerUp(Dynomite* theBoard, int theKind, int theAngle)
{
	mAngle = theAngle;
	mBoard = theBoard;
	mKind = theKind;
	mRadius = 0;
	mBlinkTime = 500;
	// port: a kind outside 1-10 left the time and the flag as they were (they are zeroed here)
	mTime = 0;
	mFlag = 0;
	switch (theKind)
	{
	case 1:
		mTime = 6000;
		mFlag = 1;
		break;
	case 2:
		mTime = 3000;
		mFlag = 2;
		break;
	case 3:
		mTime = 3000;
		mFlag = 4;
		break;
	case 4:
		mTime = 3000;
		mFlag = 8;
		break;
	case 5:
		mTime = 400;
		mFlag = 0x10;
		break;
	case 6:
		mTime = 0x21;
		mFlag = 0x20;
		break;
	case 7:
		mTime = 0x21;
		mFlag = 0x40;
		break;
	case 8:
		mTime = 0x96;
		mFlag = 0x80;
		mBlinkTime = 0x14;
		break;
	case 9:
		mTime = 300;
		mFlag = 0x100;
		mBlinkTime = 0x14;
		break;
	case 10:
		mTime = 0x1c2;
		mFlag = 0x200;
		mBlinkTime = 0x14;
		break;
	}
	mBlinkCount = 0;
	mVisible = true;
}

// 0x4038c0
PowerUp::~PowerUp()
{
}

// 0x4038d0: the circle widens to 65 (45 in game type 0); kinds 8-10 burst into confetti and points at once
bool PowerUp::Update()
{
	mRadius = mRadius + 1.0f;
	if (65.0f < mRadius)
		mRadius = 65.0f;
	if ((mBoard->mGameType == 0) && (45.0f < mRadius))
		mRadius = 45.0f;
	mTime--;
	if (mTime < 0)
		return false;
	if (mTime < mBlinkTime)
	{
		mBlinkCount--;
		if (mBlinkCount < 1)
		{
			mBlinkCount = 0xf;
			mVisible = mVisible != true;
		}
	}
	if ((mFlag & 0x380) == 0)
		return true;
	int x;
	int y;
	mBoard->GetPowerUpCenter(&x, &y);
	mBoard->mApp->mSndScoremultiplier->Play();
	int i = 0x28;
	do
	{
		ConfettiEffect* aConfetti = new ConfettiEffect();
		aConfetti->Init(x, y, mBoard->mApp);
		mBoard->mEffects.Add(aConfetti);
		i--;
	} while (i != 0);
	if (mFlag == 0x80)
		mBoard->AddPoints(x, y, 5, 0, false);
	else if (mFlag == 0x100)
		mBoard->AddPoints(x, y, 10, 0, false);
	else if (mFlag == 0x200)
		mBoard->AddPoints(x, y, 0xf, 0, false);
	return false;
}

// 0x403ba0: on its circle, theSpin degrees on
void PowerUp::Draw(int theSpin)
{
	mSpin = theSpin;
	if (mVisible)
	{
		int anAngle = mAngle + theSpin;
		if (anAngle >= 0x168)
			anAngle = anAngle - 0x168;
		if (anAngle < 0)
			anAngle = anAngle + 0x168;
		int x;
		int y;
		mBoard->GetPowerUpCenter(&x, &y);
		FossilizedApp* anApp = mBoard->mApp;
		AngleTable* anAngles = anApp->mAngles;
		// port: the original's app +0x1860 + mKind * 4 is the bonus view sheet's cell mKind - 1
		anApp->mBonusViewCells[mKind - 1]->Draw((int) (anAngles->mDX[anAngle] * mRadius + x), (int) (anAngles->mDY[anAngle] * mRadius + y));
	}
}

// 0x403e20
BoomEffect::BoomEffect()
{
	mStretched = false;
	mType = 4;
}

// 0x403e40
BoomEffect::~BoomEffect()
{
}

// 0x403e50
void BoomEffect::Init(int x, int y, FossilizedApp* theApp, float theSpeed)
{
	mX = (float) x;
	mApp = theApp;
	mFrame = 0;
	mY = (float) (y - 5);
	mSpeed = theSpeed;
}

// 0x403e90
bool BoomEffect::Update()
{
	mFrame = mSpeed + mFrame;
	mY = (float) (mY - 0.25);
	return mFrame < 15.0f;
}

// 0x403f60
void BoomEffect::Draw()
{
	if (mStretched == false)
	{
		float x = mX;
		float y = mY;
		Cell* aCell = mApp->mBoomCells[(int) mFrame];
		aCell->Draw((int) x - (aCell->mWidth >> 1), (int) y - (aCell->mHeight >> 1));
		return;
	}
	mApp->mBoomCells[(int) mFrame]->DrawStretched((int) (mX - 75.0f), (int) (mY - 100.0f), (int) (mX + 75.0f), (int) (mY + 100.0f));
}

// 0x4147c0
FallingEggEffect::FallingEggEffect()
{
	mType = 1;
}

// 0x4147e0
FallingEggEffect::~FallingEggEffect()
{
}

// 0x4147f0
void FallingEggEffect::Init(FossilizedApp* theApp, int x, int y, int theColor, int theKind)
{
	mApp = theApp;
	mX = (float) x;
	mColor = theColor;
	mKind = theKind;
	mVY = 0;
	mY = (float) y;
	mGravity = (float) y * 0.015151516f * 0.1f + 1.0f;
}

// 0x414840
bool FallingEggEffect::Update()
{
	mY = mVY * 0.16666667f + mY;
	mVY = mGravity + mVY;
	if (mY <= 650.0f)
		return true;
	if (0x32 <= mColor)
		mApp->mBoard->PieceFell(mColor);
	return false;
}

// 0x414930
void FallingEggEffect::Draw()
{
	if (0x32 <= mColor)
	{
		mApp->mBoard->DrawPiece(mColor, (int) mX, (int) mY);
		return;
	}
	float x = mX;
	float y = mY;
	// port: the original's app +0x13c0 + mColor * 4 is the bomb's cell or the egg sheet's cell mColor - 1
	Cell* aCell = (mColor == 0) ? mApp->mPicBomb_c0 : mApp->mEggCells[mColor - 1];
	aCell->Draw((int) x - (aCell->mWidth >> 1), (int) y - (aCell->mHeight >> 1));
	if (0 < mKind)
	{
		x = mX;
		y = mY;
		// port: the original's app +0x1830 + mKind * 4 is the bonus egg sheet's cell mKind - 1
		aCell = mApp->mBonusEggCells[mKind - 1];
		aCell->Draw((int) x - (aCell->mWidth >> 1), (int) y - (aCell->mHeight >> 1));
	}
}

// The saved games. Where the original writes two neighbouring fields as one block of 8 bytes, the port writes them
// one at a time (the same bytes), and a float the original writes through WriteInt goes through WriteFloat.

// 0x4034e0
void Effect::Save(BinFile* theFile)
{
	theFile->WriteFloat(mX);
	theFile->WriteFloat(mY);
}

// 0x403500
void Effect::Load(BinFile* theFile)
{
	mX = theFile->ReadFloat();
	mY = theFile->ReadFloat();
}

// 0x403660
void BabyEffect::Save(BinFile* theFile)
{
	Effect::Save(theFile);
	theFile->WriteInt(mBaby);
	theFile->WriteFloat(mVY);
	theFile->WriteFloat(mGravity);
	theFile->WriteInt(mDelay);
}

// 0x4036a0
void BabyEffect::Load(BinFile* theFile)
{
	Effect::Load(theFile);
	mBaby = theFile->ReadInt();
	mVY = theFile->ReadFloat();
	mGravity = theFile->ReadFloat();
	mDelay = theFile->ReadInt();
}

// 0x403a90
void PowerUp::SaveState(BinFile* theFile)
{
	theFile->WriteInt(mAngle);
	theFile->WriteInt(mSpin);
	theFile->WriteInt(mKind);
	theFile->WriteInt(mTime);
	theFile->WriteInt(mBlinkTime);
	theFile->WriteFloat(mRadius);
	theFile->WriteBool(mVisible);
	theFile->WriteInt(mBlinkCount);
	theFile->WriteInt(mFlag);
}

// 0x403b10
void PowerUp::LoadState(BinFile* theFile)
{
	mAngle = theFile->ReadInt();
	mSpin = theFile->ReadInt();
	mKind = theFile->ReadInt();
	mTime = theFile->ReadInt();
	mBlinkTime = theFile->ReadInt();
	mRadius = theFile->ReadFloat();
	mVisible = theFile->ReadBool();
	mBlinkCount = theFile->ReadInt();
	mFlag = theFile->ReadInt();
}

// 0x403d00
void BonusItemEffect::Save(BinFile* theFile)
{
	Effect::Save(theFile);
	theFile->WriteFloat(mTargetX);
	theFile->WriteFloat(mTargetY);
	theFile->WriteFloat(mVX);
	theFile->WriteFloat(mVY);
	theFile->WriteInt(mKind);
	theFile->WriteInt(mFrames);
}

// 0x403d50
void BonusItemEffect::Load(BinFile* theFile)
{
	Effect::Load(theFile);
	mTargetX = theFile->ReadFloat();
	mTargetY = theFile->ReadFloat();
	mVX = theFile->ReadFloat();
	mVY = theFile->ReadFloat();
	mKind = theFile->ReadInt();
	mFrames = theFile->ReadInt();
}

// 0x403ec0
void BoomEffect::Save(BinFile* theFile)
{
	Effect::Save(theFile);
	theFile->WriteFloat(mFrame);
	theFile->WriteFloat(mSpeed);
	theFile->WriteBool(mStretched);
}

// 0x403f00
void BoomEffect::Load(BinFile* theFile)
{
	Effect::Load(theFile);
	mFrame = theFile->ReadFloat();
	mSpeed = theFile->ReadFloat();
	mStretched = theFile->ReadBool();
}

// 0x414890
void FallingEggEffect::Save(BinFile* theFile)
{
	Effect::Save(theFile);
	theFile->WriteFloat(mVY);
	theFile->WriteFloat(mGravity);
	theFile->WriteInt(mColor);
	theFile->WriteInt(mKind);
}

// 0x4148d0
void FallingEggEffect::Load(BinFile* theFile)
{
	Effect::Load(theFile);
	mVY = theFile->ReadFloat();
	mGravity = theFile->ReadFloat();
	mColor = theFile->ReadInt();
	mKind = theFile->ReadInt();
}

// 0x415490
void ConfettiEffect::Save(BinFile* theFile)
{
	Effect::Save(theFile);
	theFile->WriteInt(mColor);
	theFile->WriteFloat(mVX);
	theFile->WriteFloat(mVY);
}

// 0x4154c0
void ConfettiEffect::Load(BinFile* theFile)
{
	Effect::Load(theFile);
	mColor = theFile->ReadInt();
	mVX = theFile->ReadFloat();
	mVY = theFile->ReadFloat();
}

// 0x434820
void ScorePopup::Save(BinFile* theFile)
{
	Effect::Save(theFile);
	theFile->WriteInt(mLength);
	theFile->WriteString(mText);
	theFile->WriteBool(mBlink);
	theFile->WriteInt(mBlinkCount);
	theFile->WriteInt(mLife);
	theFile->WriteFloat(mSpeed);
}

// 0x434880
void ScorePopup::Load(BinFile* theFile)
{
	Effect::Load(theFile);
	free(mText);
	mLength = theFile->ReadInt();
	mText = (char*) theFile->ReadAlloc();
	mBlink = theFile->ReadBool();
	mBlinkCount = theFile->ReadInt();
	mLife = theFile->ReadInt();
	mSpeed = theFile->ReadFloat();
}

// 0x434c10
void ShellEffect::Save(BinFile* theFile)
{
	Effect::Save(theFile);
	theFile->WriteInt(mColor);
	theFile->WriteInt(mQuarter);
	theFile->WriteFloat(mGravity);
	theFile->WriteFloat(mVX);
	theFile->WriteFloat(mVY);
}

// 0x434c60
void ShellEffect::Load(BinFile* theFile)
{
	Effect::Load(theFile);
	mColor = theFile->ReadInt();
	mQuarter = theFile->ReadInt();
	mGravity = theFile->ReadFloat();
	mVX = theFile->ReadFloat();
	mVY = theFile->ReadFloat();
}

// 0x442400
void EggWhiteEffect::Save(BinFile* theFile)
{
	Effect::Save(theFile);
	theFile->WriteFloat(mVX);
	theFile->WriteFloat(mVY);
}

// 0x442420
void EggWhiteEffect::Load(BinFile* theFile)
{
	Effect::Load(theFile);
	mVX = theFile->ReadFloat();
	mVY = theFile->ReadFloat();
}

// 0x442560
void YolkEffect::Save(BinFile* theFile)
{
	Effect::Save(theFile);
	theFile->WriteFloat(mVY);
	theFile->WriteFloat(mGravity);
	theFile->WriteBool(mYolk);
	theFile->WriteInt(mWhich);
}

// 0x4425b0
void YolkEffect::Load(BinFile* theFile)
{
	Effect::Load(theFile);
	mVY = theFile->ReadFloat();
	mGravity = theFile->ReadFloat();
	mYolk = theFile->ReadBool();
	mWhich = theFile->ReadInt();
}
