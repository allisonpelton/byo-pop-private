// Brick: the playfield objects. This file is the logic of the base class (the plain coloured brick); the drawing is
// in BrickDraw.cpp and the four classes made from it in BrickKinds.cpp. Each function says the original's address.
// "(listener)" marks the functions of the listener part: the original gets the object + 0xc for them, so the
// offsets of their fields in the decompilation are 0xc lower than the object's own, and the comments use the object's.

#include "Brick.h"
#include "Board.h"
#include "AstroPopApp.h"
#include "Res.h"
#include "SoundManager.h"
#include <math.h>

using namespace Sexy;

// 0x4067b0
Brick::Brick(Board* theBoard, EventBus* theBus, int theColumn) :
	EventListener(theBus)
{
	mBoard = theBoard;
	mRainbowIndex = -1;
	mRainbowTimer = 0x19;
	mGone = false;
	mFlag194 = false;
	mGlowAlpha = 0xff;
	mTrail = false;
	mNumParticles = 10;
	mLight = -1;
	mColor = Color(0, 0, 0);
	mVisible = true;
	mSupport = NULL;
	mTargetY = 0;
	mHidden = false;
	mFloorY = 0;
	mFlag187 = false;
	mTick = 0;
	mBouncing = false;
	mRainbow = false;
	mBounceFire = true;
	mBounceY = 0;
	mFlag18B = false;
	mNum1D8 = 0;
	mFlag17C = false;
	mBounceDelay = 0;
	mSpecial = false;
	mFlag17F = false;
	mFlag180 = false;
	mShake = false;
	mFlag18A = false;
	mShakeX = 0;
	mShakeY = 0;
	mFlag18C = true;
	mFlag17B = false;
	mWobble = false;
	mBounceState = 0;
	mColor2 = Color(0, 0, 0);
	mFlag188 = false;
	mNum1B4 = 0;
	mOffsetY = 0;
	mWobbleSpeed = 0;
	mFlag183 = false;
	mFlag184 = true;
	mFlag185 = false;
	mFade = 0;
	mMutateTimer = 0;
	mMutated = 500000000;
	mImage = NULL;
	mPulse = 500000000;
	mGlowImage = NULL;
	mSprocketOn = false;
	mSprocketAlpha = 0;
	mSprocketLight = 0;
	mSprocketCel = 0;
	mFlag190 = false;
	mLightValue = 0;
	mFlag193 = false;
	mTurbotOn = false;
	mTurbotAlpha = 0;
	mTurbotLight = 0;
	mTurbotCel = 0;
	mNum1A0 = 0;
	mType = 9999;
	mFlag181 = false;
	mNum1C0 = -1;
	mFlag16C = mFlag16D = mFlag16E = mFlag16F = false;
	mFlag172 = false;
	mFlag175 = false;
	mFlag176 = false;
	mNum1B0 = 5;

	mDeathAnim.mHoldLast = true;
	mDeathAnim.SetNumFrames(IMAGE_BRICK_DEATH_RED->mNumCols);
	mDeathAnim.SetDelay(5);
	mGlowAnim.SetNumFrames(IMAGE_BRICK_BONUS_GLOW->mNumCols);
	mGlowAnim.SetDelay(13);
	mGlowAnim.mHoldLast = false;
	mGlowAnim.mPingPong = false;
	mGlowAnim.SetStopped(true);
	mFxAnim.SetNumFrames(0);
	mColumn = theColumn;
	mAnim.SetDelay(10);
	mAnim.SetNumFrames(1);
	mNumFrames = 1;
	mAnim.mHoldLast = true;
	mY = 0;
	mX = 0;
	mSpeedX = 0;
	mSpeedY = 0;
	mState = 0;

	Listen(1);
	Listen(2);
	Listen(3);
	Listen(0xb);
	Listen(9);
	Listen(0x20);
	Listen(0x18);
	Listen(0x16);
	Listen(0x17);
	Listen(0x19);

	// the fall particles: a shower thrown up and to the sides, the first ones slower
	mParticles.resize(mNumParticles + 35, NULL);
	int aBase = mNumParticles;
	float aStepDivisor = (float) ((int) (AstroRand() % 10) + aBase);
	float aVX = -3.0f;
	for (int i = 0; i < mNumParticles + 35; i++)
	{
		float aSpeed = (float) (AstroRand() % 100) * 0.001f - 0.5f;
		if (i <= mNumParticles)
			aVX = aVX + 10.0f / aStepDivisor;
		float aVY = -0.5f - (float) (AstroRand() % 0x5dc) * 0.001f;
		float aGravity = (float) (AstroRand() % 0x259) * 0.001f + 0.55f;
		float anAngle = (float) (AstroRand() % 0x50) + 10.0f;
		float aHeight = (float) (AstroRand() % 100 + 0x19);
		mParticles[i] = new Particle(aSpeed, aVX, aVY, (int) anAngle, aGravity, aHeight, gSinTable, gCosTable);
	}
}

