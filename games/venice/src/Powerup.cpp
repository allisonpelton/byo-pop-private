// The power-ups (0x4165b0-0x418810).

#include "Powerup.h"

#include "Cupid.h"
#include "ImageRecord.h"
#include "Level.h"
#include "Profiles.h"
#include "SceneTag.h"
#include "Scoring.h"
#include "Shooter.h"
#include "TextObject.h"
#include "VeniceGlobals.h"
#include "VeniceMath.h"
#include "VeniceSound.h"

#include <cstdio>
#include <cstring>

namespace
{
// the message that comes in over the level when a power-up has been caught (the message goes when the next one is caught)
TextObject* NewEffectText(const char* theText, int theStagger, int theJustification)
{
	TextObject* aText = new TextObject();
	aText->SetText(theText, "gfx\\font1.png", 2.0f, theStagger, theJustification);
	return aText;
}

void ShowEffectText(TextObject* theText)
{
	strcpy(theText->mName, "popup_text");
	gActiveLevel->NewChild(theText);
}

// the effects that go to the shooter need it to be ready to shoot (or rising out of the water)
bool ShooterCanTakeEffect()
{
	return gActiveLevel->mNext == nullptr &&
		(strstr(gShooter->mStages.mCursor->mName, "ready") != nullptr || strstr(gShooter->mStages.mCursor->mName, "rise") != nullptr);
}

// one letter group of the WINGS message: it flies up from below with a swing
void AddWingsText(const char* theLetters, int theJustification, float theX, float theRotationAmplitude)
{
	TextObject* aText = NewEffectText(theLetters, 0, theJustification);
	aText->AddStage(3000.0f, 0, 0, 1, nullptr);
	aText->SetAttribute(AttributeY, 0, 0);
	aText->SetAttribute(AttributeX, 0, 0);
	aText->AddFilter(FilterSuperCos, AttributeAll, 2.0f, 0.0f, 0);
	aText->SetAttribute(AttributeRotation, 0.0f, 0);
	aText->SetAttribute(AttributeY, 30.0f, -330.0f);
	aText->SetAttribute(AttributeX, theX, theX);
	if (theRotationAmplitude != 0.0f)
		aText->AddFilter(FilterSined, AttributeRotation, theRotationAmplitude, 5.0f, 0);
	aText->AddFilter(FilterSined, AttributeY, 10.0f, 5.0f, 0);
	ShowEffectText(aText);
}
}

Powerup::Powerup()
{
	mType = ObjectTypePowerup;
	AddStage(1000.0f, 1, 1, 1, nullptr);
	Profile& aProfile = CurrentProfile();
	// the kinds a mode has no use for are drawn again: no rewind in the Journey mode (and no cupid in its first nine levels), no
	// extra try, multiplier or cupid in the Survival mode, no extra try or cupid in the others
	if (aProfile.mMode == GameModeJourney) {
		do {
			do {
				mKind = RandomBetween(0, 7);
			} while (mKind == PowerupRewind);
		} while (aProfile.mLevelIndex[GameModeJourney] < 9 && mKind == PowerupCupid);
	} else if (aProfile.mMode == GameModeSurvival) {
		do {
			do {
				mKind = RandomBetween(0, 7);
			} while (mKind == PowerupExtraTry);
		} while (mKind == PowerupCupid || mKind == PowerupScoreMultiplier);
	} else {
		do {
			do {
				mKind = RandomBetween(0, 7);
			} while (mKind == PowerupExtraTry);
		} while (mKind == PowerupCupid);
	}
	if (gTutorial != nullptr)
		mKind = PowerupPowerShot;
	SetKind(mKind);
}

void Powerup::Load(const char* theText, SceneTag* theTag)
{
	mStages.Clear();
	snprintf(mTypeName, sizeof(mTypeName), "%s", theTag->GetString("powerup_type"));
	if (strcmp(mTypeName, "back") == 0)
		SetKind(PowerupRewind);
	else if (strcmp(mTypeName, "super_shot") == 0)
		SetKind(PowerupComboShot);
	else if (strcmp(mTypeName, "fast_shot") == 0)
		SetKind(PowerupPowerShot);
	else if (strcmp(mTypeName, "wings") == 0)
		SetKind(PowerupWings);
	else if (strcmp(mTypeName, "wild") == 0)
		SetKind(PowerupWild);
	else if (strcmp(mTypeName, "plus_one") == 0)
		SetKind(PowerupExtraTry);
	else if (strcmp(mTypeName, "cupid") == 0)
		SetKind(PowerupCupid);
	else if (strcmp(mTypeName, "x2") == 0)
		SetKind(PowerupScoreMultiplier);
	SceneObject::Load(theText, theTag);
}

