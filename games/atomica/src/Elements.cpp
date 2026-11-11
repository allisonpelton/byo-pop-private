// The bonus atoms of the five elements (see Atom.h): a bonus molecule makes one; a double click on it sets its effect off:
// fire a shock wave that takes the atoms it reaches, water a freeze of the field, earth bolts that run through the field,
// air whirlwinds, ether a time when the atoms are no obstacle for a path.

#include "Atom.h"
#include "AtomicaApp.h"
#include "Atomicus.h"
#include "BonusMolecules.h"
#include "Board.h"
#include "Effects.h"
#include "Grid.h"

using namespace Sexy;

// 0x4043d0: the colour of the element, its level, and the voice that cheers a new one
ElementAtom::ElementAtom(Grid* theGrid, int theType, bool theLoading) :
	Atom(theGrid, theType, true)
{
	mIsElementAtom = true;
	mElement = 0;
	mLevel = 0;
	mElementSavedZero = 0;
	switch (theType)
	{
	case 0xc:
	case 0xd:
	case 0xe:
		mColor = Color(0xff, 0x6e, 0x12, 0xff);
		mElement = 0;
		mLevel = theType - 0xc;
		break;
	case 0xf:
	case 0x10:
	case 0x11:
		mColor = Color(0x17, 0xe9, 0xfc, 0xff);
		mElement = 1;
		mLevel = theType - 0xf;
		break;
	case 0x12:
	case 0x13:
	case 0x14:
		mColor = Color(0xff, 0xac, 0xfe, 0xff);
		mElement = 2;
		mLevel = theType - 0x12;
		break;
	case 0x15:
	case 0x16:
	case 0x17:
		mColor = Color(0xe4, 0xe4, 0xe4, 0xff);
		mElement = 3;
		mLevel = theType - 0x15;
		break;
	case 0x18:
	case 0x19:
	case 0x1a:
		mColor = Color(100, 0xff, 100, 0xff);
		mElement = 4;
		mLevel = theType - 0x18;
		break;
	}
	mPictureFrame = 0;
	static const float kSteps[4] = {0.1f, 0.15f, 0.2f, 0.25f};
	for (int i = 0; i < 5; i++)
	{
		mFlicker[i] = (float) mApp->mRand.Get(3, false);
		mFlickerStep[i] = kSteps[mApp->mRand.Get(4, false)];
	}
	if (theLoading)
		return;
	if (mLevel == 0)
	{
		mApp->mBoard->mAtomicus->Say(3);
		mGrid->mLevelBonusMolecules++;
	}
	else
	{
		mApp->mBoard->mAtomicus->Say(5);
		mGrid->mLevelBonusUpgrades++;
	}
	if (mApp->mMode == 1)
		mApp->mBoard->mMeter.mValue = mApp->mBoard->mMeter.mValue * 0.5f;
}

// 0x404630
ElementAtom::~ElementAtom()
{
}

// 0x404640: the pictures turn; when a placed element atom is ready the field is told it wants to act
void ElementAtom::Update()
{
	Atom::Update();
	if (!mPre)
	{
		mPictureFrame += 0.1f;
		if (8.0f < mPictureFrame)
		{
			mGrid->mElementReady = true;
			mGrid->mReadyElement = (unsigned char) (mElement + 1);
			mGrid->mReadyElementX = mGrid->mX + mX;
			mGrid->mReadyElementY = mGrid->mY + mY;
		}
		if (16.0f <= mPictureFrame)
			mPictureFrame -= 16.0f;
	}
	for (int i = 0; i < 5; i++)
	{
		mFlicker[i] += mFlickerStep[i];
		if (32.0f <= mFlicker[i])
			mFlicker[i] -= 32.0f;
	}
}

// 0x404b00: the picture of the element, and over it the rings of its level
void ElementAtom::Draw(Graphics* g)
{
	g->SetColorizeImages(false);
	if (mPre)
	{
		Atom::Draw(g);
		return;
	}
	mApp->mElemental[mElement][(int) mPictureFrame]->DrawCentered(g, mX, mY);
	g->SetColorizeImages(true);
	g->SetColor(mColor);
	mApp->mBonusLevels[(int) mFlicker[0]]->DrawCentered(g, mX, mY);
	if (0 < mLevel)
	{
		mApp->mBonusLevels[32 + (int) mFlicker[1]]->DrawCentered(g, mX, mY);
		mApp->mBonusLevels[64 + (int) mFlicker[2]]->DrawCentered(g, mX, mY);
	}
	if (1 < mLevel)
	{
		mApp->mBonusLevels[96 + (int) mFlicker[3]]->DrawCentered(g, mX, mY);
		mApp->mBonusLevels[128 + (int) mFlicker[4]]->DrawCentered(g, mX, mY);
	}
	g->SetColorizeImages(true);
}

