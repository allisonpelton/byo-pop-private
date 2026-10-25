// The bonus molecules (see BonusMolecules.h).

#include "BonusMolecules.h"
#include "AtomicaApp.h"
#include "Graphics.h"

using namespace Sexy;

// 0x414830
BonusMolecules::BonusMolecules(AtomicaApp* theApp) :
	mApp(theApp),
	mI8(0),
	mRound(0),
	mTarget(3),
	mCount(0),
	mAlpha(0),
	mI0cc(0)
{
	mI8 = -mApp->mBonusFrame->mWidth;
	for (int i = 0; i < 25; i++)
		mI18[i] = 0;
}

// 0x415540: port: the shapes are not looked for yet
void BonusMolecules::Check(Grid* theGrid)
{
	(void) theGrid;
}

// 0x4153f0
void BonusMolecules::Upgrade()
{
	mCount++;
	mRound++;
}

void BonusMolecules::Update()
{
}

void BonusMolecules::Draw(Graphics* g)
{
	(void) g;
}

// 0x4146d0
void BonusMolecules::Save(FILE* f)
{
	fwrite(&mI8, 4, 1, f);
	fwrite(&mRound, 4, 1, f);
	fwrite(&mI0cc, 4, 1, f);
	fwrite(&mAlpha, 4, 1, f);
	fwrite(mI18, 100, 1, f);
	unsigned char aHasShape = 0;
	fwrite(&aHasShape, 1, 1, f);
}

// 0x414750
void BonusMolecules::Load(FILE* f)
{
	fread(&mI8, 4, 1, f);
	fread(&mRound, 4, 1, f);
	fread(&mI0cc, 4, 1, f);
	fread(&mAlpha, 4, 1, f);
	fread(mI18, 100, 1, f);
	unsigned char aHasShape = 0;
	fread(&aHasShape, 1, 1, f);
}