// A rewind has no use in the Journey mode, a wings power-up takes its place
void Powerup::OnObjectClosed()
{
	if (CurrentProfile().mMode == GameModeJourney && mKind == PowerupRewind)
		mKind = PowerupWings;
	SetKind(mKind);
}

void Powerup::SetKind(int theKind)
{
	mKind = theKind;
	switch (mKind) {
	case PowerupWings:
		SetImage("gfx\\pwrup_wings.png", 0, 0, 0, 0);
		strcpy(mName, "-=WINGS=-");
		break;
	case PowerupPowerShot:
		SetImage("gfx\\pwrup_fast_shot.png", 0, 0, 0, 0);
		strcpy(mName, "POWER SHOT");
		break;
	case PowerupRewind:
		SetImage("gfx\\pwrup_back.png", 0, 0, 0, 0);
		strcpy(mName, "REWIND");
		break;
	case PowerupWild:
		SetImage("gfx\\pwrup_wild.png", 0, 0, 0, 0);
		strcpy(mName, "WILD SHOT");
		break;
	case PowerupComboShot:
		SetImage("gfx\\pwrup_supershot.png", 0, 0, 0, 0);
		strcpy(mName, "COMBO SHOT");
		break;
	case PowerupExtraTry:
		SetImage("gfx\\pwrup_+1.png", 0, 0, 0, 0);
		strcpy(mName, "EXTRA TRY");
		break;
	case PowerupScoreMultiplier:
		SetImage("gfx\\pwrup_x2.png", 0, 0, 0, 0);
		strcpy(mName, "SCORE MULTIPLIER");
		break;
	case PowerupCupid:
		SetImage("gfx\\pwrup_cupid.png", 0, 0, 0, 0);
		strcpy(mName, "CUPID");
		break;
	}
}

void Powerup::SpawnFrom(SceneObject* theSource)
{
	if (1 < gPlateLayer->CountChildrenOfType(ObjectTypePlate) && CurrentProfile().mMode != GameModeTrickShot &&
		strstr(gMiddleLayer->mStages.mCursor->mName, "still") == nullptr) {
		Powerup* aPowerup = new Powerup();
		aPowerup->mLocal[AttributeX] = theSource->mLocal[AttributeX];
		aPowerup->mLocal[AttributeY] = theSource->mLocal[AttributeY];
		bool aCupidAround = gActiveLevel->FindChildByType(ObjectTypeCupid, nullptr) != nullptr;
		if (CurrentProfile().mMode == GameModeJourney && gTutorial == nullptr) {
			// the Journey mode draws again for the kinds it cannot use, and for a second cupid
			Powerup* anOther = nullptr;
			while ((anOther = static_cast<Powerup*>(gActiveLevel->FindChildByType(ObjectTypePowerup, anOther))) != nullptr) {
				if (anOther->mKind == PowerupCupid)
					aCupidAround = true;
			}
			do {
				do {
					aPowerup->SetKind(RandomBetween(0, 7));
				} while (aPowerup->mKind == PowerupRewind);
			} while ((aCupidAround && aPowerup->mKind == PowerupCupid) ||
				(CurrentProfile().mLevelIndex[GameModeJourney] < 9 && aPowerup->mKind == PowerupCupid));
		}
		theSource->mParent->NewChild(aPowerup);
		aPowerup->StartFlying();
	}
}

