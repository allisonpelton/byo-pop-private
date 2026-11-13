#include "Popper.h"

#include <cmath>

#include "Application.h"
#include "Color.h"
#include "Graphics.h"
#include "Image.h"
#include "Particles.h"
#include "Rect.h"
#include "Res.h"

using namespace Sexy;

static const double HALF_PI_FLOAT = 1.5707963705062866;		// pi / 2 as a float, the exe's constant
static const double PI_FLOAT = 3.1415927410125732;			// pi as a float
static const float SIXTEENTH_OF_A_SECOND = 0.06666667f;		// 1 / 15: the explosion shows each cel for 15 steps

// 0x440830. Each type has an emitter and a fragment definition by name; the types that fly the way a hit came from take
// the direction (an arrow in the hit's plane) into their own copy of the emitter's definition.
Popper::Popper(double theX, double theY, int theType, const double* theDirection) :
	Object(0x80),
	mType(theType),
	mEmitter(nullptr),
	mEffect(nullptr),
	mX(theX),
	mY(theY)
{
	const char* anEmitterName = nullptr;
	const char* aFragmentName = nullptr;
	bool aTakesDirection = false;

	switch (theType)
	{
	case POPPER_STARS:
		anEmitterName = "GnomeStars";
		aFragmentName = "GnomeStarFragment";
		aTakesDirection = true;
		break;
	case POPPER_STARS_FAST:
		anEmitterName = "GnomeStarsFast";
		aFragmentName = "GnomeStarFastFragment";
		aTakesDirection = true;
		break;
	case POPPER_SPARKLES:
		anEmitterName = "GnomeSparkles";
		aFragmentName = "GnomeSparkleFragment";
		aTakesDirection = true;
		break;
	case POPPER_STICKER:
		anEmitterName = "GnomeSticker";
		aFragmentName = "GnomeStickerFragment";
		break;
	case POPPER_FREEZE:
		anEmitterName = "GnomeFreeze";
		aFragmentName = "GnomeFreezeFragment";
		break;
	case POPPER_HEARTS:
		anEmitterName = "GnomeHearts";
		aFragmentName = "GnomeHeartFragment";
		aTakesDirection = true;
		break;
	case POPPER_EXPLOSION:
		gApp->PlaySample(SOUND_BOMB_FACE_EXPLOSION);
		return;
	case POPPER_DUST:
		anEmitterName = "GnomeDust";
		aFragmentName = "GnomeDustFragment";
		break;
	case POPPER_DUST_KING:
		anEmitterName = "GnomeDustKing";
		aFragmentName = "GnomeDustKingFragment";
		break;
	default:
		return;
	}

	mEmitter = new EmitterDef(*gApp->mParticleDefs->FindEmitter(anEmitterName));
	if (aTakesDirection)
	{
		ParticleVector aDirection = { theDirection[0], theDirection[1], 0.0 };
		mEmitter->mDirection = NormalizeParticleVector(aDirection);
	}

	const FragmentDef* aFragment = gApp->mParticleDefs->FindFragment(aFragmentName);
	ParticleVector aPos = { theX, theY, 0.0 };
	ParticleVector aVel = { 0.0, 0.0, 0.0 };
	mEffect = new ParticleEffect(*gApp->mParticleRand, mEmitter, aFragment, Color(255, 255, 255, 255), aPos, aVel);
}

// 0x441060
Popper::~Popper()
{
	delete mEmitter;
	delete mEffect;
}

// 0x440d90: the effect goes on a hundredth of a time unit; the explosion ends after its last cel
void Popper::FixedUpdate()
{
	Object::FixedUpdate();
	if (mDead)
		return;

	if (mEffect != nullptr)
	{
		mEffect->Update(*gApp->mParticleRand, 0.01);
		if (mEffect->IsFinished())
			Kill();
	}

	if (mType == POPPER_EXPLOSION)
	{
		if (IMAGE_GNOME_EXPLOSION->mNumCols * IMAGE_GNOME_EXPLOSION->mNumRows < mTickCount / 15)
		{
			Kill();
			return;
		}
	}
}

// 0x440e30. The explosion shows the cels of its picture before and after the current one (each a quarter turn of a sine
// wave, so that one fades in as the next fades out), added to what is behind them.
void Popper::Draw(Graphics* g)
{
	Graphics aGraphics(*g);

	if (mEffect != nullptr)
	{
		mEffect->Draw(&aGraphics, gApp->Is3DAccelerated(), 0, 0);
		return;
	}

	if (mType != POPPER_EXPLOSION)
		return;

	Graphics aExplosionGraphics(*g);
	aExplosionGraphics.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	aExplosionGraphics.SetColorizeImages(true);

	int aCelStep = mTickCount / 15;
	int aStepTick = mTickCount % 15;
	int aCelCount = IMAGE_GNOME_EXPLOSION->mNumRows * IMAGE_GNOME_EXPLOSION->mNumCols;
	if (aCelStep > aCelCount)
		return;

	int aFirst = aCelStep - 1 < 0 ? 0 : aCelStep - 1;
	int aLast = aCelCount - 1 < aCelStep + 1 ? aCelCount - 1 : aCelStep + 1;
	float aFraction = (float)((double)aStepTick * (double)SIXTEENTH_OF_A_SECOND);

	for (int aCel = aFirst; aCel <= aLast; aCel++)
	{
		Rect aSrc = IMAGE_GNOME_EXPLOSION->GetCelRect(aCel);
		double anAngle = ((double)((1 - aCelStep) + aCel) - (double)aFraction) * HALF_PI_FLOAT;
		if (0.0 <= anAngle && anAngle <= PI_FLOAT)
		{
			aExplosionGraphics.SetColor(Color(255, 255, 255, (int)(sin(anAngle) * 255.0)));
			int aY = (int)(mY - (double)(aSrc.mHeight / 2));
			int aX = (int)(mX - (double)(aSrc.mWidth / 2));
			aExplosionGraphics.DrawImage(IMAGE_GNOME_EXPLOSION, aX, aY, aSrc);
		}
	}
}

// 0x441100
Popper* Sexy::MakePopper(double theX, double theY, int theType, const double* theDirection)
{
	Popper* aPopper = new Popper(theX, theY, theType, theDirection);
	aPopper->SetName("");
	return aPopper;
}
