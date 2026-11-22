#include "BonusAttack.h"

#include "BugObj.h"
#include "Player.h"
#include "ScoreBoard.h"

#include <cstring>

using namespace Sexy;

// The value of mCountdown that means no countdown is running
const int ATTACK_NO_COUNTDOWN = -99;

// 0x406fb0: the fields are as operator new left them in the original; the port clears them
BonusAttack::BonusAttack()
{
	mEnded = false;
	mCountdown = ATTACK_NO_COUNTDOWN;
}

// 0x406fc0
void BonusAttack::Setup(int theKind)
{
	Bonus::Setup(theKind);
	mCountdown = ATTACK_NO_COUNTDOWN;
	mEnded = false;
}

// 0x406ff0: a countdown that is running puts the bonus away when it reaches 0 (the original never starts one)
bool BonusAttack::Update()
{
	if (!Bonus::Update())
		return false;
	if (mCountdown != ATTACK_NO_COUNTDOWN) {
		if (mCountdown > 0) {
			mCountdown = mCountdown - 1;
			return true;
		}
		if (mCountdown == 0) {
			mCountdown = ATTACK_NO_COUNTDOWN;
			Reset();
		}
	}
	return true;
}

// 0x407040
void BonusAttack::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey != nullptr)
		Bonus::Notify(theObj);
}

// 0x407060
void BonusAttack::Reset()
{
	mCountdown = ATTACK_NO_COUNTDOWN;
	mEnded = false;
	Bonus::Reset();
}

// 0x407080: the first End also sets the bugs' slow-down to 0 (a leftover: this bonus never changes it)
void BonusAttack::End()
{
	mCountdown = ATTACK_NO_COUNTDOWN;
	if (!mEnded) {
		mEnded = true;
		BugObj::SetSlowFrames(0);
	}
}

// 0x4070b0: the monkeys are woken to shoot, and the bonus, hidden, takes the values of an icon (depth 20, twice as big)
void BonusAttack::Start()
{
	strcpy(mName, "PEA SHOOTERS");
	Bonus::Start();
	Player::sCurrent->mScoreBoard->WakeMonkeys();
	SetHidden(true);
	SetStateInt(STATE_X, 0);
	SetStateInt(STATE_Y, 0);
	SetStateInt(STATE_Z, 20);
	SetStateInt(STATE_ALPHA, 1);
	SetState(STATE_SCALE_X, 2.0f);
	SetState(STATE_SCALE_Y, 2.0f);
	SetStateInt(STATE_FRAME, 0);
}