// The power-up leaves the plate it was on: it goes up with a speed that comes from the plate's turn (or, if the plate did not move
// it, a random one), a pull down, and a spin the other way, and the level takes it over from the plate
void Powerup::StartFlying()
{
	mStages.mCursor->mFrame = 0;
	strcpy(mStages.mCursor->mName, "flying");
	SceneFilter* aVelocityX = mStages.mCursor->FindFilter(FilterVelocity, AttributeX);
	float aSpeedX;
	float aSpeedY;
	if (aVelocityX == nullptr) {
		if ((mStages.mCursor->mAttributeMask & 8) != 0)
			mStages.mCursor->mAttributeMask = mStages.mCursor->mAttributeMask ^ 8;
		aSpeedY = -2.05f;
		aSpeedX = (float)RandomFloat(-1.0f, 1.0f);
	} else {
		mStages.mCursor->mAttributeMask = mStages.mCursor->mAttributeMask ^ 8;
		double aSpeedAlongX = VeniceCos(mParent->mLocal[AttributeRotation] * DegreesToRadians) * aVelocityX->mAmplitude;
		aSpeedX = (float)aSpeedAlongX;
		aSpeedY = (float)(-VeniceSin(mParent->mLocal[AttributeRotation] * DegreesToRadians) * aVelocityX->mAmplitude);
	}
	mStages.mCursor->mFilters.Clear();
	mStages.mCursor->AddFilter(FilterVelocity, AttributeX, aSpeedX, 0, 0, false);
	mStages.mCursor->AddFilter(FilterVelocity, AttributeRotation, (0.0f - aSpeedX) * 2.0f, 0, 0, false);
	mStages.mCursor->AddFilter(FilterVelocity, AttributeY, aSpeedY, 0.01f, 0, false);
	mZOrder = mParent->mParent->mZOrder + 5.0f;
	float aRotation = mWorld[AttributeRotation];
	mParent->mChildren.mCursor = this;
	gActiveLevel->AdoptNewestChild(mParent, true);
	mLocal[AttributeRotation] = aRotation;
}

bool Powerup::Update()
{
	Roll();
	bool aKept = Fly();
	bool anAlive = SceneObject::Update();
	return anAlive && aKept;
}

// A power-up that a data file put on a plate rolls to the side as the plate turns, spinning, until it is past the plate's edge
void Powerup::Roll()
{
	if (mStages.mCursor != nullptr && strstr(mStages.mCursor->mName, "roll") != nullptr && mParent->mType == ObjectTypePlate) {
		float aPlateRotation = mParent->mLocal[AttributeRotation];
		SceneFilter* aVelocity = mStages.mCursor->FindFilter(FilterVelocity, AttributeX);
		if (aVelocity == nullptr)
			aVelocity = mStages.mCursor->AddFilter(FilterVelocity, AttributeX, 0, 0, 0, false);
		aVelocity->mAmplitude = aVelocity->mAmplitude - aPlateRotation / 4800.0f;
		aVelocity->mOffset = 0.997f;
		mLocal[AttributeRotation] = (0.0f - mLocal[AttributeX]) * 2.2f;
		if (mParent->mImage->mFrameWidth / 2.0f < VeniceAbs(mLocal[AttributeX]))
			StartFlying();
	}
}

bool Powerup::Fly()
{
	if (mStages.mCursor == nullptr || strstr(mStages.mCursor->mName, "fly") == nullptr)
		return true;
	float aHalfWidth = mImage->mFrameWidth / 2.0f;
	if (aHalfWidth <= mWorld[AttributeX] || mStages.mCursor->FindFilter(FilterVelocity, AttributeX) == nullptr) {
		if (mWorld[AttributeX] <= gScreenWidth - mImage->mFrameWidth / 2.0f ||
			mStages.mCursor->FindFilter(FilterVelocity, AttributeX) == nullptr) {
			if (gScreenHeight < mWorld[AttributeY])
				return false;
		} else {
			// the right edge sends it back to the left
			double aSpeed = VeniceAbs(mStages.mCursor->FindFilter(FilterVelocity, AttributeX)->mAmplitude);
			mStages.mCursor->FindFilter(FilterVelocity, AttributeX)->mAmplitude = (float)-aSpeed;
		}
	} else {
		// the left edge sends it back to the right
		double aSpeed = VeniceAbs(mStages.mCursor->FindFilter(FilterVelocity, AttributeX)->mAmplitude);
		mStages.mCursor->FindFilter(FilterVelocity, AttributeX)->mAmplitude = (float)aSpeed;
	}
	if (gShooter != nullptr && gShooter->HitTest(mWorld[AttributeX], mWorld[AttributeY], 1.0f, 1.0f, 0.0f)) {
		Catch();
		MakeSparkles(20, 1.0f, 1.0f, 1.0f, 1.0f);
		return false;
	}
	return true;
}

// The shooter catches the power-up. The level's catch count (which nothing reads) is not kept.
void Powerup::Catch()
{
	PlaySoundByName("audio\\powerup_caught.ogg", -1);
	if (strstr(gMiddleLayer->mStages.mCursor->mName, "victory") == nullptr) {
		gActiveLevel->KillPopupTexts();
		switch (mKind) {
		case PowerupWings:
			GiveWings();
			break;
		case PowerupPowerShot:
			GivePowerShots();
			break;
		case PowerupRewind:
			GiveRewind();
			break;
		case PowerupWild:
			GiveWild();
			break;
		case PowerupComboShot:
			GiveComboShot();
			break;
		case PowerupExtraTry:
			GiveExtraTry();
			break;
		case PowerupScoreMultiplier:
			GiveScoreMultiplier();
			break;
		case PowerupCupid:
			GiveCupid();
			break;
		}
	} else {
		ShowCatchScore();
	}
}