// 0x4060f0 (and the deleting destructors 0x4066b0, 0x4066e0, 0x406710, 0x407d30, 0x407e30)
Brick::~Brick()
{
	for (size_t i = 0; i < mParticles.size(); i++)
		delete mParticles[i];
	// the original forgets the sparks (leak); they go here
	for (size_t i = 0; i < mSparks.size(); i++)
		delete mSparks[i];
	StopListeningToAll();
	delete mGlowImage;
}

//
// the events (listener part)
//

void Brick::OnEvent(int theEvent, EventListener* theSource, int theArg)
{
	(void) theSource;
	switch (theEvent)
	{
	case EVENT_SPAWN:		OnSpawn(); break;
	case EVENT_LAND:		OnLand(); break;
	case EVENT_SETTLE:		OnSettle(); break;
	case EVENT_DIE:			OnDie(); break;
	case EVENT_STOP:		OnStop(); break;
	case EVENT_BEGIN_FALL:	OnBeginFall(); break;
	case EVENT_1E:			OnEvent1E(); break;
	case EVENT_14:			OnEvent14(); break;
	case EVENT_16:			OnEvent16(); break;
	case EVENT_17:			OnEvent17(); break;
	case EVENT_18:			OnEvent18(); break;
	case EVENT_19:			OnEvent19(); break;
	case EVENT_20:			OnEvent20(theArg); break;
	case EVENT_21:			mSprocketLight = theArg; break;				// 0x402650
	case EVENT_22:			mSprocketCel = theArg; break;				// 0x402660
	case EVENT_23:			mTurbotLight = theArg; break;				// 0x402730
	case EVENT_24:			mTurbotCel = theArg; break;					// 0x402740
	case EVENT_25:			mFlag194 = theArg < 0xff; mGlowAlpha = theArg; break;		// 0x401b90
	case EVENT_28:			mFlag18B = theArg == 1; break;				// 0x401d40
	case EVENT_2A:			OnEvent2A(theArg); break;
	default:				break;
	}
}

// 0x402fe0 (listener) event 0: the object starts afresh
void Brick::OnSpawn()
{
	StopListeningToAll();
	Listen(1);
	Listen(2);
	Listen(3);
	Listen(9);
	Listen(0xb);
	Listen(0x20);
	Listen(0x18);
	Listen(0x19);
	Listen(0x16);
	Listen(0x17);
	if (!mGlowAnim.mStopped)
	{
		if (!IsListening(0x25))
			Listen(0x25);
	}
	SetState(0);
	SetVisible(true);
	mSpeedX = 0;
	mSpeedY = 0;
	mShake = false;
	mFlag190 = false;
	mSprocketOn = false;
	mTurbotOn = false;
	mFlag193 = false;
	mLightValue = 0;
	FireLanded();
}

// 0x401920 (listener) event 1
void Brick::OnLand()
{
	mSpeedY = 30.0f;
	if (mState == 2)
	{
		mAnim.Reset();
		mAnim.SetNumFrames(mNumFrames);
	}
	if (mY <= 423.0f)
		SetVisible(true);
	else
		SetVisible(false);
	StopListeningToAll();
	Listen(2);
	Listen(0x20);
	if (!mGlowAnim.mStopped)
	{
		if (!IsListening(0x25))
			Listen(0x25);
	}
	mShake = false;
	SetState(1);
}

