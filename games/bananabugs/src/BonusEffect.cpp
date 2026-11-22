#include "BonusEffect.h"

#include "Anim.h"
#include "Emitter.h"

using namespace Sexy;

// The cel of the particles picture a star shows, and how many degrees apart the stars go
const int EFFECT_CEL = 2;
const int EFFECT_DEGREES_APART = 0x14;
const int EFFECT_DEGREES_ROUND = 0x168;

// The star's track, four keys. It waits where it is, unseen, 16 frames (depth -1: behind); flies out to 64 pixels while
// it grows to 0.4 at half strength (20 frames); swells to 0.3 at 0.7 strength and turns half a turn out to 72 pixels,
// easing in by the square (10 frames); and ends a whole turn on at 73 pixels, shrunk to 0.1 and clear (1 frame)
const int EFFECT_FLY_FRAMES = 0x10;
const int EFFECT_FAR_FRAMES = 0x14;
const int EFFECT_SWELL_FRAMES = 10;
const int EFFECT_LAST_FRAMES = 1;
const float EFFECT_FLY_RADIUS = 64.0f;
const float EFFECT_SWELL_RADIUS = 72.0f;
const float EFFECT_LAST_RADIUS = 73.0f;
const float EFFECT_START_Z = -1.0f;
const float EFFECT_SHOWN_Z = 1.0f;
const float EFFECT_FAR_ALPHA = 0.5f;
const float EFFECT_FAR_SCALE = 0.4f;
const float EFFECT_SWELL_ALPHA = 0.7f;
const float EFFECT_SWELL_SCALE = 0.3f;
const float EFFECT_HALF_TURN = 3.14159274f; // 0x40490fdb
const float EFFECT_LAST_SCALE = 0.1f;
const float EFFECT_WHOLE_TURN = 6.28318548f; // 0x40c90fdb

// 0x40bfe0: the stars are made and the effect is put away
BonusEffect::BonusEffect()
{
	for (int i = 0; i < EFFECT_PICTURES; i++)
		mPictures[i] = nullptr;
	BuildPictures();
	Deactivate(this);
}

// 0x40c040 (EBX the effect): each star starts its track and is shown, and so is the effect
void BonusEffect::Play()
{
	for (int i = 0; i < EFFECT_PICTURES; i++) {
		mPictures[i]->StartTrack(0);
		Activate(mPictures[i], false);
	}
	Activate(this, false);
}

// 0x40c080: the 18 stars, a child each, the first pointing along 0 degrees and each next 20 degrees on (the angle in
// degrees times 3.14159 over 180 in the exe's own double, as the other builders have it)
void BonusEffect::BuildPictures()
{
	int aDegrees = 0;
	int i = 0;
	do {
		ObjBase* aStar = new ObjBase();
		mPictures[i] = aStar;
		aStar->SetParent(this, nullptr);
		Emitter::sParticlesAnim->AddRef();
		aStar->mAnim = Emitter::sParticlesAnim;
		aStar->SetStateInt(STATE_FRAME, EFFECT_CEL);
		aStar->SetStateInt(STATE_X, 0);
		aStar->SetStateInt(STATE_Y, 0);
		aStar->SetStateInt(STATE_SCALE_X, 0);
		aStar->SetStateInt(STATE_SCALE_Y, 0);
		aStar->SetStateInt(STATE_ALPHA, 0);
		aStar->SetStateInt(STATE_Z, 0);
		float anAngle = (float)((double)aDegrees * PI_F / 180.0);
		float aCos = FloatCos(anAngle);
		float aSin = FloatSin(anAngle);

		AnimKey* aStartKey = new AnimKey();
		aStartKey->SetDuration(EFFECT_FLY_FRAMES);
		aStartKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
		aStartKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		aStartKey->AddValue(STATE_Z, EFFECT_START_Z, MOD_TOWARD_NEXT);
		aStartKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
		aStartKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
		aStartKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
		aStartKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);

		AnimKey* aFarKey = new AnimKey();
		aFarKey->SetDuration(EFFECT_FAR_FRAMES);
		aFarKey->AddValue(STATE_X, aCos * EFFECT_FLY_RADIUS, MOD_TOWARD_NEXT);
		aFarKey->AddValue(STATE_Y, aSin * EFFECT_FLY_RADIUS, MOD_TOWARD_NEXT);
		aFarKey->AddValue(STATE_Z, EFFECT_SHOWN_Z, MOD_TOWARD_NEXT);
		aFarKey->AddValue(STATE_ALPHA, EFFECT_FAR_ALPHA, MOD_TOWARD_NEXT);
		aFarKey->AddValue(STATE_SCALE_X, EFFECT_FAR_SCALE, MOD_TOWARD_NEXT);
		aFarKey->AddValue(STATE_SCALE_Y, EFFECT_FAR_SCALE, MOD_TOWARD_NEXT);
		aFarKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);

		AnimKey* aSwellKey = new AnimKey();
		aSwellKey->SetDuration(EFFECT_SWELL_FRAMES);
		aSwellKey->AddValue(STATE_X, aCos * EFFECT_SWELL_RADIUS, MOD_TOWARD_NEXT);
		aSwellKey->AddValue(STATE_Y, aSin * EFFECT_SWELL_RADIUS, MOD_TOWARD_NEXT);
		aSwellKey->AddValue(STATE_Z, EFFECT_SHOWN_Z, MOD_HOLD);
		aSwellKey->AddValue(STATE_ALPHA, EFFECT_SWELL_ALPHA, MOD_TOWARD_NEXT);
		aSwellKey->AddValue(STATE_SCALE_X, EFFECT_SWELL_SCALE, MOD_TOWARD_NEXT);
		aSwellKey->AddValue(STATE_SCALE_Y, EFFECT_SWELL_SCALE, MOD_TOWARD_NEXT);
		aSwellKey->AddValue(STATE_ROTATION, EFFECT_HALF_TURN, MOD_TOWARD_NEXT);
		aSwellKey->mEasing = EASE_IN_SQUARE;

		AnimKey* aLastKey = new AnimKey();
		aLastKey->SetDuration(EFFECT_LAST_FRAMES);
		aLastKey->AddValue(STATE_X, aCos * EFFECT_LAST_RADIUS, MOD_TOWARD_NEXT);
		aLastKey->AddValue(STATE_Y, aSin * EFFECT_LAST_RADIUS, MOD_TOWARD_NEXT);
		aLastKey->AddValue(STATE_Z, EFFECT_SHOWN_Z, MOD_HOLD);
		aLastKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
		aLastKey->AddValue(STATE_SCALE_X, EFFECT_LAST_SCALE, MOD_HOLD);
		aLastKey->AddValue(STATE_SCALE_Y, EFFECT_LAST_SCALE, MOD_HOLD);
		aLastKey->AddValue(STATE_ROTATION, EFFECT_WHOLE_TURN, MOD_HOLD);

		AnimTrack* aTrack = new AnimTrack();
		aTrack->AddKey(aStartKey, nullptr);
		aTrack->AddKey(aFarKey, nullptr);
		aTrack->AddKey(aSwellKey, nullptr);
		aTrack->AddKey(aLastKey, nullptr);
		aStar->AddTrack(aTrack, nullptr);
		aDegrees += EFFECT_DEGREES_APART;
		i++;
	} while (aDegrees < EFFECT_DEGREES_ROUND);
}