// A power-up caught after the level was won only scores
void Powerup::ShowCatchScore()
{
	TextObject* aText = new TextObject();
	aText->SetText(FormatScore(500), "gfx\\font1_small.png", -4.0f, 0, JustifyCenter);
	float aX = mWorld[AttributeX] - gActiveLevel->mWorld[AttributeX];
	float aY = mWorld[AttributeY] - gActiveLevel->mWorld[AttributeY];
	aText->AddStage(200.0f, 0, 0, 1, nullptr);
	aText->SetAttribute(AttributeXScale, 0.12f, 1.5f);
	aText->SetAttribute(AttributeYScale, 1.5f, 1.5f);
	aText->SetAttribute(AttributeX, aX, aX);
	aText->SetAttribute(AttributeY, aY - 40.0f, aY - 40.0f);
	aText->SetAttribute(AttributeAlpha, 0, 1.0f);
	aText->AddFilter(FilterSquareRoot, AttributeAll, 0, 0, 0);
	aText->AddStage(50.0f, 0, 0, 1, nullptr);
	aText->SetAttribute(AttributeXScale, 1.5f, 1.2f);
	aText->SetAttribute(AttributeYScale, 1.5f, 1.2f);
	aText->SetAttribute(AttributeX, aX, aX);
	aText->SetAttribute(AttributeY, aY - 40.0f, aY - 40.0f);
	aText->AddStage(1000.0f, 0, 0, 1, nullptr);
	aText->SetAttribute(AttributeXScale, 1.2f, 1.2f);
	aText->SetAttribute(AttributeYScale, 1.2f, 1.2f);
	aText->SetAttribute(AttributeX, aX, aX);
	aText->SetAttribute(AttributeY, aY - 40.0f, (aY - 40.0f) - 20.0f);
	aText->SetAttribute(AttributeAlpha, 1.0f, 0);
	aText->AddFilter(FilterSquared, AttributeAll, 0, 0, 0);
	gActiveLevel->NewChild(aText);
}

// The level scrolls back down for a moment (the plates go back up the screen)
void Powerup::GiveRewind()
{
	if (gActiveLevel->mNext == nullptr) {
		SceneFilter* aPush = gMiddleLayer->mStages.mCursor->AddFilter(FilterVelocity, AttributeY, -2.0f, 0, 0.993f, false);
		aPush->mDuration = 450.0f;
		TextObject* aText = NewEffectText(mName, 80, JustifyCenter);
		aText->AddStage(1000.0f, 0, 0, 1, nullptr);
		aText->SetAttribute(AttributeX, 300.0f, 0);
		aText->SetAttribute(AttributeY, -20.0f, -20.0f);
		aText->SetAttribute(AttributeAlpha, 0, 1.0f);
		aText->SetAttribute(AttributeRotation, -360.0f, 0);
		aText->AddFilter(FilterSuperCos, AttributeAll, 2.0f, 0, 0);
		aText->AddStage(1000.0f, 0, 0, 1, nullptr);
		aText->AddStage(1000.0f, 0, 0, 1, nullptr);
		aText->SetAttribute(AttributeX, 0, -300.0f);
		aText->SetAttribute(AttributeY, -20.0f, -20.0f);
		aText->SetAttribute(AttributeAlpha, 1.0f, 0);
		aText->SetAttribute(AttributeRotation, -360.0f, 0);
		aText->AddFilter(FilterSuperCos, AttributeAll, 2.0f, 0, 0);
		aText->SetAttribute(AttributeAlpha, 1.0f, 0);
		ShowEffectText(aText);
	}
}