// 0x402f20 (listener) event 2
void Brick::OnSettle()
{
	if (mFloorY < mY)
		SetVisible(false);
	StopFlash();
	if (mMutated == 0)
	{
		mMutated = 500000000;
		Fire(0x1b);
	}
	StopListeningToAll();
	Listen(1);
	Listen(0x20);
	if (!mGlowAnim.mStopped)
	{
		if (!IsListening(0x25))
			Listen(0x25);
	}
	mFlag18C = false;
	mBouncing = false;
	mBounceY = 0;
	mBounceFire = false;
	mSpeedY = 30.0f;
	SetState(2);
	mShake = false;
}

// 0x403fd0 (listener) event 3: the brick is hit
void Brick::OnDie()
{
	for (int i = 0; i < mNumParticles; i++)
	{
		mParticles[i]->SetX((float) (AstroRand() % 0xf) + mX);
		mParticles[i]->SetY((float) (AstroRand() % 0xf) + mY);
	}
	mShake = false;
	StopListeningToAll();
	Listen(4);
	if (mMutated == 1)
	{
		mDeathAnim.SetNumFrames(IMAGE_BRICK_MUTATOR_DEATH->mNumCols);
		mDeathAnim.mPingPong = false;
		mDeathAnim.mHoldLast = true;
		gApp->PlaySample(SOUND_MUTATOR_DIE);
	}
	mDeathAnim.SetStopped(false);
	if (!mVisible)
	{
		if (mMutated != 1)
		{
			mFlag17B = true;
			SetState(3);
			return;
		}
	}
	else if (mMutated != 1)
	{
		mFlag17B = false;
		if (mType >= 0 && mType < 6 && (!mSpecial || mMutated == 1))
		{
			int aType = 0xc;
			if (mMutated != 1)
				aType = mType;
			Fire(0x27, aType);
			SetState(3);
			return;
		}
	}
	Fire(0x1a);
	SetState(3);
}

// 0x4019d0 (listener) event 4
void Brick::OnStop()
{
	StopListeningToAll();
	mSpeedY = 6.0f;
	mSpeedX = 18.0f;
	mShake = false;
	SetState(6);
}

// 0x401a20 (listener) event 9
void Brick::OnBeginFall()
{
	mLight = 0;
	mNum1B4 = 6;
	SetState(8);
	mShake = false;
	StopListeningToAll();
}

// 0x4026d0 (listener) event 0x1e
void Brick::OnEvent1E()
{
	mTurbotOn = false;
	mFlag193 = false;
	StopListening(0x1e);
	StopListening(0x24);
	StopListening(0x23);
	Listen(0x19);
}

// 0x401780 (listener) event 0x14
void Brick::OnEvent14()
{
	SetState(0xb);
	mSpeedY = 0;
}

// 0x4025d0 (listener), the list 0x16: the sprocket light goes on
void Brick::OnEvent16()
{
	mSprocketOn = true;
	mFlag190 = false;
	mSprocketAlpha = 0;
	mLightValue = 0;
	StopListening(0x18);
	if (IsListening(0x16))
		StopListening(0x16);
	Listen(0x21);
	Listen(0x22);
}

// 0x402670 (listener), the list 0x17: the turbot light goes on
void Brick::OnEvent17()
{
	mTurbotOn = true;
	mFlag193 = false;
	mTurbotAlpha = 0;
	mLightValue = 0;
	StopListening(0x19);
	if (IsListening(0x17))
		StopListening(0x17);
	Listen(0x23);
	Listen(0x24);
	Listen(0x1e);
}

// 0x402630 (listener), the list 0x18: the sprocket light goes off
void Brick::OnEvent18()
{
	mSprocketOn = false;
	mFlag190 = true;
	mLightValue = 0xff;
}

// 0x402710 (listener), the list 0x19: the turbot light goes off
void Brick::OnEvent19()
{
	mTurbotOn = false;
	mFlag193 = true;
	mLightValue = 0xff;
}

