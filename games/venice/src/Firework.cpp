// The firework (0x404580-0x404b00).

#include "Firework.h"

#include "ImageRecord.h"
#include "Level.h"
#include "VeniceGlobals.h"
#include "VeniceMath.h"

Firework::Firework()
{
	OnFirstUpdate();
}

void Firework::OnFirstUpdate()
{
	mStages.Clear();
	if (mImage == nullptr)
		SetImage("gfx\\white_particle.png", 0, 0, 0, 0);
	AddStage((float)RandomFloat(100.0f, 600.0f), 0, 0, 1, nullptr);
	AddStage(1000.0f, 0, 0, 1, "rise");
	float aRed;
	float aGreen;
	float aBlue;
	switch (RandomBetween(1, 6)) {
	case 1:
		aRed = 1.0f;
		aGreen = 0.1f;
		aBlue = 0.1f;
		break;
	case 2:
		aRed = 0.1f;
		aGreen = 1.0f;
		aBlue = 0.1f;
		break;
	case 3:
		aRed = 0.1f;
		aGreen = 0.1f;
		aBlue = 1.0f;
		break;
	case 4:
		aRed = 0.1f;
		aGreen = 1.0f;
		aBlue = 1.0f;
		break;
	case 5:
		aRed = 0.5f;
		aGreen = 0.1f;
		aBlue = 1.0f;
		break;
	default:
		aRed = 1.0f;
		aGreen = 0.5f;
		aBlue = 0.1f;
		break;
	}
	mBlitStyle = 1;
	SetAttribute(AttributeRed, aRed, aRed);
	SetAttribute(AttributeGreen, aGreen, aGreen);
	SetAttribute(AttributeBlue, aBlue, aBlue);
	SetAttribute(AttributeXScale, 0.5f, 0.5f);
	SetAttribute(AttributeYScale, 0.5f, 0.5f);
	SetAttribute(AttributeAlpha, 0.3f, 0.3f);
	float aBurstX = (float)RandomFloat(-370.0f, 370.0f);
	SetAttribute(AttributeX, aBurstX, aBurstX);
	SetAttribute(AttributeY, 320.0f, (float)RandomFloat(-270.0f, 0.0f));
	AddEvent(TriggerEnd, ActionCustom, nullptr, nullptr, nullptr, nullptr, nullptr);
}

void Firework::OnCustomEvent()
{
	int aSparkCount = RandomBetween(20, 90);
	UpdateWorldValues();
	for (int anAngle = 0; anAngle < 360; anAngle += 360 / aSparkCount) {
		SceneObject* aSpark = NewChild(nullptr);
		aSpark->SetImageRecord(mImage);
		aSpark->mBlitStyle = 1;
		aSpark->AddStage((float)RandomBetween(500, 1000), 0, 0, 1, nullptr);
		aSpark->SetAttribute(AttributeAlpha, 0.8f, 0.0f);
		aSpark->SetAttribute(AttributeRed, mLocal[AttributeRed], mLocal[AttributeRed]);
		aSpark->SetAttribute(AttributeGreen, mLocal[AttributeGreen], mLocal[AttributeGreen]);
		aSpark->SetAttribute(AttributeBlue, mLocal[AttributeBlue], mLocal[AttributeBlue]);
		aSpark->SetAttribute(AttributeXScale, 1.0f, 1.9f);
		aSpark->SetAttribute(AttributeYScale, 1.0f, 1.9f);
		// the sparks fly out at one speed each, in a ring, slowing as they go, and pulled down a little
		float aSpeed = (float)(RandomFloat(8.0f, 20.0f) / 8.0);
		aSpark->AddFilter(FilterVelocity, AttributeX, (float)(VeniceCos((float)anAngle * DegreesToRadians) * aSpeed), 0.0f, 0.97f);
		aSpark->AddFilter(FilterVelocity, AttributeY, (float)(VeniceSin((float)anAngle * DegreesToRadians) * aSpeed), 0.002f,
			0.97f);
		aSpark->AddFilter(FilterVelocity, AttributeRotation, (float)RandomFloat(-0.57f, 0.57f), 0.0f, 0.0f);
		aSpark->mZOrder = mZOrder;
		gActiveLevel->AdoptNewestChild(this, false);
		aSpark->SetZOrder(6.0f);
	}
	OnFirstUpdate();
}
