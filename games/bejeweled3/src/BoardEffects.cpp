// The board's effects (bejeweled3): the persistent looks of the special gems, the flame gem's creation and the hint's flash, made through
// the two EffectsManagers (+0x12bc under the gems, +0x12c0 over them). See Effects.h.
#include "Board.h"

#include "Bej3App.h"
#include "Effects.h"
#include "Graphics.h"
#include "PIEffect.h"
#include "PopAnim.h"
#include "Res.h"

#include <cctype>
#include <string>

namespace Sexy {

namespace {

bool SameName(const std::string& theFirst, const char* theSecond)
{
	size_t aLength = std::string(theSecond).size();
	if (theFirst.size() != aLength)
		return false;
	for (size_t i = 0; i < aLength; i++) {
		if (std::tolower((unsigned char)theFirst[i]) != std::tolower((unsigned char)theSecond[i]))
			return false;
	}
	return true;
}

// 0x56fc40 (bejeweled3): the effect's layer at an index, or null
PILayer* GetLayer(PIEffect* theEffect, int theIndex)
{
	if (theIndex < 0 || theIndex >= (int)theEffect->mLayers.size())
		return nullptr;
	return theEffect->mLayers[theIndex];
}

// 0x56fca0 (bejeweled3): the effect's layer that the definition's layer of that name (any case) is for, or null
PILayer* FindLayer(PIEffect* theEffect, const char* theName)
{
	for (size_t i = 0; i < theEffect->mLayers.size(); i++) {
		if (SameName(theEffect->mLayers[i]->mDef->mName, theName))
			return theEffect->mLayers[i];
	}
	return nullptr;
}

// 0x565f90 (bejeweled3): the layer's emitter of that name (any case), or null
PIEmitterInstance* FindEmitter(PILayer* theLayer, const char* theName)
{
	for (size_t i = 0; i < theLayer->mEmitterInstances.size(); i++) {
		if (SameName(theLayer->mEmitterInstances[i]->mDef->mName, theName))
			return theLayer->mEmitterInstances[i];
	}
	return nullptr;
}

// 0x748dc0 and 0x748f30 (bejeweled3), the parts they have: a copy of PIEFFECT_STARGEM that loops, has its matrix in the effect's draw
// transform, and shows the layer of the gem's colour only (the effect's layers 1 to 7 are the seven colours; each i from 0 to 6 is the
// layer i + 1)
ParticleEffect* MakeStarEffectCopy(int theColor)
{
	ParticleEffect* anEffect = new ParticleEffect(PIEFFECT_STARGEM);
	anEffect->mEffect->mLoop = true;
	anEffect->mPieceMatrix = true;
	for (int i = 0; i < 7; i++) {
		PILayer* aLayer = GetLayer(anEffect->mEffect, i + 1);
		if (aLayer != nullptr)
			aLayer->mVisible = (i == theColor);
	}
	return anEffect;
}

}  // namespace

// 0x749220 (bejeweled3): by the gem's flags, the first of these that is set: 0x10 (0x748c50), 4 the Star Gem (0x749050), 0x80 (0x748ad0), 2 the
// Hypercube (0x748a00), and with 0x800 and 0x1000 off 0x80000 (0x748930) and 0x20000 (0x741130). Bits 0x10, 0x80, 0x800, 0x1000, 0x20000
// and 0x80000 are set by no mode this port makes; the Flame Gem (bit 1) has no persistent effect here
void Board::MakeSpecialEffect(Piece* thePiece)
{
	if ((thePiece->mFlags & Piece::FLAG_STAR) != 0)
		MakeStarEffects(thePiece);
	else if ((thePiece->mFlags & Piece::FLAG_BUTTERFLY) != 0)
		MakeButterflyEffect(thePiece);
	else if ((thePiece->mFlags & Piece::FLAG_HYPERCUBE) != 0)
		MakeHypercubeEffect(thePiece);
}

// 0x748ad0 (bejeweled3): a looping copy of PIEFFECT_BUTTERFLY follows the piece, over the gems; the piece does not hold it. port: the tint of
// the two layers by the gem's colour (0x56fc40, 0x565f40: +0x80 to +0x8c of an emitter) is left out
void Board::MakeButterflyEffect(Piece* thePiece)
{
	if (PIEFFECT_BUTTERFLY == nullptr)
		return;
	ParticleEffect* anEffect = new ParticleEffect(PIEFFECT_BUTTERFLY);
	anEffect->mPieceId = thePiece->mId;
	anEffect->mEffect->mLoop = true;
	anEffect->mPieceMatrix = false;
	mEffectsOver->Add(anEffect);
}

// 0x749050 (bejeweled3), run by 0x7494d0 for a new Star Gem: two copies of PIEFFECT_STARGEM follow the piece, each held by it (users +1, in
// the piece's list). The first, 0x748dc0, is under the gems (+0x12bc), updates twice in each update (+0x184) and has its layer "Top" and
// the "Stars" emitter of the colour's layer off; the second, 0x748f30, is over them (+0x12c0) and has the "Glow" emitter of the colour's
// layer off. Neither draws a Rand() here: each draws one at its second update (PIEffect::Update seeds its generator)
void Board::MakeStarEffects(Piece* thePiece)
{
	if (PIEFFECT_STARGEM == nullptr)
		return;
	for (int aPass = 0; aPass < 2; aPass++) {
		bool anOver = aPass == 1;
		ParticleEffect* anEffect = MakeStarEffectCopy(thePiece->mColor);
		if (!anOver) {
			anEffect->mUpdateTwice = true;
			PILayer* aTop = FindLayer(anEffect->mEffect, "Top");
			if (aTop != nullptr)
				aTop->mVisible = false;
		}
		PILayer* aColorLayer = GetLayer(anEffect->mEffect, thePiece->mColor + 1);
		if (aColorLayer != nullptr) {
			PIEmitterInstance* anEmitter = FindEmitter(aColorLayer, anOver ? "Glow" : "Stars");
			if (anEmitter != nullptr)
				anEmitter->mVisible = false;
		}
		anEffect->mPieceId = thePiece->mId;
		anEffect->mUsers++;
		thePiece->mEffects.push_back(anEffect);
		(anOver ? mEffectsOver : mEffectsUnder)->Add(anEffect);
	}
}

// 0x748a00 (bejeweled3): a looping copy of PIEFFECT_HYPERCUBE that follows the piece, under the gems. It is not held by the piece: when the piece
// goes the effect finds it gone and runs out
void Board::MakeHypercubeEffect(Piece* thePiece)
{
	if (PIEFFECT_HYPERCUBE == nullptr)
		return;
	ParticleEffect* anEffect = new ParticleEffect(PIEFFECT_HYPERCUBE);
	anEffect->mPieceId = thePiece->mId;
	anEffect->mEffect->mLoop = true;
	anEffect->mPieceMatrix = true;
	mEffectsUnder->Add(anEffect);
}

// 0x745ca0 (bejeweled3), after the flame gem's flags are made: two PopAnimEffects of POPANIM_FLAMEGEMCREATION, at the gem's middle at twice the
// scale and following no piece. The first (under the gems) plays from the animation's first frame, the second (over them, drawn in the
// overlay pass) from its label "Creation_Above Gem". Each is updated once at once. The click sound (0x76aaf0) is left out
void Board::MakeFlameCreationEffects(Piece* thePiece)
{
	if (POPANIM_FLAMEGEMCREATION == nullptr)
		return;
	for (int aPass = 0; aPass < 2; aPass++) {
		PopAnimEffect* anEffect = new PopAnimEffect(POPANIM_FLAMEGEMCREATION);
		float aGridX = (float)GetGridX() + thePiece->mX;
		anEffect->mX = (float)((double)aGridX + 64.0);
		float aGridY = (float)GetGridY() + thePiece->mY;
		anEffect->mY = (float)((double)aGridY + 64.0);
		anEffect->mScale = 2.0f;
		if (aPass == 1)
			anEffect->mOverlay = true;
		anEffect->mManager = aPass == 0 ? mEffectsUnder : mEffectsOver;
		anEffect->PlayLabel(aPass == 0 ? "Creation_Below Gem_Horizontal" : "Creation_Above Gem");   // 0x85eea8 and 0x85eec8
		anEffect->Update();
		(aPass == 0 ? mEffectsUnder : mEffectsOver)->Add(anEffect);
	}
}

// 0x748480 (bejeweled3), after the hint's piece is found (0x75aeb0): for a hint the player asked for, a copy of PIEFFECT_HINTFLASH follows the
// piece in the layer over the gems and updates twice in each update. Its matrix puts only the art scale in the draw transform, the place in
// the emitters' transform. port: the three curves 0x748480 starts on the piece (+0x440, +0x280 and +0x2f0, the hint's pulses of the gem)
// are not read by anything the 2D screen draws that was found, and are not made
void Board::MakeHintEffect(Piece* thePiece, bool theUser)
{
	if (!theUser || PIEFFECT_HINTFLASH == nullptr)
		return;
	ParticleEffect* anEffect = new ParticleEffect(PIEFFECT_HINTFLASH);
	anEffect->mPieceId = thePiece->mId;
	anEffect->mUpdateTwice = true;
	mEffectsOver->Add(anEffect);
}

// 0x729ca0 (bejeweled3), for both layers: the pieces let go of their effects first (the effects are deleted here)
void Board::ClearEffects()
{
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			if (mGrid[aRow][aCol] != nullptr)
				mGrid[aRow][aCol]->mEffects.clear();
		}
	}
	if (mEffectsUnder != nullptr)
		mEffectsUnder->Clear();
	if (mEffectsOver != nullptr)
		mEffectsOver->Clear();
}

// slot 44, 0x72e3c0 (bejeweled3) of the over layer, which is the board's child: its overlay pass, drawn after the widgets
void Board::DrawOverlay(Graphics* g)
{
	mEffectsOver->Draw(g, true);
}

}  // namespace Sexy