// 0x401a60 (listener) event 0x20: the number is the type of brick that was hit (or its negative: the highlight goes)
void Brick::OnEvent20(int theArg)
{
	if ((theArg == -mType && theArg != 0) || (theArg == -500000000 && mType == 0))
	{
		mNum1C0 = -1;
		mGlowAnim.Reset();
		mGlowAnim.SetStopped(true);
		mFlag194 = false;
		if (IsListening(0x25))
			StopListening(0x25);
	}
	else
	{
		int aType = 0xc;
		if (mMutated != 1)
			aType = mType;
		if (theArg == aType && (!mSpecial || mMutated == 1) &&
			(mMutated == 1 || (mType != 6 && (mMutated == 1 || mType != 7))) &&
			GetEffectiveType() != 9 && GetEffectiveType() != 0xb && mNum1C0 == -1)
		{
			if (!IsListening(0x25))
				Listen(0x25);
			mFlag194 = false;
			mNum1C0 = 0;
			mFlag181 = true;
			mGlowAnim.SetStopped(false);
		}
	}
}

//
// the object part
//

// 0x402ee0 slot 1
void Brick::SetState(int theState)
{
	mState = theState;
	if (mMutated == 0 || mMutated == 1)
	{
		if (!IsListening(0x28))
			Listen(0x28);
	}
}

// 0x402e10 slot 15
bool Brick::Update()
{
	if (!mGone)
	{
		mTick++;
		bool aStepDone = false;
		switch (mState)
		{
		case 0:		UpdateState0(); break;
		case 2:		UpdateState2(); break;
		case 1:		UpdateState1(); break;
		case 3:		UpdateState3(); break;
		case 6:		aStepDone = UpdateState6(); break;
		case 8:		UpdateState8(); break;
		}
		bool aParticlesDone = CheckParticlesDone();
		mGlowAnim.Update();
		UpdateRainbow();
		UpdateFade();
		UpdateLight();
		UpdateMutation();
		UpdateSparks();
		if (mWobble)
			UpdateWobble();
		return aStepDone ? true : aParticlesDone;
	}
	return false;
}

// 0x401310 slot 2: waiting (and the bounce after a landing)
void Brick::UpdateState0()
{
	if ((mAnim.mStopped || mAnim.mDone || (0 < mAnim.mMaxLoops && mAnim.mMaxLoops <= mAnim.mLoopCount)) &&
		mTick % 700 == 0)
		mAnim.Reset();
	mAnim.Update();

	if (mShake)
	{
		mShakeX = (int) (AstroRand() & 1) - 1;
		mShakeY = (int) (AstroRand() & 1) - 1;
	}

	if (mBouncing)
	{
		if (mBounceDelay < 1)
		{
			if (mBounceState == 2)
			{
				mY = mSpeedY + mY;
				if (mY >= mBounceY)
				{
					mY = mBounceY;
					mBounceY = 0;
					mBouncing = false;
					if (mBounceFire)
						Fire(0xb);
				}
			}
			else if (mBounceState == 0)
			{
				mY = mY + mSpeedY;
				if (!(mY < mBounceY))
				{
					mY = mBounceY;
					mBounceState = 1;
					mSpeedY = -2.5f;
				}
			}
			else if (mBounceState == 1)
			{
				mY = mY + mSpeedY;
				if (!(mY > mBounceY))
				{
					mY = mBounceY;
					mBounceState = 2;
					mSpeedY = 1.5f;
				}
			}
		}
		else
			mBounceDelay--;
	}

	if (!mSprocketOn)
	{
		if (mFlag190 && mLightValue > 0)
		{
			mLightValue -= 4;
			if (mLightValue < 1)
				mLightValue = 0;
		}
	}
	else
	{
		mSprocketAlpha += 4;
		if (mSprocketAlpha > 0xfe)
			mSprocketAlpha = 0xff;
	}

	if (!mTurbotOn)
	{
		if (mFlag193 && mLightValue > 0)
		{
			mLightValue -= 4;
			if (mLightValue < 1)
				mLightValue = 0;
		}
	}
	else
	{
		mTurbotAlpha += 4;
		if (mTurbotAlpha > 0xfe)
			mTurbotAlpha = 0xff;
	}
}

