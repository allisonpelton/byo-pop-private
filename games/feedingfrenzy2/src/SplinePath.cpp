#include "SplinePath.h"

#include "Engine.h"
#include "Params.h"

#include <cmath>

using namespace engine;

namespace
{

// The step between two measured places of a stretch (0x54ddd4 (feedingfrenzy2), a float)
const float SPLINE_STEP = 0.1f;

// 0x43cde0 (feedingfrenzy2): the length of a step, its squares and their sum each a float
float lengthOf(float theX, float theY)
{
	float aSquareY = theY * theY;
	float aSquareX = theX * theX;
	return sqrtf(aSquareX + aSquareY);
}

// 0x4364b0 (feedingfrenzy2): a node's share of the curve inside its span, at the place in the span (0 to 1), from its
// pull (the negative sv's half) and its span's squared length: 6x^5 - 15x^4 + 10x^3 when both are 0. Its x^2 is
// rounded to a float, its other powers are not.
float blendInside(float thePlace, float thePull, float theSpanSquare)
{
	double aPlace = thePlace;
	float aSquare = (float)(aPlace * aPlace);
	double aCube = aPlace * aPlace * aPlace;
	double aFourth = aPlace * aPlace * aPlace * aPlace;
	double aFifth = aPlace * aPlace * aPlace * aPlace * aPlace;
	double aFourthFactor = ((double)theSpanSquare * 2.0 + 14.0 * thePull) - 15.0;
	double aFifthFactor = (6.0 - 5.0 * thePull) - theSpanSquare;
	double aCubeFactor = (10.0 - 12.0 * thePull) - theSpanSquare;
	double aHigher = aFifth * aFifthFactor + aFourth * aFourthFactor;
	aHigher = aCube * aCubeFactor + aHigher;
	return (float)((aPlace + 2.0 * aSquare) * thePull + aHigher);
}

// 0x427200 (feedingfrenzy2): the share of a pulled node (its pull above 0) past the outer end of its span
float blendOutside(float thePlace, float thePull)
{
	double aPlace = thePlace;
	double aSquare = aPlace * aPlace;
	double aFourth = aPlace * aPlace * aPlace * aPlace;
	double aFifth = aPlace * aPlace * aPlace * aPlace * aPlace;
	return (float)(((aPlace + 2.0 * aSquare) * thePull - 2.0 * (aFourth * thePull)) - aFifth * thePull);
}

// A node's sv as it moves the knots: a positive one, else 0
float knotShift(float theShape)
{
	return 0.0f < theShape ? theShape : 0.0f;
}

// A node's sv as it pulls the curve: half a negative one, made positive, else 0
float pullOf(float theShape)
{
	return theShape < 0.0f ? theShape * -0.5f : 0.0f;
}

} // namespace

Vector2f SplinePath::getPositionAt(float theFraction) const
{
	if (mNodes.empty())
		return Vector2f(0, 0);
	if (!(0.0f < theFraction) || !(0.0f < mLength))
		return mNodes[0].mPos;
	if (theFraction >= 1.0f)
		return mNodes.back().mPos;
	for (int aNode = 0; aNode < (int)mNodes.size(); aNode++)
	{
		const SplineNode& aEntry = mNodes[aNode];
		for (int aStep = 0; aStep <= SPLINE_STRETCH_STEPS; aStep++)
		{
			if (!(theFraction <= aEntry.mLengthFractions[aStep]))
				continue;
			if (aStep == 0)
				return aEntry.mPos;
			// The parameter between the two tenths' as the fraction is between theirs
			float aBelow = aEntry.mLengthFractions[aStep - 1];
			double aLowParam = (aStep - 1.0) * SPLINE_STEP + aNode;
			double aHighParam = aNode + SPLINE_STEP * (double)aStep;
			double aBetween = ((double)theFraction - aBelow) / ((double)aEntry.mLengthFractions[aStep] - aBelow);
			return evaluate((float)(aLowParam + aBetween * (aHighParam - aLowParam)));
		}
	}
	return mNodes.back().mPos;
}

Vector2f SplinePath::getNodePosition(int theIndex) const
{
	if (theIndex >= (int)mNodes.size())
		return Vector2f(0, 0);
	return mNodes[theIndex].mPos;
}

void SplinePath::addNode(const SplineNode& theNode)
{
	mNodes.push_back(theNode);
	finish();
}

float SplinePath::measure()
{
	int aCount = getNodeCount();
	if (aCount <= 0)
		return 0.0f;
	// (A walk over the nodes that does nothing comes first in the original)
	float aLength = 0.0f;
	for (int aNode = 0; aNode < aCount - 1; aNode++)
	{
		float aStretch = measureStretch(aNode);
		mNodes[aNode].mStretchLength = aStretch;
		aLength = aStretch + aLength;
	}
	mNodes[aCount - 1].mStretchLength = 0.0f;
	mNodes[aCount - 1].mLengthFractions[0] = 1.0f;

	Vector2f aLast = evaluate(0.0f);
	float aReached = 0.0f;
	for (int aNode = 0; aNode < aCount - 1; aNode++)
	{
		float aNodeParam = (float)aNode;
		float aPerLength = 1.0f / aLength;
		for (int aStep = 0; aStep <= SPLINE_STRETCH_STEPS; aStep++)
		{
			Vector2f aPlace = evaluate((float)((double)aStep * SPLINE_STEP + aNodeParam));
			float aStepLength = lengthOf(aPlace.x - aLast.x, aPlace.y - aLast.y);
			double aSoFar = (double)aStepLength + aReached;
			mNodes[aNode].mLengthFractions[aStep] = (float)(aPerLength * aSoFar);
			aReached = (float)aSoFar;
			aLast = aPlace;
		}
	}
	return aLength;
}