// Six power shots more (a message is shown unless the tutorial has its own)
void Powerup::GivePowerShots()
{
	if (gActiveLevel->mNext == nullptr) {
		if (gShooter != nullptr) {
			int aShots = gShooter->mPowerShots < 0 ? 0 : gShooter->mPowerShots;
			gShooter->mPowerShots = aShots + 6;
		}
		if (gActiveLevel->FindChildByType(ObjectTypeTutorial, nullptr) == nullptr) {
			TextObject* aText = NewEffectText(mName, 0, JustifyCenter);
			aText->AddStage(200.0f, 0, 0, 1, nullptr);
			aText->SetAttribute(AttributeX, 300.0f, 0);
			aText->SetAttribute(AttributeY, -20.0f, -20.0f);
			aText->SetAttribute(AttributeAlpha, 0, 1.0f);
			aText->AddFilter(FilterSuperCos, AttributeAll, 2.0f, 0, 0);
			aText->AddStage(2000.0f, 0, 0, 1, nullptr);
			aText->AddStage(200.0f, 0, 0, 1, nullptr);
			aText->SetAttribute(AttributeX, 0, -300.0f);
			aText->SetAttribute(AttributeY, -20.0f, -20.0f);
			aText->SetAttribute(AttributeAlpha, 1.0f, 0);
			aText->AddFilter(FilterSuperCos, AttributeAll, 2.0f, 0, 0);
			aText->SetAttribute(AttributeAlpha, 1.0f, 0);
			ShowEffectText(aText);
		}
	}
}

// The wings: the message is made of three parts that fly up one on each side of the letters in the middle
void Powerup::GiveWings()
{
	if (ShooterCanTakeEffect()) {
		gShooter->ActivateWings();
		AddWingsText("-=WI", JustifyRight, -20.0f, 30.0f);
		AddWingsText("GS=-", JustifyLeft, 20.0f, -30.0f);
		AddWingsText("N", JustifyCenter, 0.0f, 0.0f);
	}
}

// Two wild jewels in the row
void Powerup::GiveWild()
{
	if (ShooterCanTakeEffect()) {
		gShooter->mWildShots = gShooter->mWildShots + 2;
		gShooter->ActivateWild();
		TextObject* aText = NewEffectText(mName, 40, JustifyCenter);
		aText->AddStage(200.0f, 0, 0, 1, nullptr);
		aText->SetAttribute(AttributeX, 0, 0);
		aText->SetAttribute(AttributeY, -300.0f, -20.0f);
		aText->SetAttribute(AttributeAlpha, 0, 1.0f);
		aText->AddFilter(FilterSined, AttributeY, 60.0f, 2.0f, 0);
		aText->AddFilter(FilterSined, AttributeX, 60.0f, 2.0f, 0);
		aText->AddFilter(FilterSuperCos, AttributeAll, 1.0f, 0, 0);
		aText->AddStage(1000.0f, 0, 0, 1, nullptr);
		aText->AddStage(1000.0f, 0, 0, 1, nullptr);
		aText->SetAttribute(AttributeX, 0, 0);
		aText->SetAttribute(AttributeY, -20.0f, 300.0f);
		aText->SetAttribute(AttributeAlpha, 1.0f, 0);
		aText->AddFilter(FilterSuperCos, AttributeAll, 1.0f, 0, 0);
		aText->AddFilter(FilterSined, AttributeY, 60.0f, 2.0f, 0);
		aText->AddFilter(FilterSined, AttributeX, -60.0f, 2.0f, 0);
		aText->SetAttribute(AttributeAlpha, 1.0f, 0);
		ShowEffectText(aText);
	}
}

// The next shot is a combo shot
void Powerup::GiveComboShot()
{
	if (ShooterCanTakeEffect()) {
		gShooter->mComboShot = 1;
		TextObject* aText = NewEffectText(mName, 120, JustifyCenter);
		aText->AddStage(100.0f, 0, 0, 1, nullptr);
		aText->SetAttribute(AttributeX, 0, 0);
		aText->SetAttribute(AttributeY, -300.0f, -20.0f);
		aText->SetAttribute(AttributeAlpha, 0, 1.0f);
		aText->AddStage(2000.0f, 0, 0, 1, nullptr);
		aText->AddStage(100.0f, 0, 0, 1, nullptr);
		aText->SetAttribute(AttributeX, 0, 0);
		aText->SetAttribute(AttributeY, -20.0f, -300.0f);
		aText->SetAttribute(AttributeAlpha, 1.0f, 0);
		aText->SetAttribute(AttributeAlpha, 1.0f, 0);
		ShowEffectText(aText);
	}
}

