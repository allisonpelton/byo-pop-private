// Effect.cpp: Effect, Sparkle, FlyingPieceList, EffectMgr, Ember, EmberListMgr (0x41e880-0x421860).
// Behaviour spec: re/bookworm/analysis/board_draw.md section 3 (records: section 6).

#include "Effect.h"
#include "BookWormApp.h"
#include "Board.h"
#include "Piece.h"
#include "Lex.h"
#include "Res.h"
#include "Compat2004.h"
#include "Common.h"
#include "Debug.h"
#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "MTRand.h"
#include "Rect.h"
#include <cstdlib>
#include <string>

using namespace Sexy;

// The 2004 MTRand::SRand(const std::string&): the 624 state words when the string has exactly that size
// (the generator's index is kept, as the port's SRand does), else SRand(4357), whose 1.2 seeding the
// port's SRand(4357) would not reproduce.
static void RestoreMTRand2004(MTRand* theRand, const std::string& theState)
{
	if (theState.size() == MTRAND_N * 4)
		theRand->SRand(theState);
	else
		SeedMTRand2004(*theRand, 4357);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Effect
//////////////////////////////////////////////////////////////////////////////////////////////////////////

// 0x41e880
Effect::Effect(EffectMgr* theEffectMgr)
{
	if (theEffectMgr == NULL)
		OutputDebug("warning: (theEffectMgr == NULL)!\n"); // port: OutputDebugStringA -> the port's debug log
	mEffectMgr = theEffectMgr;
	mUpdateCnt = 0;
	mDead = false;
}

// 0x41f270 (body), 0x41f280 (scalar deleting destructor, slot 7)
Effect::~Effect()
{
}

// 0x41e950 slot 0
// Deletes the effect passed in, not this; EffectMgr::Update calls it as e->DeleteEffect(e).
void Effect::DeleteEffect(Effect* theEffect)
{
	if (theEffect != NULL)
		delete theEffect;
}

// 0x41e8c0 slot 1
void Effect::Update()
{
	mUpdateCnt++;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sparkle
//////////////////////////////////////////////////////////////////////////////////////////////////////////

// 0x41f2b0
Sparkle::Sparkle(EffectMgr* theEffectMgr) :
	Effect(theEffectMgr)
{
	mX = 0.0;
	mY = 0.0;
	mVX = 0.0;
	mVY = 0.0;
	mGravity = 0.0;
	mDelay = 0;
	mFrame = 0;
	mImage = IMAGE_SPARKLE_LARGE;
}

// 0x41f8a0 slot 7 (no body of its own: the Effect destructor)
Sparkle::~Sparkle()
{
}

// 0x41e970 slot 1
// After the delay: moves, and every 8 updates steps the 14-cel animation; dead when it wraps to cel 0.
// The animation step marks the Board dirty (the overlay's sparkles too).
void Sparkle::Update()
{
	Effect::Update();

	if (mDelay > 0)
	{
		mDelay--;
		return;
	}

	mX += mVX;
	mVY += mGravity;
	mY += mVY;

	if (mUpdateCnt % 8 == 0)
	{
		mFrame = (mFrame + 1) % 14;
		if (mFrame % 14 == 0)
			mDead = true;
		mEffectMgr->mApp->mBoard->MarkDirty();
	}
}

// 0x41e9e0 slot 2
// One square cel (the image height) centred in the 57x57 box, additive at alpha 200. Also drawn during
// the delay (cel 0 at the start position).
void Sparkle::Draw(Graphics* g)
{
	Image* anImage = mEffectMgr->mApp->mImages[mImage];
	int aSize = anImage->GetHeight();
	int anOffset = (57 - aSize) / 2;
	int aX = (int)mX + anOffset;
	int aY = (int)mY + anOffset;

	Graphics aSparkleG(*g);
	aSparkleG.ClipRect(aX, aY, aSize, aSize);
	aSparkleG.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	aSparkleG.SetColor(Color(255, 255, 255, 200));
	aSparkleG.SetColorizeImages(true);
	aSparkleG.DrawImage(anImage, aX - mFrame * aSize, aY);
	aSparkleG.SetColorizeImages(false);
	aSparkleG.SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// FlyingPieceList
//////////////////////////////////////////////////////////////////////////////////////////////////////////

// 0x420920
// Takes the non-null pieces of thePieces (Board::Submit passes its selection); NULL from EffectMgr::Load.
FlyingPieceList::FlyingPieceList(EffectMgr* theEffectMgr, std::list<Piece*>* thePieces) :
	Effect(theEffectMgr)
{
	mLastChompCnt = mUpdateCnt;
	if (thePieces != NULL)
	{
		for (std::list<Piece*>::iterator anItr = thePieces->begin(); anItr != thePieces->end(); ++anItr)
		{
			if (*anItr != NULL)
				mPieces.push_back(*anItr);
		}
	}
}

// 0x4200f0 (body), 0x4202a0 (scalar deleting destructor, slot 7)
// Deletes the pieces still in flight.
FlyingPieceList::~FlyingPieceList()
{
	for (std::list<Piece*>::iterator anItr = mPieces.begin(); anItr != mPieces.end(); ++anItr)
	{
		if (*anItr != NULL)
			delete *anItr;
	}
	mPieces.clear();
}

// 0x421150 slot 1
// Each piece shrinks and moves towards the piece ahead of it in the list; the first one towards Lex's
// mouth (which opens when it is close). A step is trunc(distance) times the piece's speed factor, which
// grows by 0.003 per moving step. A piece within 2 px of its target is eaten: removed now, deleted at the
// end of the update (the next piece targets it this tick, or Lex if it was the first); a fire tile makes
// Lex burp later. Pieces in flight throw sparks (fire tiles, every 3rd update) or sparkles (gem tiles,
// every 6th) from the EffectMgr's MTRand.
void FlyingPieceList::Update()
{
	Lex* aLex = mEffectMgr->mApp->mBoard->mLex;
	Effect::Update();

	if (mPieces.size() == 0)
	{
		mDead = true;
		aLex->PlayAnim(LEXANIM_EAT_END, false);
		return;
	}

	std::list<Piece*> anEatenList;

	Piece* aFirstPiece = mPieces.front();
	if ((aFirstPiece != NULL) && (aFirstPiece->mX < 233.0) && (!aLex->IsBusy() || (aLex->mAnim == LEXANIM_SWEAT)))
		aLex->PlayAnim(LEXANIM_EAT, false);

	Piece* aPrevPiece = NULL;
	std::list<Piece*>::iterator anItr = mPieces.begin();
	while (anItr != mPieces.end())
	{
		// The first piece (begin() is re-read: after an eaten first piece the next one targets Lex in the
		// same tick) flies to Lex's mouth, every other one to the piece before it, even one eaten this tick.
		int aTargetX;
		int aTargetY;
		if (anItr == mPieces.begin())
		{
			Lex* aTargetLex = mEffectMgr->mApp->mBoard->mLex;
			aTargetX = aTargetLex->mX + 0x22;
			aTargetY = aTargetLex->mY + 0x2c;
		}
		else
		{
			aTargetX = (int)aPrevPiece->mX;
			aTargetY = (int)aPrevPiece->mY;
		}

		Piece* aPiece = *anItr;
		if (aPiece == NULL)
		{
			++anItr;
			aPrevPiece = aPiece;
			continue;
		}

		Board* aBoard = mEffectMgr->mApp->mBoard;

		// The target is offset by the shrinking tile's size (k is negative: target + trunc(57 * scale)).
		aPiece->mScale *= 0.98;
		int aSizeOffset = (int)(aPiece->mScale * -57.0);
		aTargetX -= aSizeOffset;
		aTargetY -= aSizeOffset;

		double aTargetXD = (double)aTargetX;
		if (aPiece->mX != aTargetXD)
		{
			int aStep = (int)(aTargetXD - aPiece->mX);
			aPiece->mX = (double)aStep * aPiece->mFlySpeed + aPiece->mX;
			aPiece->mFlySpeed += 0.003;
			aBoard->mPiecesFlying = true;
			aBoard->MarkDirty();
		}

		// Quirk (NAMES.md D4): the y step also uses mFlySpeed; the 0.003 goes to mVY, which is otherwise
		// unused in flight.
		double aTargetYD = (double)aTargetY;
		if (aPiece->mY != aTargetYD)
		{
			int aStep = (int)(aTargetYD - aPiece->mY);
			aPiece->mY = (double)aStep * aPiece->mFlySpeed + aPiece->mY;
			aPiece->mVY += 0.003;
			aBoard->mPiecesFlying = true;
			aBoard->MarkDirty();
		}

		if ((abs((int)(aPiece->mY - aTargetYD)) < 2) && (abs((int)(aPiece->mX - aTargetXD)) < 2))
		{
			if (aPiece->mType == TILE_FIRE)
				aBoard->mBurpDelay = 50;
			aPiece->mFlySpeed = 0.0;
			aPiece->mVY = 0.0;
			anEatenList.push_back(aPiece);
			anItr = mPieces.erase(anItr);

			if (mUpdateCnt - mLastChompCnt >= 4)
			{
				mEffectMgr->mApp->PlaySample(SOUND_CHOMP);
				mLastChompCnt = mUpdateCnt;
			}
		}
		else
		{
			if (aPiece->mType == TILE_FIRE)
			{
				if (mUpdateCnt % 3 == 0)
				{
					// A spark from a random point of the tile; random draws in this order: x, y, vx, vy, decay.
					Ember* anEmber = aBoard->mEmberMgr->NewEmber();
					anEmber->mType = EMBER_SPARK_SM;
					anEmber->mX = (double)(mEffectMgr->mRand->Next() % 57) + aPiece->mX;
					anEmber->mY = (double)(mEffectMgr->mRand->Next() % 57) + aPiece->mY;
					anEmber->mVX = ((double)(mEffectMgr->mRand->Next() % 1000) - 500.0) * 0.00125;
					unsigned long aVYRand = mEffectMgr->mRand->Next() % 1000;
					anEmber->mAlpha = 0.95;
					anEmber->mVY = ((double)aVYRand - 1000.0) * 0.0006666666666666666;
					anEmber->mAlphaDecay = (double)(mEffectMgr->mRand->Next() % 1000) * 0.001 * 0.01 + 0.006;
				}
			}
			else if ((aPiece->mType > TILE_FIRE) && (aPiece->mType < 6) && (mUpdateCnt % 6 == 0))
			{
				// Sapphire and diamond: a sparkle centred on a random point of the tile (random x, then y).
				Sparkle* aSparkle = new Sparkle(mEffectMgr);
				aSparkle->mX = ((double)(mEffectMgr->mRand->Next() % 57) + aPiece->mX) - 28.0;
				aSparkle->mY = ((double)(mEffectMgr->mRand->Next() % 57) + aPiece->mY) - 28.0;
				mEffectMgr->AddEffect(aSparkle);
			}
			++anItr;
		}

		aPrevPiece = aPiece;
	}

	for (std::list<Piece*>::iterator anEatenItr = anEatenList.begin(); anEatenItr != anEatenList.end(); ++anEatenItr)
	{
		if (*anEatenItr != NULL)
			delete *anEatenItr;
	}
}

// 0x41fba0 slot 2
// The flying tiles stretched to their shrinking size (fast stretch) in their lit colours. Gem tiles are
// drawn from the app's copies of the gem images with the letter glyph stretched over them in black.
void FlyingPieceList::Draw(Graphics* g)
{
	BookWormApp* anApp = mEffectMgr->mApp;

	for (std::list<Piece*>::iterator anItr = mPieces.begin(); anItr != mPieces.end(); ++anItr)
	{
		Piece* aPiece = *anItr;
		if (aPiece == NULL)
			continue;

		int aSize = (int)(aPiece->mScale * 57.0);
		Rect aDestRect((int)aPiece->mX, (int)aPiece->mY, aSize, aSize);

		if ((aPiece->mType == TILE_SAPPHIRE) || (aPiece->mType == TILE_DIAMOND))
		{
			MemoryImage* aGemImage = (aPiece->mType == TILE_SAPPHIRE) ? anApp->mTileSapphireImage :
				anApp->mTileDiamondImage;
			g->SetFastStretch(true);
			g->DrawImage(aGemImage, aDestRect, Rect(0, 0, 57, 57));

			Image* aLettersImage = anApp->mImages[IMAGE_FONT_TILE_LETTERS];
			int aLetterHeight = aLettersImage->GetHeight() / 26;
			g->SetColor(Color(0, 0, 0, 255));
			g->SetColorizeImages(true);
			g->DrawImage(aLettersImage, aDestRect,
				Rect(0, (aPiece->mLetter - 'A') * aLetterHeight, aLettersImage->GetWidth(), aLetterHeight));
			g->SetColorizeImages(false);
		}
		else
		{
			int aTileSet;
			if (aPiece->mType == TILE_FIRE)
				aTileSet = TILESET_FIRE_HILITE;
			else if (aPiece->mType == TILE_GREEN)
				aTileSet = TILESET_GREEN_HILITE;
			else if (aPiece->mType == TILE_GOLD)
				aTileSet = TILESET_GOLD_HILITE;
			else
				aTileSet = TILESET_NORMAL_HILITE;

			g->SetFastStretch(true);
			g->DrawImage(anApp->mTileSetImages[aTileSet], aDestRect, Rect((aPiece->mLetter - 'A') * 57, 0, 57, 57));
		}
		g->SetFastStretch(false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// EffectMgr
//////////////////////////////////////////////////////////////////////////////////////////////////////////

// 0x4201c0
// One Sexy::Rand() draw seeds the manager's own generator (new MTRand(Rand()), 1.2 seeding).
EffectMgr::EffectMgr(BookWormApp* theApp)
{
	mApp = theApp;
	unsigned long aSeed = (unsigned long)Rand();
	mRand = new MTRand();
	SeedMTRand2004(*mRand, aSeed);
	mEffects.clear();
	mUpdateCnt = 0;
}

// 0x420060 (body), 0x420270 (scalar deleting destructor, slot 0)
EffectMgr::~EffectMgr()
{
	if (mRand != NULL)
		delete mRand;
	Clear();
}

// 0x41f930
void EffectMgr::Clear()
{
	std::list<Effect*>::iterator anItr = mEffects.begin();
	while (anItr != mEffects.end())
	{
		if (*anItr != NULL)
			delete *anItr;
		anItr = mEffects.erase(anItr);
	}
}

// 0x41f990
// Updates every effect in order (effects added meanwhile are updated in the same pass) and deletes the
// dead ones. Returns whether there were effects before the update.
bool EffectMgr::Update()
{
	bool hadEffects = (mEffects.size() != 0);

	std::list<Effect*>::iterator anItr = mEffects.begin();
	while (anItr != mEffects.end())
	{
		Effect* anEffect = *anItr;
		anEffect->Update();
		if (anEffect->IsDead())
		{
			anEffect->DeleteEffect(anEffect);
			anItr = mEffects.erase(anItr);
		}
		else
			++anItr;
	}

	mUpdateCnt++;
	return hadEffects;
}

// 0x41fa10
void EffectMgr::Draw(Graphics* g)
{
	for (std::list<Effect*>::iterator anItr = mEffects.begin(); anItr != mEffects.end(); ++anItr)
		(*anItr)->Draw(g);
}

// 0x4207f0
void EffectMgr::AddEffect(Effect* theEffect)
{
	mEffects.push_back(theEffect);
}

// 0x41fa50
bool EffectMgr::HasEffectOfType(int theType)
{
	if (mEffects.size() != 0)
	{
		for (std::list<Effect*>::iterator anItr = mEffects.begin(); anItr != mEffects.end(); ++anItr)
		{
			if ((*anItr != NULL) && ((*anItr)->GetType() == theType))
				return true;
		}
	}
	return false;
}

// 0x420830
// A new list of the effects of that type (possibly empty), or NULL when the manager has no effects at
// all. The caller deletes the list.
std::list<Effect*>* EffectMgr::GetEffectsOfType(int theType)
{
	if (mEffects.size() == 0)
		return NULL;

	std::list<Effect*>* aList = new std::list<Effect*>();
	for (std::list<Effect*>::iterator anItr = mEffects.begin(); anItr != mEffects.end(); ++anItr)
	{
		Effect* anEffect = *anItr;
		if ((anEffect != NULL) && (anEffect->GetType() == theType))
			aList->push_back(anEffect);
	}
	return aList;
}

// 0x41faa0
// The RNG state (MTRand::Serialize: length, then the 624 words without the index), then the effect count.
// The original writes a presence byte and a record for each effect after the count; Board::SaveGame has
// cleared every effect by then, so the count is 0 and no record follows (port: the effects have no save code).
void EffectMgr::Save(FILE* theFile)
{
	std::string aState = mRand->Serialize();
	unsigned int aLength = (unsigned int)aState.size();
	fwrite(&aLength, 1, 4, theFile);
	fwrite(aState.data(), 1, aLength, theFile);

	unsigned int aCount = 0;
	fwrite(&aCount, 1, 4, theFile);
}

// 0x420b70
// Restores the RNG words (the generator's index is kept) and reads the effect count. A file the game wrote has
// 0 there and no records (see Save); the original reads a record for each effect, the port has no code to
// read one. Callers Clear() first.
void EffectMgr::Load(FILE* theFile)
{
	unsigned int aLength = 0;
	fread(&aLength, 1, 4, theFile);
	std::string aState(aLength, '\0');
	if (aLength != 0)
		fread(&aState[0], 1, aLength, theFile);
	RestoreMTRand2004(mRand, aState);

	unsigned int aCount = 0;
	fread(&aCount, 1, 4, theFile);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Ember
//////////////////////////////////////////////////////////////////////////////////////////////////////////

// 0x41f300
Ember::Ember(EmberListMgr* theMgr)
{
	Reset(theMgr);
}

// 0x41ec40
Ember::~Ember()
{
	mMgr = NULL;
}

// 0x41ec50
// Every field to its default (also used when a pooled ember is reused).
void Ember::Reset(EmberListMgr* theMgr)
{
	mMgr = theMgr;
	mUpdateCnt = 0;
	mDead = false;
	mX = 0.0;
	mY = 0.0;
	mVX = 0.0;
	mVY = 0.0;
	mGravity = 0.01;
	mAlpha = 0.0;
	mAlphaDecay = 0.0;
	mDelay = 0;
	mLoop = false;
	mFrame = 0;
	mNumFrames = 0;
	mType = EMBER_SPARK_SM;
}

// 0x41ecb0
// The cel count is taken from the image on the first update (vertical strips for the sparks, horizontal
// for the others). After the delay: the frame follows the life (full alpha = cel 0) or, looping, cycles
// every update (mUpdateCnt never changes, so the "every 4th update" test always passes); then the
// motion, the fade, and death when the life runs out or the ember leaves the screen moving outwards.
void Ember::Update()
{
	BookWormApp* anApp = mMgr->mApp;

	if (mNumFrames == 0)
	{
		Image* anImage;
		switch (mType)
		{
		case EMBER_SPARK_SM:
			anImage = anApp->mImages[IMAGE_FX_SPARK_SM];
			mNumFrames = anImage->GetHeight() / anImage->GetWidth();
			break;
		case EMBER_SPARK_MED:
			anImage = anApp->mImages[IMAGE_FX_SPARK_MED];
			mNumFrames = anImage->GetHeight() / anImage->GetWidth();
			break;
		case EMBER_SPARK_LRG:
			anImage = anApp->mImages[IMAGE_FX_SPARK_LRG];
			mNumFrames = anImage->GetHeight() / anImage->GetWidth();
			break;
		case EMBER_EMBER_SM:
			anImage = anApp->mImages[IMAGE_FX_EMBER_SM];
			mNumFrames = anImage->GetWidth() / anImage->GetHeight();
			break;
		case EMBER_EMBER_MED:
			anImage = anApp->mImages[IMAGE_FX_EMBER_MED];
			mNumFrames = anImage->GetWidth() / anImage->GetHeight();
			break;
		case EMBER_EMBER_LRG:
			anImage = anApp->mImages[IMAGE_FX_EMBER_LRG];
			mNumFrames = anImage->GetWidth() / anImage->GetHeight();
			break;
		case EMBER_SMOKE_SM:
			anImage = anApp->mImages[IMAGE_FX_EMBER_SMOKE_SM];
			mNumFrames = anImage->GetWidth() / anImage->GetHeight();
			break;
		case EMBER_SMOKE_MED:
			anImage = anApp->mImages[IMAGE_FX_EMBER_SMOKE_MED];
			mNumFrames = anImage->GetWidth() / anImage->GetHeight();
			break;
		case EMBER_SMOKE_LRG:
			anImage = anApp->mImages[IMAGE_FX_EMBER_SMOKE_LRG];
			mNumFrames = anImage->GetWidth() / anImage->GetHeight();
			break;
		case EMBER_FLAME_LRG:
			anImage = anApp->mImages[IMAGE_FX_FLAME_LRG];
			mNumFrames = anImage->GetWidth() / anImage->GetHeight();
			break;
		case EMBER_SMOKE_PUFF:
			mNumFrames = 1;
			break;
		}
	}

	if (mDelay > 0)
	{
		mDelay--;
		return;
	}

	if (mType >= 0)
	{
		if (mType < EMBER_SMOKE_PUFF)
		{
			if (mLoop)
			{
				if (mUpdateCnt % 4 == 0)
					mFrame = (mFrame + 1) % mNumFrames;
			}
			else
			{
				// mNumFrames - 1 - trunc(min(n * alpha - 1, n - 1)): cel 0 at full life, the last cel
				// near the end (cel n when the value is exactly -1).
				double aFrame = (double)mNumFrames * mAlpha - 1.0;
				double aLastFrame = (double)(mNumFrames - 1);
				if (!(aFrame < aLastFrame))
					aFrame = aLastFrame;
				mFrame = mNumFrames - (int)aFrame - 1;
			}
		}
		else if (mType == EMBER_SMOKE_PUFF)
			mFrame = 0;
	}

	mX += mVX;
	mVY += mGravity;
	mY += mVY;
	mAlpha -= mAlphaDecay;

	if ((mAlpha < 0.0) ||
		((mX < 0.0) && (mVX <= 0.0)) ||
		((mX > (double)anApp->mWidth) && (mVX >= 0.0)) ||
		((mY < 0.0) && (mVY <= 0.0)) ||
		((mY > (double)anApp->mHeight) && (mVY >= 0.0)))
		mDead = true;
}

// 0x41f320
// One cel centred on (mX, mY), additive; the medium ember adds its smoke cel at alpha 176 in normal
// mode, the smoke puff is drawn whole at (x - 15, y - 15) fading with the life.
void Ember::Draw(Graphics* g)
{
	BookWormApp* anApp = mMgr->mApp;
	Image* anImage;
	int aCelSize;

	switch (mType)
	{
	case EMBER_SPARK_SM:
	case EMBER_SPARK_MED:
	case EMBER_SPARK_LRG:
	{
		// vertical strips of square cels
		int anImageId = (mType == EMBER_SPARK_SM) ? IMAGE_FX_SPARK_SM :
			(mType == EMBER_SPARK_MED) ? IMAGE_FX_SPARK_MED : IMAGE_FX_SPARK_LRG;
		anImage = anApp->mImages[anImageId];
		aCelSize = anImage->GetWidth();
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->DrawImage(anImage, (int)mX - aCelSize / 2, (int)mY - aCelSize / 2,
			Rect(0, mFrame * aCelSize, aCelSize, aCelSize));
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		break;
	}

	case EMBER_EMBER_SM:
	case EMBER_EMBER_LRG:
	case EMBER_SMOKE_SM:
	case EMBER_SMOKE_MED:
	case EMBER_SMOKE_LRG:
	case EMBER_FLAME_LRG:
	{
		// horizontal strips of square cels
		int anImageId;
		switch (mType)
		{
		case EMBER_EMBER_SM:	anImageId = IMAGE_FX_EMBER_SM; break;
		case EMBER_EMBER_LRG:	anImageId = IMAGE_FX_EMBER_LRG; break;
		case EMBER_SMOKE_SM:	anImageId = IMAGE_FX_EMBER_SMOKE_SM; break;
		case EMBER_SMOKE_MED:	anImageId = IMAGE_FX_EMBER_SMOKE_MED; break;
		case EMBER_SMOKE_LRG:	anImageId = IMAGE_FX_EMBER_SMOKE_LRG; break;
		default:				anImageId = IMAGE_FX_FLAME_LRG; break;
		}
		anImage = anApp->mImages[anImageId];
		aCelSize = anImage->GetHeight();
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->DrawImage(anImage, (int)mX - aCelSize / 2, (int)mY - aCelSize / 2,
			Rect(mFrame * aCelSize, 0, aCelSize, aCelSize));
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		break;
	}

	case EMBER_EMBER_MED:
	{
		anImage = anApp->mImages[IMAGE_FX_EMBER_MED];
		aCelSize = anImage->GetHeight();
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->DrawImage(anImage, (int)mX - aCelSize / 2, (int)mY - aCelSize / 2,
			Rect(mFrame * aCelSize, 0, aCelSize, aCelSize));
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);

		Image* aSmokeImage = anApp->mImages[IMAGE_FX_EMBER_SMOKE_MED];
		int aSmokeSize = aSmokeImage->GetHeight();
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, 0xb0));
		g->DrawImage(aSmokeImage, (int)mX - aSmokeSize / 2, (int)mY - aSmokeSize / 2,
			Rect(mFrame * aSmokeSize, 0, aSmokeSize, aSmokeSize));
		g->SetColorizeImages(false);
		break;
	}

	case EMBER_SMOKE_PUFF:
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, (int)(mAlpha * 255.0)));
		g->DrawImage(anApp->mImages[IMAGE_FX_SMOKE_SM], (int)mX - 15, (int)mY - 15);
		g->SetColorizeImages(false);
		break;

	default:
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// EmberListMgr
//////////////////////////////////////////////////////////////////////////////////////////////////////////

// 0x421070
// One Sexy::Rand() draw seeds the (never drawn) generator; then thePoolSize pooled embers.
EmberListMgr::EmberListMgr(BookWormApp* theApp, int thePoolSize)
{
	mApp = theApp;
	unsigned long aSeed = (unsigned long)Rand();
	mRand = new MTRand();
	SeedMTRand2004(*mRand, aSeed);

	for (int i = 0; i < thePoolSize; i++)
		mFreeEmbers.push_back(new Ember(this));
}

// 0x4203a0 (body), 0x420520 (scalar deleting destructor, slot 0)
EmberListMgr::~EmberListMgr()
{
	Clear();
	if (mRand != NULL)
		delete mRand;
	mApp = NULL;
}

// 0x4202d0
// Deletes the active embers, then the pool (front to back); both vectors end up empty, so new embers
// are allocated afterwards until dead ones refill the pool.
void EmberListMgr::Clear()
{
	while (mEmbers.size() != 0)
	{
		if (mEmbers.front() != NULL)
			delete mEmbers.front();
		mEmbers.erase(mEmbers.begin());
	}
	std::vector<Ember*>().swap(mEmbers);

	while (mFreeEmbers.size() != 0)
	{
		if (mFreeEmbers.front() != NULL)
			delete mFreeEmbers.front();
		mFreeEmbers.erase(mFreeEmbers.begin());
	}
	std::vector<Ember*>().swap(mFreeEmbers);
}

// 0x420d60
// The pool's last ember, reset, or a new one; appended to the active embers.
Ember* EmberListMgr::NewEmber()
{
	Ember* anEmber;
	if (mFreeEmbers.size() != 0)
	{
		anEmber = mFreeEmbers.back();
		mFreeEmbers.pop_back();
		anEmber->Reset(this);
	}
	else
		anEmber = new Ember(this);

	mEmbers.push_back(anEmber);
	return anEmber;
}

// 0x420e00
// A new ember with gravity 0.01 and no delay.
void EmberListMgr::AddEmber(int theType, double theX, double theY, double theVX, double theVY, bool theLoop,
	double theAlpha, double theAlphaDecay)
{
	Ember* anEmber = NewEmber();
	if (anEmber != NULL)
	{
		anEmber->mType = theType;
		anEmber->mX = theX;
		anEmber->mY = theY;
		anEmber->mVX = theVX;
		anEmber->mVY = theVY;
		anEmber->mLoop = theLoop;
		anEmber->mAlpha = theAlpha;
		anEmber->mAlphaDecay = theAlphaDecay;
	}
}

// 0x420fd0
// A dead ember goes back to the pool and the last active ember takes its place (updated in the same
// pass): the draw order changes. Returns whether there were embers before the update.
bool EmberListMgr::Update()
{
	bool hadEmbers = (mEmbers.size() != 0);

	for (int i = 0; i < (int)mEmbers.size(); i++)
	{
		Ember* anEmber = mEmbers[i];
		anEmber->Update();
		if (anEmber->mDead)
		{
			mFreeEmbers.push_back(anEmber);
			mEmbers[i] = mEmbers.back();
			mEmbers.pop_back();
			i--;
		}
	}

	return hadEmbers;
}

// 0x41f8d0
void EmberListMgr::Draw(Graphics* g)
{
	for (std::vector<Ember*>::iterator anItr = mEmbers.begin(); anItr != mEmbers.end(); ++anItr)
	{
		if (*anItr != NULL)
			(*anItr)->Draw(g);
	}
}

// 0x41ff80
// The RNG state as in EffectMgr::Save, then the count of active embers. The original writes a presence byte and
// a record for each ember after the count; Board::SaveGame has cleared the embers by then, so the count is 0 and
// no record follows (port: the embers have no save code). The pool is not saved.
void EmberListMgr::Save(FILE* theFile)
{
	std::string aState = mRand->Serialize();
	unsigned int aLength = (unsigned int)aState.size();
	fwrite(&aLength, 1, 4, theFile);
	fwrite(aState.data(), 1, aLength, theFile);

	int aCount = 0;
	fwrite(&aCount, 1, 4, theFile);
}

// 0x420e70
// Restores the RNG words and reads the ember count, as EffectMgr::Load does (a file the game wrote has 0
// there). Callers Clear() first: the original frees the active vector's buffer here without deleting the
// embers in it (port: the same, the vector is emptied).
void EmberListMgr::Load(FILE* theFile)
{
	unsigned int aLength = 0;
	fread(&aLength, 1, 4, theFile);
	std::string aState(aLength, '\0');
	if (aLength != 0)
		fread(&aState[0], 1, aLength, theFile);
	RestoreMTRand2004(mRand, aState);

	std::vector<Ember*>().swap(mEmbers);

	int aCount = 0;
	fread(&aCount, 1, 4, theFile);
}

// 0x4216e0 MapTableIndex not ported: only FameWidget's internet high-score code calls it (NAMES.md 1.3).