// 0x401570 slot 3: moving to the floor
void Brick::UpdateState2()
{
	mAnim.Update();
	if (mVisible)
	{
		mY = mSpeedY + mY;
		if (mY >= mFloorY)
		{
			mFlag18C = true;
			mY = mFloorY;
			SetVisible(false);
			Fire(7);
		}
	}
}

// 0x4015d0 slot 4: rising to its place
void Brick::UpdateState1()
{
	if (!(mY < mTargetY) || mFlag17C)
	{
		mY = mY - mSpeedY;
		if (!mVisible && mY < 423.0f)
			SetVisible(true);
		if (mFlag188 && mSupport != NULL)
		{
			float aMin = mSupport->mY + 25.0f;
			if (mY < aMin)
				mY = aMin;
		}
		if (mY < mTargetY)
		{
			if (mSupport == NULL || !mFlag188)
				mY = mTargetY;
			if (mFlag17C)
				mFlag17C = false;
			if (IsListening(2))
				StopListening(2);
			if (IsListening(3))
				Listen(3);			// (the original registers it a second time)
			if (!IsListening(0))
				Listen(0);
			Fire(5);
		}
	}
}

// 0x401700 slot 5
void Brick::UpdateState3()
{
	mGlowAnim.Update();
	mDeathAnim.Update();
	if (mDeathAnim.mStopped && mDeathAnim.mDone)
	{
		SetState(4);
		Fire(6);
	}
}

// 0x401750 slot 6
bool Brick::UpdateState6()
{
	if (mVisible)
	{
		Fire(8);
		SetVisible(false);
		SetState(7);
		return true;
	}
	return false;
}

// 0x4017f0 slot 7
void Brick::UpdateState8()
{
	mLight = mNum1B4 + mLight;
	if (mLight > 0xff)
		mLight = 0xff;
}

// 0x4017a0 slot 8
bool Brick::UpdateFallOff()
{
	mSpeedY = mSpeedY + 1.0f;
	mY = mSpeedY + mY;
	if (mY >= 480.0f)
	{
		Fire(0x15);
		mGone = true;
		return true;
	}
	return false;
}

// 0x403f50 slot 9: the death animation is over when every particle has played
bool Brick::CheckParticlesDone()
{
	if (mState == 3 || mState == 4 || mState == 6 || mState == 7)
	{
		bool allDone = true;
		for (int i = 0; i < mNumParticles; i++)
		{
			mParticles[i]->Update();
			if (!mParticles[i]->mDone)
				allDone = false;
		}
		if (allDone && mState == 7)
		{
			SetState(10);
			Fire(0xc);
			return true;
		}
	}
	return false;
}

// 0x401820 slot 10: the fade of the dimming
void Brick::UpdateFade()
{
	if (mFlag185)
	{
		if (!mFlag184)
		{
			mFade -= 4;
			if (mFade < 1)
			{
				mFade = 0;
				mFlag184 = true;
			}
		}
		else
		{
			mFade += 4;
			if (mFade > 0xfe)
			{
				mFade = 0xff;
				mFlag184 = false;
			}
		}
	}
}

// 0x4062d0 slot 11: a rainbow brick shows its colours in turn
void Brick::UpdateRainbow()
{
	if (mRainbow && mRainbowIndex >= 0)
	{
		mRainbowTimer--;
		if (mRainbowTimer < 1)
		{
			mRainbowTimer = 5;
			// (the original shifts the list down by one slot first: a no-op for what is drawn)
			if (!mRainbowImages.empty())
				mRainbowIndex = (int) (((int) AstroRand() % 500000000) % (unsigned int) mRainbowImages.size());
			else
			{
				mRainbowIndex = -1;
				mRainbow = false;
			}
		}
	}
}

// 0x406d70: the brick becomes a rainbow brick
void Brick::MakeRainbow()
{
	mRainbowImages.clear();
	mRainbowImages.push_back(IMAGE_BRICK_RED);
	mRainbowImages.push_back(IMAGE_BRICK_GREEN);
	mRainbowImages.push_back(IMAGE_BRICK_BLUE);
	mRainbowImages.push_back(IMAGE_BRICK_YELLOW);
	mRainbowImages.push_back(IMAGE_BRICK_PINK);
	unsigned int aCount = (unsigned int) mRainbowImages.size();
	int aRand = (int) AstroRand();
	mRainbowTimer = 5;
	mRainbow = true;
	mRainbowIndex = (int) (((unsigned int) (aRand % 500000000)) % aCount);
}