// 0x4047e0: the atom bursts into sparks and flies away
void ElementAtom::ShatterIn()
{
	for (int i = 0; i < 0x2d; i++)
	{
		for (int aCorner = 0; aCorner < 4; aCorner++)
		{
			Sparkle* aSparkle = new Sparkle(mApp, mCol, mRow, aCorner);
			mGrid->mSparkles.Add(aSparkle);
			aSparkle->mAlpha = 0xff;
			aSparkle->mFadeStep = 1;
		}
	}
	mApp->mSndWhoosh->Play();
	FlyAway(mApp->mBoard->LastTarget());
}

// the banner with the element's word, in the element's colour made brighter, at the atom
static void AddBanner(ElementAtom* theAtom, int theWord)
{
	AtomicaApp* anApp = theAtom->mApp;
	Color aColor(theAtom->mColor.mRed * 3 > 0xfe ? 0xff : theAtom->mColor.mRed * 3,
		theAtom->mColor.mGreen * 3 > 0xfe ? 0xff : theAtom->mColor.mGreen * 3,
		theAtom->mColor.mBlue * 3 > 0xfe ? 0xff : theAtom->mColor.mBlue * 3, 255);
	Flash* aFlash = new Flash(anApp, anApp->mBonusWords[theWord], aColor.mRed, aColor.mGreen, aColor.mBlue, aColor.mAlpha);
	aFlash->mX = (float) (anApp->mBoard->mGrid->mX + theAtom->mX);
	aFlash->mY = (float) (anApp->mBoard->mGrid->mY + theAtom->mY);
	anApp->mBoard->mEffects.Add(aFlash);
}

// 0x404e30: a shock wave that takes the atoms it reaches (as strong as the level)
void ElementAtom::Fire(int theLevel)
{
	Blast* aBlast = new Blast(mApp, 0, true, theLevel);
	aBlast->mX = (float) (mGrid->mX + mX);
	aBlast->mY = (float) (mGrid->mY + mY);
	mApp->mBoard->mEffects.Add(aBlast);
	AddBanner(this, 0);
	ShatterIn();
}

// 0x404fd0: the field is frozen for three moves a level
void ElementAtom::Water(int theLevel)
{
	mApp->mBoard->StartFreeze(theLevel * 3);
	AddBanner(this, 1);
	ShatterIn();
}

// 0x405100: bolts through the field, one for each level
void ElementAtom::Earth(int theLevel)
{
	AddBanner(this, 2);
	for (int i = 0; i < theLevel; i++)
	{
		EarthBolt* aBolt = new EarthBolt(mApp);
		aBolt->mX = (float) (mGrid->mX + mX);
		aBolt->mY = (float) (mGrid->mY + mY);
		mApp->mBoard->mEffects.Add(aBolt);
	}
	ShatterIn();
}

// 0x4052a0: whirlwinds, one for each level
void ElementAtom::Air(int theLevel)
{
	for (int i = 0; i < theLevel; i++)
	{
		int aDX = mApp->mRand.Get(0x28, true);
		int aDY = mApp->mRand.Get(0x28, true);
		mApp->mBoard->mEffects.Add(new Vortex(mApp, aDX + mGrid->mX + mX, aDY + mGrid->mY + mY));
	}
	AddBanner(this, 3);
	ShatterIn();
}

// 0x405480: three moves a level in which the atoms are no obstacle
void ElementAtom::Ether(int theLevel)
{
	mApp->mBoard->StartGhost(theLevel * 3);
	AddBanner(this, 4);
	ShatterIn();
}

// 0x4055b0: a double click sets the element's effect off
void ElementAtom::OnPlaced()
{
	int aVoice = 0;
	switch (mType)
	{
	case 0xc:
	case 0xd:
	case 0xe:
		Fire(mType - 0xb);
		aVoice = 10;
		break;
	case 0xf:
	case 0x10:
	case 0x11:
		Water(mType - 0xe);
		aVoice = 11;
		break;
	case 0x12:
	case 0x13:
	case 0x14:
		Earth(mType - 0x11);
		aVoice = 12;
		break;
	case 0x15:
	case 0x16:
	case 0x17:
		Air(mType - 0x14);
		aVoice = 13;
		break;
	case 0x18:
	case 0x19:
	case 0x1a:
		Ether(mType - 0x17);
		aVoice = 14;
		break;
	default:
		return;
	}
	mApp->mBoard->mAtomicus->Say(aVoice);
	mApp->mBoard->mBonus->Revalidate();
}

// 0x4049d0
void ElementAtom::Save(FILE* f)
{
	Atom::Save(f);
	fwrite(&mPictureFrame, 4, 1, f);
	fwrite(mFlicker, 4, 5, f);
	fwrite(mFlickerStep, 4, 5, f);
	fwrite(&mElement, 4, 1, f);
	fwrite(&mLevel, 4, 1, f);
	fwrite(&mElementSavedZero, 4, 1, f);
}

// 0x404a50
void ElementAtom::Load(FILE* f)
{
	Atom::Load(f);
	fread(&mPictureFrame, 4, 1, f);
	fread(mFlicker, 4, 5, f);
	fread(mFlickerStep, 4, 5, f);
	fread(&mElement, 4, 1, f);
	fread(&mLevel, 4, 1, f);
	fread(&mElementSavedZero, 4, 1, f);
}
