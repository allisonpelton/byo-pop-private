// Sexy::PIEffect (PIEffect.h): drawing an effect: Draw, DrawLayer and DrawParticleGroup, with the small color helpers
// they use. The exe's x87 arithmetic is kept: sums and products are done in double and rounded to a float where the exe
// stores a float, and a float turned into an int is truncated (Ftol).
//
// Not translated: what the exe draws for the editor (the layer's blockers, the shown deflectors and forces, and
// everything mDebugDraw turns on). No file of the game has a blocker or a deflector or force with its flag set, and
// nothing sets mDebugDraw, so none of it is reached; the blockers' polygon fill would also need the exe's polygon
// splitter (0x5f2390 (bejeweled3)).
#include "PIEffect.h"

#include "Graphics.h"
#include "Image.h"
#include "PIEffectMath.h"
#include "X86Math.h"

namespace Sexy {

namespace {

// the colors of the curves are the exe's packed 0xAARRGGBB, the alpha curve's values the same slot holding 0 to 255
unsigned int PointValue(const PIParticleDef::ColorPoint& thePoint) { return thePoint.mColor; }
unsigned int PointValue(const PIParticleDef::AlphaPoint& thePoint) { return (unsigned int)thePoint.mAlpha; }

// 0x565d10 (bejeweled3): how far along one of the cycles the age is when the curve repeats theCycles times
float FractionOverCycles(float theRatio, int theCycles)
{
	if (theCycles == 1)
		return theRatio;
	float aScaled = (float)((double)theCycles * (double)theRatio);
	return (float)((double)aScaled - (double)Ftol(aScaled));
}

// 0x565d70 (bejeweled3): blends two packed colors, byte by byte, theRatio of the way from theFrom to theTo (the
// products are divided by 256, so full weight on theTo gives 255/256 of it, as the exe does)
unsigned int MixColors(unsigned int theFrom, unsigned int theTo, float theRatio)
{
	int aWeight = Ftol((double)theRatio * 255.0);
	if (aWeight < 0)
		aWeight = 0;
	else if (aWeight > 0xff)
		aWeight = 0xff;
	int aRest = 0xff - aWeight;
	unsigned int aToShifted = theTo >> 8;
	unsigned int aFromShifted = theFrom >> 8;
	return (((theFrom & 0xff) * aRest + (theTo & 0xff) * aWeight) >> 8 & 0xff) |
	       (((aToShifted & 0xff0000) * aWeight + (aFromShifted & 0xff0000) * aRest) & 0xff000000) |
	       (((aToShifted & 0xff) * aWeight + (aFromShifted & 0xff) * aRest) & 0xff00) |
	       (((aToShifted & 0xff00) * aWeight + (aFromShifted & 0xff00) * aRest) & 0xff0000);
}

// 0x566960 (bejeweled3): the value of a curve of points (color or alpha, each with a time) at theRatio of its span:
// the two points around that time, blended
template <typename PointType>
unsigned int ValueAlongCurve(const std::vector<PointType>& thePoints, float theRatio)
{
	size_t aCount = thePoints.size();
	if (aCount == 1)
		return PointValue(thePoints[0]);
	if (aCount == 0)
		return 0;   // port: the exe reads past the end of the empty list

	float aFirstTime = thePoints.front().mTime;
	float aLastTime = thePoints.back().mTime;
	float aTime = (float)(((double)aLastTime - (double)aFirstTime) * (double)theRatio + (double)aFirstTime);
	for (size_t i = 1; i < aCount; i++) {
		const PointType& aPrevious = thePoints[i - 1];
		const PointType& aNext = thePoints[i];
		if ((aPrevious.mTime <= aTime && !(aNext.mTime < aTime)) || i == aCount - 1) {
			double aPosition = ((double)aTime - (double)aPrevious.mTime) / ((double)aNext.mTime - (double)aPrevious.mTime);
			if (1.0 < aPosition)
				aPosition = 1.0;
			return MixColors(PointValue(aPrevious), PointValue(aNext), (float)aPosition);
		}
	}
	return 0;
}

// 0x566ad0 (bejeweled3): the value of the point with the given number (counting round when it is past the end)
template <typename PointType>
unsigned int ValueAtIndex(const std::vector<PointType>& thePoints, unsigned int theIndex)
{
	if (thePoints.empty())
		return 0;
	return PointValue(thePoints[theIndex % thePoints.size()]);
}

// 0x4362d0 (bejeweled3): a color packed as 0xAARRGGBB
unsigned int PackColor(const Color& theColor)
{
	return (((((unsigned int)theColor.mAlpha << 8) | (unsigned int)theColor.mRed) << 8 | (unsigned int)theColor.mGreen) << 8) |
	       (unsigned int)theColor.mBlue;
}

// 0x4362f0 (bejeweled3): the color a packed 0xAARRGGBB stands for
Color UnpackColor(unsigned int thePacked)
{
	return Color((int)((thePacked >> 16) & 0xff), (int)((thePacked >> 8) & 0xff), (int)(thePacked & 0xff),
	             (int)(thePacked >> 24));
}

}   // namespace

// 0x574060 (bejeweled3): draws the particles of one group (an emitter instance's, or one free emitter's). A particle
// is drawn in two passes at most: the first pass draws only those that are both additive and also drawn normally (in
// black, with their alpha), the second pass draws every particle, the additive ones with the additive blend
void PIEffect::DrawParticleGroup(Graphics* g, PILayer* theLayer, PIEmitterInstance* theInstance, PIParticleGroup* theGroup,
                                 bool theFirstPass)
{
	if (!theInstance->mWasInRange)
		return;

	// the color every particle starts from: the layer's and the effect's together
	Color aBaseColor((mColor.mRed * theLayer->mColor.mRed) / 255, (mColor.mGreen * theLayer->mColor.mGreen) / 255,
	                 (mColor.mBlue * theLayer->mColor.mBlue) / 255, (mColor.mAlpha * theLayer->mColor.mAlpha) / 255);
	bool aBaseColored = aBaseColor.mRed != 255 || aBaseColor.mGreen != 255 || aBaseColor.mBlue != 255 ||
	                    aBaseColor.mAlpha != 255;

	PIParticleInstance* aNextParticle;
	for (PIParticleInstance* aParticle = theGroup->mHead; aParticle != NULL; aParticle = aNextParticle) {
		PIParticleDef* aParticleDef = aParticle->mParticleDef;
		float aLifeRatio = aParticle->mLifeRatio;
		aNextParticle = aParticle->mNext;

		if (!(aParticleDef->mAdditive && aParticleDef->mAlsoDrawNormal) && theFirstPass)
			continue;

		PIEmitter* anEmitter = aParticle->mEmitter;
		g->SetDrawMode(aParticleDef->mAdditive && !theFirstPass ? Graphics::DRAWMODE_ADDITIVE : Graphics::DRAWMODE_NORMAL);

		// which part of which image
		Image* anImage = mDef.mTextures[aParticleDef->mTextureIndex].mImage;
		if (anImage == NULL)
			continue;   // port: the exe has no such case (Load finds every image)
		Rect aSourceRect = anImage->GetCelRect(aParticle->mFrame);

		// the color along the particle's color curve
		unsigned int aColor;
		if (aParticleDef->mRandomColor) {
			if (!aParticleDef->mColorStepped) {
				aColor = ValueAlongCurve(aParticleDef->mColorPoints, aParticle->mColorOffset);
			}
			else {
				double aPlace = (double)(unsigned int)aParticleDef->mColorPoints.size() * (double)aParticle->mColorOffset;
				if ((double)(unsigned int)(aParticleDef->mColorPoints.size() - 1) <= aPlace)
					aColor = ValueAtIndex(aParticleDef->mColorPoints, (unsigned int)(aParticleDef->mColorPoints.size() - 1));
				else
					aColor = ValueAtIndex(aParticleDef->mColorPoints, (unsigned int)Ftol(aPlace));
			}
		}
		else if (aParticleDef->mColorByIndex) {
			if (aParticleDef->mColorIndexDivisor == 0 || aParticleDef->mColorPoints.empty())
				aColor = 0;   // port: the exe divides by zero
			else
				aColor = ValueAtIndex(aParticleDef->mColorPoints,
				                      (unsigned int)((aParticle->mIndex / aParticleDef->mColorIndexDivisor) %
				                                     (int)aParticleDef->mColorPoints.size()));
		}
		else {
			aColor = ValueAlongCurve(aParticleDef->mColorPoints, FractionOverCycles(aLifeRatio, aParticleDef->mColorCycles + 1));
		}

		// the color the layer's picture gave it, on top of the curve's
		if (aParticleDef->mUseInstanceColor)
			aColor = aColor ^ ((aParticle->mInstanceColor ^ aColor) & 0xffffff);
		if (aParticleDef->mUseInstanceAlpha)
			aColor = ((aParticle->mInstanceColor ^ aColor) & 0xffffff) ^ aParticle->mInstanceColor;

		// and toward the emitter instance's color by its tint
		float aTint = theInstance->mDef->mTint.GetValueAt(mFrameNum, 0.0f);
		float aTintFactor = anEmitter->mTintFactor.GetValueAt(mFrameNum, 1.0f);
		float aTintAmount = (float)((double)aTintFactor * (double)aTint);
		aColor = MixColors(aColor, PackColor(theInstance->mColor), aTintAmount);

		// the alpha along the alpha curve, with the instance's and the emitter's
		unsigned int anAlphaValue = ValueAlongCurve(aParticleDef->mAlphaPoints,
		                                            FractionOverCycles(aLifeRatio, aParticleDef->mAlphaCycles + 1));
		float anInstanceAlpha = theInstance->mDef->mAlpha.GetValueAt(mFrameNum, 0.0f);
		float aVisibility = aParticleDef->mVisibility.GetValueAt(mFrameNum, 0.0f);
		double aShownAlpha = (double)aVisibility * (double)anInstanceAlpha;
		float anAlphaFactor = anEmitter->mAlphaFactor.GetValueAt(mFrameNum, 1.0f);
		unsigned int anAlpha = (unsigned int)Ftol((double)anAlphaValue * ((double)anAlphaFactor * aShownAlpha));
		unsigned int aPacked = (anAlpha << 24) | ((theFirstPass ? 0u : aColor) & 0xffffff);

		Color aDrawColor = UnpackColor(aPacked);
		if (aBaseColored) {
			aDrawColor = Color((aBaseColor.mRed * aDrawColor.mRed) / 255, (aBaseColor.mGreen * aDrawColor.mGreen) / 255,
			                   (aBaseColor.mBlue * aDrawColor.mBlue) / 255, (aBaseColor.mAlpha * aDrawColor.mAlpha) / 255);
		}

		if (aDrawColor.mAlpha != 0) {
			g->SetColor(aDrawColor);
			CalcParticleTransform(theLayer, theInstance, anEmitter, aParticleDef, theGroup, aParticle);
			SexyMatrix3 aMatrix = PIMath::Multiply(mDrawTransform, aParticle->mTransform);
			g->DrawImageMatrix(anImage, aMatrix, aSourceRect, 0, 0);

			// the statistic: the picture's area, as scaled by the particle, added up
			float aParticleArea = aParticle->mDrawArea < 0.0f ? -aParticle->mDrawArea : aParticle->mDrawArea;
			mDrawArea = Ftol((double)aParticleArea * (double)(anImage->mHeight * anImage->mWidth) + (double)mDrawArea);
			aParticle->mDrawn = true;
		}
	}
}

// 0x574700 (bejeweled3): draws one layer: for each emitter instance that is shown, its particles in two passes (see
// DrawParticleGroup). A super emitter's instance draws the particles of its free emitters, once for each of the
// emitters its definition lists (the exe's loop does not use that count's index)
void PIEffect::DrawLayer(Graphics* g, PILayer* theLayer)
{
	g->PushState();
	g->SetColorizeImages(true);

	PILayerDef* aLayerDef = theLayer->mDef;
	for (size_t i = 0; i < theLayer->mEmitterInstances.size() && i < aLayerDef->mEmitterInstances.size(); i++) {
		PIEmitterInstance* anInstance = theLayer->mEmitterInstances[i];
		PIEmitterInstanceDef& anInstanceDef = aLayerDef->mEmitterInstances[i];
		if (!anInstance->mVisible)
			continue;

		for (int aPass = 0; aPass < 2; aPass++) {
			bool aFirstPass = aPass == 0;
			if (!anInstanceDef.mHasFreeEmitters) {
				DrawParticleGroup(g, theLayer, anInstance, &anInstance->mParticles, aFirstPass);
			}
			else {
				for (size_t anIndex = 0; anIndex < anInstanceDef.mFreeEmitterIndices.size(); anIndex++) {
					for (PIParticleInstance* aFreeEmitter = anInstance->mFreeEmitters.mHead; aFreeEmitter != NULL;
					     aFreeEmitter = aFreeEmitter->mNext) {
						DrawParticleGroup(g, theLayer, anInstance, &static_cast<PIFreeEmitterInstance*>(aFreeEmitter)->mParticles,
						                  aFirstPass);
					}
				}
			}
		}
	}

	g->PopState();
}

// 0x575650 (bejeweled3): draws the effect: every layer that is shown. mDrawArea, which DrawParticleGroup added the
// pictures' areas to, ends as that many pixels as the effect's own matrix scales them
void PIEffect::Draw(Graphics* g)
{
	mDrawArea = 0;
	for (size_t i = 0; i < mDef.mLayers.size() && i < mLayers.size(); i++) {
		if (mLayers[i]->mVisible)
			DrawLayer(g, mLayers[i]);
	}

	// 0x565c60 (bejeweled3): how much the matrix scales an area: the lengths of its two rows' first two numbers
	float aFirstLength = PIMath::Sqrt((float)((double)mDrawTransform.m00 * mDrawTransform.m00 +
	                                          (double)mDrawTransform.m01 * mDrawTransform.m01));
	float aSecondLength = PIMath::Sqrt((float)((double)mDrawTransform.m10 * mDrawTransform.m10 +
	                                           (double)mDrawTransform.m11 * mDrawTransform.m11));
	float anAreaScale = (float)((double)aSecondLength * (double)aFirstLength);
	mDrawArea = Ftol((double)anAreaScale * (double)mDrawArea);
}

}