// 0x4018f0 slot 25
void Brick::SetVisible(bool theVisible)
{
	mVisible = theVisible;
}

// 0x401900 slot 19
void Brick::Finish()
{
	mFlag18C = true;
	Fire(7);
	OnLand();
}

// 0x401c60 slot 18
void Brick::StartFlash()
{
	mFlag180 = true;
	mLight = 0;
	mFlag17F = true;
}

// 0x406270 slot 20
bool Brick::IsIdle()
{
	return mVisible && (mState == 0 || mState == 1 || mState == 5);
}

// 0x4062a0 slot 23
void Brick::SetSpeedX(float theSpeed)
{
	mSpeedX = theSpeed;
}

// 0x4062b0 slot 24
void Brick::SetSpeedY(float theSpeed)
{
	mSpeedY = theSpeed;
}

// 0x404180 slot 21
void Brick::SetParticleX(float theX)
{
	mX = theX;
	for (int i = 0; i < mNumParticles; i++)
		mParticles[i]->SetX((float) (AstroRand() % 0xf) + theX);
}

// 0x4041f0 slot 22
void Brick::SetParticleY(float theY)
{
	mY = theY;
	for (int i = 0; i < mNumParticles; i++)
		mParticles[i]->SetY((float) (AstroRand() % 0xf) + theY);
}

// 0x401230 slot 26
void Brick::StartMutatorCountdown()
{
	if (!IsListening(0x28))
		Listen(0x28);
	mMutated = 0;
	mMutateTimer = 0;
	mPulse = 0;
}

// 0x4010f0: the type the rules go by (a mutated brick is a mutator, 12)
int Brick::GetEffectiveType()
{
	int aType = 0xc;
	if (mMutated != 1)
		aType = mType;
	return aType;
}

// 0x401110
bool Brick::IsJammed()
{
	return mSpecial && mMutated != 1;
}

// 0x401130
void Brick::Retire()
{
	mFlag183 = true;
	StopListeningToAll();
}

// 0x401270: where the brick is to rise to
void Brick::SetTargetY(int theY)
{
	if (mTargetY == (float) theY)
	{
		if (!mFlag188)
			Fire(0x10);
		mFlag188 = true;
	}
	mTargetY = (float) theY;
	StopListeningToAll();
	Listen(0x20);
	if (!mGlowAnim.mStopped)
	{
		if (!IsListening(0x25))
			Listen(0x25);
	}
	Listen(2);
	if (mY < mTargetY)
		Listen(0);
	mFlag17C = true;
}

// 0x4025a0
void Brick::FireLanded()
{
	if (mFlag188)
		Fire(0x11);
	mFlag188 = false;
	mSupport = NULL;
}

// 0x401bb0: the drop by one row
void Brick::StartBounce()
{
	if (mY == 4.0f)
		mY = mY + 5.0f;
	mBouncing = true;
	mBounceState = 0;
	mBounceFire = true;
	mSpeedY = 2.0f;
	mBounceY = mY + 25.0f;
}

// 0x401c10
void Brick::StartWobble()
{
	if (mState != 6)
	{
		mWobble = true;
		mOffsetY = 0;
		mWobbleSpeed = -1.0f;
	}
}

// 0x401880
void Brick::UpdateWobble()
{
	mOffsetY = mOffsetY + mWobbleSpeed;
	if (mOffsetY < -5.0f)
	{
		mOffsetY = -5.0f;
		mWobbleSpeed = -mWobbleSpeed;
		return;
	}
	if (0.0f < mOffsetY)
	{
		mWobble = false;
		mOffsetY = 0;
	}
}

// 0x401c80
void Brick::StopFlash()
{
	mFlag180 = true;
	mFlag17F = false;
}