Vector2f SplinePath::evaluate(float theParam) const
{
	int aCount = (int)mNodes.size();
	float aCountF = (float)aCount;
	if (!(theParam < aCountF - 1.0f))
		return mNodes.back().mPos;
	if (!(theParam >= 0.0f))
		return mNodes[0].mPos;

	// The four nodes from the one before the stretch's, and the spans where each counts: a node's knots, moved apart
	// by a positive sv
	float aBase = floorf(theParam) - 1.0f;
	int aFirst = (int)aBase;
	float aShape1 = mNodes.at(aFirst + 1).mShape;
	float aShape2 = mNodes.at(aFirst + 2).mShape;
	float aStart1 = (float)((double)knotShift(aShape1) + aBase + 1.0);
	float aStart2 = (float)((double)knotShift(aShape2) + aBase + 2.0);
	float aBase1 = aBase + 1.0f;
	float aEnd1 = aBase1 - knotShift(aShape1);
	float aEnd2 = (float)((double)aBase + 2.0 - knotShift(aShape2));
	float aSpan0 = aBase - aStart1;
	float aSpanSquare0 = (float)((double)aSpan0 * aSpan0 * 2.0);
	float aSpan1 = aBase1 - aStart2;
	float aSpanSquare1 = (float)((double)aSpan1 * aSpan1 * 2.0);
	float aSpan2 = (float)((double)aBase + 2.0 - aEnd1);
	float aSpanSquare2 = (float)((double)aSpan2 * aSpan2 * 2.0);
	float aSpan3 = (float)((double)aBase + 3.0 - aEnd2);
	float aSpanSquare3 = (float)((double)aSpan3 * aSpan3 * 2.0);
	float aPull1 = pullOf(aShape1);
	float aPull2 = pullOf(aShape2);

	float aWeights[4];
	if (aStart1 < theParam)
		aWeights[0] = 0.0f < aPull1 ? blendOutside((float)(((double)theParam - aStart1) / aSpan0), aPull1) : 0.0f;
	else
		aWeights[0] = blendInside((float)(((double)theParam - aStart1) / aSpan0), aPull1, aSpanSquare0);
	aWeights[1] = blendInside((float)(((double)theParam - aStart2) / aSpan1), aPull2, aSpanSquare1);
	aWeights[2] = blendInside((float)(((double)theParam - aEnd1) / aSpan2), aPull1, aSpanSquare2);
	if (!(aEnd2 <= theParam))
		aWeights[3] = 0.0f < aPull2 ? blendOutside((float)(((double)theParam - aEnd2) / aSpan3), aPull2) : 0.0f;
	else
		aWeights[3] = blendInside((float)(((double)theParam - aEnd2) / aSpan3), aPull2, aSpanSquare3);
	float aWeightSum = (float)((double)aWeights[3] + aWeights[2] + aWeights[1] + aWeights[0]);

	// Each node's place by its weight, (0, 0) for one outside the list
	Vector2f aWeighted[4];
	for (int i = 0; i < 4; i++)
	{
		int aIndex = aFirst + i;
		if (aIndex < 0 || !((float)aIndex < aCountF))
			continue;
		aWeighted[i] = Vector2f(mNodes[aIndex].mPos.x * aWeights[i], mNodes[aIndex].mPos.y * aWeights[i]);
	}
	double aSumX = (double)aWeighted[1].x + aWeighted[0].x + aWeighted[2].x + aWeighted[3].x;
	double aSumY = (double)aWeighted[0].y + aWeighted[1].y + aWeighted[2].y + aWeighted[3].y;
	double aPerWeight = 1.0 / aWeightSum;
	return Vector2f((float)(aSumX * aPerWeight), (float)(aSumY * aPerWeight));
}

float SplinePath::measureStretch(int theIndex) const
{
	float aNodeParam = (float)theIndex;
	Vector2f aLast = evaluate(aNodeParam);
	float aLength = 0.0f;
	float aStep = 1.0f;
	do
	{
		Vector2f aPlace = evaluate((float)((double)aStep * SPLINE_STEP + aNodeParam));
		aLength = (float)((double)lengthOf(aPlace.x - aLast.x, aPlace.y - aLast.y) + aLength);
		aLast = aPlace;
		aStep = aStep + 1.0f;
	} while (!((float)SPLINE_STRETCH_STEPS < aStep));
	return aLength;
}

void SplinePathDocHandler::startElement(const std::string& theName, Params& theAttributes)
{
	if (theName != "splineNode")
		return;
	SplineNode aNode;
	aNode.mPos = theAttributes.getFloatPair("pos", Vector2f(0, 0));
	aNode.mShape = theAttributes.getFloat("sv", -1.0f);
	mPath->addNode(aNode);
}

void SplinePathDocHandler::endElement(const std::string& theName)
{
	if (theName == "splinePath")
		mPath->finish();
}

Ref<SplinePath> loadSplinePath(const std::string& thePath)
{
	Ref<SplinePath> aPath = new SplinePath();
	MasterDocHandler aMaster;
	SplinePathDocHandler* aHandler = new SplinePathDocHandler(aPath.get());
	aHandler->attach(&aMaster, "splinePath");
	if (!getEngine()->parseXml(thePath, &aMaster))
		return Ref<SplinePath>();
	return aPath;
}