// One more try
void Powerup::GiveExtraTry()
{
	if (gActiveLevel->mNext == nullptr) {
		CurrentProfile().mTriesLeft[CurrentProfile().mMode]++;
		TextObject* aText = NewEffectText(mName, 40, JustifyCenter);
		aText->AddStage(200.0f, 0, 0, 1, nullptr);
		aText->SetAttribute(AttributeXScale, 1.0f, 1.0f);
		aText->SetAttribute(AttributeYScale, 1.0f, 1.0f);
		aText->SetAttribute(AttributeX, 0, 0);
		aText->SetAttribute(AttributeY, -20.0f, -20.0f);
		aText->SetAttribute(AttributeAlpha, 0, 1.0f);
		aText->AddFilter(FilterSuperCos, AttributeAll, 1.0f, 0, 0);
		aText->AddStage(1000.0f, 0, 0, 1, nullptr);
		aText->SetAttribute(AttributeXScale, 1.0f, 1.0f);
		aText->SetAttribute(AttributeYScale, 1.0f, 1.0f);
		aText->SetAttribute(AttributeX, 0, 0);
		aText->SetAttribute(AttributeY, -20.0f, -20.0f);
		aText->AddFilter(FilterSined, AttributeXScale, 0.4f, 1.0f, 0);
		aText->AddFilter(FilterSuperCos, AttributeAll, 1.0f, 0, 0);
		aText->AddStage(1000.0f, 0, 0, 1, nullptr);
		aText->SetAttribute(AttributeXScale, 1.0f, 1.0f);
		aText->SetAttribute(AttributeYScale, 1.0f, 1.0f);
		aText->SetAttribute(AttributeX, 0, 0);
		aText->SetAttribute(AttributeY, -20.0f, -20.0f);
		aText->SetAttribute(AttributeAlpha, 1.0f, 0);
		aText->AddFilter(FilterSined, AttributeXScale, 1.4f, 1.0f, 0);
		aText->SetAttribute(AttributeAlpha, 1.0f, 0);
		ShowEffectText(aText);
	}
}

// The cupid comes in next to the shooter
void Powerup::GiveCupid()
{
	if (gActiveLevel->mNext == nullptr) {
		Cupid* aCupid = new Cupid();
		gShooter->mParent->NewChild(aCupid);
	}
}

// The score of everything the shooter does from now on doubles (a doubling of a doubling and so on)
void Powerup::GiveScoreMultiplier()
{
	if (ShooterCanTakeEffect()) {
		gShooter->mScoreMultiplier = gShooter->mScoreMultiplier << 1;
		TextObject* aText = NewEffectText(mName, 30, JustifyCenter);
		aText->AddStage(100.0f, 0, 0, 1, nullptr);
		aText->SetAttribute(AttributeX, 0, 0);
		aText->SetAttribute(AttributeY, -20.0f, -20.0f);
		aText->SetAttribute(AttributeXScale, 1.0f, 1.0f);
		aText->SetAttribute(AttributeYScale, 1.0f, 1.0f);
		aText->SetAttribute(AttributeAlpha, 0, 1.0f);
		aText->AddStage(1400.0f, 0, 0, 1, nullptr);
		aText->SetAttribute(AttributeX, 0, 0);
		aText->SetAttribute(AttributeY, -20.0f, -20.0f);
		aText->SetAttribute(AttributeXScale, 1.0f, 1.0f);
		aText->SetAttribute(AttributeYScale, 1.0f, 1.0f);
		aText->AddStage(100.0f, 0, 0, 1, nullptr);
		aText->SetAttribute(AttributeX, 0, 0);
		aText->SetAttribute(AttributeY, -20.0f, -20.0f);
		aText->SetAttribute(AttributeXScale, 1.0f, 1.0f);
		aText->SetAttribute(AttributeYScale, 1.0f, 1.0f);
		aText->AddFilter(FilterSined, AttributeXScale, 0.6f, 0.5f, 0);
		aText->AddFilter(FilterSined, AttributeYScale, 0.6f, 0.5f, 0);
		aText->AddStage(400.0f, 0, 0, 1, nullptr);
		aText->SetAttribute(AttributeX, 0, 0);
		aText->SetAttribute(AttributeY, -20.0f, -20.0f);
		aText->SetAttribute(AttributeXScale, 1.0f, 1.0f);
		aText->SetAttribute(AttributeYScale, 1.0f, 1.0f);
		aText->AddStage(100.0f, 0, 0, 1, nullptr);
		aText->SetAttribute(AttributeXScale, 1.0f, 1.0f);
		aText->SetAttribute(AttributeYScale, 1.0f, 1.0f);
		aText->SetAttribute(AttributeX, 0, 0);
		aText->SetAttribute(AttributeY, -20.0f, -20.0f);
		aText->SetAttribute(AttributeAlpha, 1.0f, 0);
		ShowEffectText(aText);
	}
}
