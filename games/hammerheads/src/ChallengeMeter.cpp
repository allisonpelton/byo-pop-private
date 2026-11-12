#include "ChallengeMeter.h"

#include "DrawUtil.h"
#include "Graphics.h"
#include "Image.h"
#include "Rect.h"
#include "Res.h"
#include "Stats.h"

using namespace Sexy;

static ObjectRef gChallengeMeterRef;		// 0x597f80

// 0x443d60
ChallengeMeter::ChallengeMeter() :
	Object(0x80)
{
}

// 0x443a30
ChallengeMeter::~ChallengeMeter()
{
}

// 0x4438d0
ChallengeMeter* Sexy::GetChallengeMeter()
{
	return dynamic_cast<ChallengeMeter*>(gChallengeMeterRef.Get());
}

// 0x443d60
ChallengeMeter* Sexy::MakeChallengeMeter()
{
	ChallengeMeter* aMeter = new ChallengeMeter();
	aMeter->SetName("");
	gChallengeMeterRef = ObjectRef(aMeter);
	return aMeter;
}

// 0x443900
void Sexy::GetNextCellPos(double* thePos)
{
	int aCount = GetStats()->GetInt(STAT_CHALLENGE_COUNT);
	int aCellWidth = IMAGE_CHALLENGE_METER->GetCelRect(0).mWidth;
	thePos[0] = (double)(aCellWidth * aCount + 5);
	thePos[1] = 10.0;
}

// 0x443a60: the time of each flight runs down, and a flight that has run out is taken away (the loop goes on to the entry
// after the next one, so the one that moves up into its place is left until the following step, as in the original)
void ChallengeMeter::Update(float theFrac)
{
	Object::Update(theFrac);

	for (int i = 0; i < (int)mFlights.size(); i++)
	{
		if (mFlights[i].mTime > 0.0)
		{
			mFlights[i].mTime = mFlights[i].mTime - (double)theFrac * 0.01;
			if (mFlights[i].mTime <= 0.0)
			{
				mFlights[i].mTime = 0.0;
				mFlights.erase(mFlights.begin() + i);
			}
		}
	}
}

// 0x443b00
void ChallengeMeter::Draw(Graphics* g)
{
	if (mDead)
		return;

	int aCount = GetStats()->GetInt(STAT_CHALLENGE_COUNT);
	if (!mFlights.empty())
		aCount = mFlights[0].mCount;

	for (int i = 0; i < GetStats()->GetInt(STAT_CHALLENGE_GOAL); i++)
	{
		Rect aCell = IMAGE_CHALLENGE_METER->GetCelRect(1);
		g->DrawImage(IMAGE_CHALLENGE_METER, aCell.mWidth * i + 5, 10, aCell);
	}

	for (int i = 0; i < aCount; i++)
	{
		Rect aCell = IMAGE_CHALLENGE_METER->GetCelRect(0);
		g->DrawImage(IMAGE_CHALLENGE_METER, aCell.mWidth * i + 5, 10, aCell);
	}
}

// 0x443cc0
void ChallengeMeter::AddCount(int theKind, int theAmount)
{
	if (theKind != 0)
		return;

	CountFlight aFlight = { GetStats()->GetInt(STAT_CHALLENGE_COUNT), 1.0 };
	mFlights.push_back(aFlight);

	if (GetStats()->GetInt(STAT_CHALLENGE_GOAL) < GetStats()->GetInt(STAT_CHALLENGE_COUNT) + theAmount)
		GetStats()->SetInt(STAT_CHALLENGE_COUNT, GetStats()->GetInt(STAT_CHALLENGE_GOAL));
	else
		GetStats()->SetInt(STAT_CHALLENGE_COUNT, GetStats()->GetInt(STAT_CHALLENGE_COUNT) + theAmount);
}