// 0x401c90
void Brick::UpdateLight()
{
	if (mFlag180)
	{
		if (!mFlag17F)
		{
			mLight -= 8;
			if (mLight < 0)
			{
				mLight = 0;
				mFlag180 = false;
			}
		}
		else
		{
			int aLimit = 0x80;
			if (mType == 2)
				aLimit = 0xff;
			else if (mType == 0)
				aLimit = 0xd0;
			mLight += 8;
			if (aLimit < mLight)
			{
				mLight = aLimit;
				mFlag180 = false;
			}
		}
	}
}

// 0x401d00
void Brick::StartShake()
{
	mShake = true;
	mShakeX = 0;
	mShakeY = 0;
	mTrail = true;
}

// 0x401d20
void Brick::StopShake()
{
	mShake = false;
	mTrail = false;
}

// 0x401d30
void Brick::StopFade()
{
	mFlag185 = false;
}

// 0x401d60: a brick that stays long enough mutates (the green glow, drops by itself later)
void Brick::UpdateMutation()
{
	bool aMoved = mState != 0;
	bool aHigh = mY < 9.0f;
	if (mMutated != 1 || aHigh || aMoved || mFlag18B)
	{
		if (mMutated == 0 && !aHigh && !aMoved && !mFlag18B && (++mMutateTimer > 299))
		{
			mMutateTimer = 0;
			mMutated = 1;
			mNum1C0 = -1;
			mGlowAnim.Reset();
			mGlowAnim.SetStopped(true);
			if (!IsListening(0x28))
				Listen(0x28);
			mAnim.Reset();
			mAnim.SetNumFrames(IMAGE_BRICK_MUTATOR->mNumCols);
			mColor = Color(0, 0xff, 0x8a);
			mColor2 = Color(0x80, 0x80, 0x80, 0x80);
			RebuildGlow();
			gApp->PlaySampleAt(SOUND_MUTATION, ((int) mX - 0xa6) / 44);
		}
	}
	else
	{
		mMutateTimer++;
		if (mMutateTimer >= mBoard->mDropTicks)
		{
			mMutateTimer = 0;
			Fire(0xf);
		}
	}

	if (!mFlag18B && (mMutated == 0 || mMutated == 1))
	{
		bool aUp = mFlag186;
		if (!aUp)
			mPulse = mPulse - 3;
		else
			mPulse = mPulse + 3;
		if (mPulse > 0xfe)
		{
			mPulse = 0xff;
			mFlag186 = !aUp;
			return;
		}
		if (mPulse < 0)
		{
			mPulse = 0;
			mFlag186 = !aUp;
		}
	}
}

// 0x401fa0: the brick is a mutator from the start
void Brick::StartMutation()
{
	if (!IsListening(0x28))
		Listen(0x28);
	mMutated = 1;
	mMutateTimer = 0;
	mColor = Color(0, 0xff, 0x8a);
	mColor2 = Color(0x80, 0x80, 0x80, 0x80);
	RebuildGlow();
	mAnim.SetNumFrames(IMAGE_BRICK_MUTATOR->mNumCols);
	mAnim.SetDelay(0xc);
	mAnim.mPingPong = true;
	mAnim.mHoldLast = true;
	mAnim.SetStopped(false);
}

// 0x401140: the glow image in the brick's colour
void Brick::RebuildGlow()
{
	delete mGlowImage;
	mGlowImage = new MemoryImage(gApp);
	mGlowImage->Create(IMAGE_BRICK_GLOW->GetWidth(), IMAGE_BRICK_GLOW->GetHeight());
	Graphics g(mGlowImage);
	g.SetColorizeImages(true);
	g.SetColor(mColor);
	g.DrawImage(IMAGE_BRICK_GLOW, 0, 0);
	g.SetColorizeImages(false);
}

// 0x401c40: plays a sample through the board's application (the object at the board's +0x358, vtable slot 41)
void Brick::PlayBoardSound(int theSound)
{
	mBoard->mApp->PlaySample(theSound);
}

// 0x4033b0
void Brick::AdvanceParticleCount()
{
	if ((!mSpecial || mMutated == 1) && mType < 6)
	{
		mNumParticles += 0x23;
		if (mParticles.empty())
		{
			mNumParticles = -1;
			return;
		}
		if ((unsigned int) mParticles.size() <= (unsigned int) mNumParticles)
			mNumParticles = (int) mParticles.size() - 1;
	}
}

// 0x404750, 0x404790, 0x4047d0, 0x404810: every particle gets the image, drawn additively
void Brick::SetParticleImages(Image* theImage)
{
	for (size_t i = 0; i < mParticles.size(); i++)
	{
		mParticles[i]->SetImage(theImage, 4);
		mParticles[i]->mAdditive = true;
	}
}

// 0x406e20 slot 12: sparks rise from a brick that is up in the field
void Brick::UpdateSparks()
{
	if (mY >= 384.0f)
	{
		if (mTick % 0x32 == 0)
		{
			float aSpeed = -0.5f - (float) (AstroRand() % 0x4b) * 0.01f;
			float aVX = (float) (AstroRand() % 100) * 0.001f - 0.5f;
			float aVY = -0.5f - (float) (AstroRand() % 0x5dc) * 0.001f;
			float aGravity = (float) (AstroRand() % 0x259) * 0.001f + 0.55f;
			float anAngle = (float) (AstroRand() % 0x2d);
			float aHeight = (float) (AstroRand() % 100 + 0x19);
			Particle* aParticle = new Particle(aSpeed, aVX, aVY, (int) anAngle, aGravity, aHeight, gSinTable, gCosTable);
			mSparks.push_back(aParticle);
			aParticle->SetX((float) (AstroRand() % 0x2c >> 1) + mX);
			aParticle->SetY((float) (AstroRand() % 0x19) + mY);
			aParticle->SetImage(GetImageById(0x25b), 5);

			aSpeed = (float) (AstroRand() % 0x4b) * 0.01f + 0.5f;
			aVX = (float) (AstroRand() % 100) * 0.001f - 0.5f;
			aVY = -0.5f - (float) (AstroRand() % 0x5dc) * 0.001f;
			aGravity = (float) (AstroRand() % 0x259) * 0.001f + 0.55f;
			anAngle = (float) (AstroRand() % 0x2d);
			aHeight = (float) (AstroRand() % 100 + 0x19);
			aParticle = new Particle(aSpeed, aVX, aVY, (int) anAngle, aGravity, aHeight, gSinTable, gCosTable);
			mSparks.push_back(aParticle);
			aParticle->SetX((float) (AstroRand() % 0x2c >> 1) + mX + 22.0f);
			aParticle->SetY((float) (AstroRand() % 0x19) + mY);
			aParticle->SetImage(GetImageById(0x25b), 5);
		}
		for (size_t i = 0; i < mSparks.size(); i++)
		{
			mSparks[i]->Update();
			if (mSparks[i]->mDone)
			{
				delete mSparks[i];
				mSparks.erase(mSparks.begin() + i);
				i--;
			}
		}
	}
}

// 0x403c60 slot 16: the brick takes the colour of its type
void Brick::SetType(int theType)
{
	mType = theType;
	switch (theType)
	{
	case 0:		mColor = Color(0xb8, 0, 0);			mColor2 = Color(0xb8, 0, 0, 0x80); break;
	case 1:		mColor = Color(0, 0x80, 0);			mColor2 = Color(0, 0x80, 0, 0x80); break;
	case 2:		mColor = Color(0, 0, 0xff);			mColor2 = Color(0x32, 0x32, 0xff, 0x80); break;
	case 3:		mColor = Color(0x80, 0x80, 0);		mColor2 = Color(0x80, 0x80, 0, 0x80); break;
	case 4:		mColor = Color(0x80, 0, 0x80);		mColor2 = Color(0x80, 0, 0x80, 0x80); break;
	case 5:		mColor = Color(0xb, 0xcb, 0xd7);	mColor2 = Color(0xb, 0xcb, 0xd7); break;
	default:	break;
	}
	mDeathAnim.SetNumFrames(GetImageById(mType + 0x230)->mNumCols);
	for (size_t i = 0; i < mParticles.size(); i++)
		mParticles[i]->SetImage(GetImageById(mType + 0x23f), 5);
	mImage = TypeImage(mType, mSpecial);
	RebuildGlow();
}
