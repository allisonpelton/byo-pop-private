// Sexy::TimeBonusEffect (bejeweled3, 0x72a3b0, 0x730de0, 0x72ec70, 0x741130). See TimeBonusEffect.h.
#include "TimeBonusEffect.h"

#include "Bej3App.h"
#include "Board.h"
#include "Color.h"
#include "Common.h"
#include "GemSilhouettes.h"
#include "Graphics.h"
#include "LightningBoard.h"
#include "Res.h"
#include "SexyMatrix.h"
#include "TriVertex.h"
#include "X86Math.h"

#include <cmath>

namespace Sexy {

namespace {

const double PI_AS_DOUBLE = 3.1415927410125732;   // 0x880e88
const float ART_RES_BASE = 1200.0f;
const int MOST_BOLTS = 3;   // the most bolts a gem has at once: its seconds times 2 less 1, up to this (0x73126e)

// One draw of the app's generator as the exe takes it: a share from 0 up to 1 (not reached)
float Rand01()
{
	return (float)((double)(Rand() % 1000000) / 1000000.0);
}

// 0x712da0 and the exe's inline form of it: a share from -1 up to 1 (not reached)
float RandSigned()
{
	return (float)((double)(Rand() % 1000000 - 500000) / 500000.0);
}

// the gem's turn as the exe takes it from the piece: the frame of its 20 (the phase times 20, up to 19)
int FrameOf(const Piece* thePiece)
{
	if (thePiece == nullptr)
		return 0;
	double aTurn = (double)thePiece->mSpinPhase * 20.0;
	return Ftol(aTurn <= 19.0 ? aTurn : 19.0);
}

// The two tables of glow colours (built at run time by 0x7bcba0 and 0x7bcc40, read at 0x8e1e98 and 0x8e1e18): the first is the sparks' and
// the bolts', the second the circling bolts'; red, white, green, yellow, purple, orange, blue; any other colour is black
Color GlowColor(bool theCircling, int theColor)
{
	static const int FIRST[7][3] = {{255, 96, 96}, {255, 255, 255}, {128, 255, 64}, {225, 225, 128}, {225, 128, 225}, {255, 160, 64}, {48, 160, 255}};
	static const int SECOND[7][3] = {{255, 200, 200}, {255, 250, 255}, {220, 255, 128}, {255, 255, 200}, {255, 128, 255}, {255, 160, 64}, {80, 200, 255}};
	if (theColor < 0 || theColor > 6)
		return Color(0, 0, 0, 255);
	const int* anAmounts = theCircling ? SECOND[theColor] : FIRST[theColor];
	return Color(anAmounts[0], anAmounts[1], anAmounts[2], 255);
}

unsigned long PackColor(const Color& theColor)
{
	return ((unsigned long)theColor.mAlpha << 24) | ((unsigned long)theColor.mRed << 16) | ((unsigned long)theColor.mGreen << 8) |
		(unsigned long)theColor.mBlue;
}

// 0x729da0 (bejeweled3): a strip of 8 points about the line from one point to another, as wide as the thickness to each side, with a
// tip of the thickness's length beyond each end (the picture's first and last halves are the tips' and the middle's, 0.5 across), the
// colour going from the first to the second, added. port: the exe adds the graphics' x translation to both points by hand (the
// framework does it for every vertex)
void DrawRibbon(Graphics* g, Image* theImage, float theX1, float theY1, float theX2, float theY2, float theThickness, const Color& theColor1,
	const Color& theColor2)
{
	const float MIDDLE_U = 0.5f;   // 0x7d6818
	double anAngle = atan2((double)(theY2 - theY1), (double)(theX2 - theX1));
	float aTipX = (float)((double)theThickness * cos(anAngle));
	float aSideX = (float)((double)theThickness * cos(anAngle + 1.5707963705062866));
	float aSideY = (float)((double)theThickness * sin(anAngle + 1.5707963705062866));
	unsigned long aFirst = PackColor(theColor1);
	unsigned long aSecond = PackColor(theColor2);
	TriVertex aPoints[8] = {
		TriVertex(theX1 - aTipX + aSideX, theY1 + aSideY, 0.0f, 0.0f, aFirst),
		TriVertex(theX1 - aTipX - aSideX, theY1 - aSideY, 0.0f, 1.0f, aFirst),
		TriVertex(theX1 + aSideX, theY1 + aSideY, MIDDLE_U, 0.0f, aFirst),
		TriVertex(theX1 - aSideX, theY1 - aSideY, MIDDLE_U, 1.0f, aFirst),
		TriVertex(theX2 + aSideX, theY2 + aSideY, MIDDLE_U, 0.0f, aSecond),
		TriVertex(theX2 - aSideX, theY2 - aSideY, MIDDLE_U, 1.0f, aSecond),
		TriVertex(theX2 + aTipX + aSideX, theY2 + aSideY, 1.0f, 0.0f, aSecond),
		TriVertex(theX2 + aTipX - aSideX, theY2 - aSideY, 1.0f, 1.0f, aSecond),
	};
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	for (int i = 0; i < 6; i++)
		g->DrawTriangleTex(theImage, aPoints[i], aPoints[i + 1], aPoints[i + 2]);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

Image* GemNumbers(int theColor)
{
	Image* aNumbers[7] = {IMAGE_LIGHTNING_GEMNUMS_RED, IMAGE_LIGHTNING_GEMNUMS_WHITE, IMAGE_LIGHTNING_GEMNUMS_GREEN,
		IMAGE_LIGHTNING_GEMNUMS_YELLOW, IMAGE_LIGHTNING_GEMNUMS_PURPLE, IMAGE_LIGHTNING_GEMNUMS_ORANGE, IMAGE_LIGHTNING_GEMNUMS_BLUE};
	return theColor >= 0 && theColor < 7 ? aNumbers[theColor] : nullptr;
}

}  // namespace

// 0x72a3b0 (bejeweled3): the base constructor 0x729460 (type 0x15), the alpha change 0 (+0x50), and the piece's colour, seconds and id.
// port: the effect's two curves (+0x1e0, which starts at 0, and +0x250, set to 1) are stepped by every update but nothing starts them,
// so the bolts' draw reads them as the constants 0 and 1
TimeBonusEffect::TimeBonusEffect(const Piece* thePiece) : Effect(TYPE_TIME_BONUS)
{
	mAlphaChange = 0.0f;
	mPieceId = thePiece->mId;
	mColor = thePiece->mColor;
	mSeconds = thePiece->mTimeGemSeconds;
}

// 0x741130 (bejeweled3): called by the fill (0x716c60) for the time gem it makes: the place 64 and 64 and the +0x18 of 0.08 (its depth,
// which nothing here reads), added to the layer over the gems without being held by the piece
void TimeBonusEffect::MakeFor(Board* theBoard, const Piece* thePiece)
{
	TimeBonusEffect* anEffect = new TimeBonusEffect(thePiece);
	anEffect->mX = 64.0f;
	anEffect->mY = 64.0f;
	theBoard->mEffectsOver->Add(anEffect);
}

// 0x730de0 (bejeweled3), the part that makes a spark (0x730f9a to 0x731186): a SparkEffect on the gem's edge at a random angle, moving
// away from the middle, in the glow colour, added to the layer under the gems. The edge is the table's radius times the art size over 1200
// (the exe puts it so, the place being in 1200ths: at the 1200 art it is the table's value)
void TimeBonusEffect::MakeSpark(int theFrame)
{
	SparkEffect* aSpark = new SparkEffect();
	float aSignedUnit = RandSigned();
	float anAngle = (float)((double)aSignedUnit * PI_AS_DOUBLE);
	double anArtRes = (double)gApp->mArtRes;
	float aRadius = (float)((double)GemSilhouettes::RadiusAt(anAngle, theFrame, mColor) * anArtRes / 1200.0);
	float aSpeedUnit = Rand01();
	float aSpeed = (float)(((double)aSpeedUnit * 0.10000000149011612 + 0.3499999940395355) * anArtRes / 1200.0);
	aSpark->mScaleChange = -0.0f;
	aSpark->mAlpha = 1.0f;
	aSpark->mScale = 1.0f;
	aSpark->mAlphaChange = -0.012000000104308128f;
	aSpark->mImage = IMAGE_SPARKLET;
	float aCos = (float)cos((double)anAngle);
	float aSin = (float)sin((double)anAngle);
	aSpark->mX = (float)((double)(aCos * aRadius) + 64.0);
	aSpark->mY = (float)((double)(aSin * aRadius) + 64.0);
	aSpark->mVelocityX = aSpeed * aCos;
	aSpark->mVelocityY = aSpeed * aSin;
	aSpark->mColor = GlowColor(false, mColor);
	aSpark->mPieceId = mPieceId;
	mManager->mBoard->mEffectsUnder->Add(aSpark);
}

// 0x730de0 (bejeweled3), the part that makes a bolt (0x731291 to 0x731750). Of the draws, in order: a share under 0.075 makes it a
// candidate for the bolt that jumps to a neighbour; if not, a second under 0.02 makes the circling bolt; the circling bolt draws its
// angle, its turn and the end's; a candidate draws an angle and looks at the gem 0.6 of a cell from the middle in that direction: if
// it is another gem it draws where on its edge the bolt lands and the turns, else (and for every other bolt) the plain bolt's angle,
// end and two turns; then the number of points (3 or 4) and, for each, its height and its speed
void TimeBonusEffect::MakeBolt(const Piece* thePiece)
{
	TimeBonusBolt aBolt;
	for (int i = 0; i < 8; i++) {
		aBolt.mHeight[i] = 0.0f;
		aBolt.mHeightChange[i] = 0.0f;
	}
	aBolt.mCircling = false;
	aBolt.mTargeted = false;
	aBolt.mTargetPieceId = 0;
	bool aCandidate = false;
	if ((double)Rand01() < 0.07500000298023224)
		aCandidate = true;
	else if ((double)Rand01() < 0.019999999552965164)
		aBolt.mCircling = true;
	if (aBolt.mCircling) {
		float anAngle = (float)((double)fabsf(RandSigned()) * PI_AS_DOUBLE * 2.0);
		aBolt.mStartAngle = anAngle;
		aBolt.mEndAngle = anAngle;
		float aTurn = (float)((double)RandSigned() * 0.019999999552965164);
		aTurn = aTurn >= 0.0f ? (float)((double)aTurn + 0.019999999552965164) : (float)((double)aTurn - 0.019999999552965164);
		aBolt.mStartTurn = aTurn;
		aBolt.mEndTurn = (float)((double)RandSigned() * 0.019999999552965164 - (double)aTurn);
	}
	else {
		if (aCandidate) {
			Board* aBoard = mManager->mBoard;
			float anAngle = (float)((double)fabsf(RandSigned()) * PI_AS_DOUBLE * 2.0);
			aBolt.mStartAngle = anAngle;
			int aPlaceY = Ftol(sin((double)anAngle) * 128.0 * 0.6000000238418579 + ((double)mY + 64.0));
			int aPlaceX = Ftol(cos((double)anAngle) * 128.0 * 0.6000000238418579 + ((double)mX + 64.0));
			Piece* anOther = aBoard->GetPieceAt((float)(aPlaceX - aBoard->GetGridX()), (float)(aPlaceY - aBoard->GetGridY()));
			if (anOther != nullptr && anOther != thePiece) {
				aBolt.mTargeted = true;
				aBolt.mTargetPieceId = anOther->mId;
				aBolt.mEndAngle = (float)((double)anAngle + PI_AS_DOUBLE + (double)fabsf(RandSigned()) * 0.5);
				aBolt.mStartTurn = (float)((double)RandSigned() * 0.029999999329447746);
				aBolt.mEndTurn = (float)((double)RandSigned() * 0.029999999329447746);
			}
		}
		if (!aBolt.mTargeted) {
			float anAngle = (float)((double)fabsf(RandSigned()) * PI_AS_DOUBLE * 2.0);
			aBolt.mStartAngle = anAngle;
			aBolt.mEndAngle = (float)(0.5 + (double)anAngle + (double)fabsf(RandSigned()) * 0.5);
			aBolt.mStartTurn = (float)((double)RandSigned() * 0.0074999998323619366);
			aBolt.mEndTurn = (float)((double)RandSigned() * 0.0020000000949949026 + (double)aBolt.mStartTurn);
		}
	}
	// port: the 3D game draws two numbers for the count (2 to 6); the 2D game's is 3 or 4
	aBolt.mPointCount = (Rand() & 1) + 3;
	for (int i = 0; i < aBolt.mPointCount; i++) {
		aBolt.mHeight[i] = (float)((double)RandSigned() * 10.0);
		aBolt.mHeightChange[i] = (float)((double)RandSigned() * 0.20000000298023224);
	}
	mBolts.push_back(aBolt);
}

// 0x730de0 (bejeweled3), the loop over the bolts (0x73175b to 0x73213c): the two angles turn; each point moves by its speed and then
// takes a chance to jump, by the kind of the bolt: the plain bolt falls back to the line and sets off (a height of 0 or less makes it start
// again at a small speed), smooths with its neighbours or is pulled towards one; the circling and the targeted bolt bounce between
// -25 and 25. A point over 18 ends the bolt, and for a gem with no seconds to spare (a quarter-share of 0) a chance of 0.0075 does so.
// Then the bolt's own end: the plain one 0.005 each update, the circling one 0.001 and when its angles are a whole turn apart, the targeted
// one when its target is gone, is farther than 90 or turned away (less than 0.75 along the line between) and 0.001. Returns true when
// the bolt goes.
bool TimeBonusEffect::UpdateBolt(TimeBonusBolt& theBolt, int theFrame)
{
	theBolt.mStartAngle = theBolt.mStartTurn + theBolt.mStartAngle;
	theBolt.mEndAngle = theBolt.mEndTurn + theBolt.mEndAngle;
	bool anEnds = false;
	int aSecondsLess = mSeconds - 1;
	int aBoltSeconds = aSecondsLess < 5 ? aSecondsLess : 4;
	float aBoltScale = (float)aBoltSeconds * 0.25f;
	for (int aPoint = 1; aPoint <= theBolt.mPointCount; aPoint++) {
		float& aHeight = theBolt.mHeight[aPoint - 1];
		float& aChange = theBolt.mHeightChange[aPoint - 1];
		aHeight = aHeight + aChange;
		if (theBolt.mTargeted || theBolt.mCircling) {
			// the targeted bolt's chances are 0.2, 0.05 and 0.05, the circling one's 0.2, 0.1 and 0.1
			double aChance = theBolt.mTargeted ? 0.05000000074505806 : 0.10000000149011612;
			if (fabs((double)aHeight) < 25.0) {
				if ((double)Rand01() < 0.20000000298023224)
					aHeight = (float)((double)RandSigned() * 15.0);
				else if ((double)Rand01() < aChance)
					aChange = (float)((double)RandSigned() * 1.5 + (double)aChange);
				else if ((double)Rand01() < aChance)
					aChange = (float)((double)RandSigned() * 1.5);
			}
			else
				aChange = (float)((double)aChange * -0.6499999761581421);
		}
		else {
			if (aHeight > 0.0f) {
				if ((double)Rand01() < 0.05000000074505806)
					aChange = (float)((double)RandSigned() + (double)(4.0f - aHeight) * 0.10000000149011612);
				else if ((double)Rand01() < 0.02500000037252903)
					aHeight = (float)((double)Rand01() * 18.0);
				else if ((double)Rand01() < 0.03999999910593033)
					aChange = (float)((double)RandSigned() * 2.5 + (double)aChange);
			}
			else {
				aHeight = 0.0f;
				aChange = (float)((double)Rand01() * 0.10000000149011612);
			}
			if ((double)Rand01() < 0.10000000149011612) {
				float aBefore = aPoint - 2 >= 0 ? theBolt.mHeight[aPoint - 2] : 0.0f;
				float anAfter = aPoint < theBolt.mPointCount ? theBolt.mHeight[aPoint] : 0.0f;
				aHeight = (float)(((double)aHeight + (double)aBefore + (double)anAfter) / 3.0);
			}
			if ((double)Rand01() < 0.20000000298023224) {
				int aNeighbour = Rand() % 3 - 2 + aPoint;
				if (aNeighbour >= 0 && aNeighbour < theBolt.mPointCount) {
					float aDifference = theBolt.mHeight[aNeighbour] - aHeight;
					aChange = (float)(0.20000000298023224 * (double)aDifference + (double)aChange);
				}
			}
			if ((double)Rand01() < 0.10000000149011612) {
				float aBefore = aPoint - 2 >= 0 ? theBolt.mHeightChange[aPoint - 2] : 0.0f;
				float anAfter = aPoint < theBolt.mPointCount ? theBolt.mHeightChange[aPoint] : 0.0f;
				aChange = (float)((double)(anAfter + aBefore) * 0.5);
			}
		}
		if (aHeight > 18.0f || ((double)aBoltScale <= 0.0 && (double)Rand01() < 0.0074999998323619366))
			anEnds = true;
	}
	if (!theBolt.mTargeted) {
		if (!theBolt.mCircling)
			return (double)Rand01() < 0.004999999888241291 || anEnds;
		if ((double)Rand01() < 0.0010000000474974513)
			anEnds = true;
		return fabs((double)(theBolt.mStartAngle - theBolt.mEndAngle)) >= 6.2831854820251465 || anEnds;
	}
	// the targeted bolt: where it starts and where it lands, in the gem's own measure
	Board* aBoard = mManager->mBoard;
	float aStartRadius = GemSilhouettes::RadiusAt(theBolt.mStartAngle, theFrame, mColor);
	float aStartCos = (float)cos((double)theBolt.mStartAngle);
	float aStartSin = (float)sin((double)theBolt.mStartAngle);
	Piece* aTarget = aBoard->FindPieceById(theBolt.mTargetPieceId);
	if (aTarget == nullptr)
		anEnds = true;
	else {
		float aEndRadius = GemSilhouettes::RadiusAt(theBolt.mEndAngle, FrameOf(aTarget), aTarget->mColor);
		float anEndCos = (float)cos((double)theBolt.mEndAngle);
		float anEndSin = (float)sin((double)theBolt.mEndAngle);
		float aDeltaX = aTarget->mX - mX;
		float aDeltaY = aTarget->mY - mY;
		float aScale = (float)gApp->mArtRes / ART_RES_BASE;
		float aGapX = (float)((double)(aStartCos * aStartRadius) - (double)((float)((double)aEndRadius * anEndCos) + aDeltaX / aScale));
		float aGapY = (float)((double)(aStartSin * aStartRadius) - (double)((float)((double)aEndRadius * anEndSin) + aDeltaY / aScale));
		if (sqrt((double)(aGapX * aGapX + aGapY * aGapY)) > 90.0)
			anEnds = true;
		float aDirection = (float)atan2((double)aDeltaY, (double)aDeltaX);
		float aStartAlong = (float)((double)((float)sin((double)aDirection) * aStartSin) + (double)((float)cos((double)aDirection) * aStartCos));
		if (aStartAlong < 0.75f)
			anEnds = true;
		float aTurned = (float)((double)aDirection + PI_AS_DOUBLE);
		float anEndAlong = (float)((double)((float)sin((double)aTurned) * anEndSin) + (double)((float)cos((double)aTurned) * anEndCos));
		if (anEndAlong < 0.75f)
			anEnds = true;
	}
	return (double)Rand01() < 0.0010000000474974513 || anEnds;
}

// slot 1, 0x730de0 (bejeweled3): the two curves are stepped (constants here); the piece it follows gives the place, the seconds and the
// turn of the gem, and the layer (over, when a hypercube's group is clearing its colour); then, in the exe's order, a spark may be made,
// a bolt may be made, the bolts update, and one update in 25 makes a flash.
// port: the flash (type 0x11, which the manager's draw does not draw: its case of the switch is empty) is not made, only its 4 draws
// (the 25th, then 2 by 0x712da0 for its place and 1 for the piece it follows)
void TimeBonusEffect::Update()
{
	Board* aBoard = mManager->mBoard;
	Piece* aPiece = aBoard->FindPieceById(mPieceId);
	if (aPiece != nullptr) {
		mOverlay = false;
		mX = (float)((double)((float)aBoard->GetGridX() + aPiece->mX) + 64.0);
		mY = (float)((double)((float)aBoard->GetGridY() + aPiece->mY) + 64.0);
		mSeconds = aPiece->mTimeGemSeconds;
		for (size_t i = 0; i < aBoard->mMatchGroups.size(); i++) {
			const MatchGroup* aGroup = aBoard->mMatchGroups[i].get();
			if (aGroup->mType == 7 && aGroup->mColor == aPiece->mColor)
				mOverlay = true;
		}
	}
	int aFrame = FrameOf(aPiece);
	int aSecondsLess = mSeconds - 1;
	int aSparkSeconds = aSecondsLess < 6 ? aSecondsLess : 5;
	int aBoltSeconds = aSecondsLess < 5 ? aSecondsLess : 4;
	float aBoltScale = (float)aBoltSeconds * 0.25f;
	if ((double)Rand01() < (double)(float)aSparkSeconds * 0.03999999910593033)
		MakeSpark(aFrame);
	// the 3D game starts a bolt on a share under the scale times 0.15 (0x73120b); the 2D game draws that share too and then another,
	// under the scale times 0.001, which is the one it uses
	Rand01();
	bool aStartBolt = (double)Rand01() < (double)aBoltScale * 0.0010000000474974513;
	int aMostBolts = mSeconds * 2 - 1;
	if (aMostBolts > MOST_BOLTS)
		aMostBolts = MOST_BOLTS;
	if ((int)mBolts.size() < aMostBolts || aStartBolt)
		MakeBolt(aPiece);
	for (size_t aBolt = 0; aBolt < mBolts.size();) {
		if (UpdateBolt(mBolts[aBolt], aFrame))
			mBolts.erase(mBolts.begin() + aBolt);
		else
			aBolt++;
	}
	if (Rand() % 25 == 0) {
		RandSigned();
		RandSigned();
		Rand();
	}
	if (mPieceId != -1 && (aPiece == nullptr || ((aPiece->mFlags & Piece::FLAG_TIME_GEM) == 0 && mBolts.empty())))
		mDone = true;
}

// slot 2, 0x72ec70 (bejeweled3): white at the layer's alpha times the effect's; the gem's glow (the picture of its outline, added, brighter
// with each bolt) while it is not turning; each bolt as two ribbons (a coloured and a white one) for each piece of its line; and the
// number of seconds, with a white one over it that blinks in the last 5 seconds.
// port: the exe does nothing when its graphics object's byte +8 is set (0x8e171c), and puts a scale on the draw while the gem's scale curve is
// not 1 (the gem as it is cleared: 0x71fcf0, 0x71fd20, 0x713770); neither is made. The layer pushes at 0x58f620 and 0x58f920, which the 3D
// game uses for the number's glow, are left out
void TimeBonusEffect::Draw(Graphics* g)
{
	Board* aBoard = mManager->mBoard;
	int anArtRes = gApp->mArtRes;
	float anArtScale = (float)anArtRes / ART_RES_BASE;
	float aFade = mManager->mAlpha * mAlpha;
	g->PushState();
	g->SetColor(Color(255, 255, 255, Ftol((double)aFade * 255.0)));
	int aGemX = Ftol((double)mX - 64.0);
	int aGemY = Ftol((double)mY - 64.0);
	Piece* aPiece = aBoard->FindPieceById(mPieceId);
	int aFrame = FrameOf(aPiece);
	if (aPiece != nullptr && aPiece->mSpinPhase == 0.0f) {
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		double aBrightness = (double)mBolts.size() * 6.0 + 64.0;
		if (aBrightness > 255.0)
			aBrightness = 255.0;
		g->SetColor(Color(255, 255, 255, Ftol(aBrightness * (double)mAlpha)));
		Rect aRect((anArtRes * aGemX) / 1200, (anArtRes * aGemY) / 1200, (anArtRes * 128) / 1200, (anArtRes * 128) / 1200);
		g->DrawImageCel(IMAGE_GEMOUTLINES, aRect, mColor);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
	// the curves at +0x1e0 and +0x250 (constants here, 0 and 1) leave the gem's edge as it is
	for (size_t aBoltIndex = 0; aBoltIndex < mBolts.size(); aBoltIndex++) {
		const TimeBonusBolt& aBolt = mBolts[aBoltIndex];
		Color aGlow = GlowColor(aBolt.mCircling, mColor);
		int anEffectAlpha = Ftol((double)mAlpha * 255.0);
		float aStartRadius = GemSilhouettes::RadiusAt(aBolt.mStartAngle, aFrame, mColor);
		float anEndRadius = GemSilhouettes::RadiusAt(aBolt.mEndAngle, aFrame, mColor);
		float aStartX = (float)cos((double)aBolt.mStartAngle) * aStartRadius;
		float aStartY = (float)sin((double)aBolt.mStartAngle) * aStartRadius;
		float anEndX = (float)cos((double)aBolt.mEndAngle) * anEndRadius;
		float anEndY = (float)sin((double)aBolt.mEndAngle) * anEndRadius;
		if (aBolt.mTargeted) {
			Piece* aTarget = aBoard->FindPieceById(aBolt.mTargetPieceId);
			if (aTarget != nullptr) {
				float aTargetRadius = GemSilhouettes::RadiusAt(aBolt.mEndAngle, FrameOf(aTarget), aTarget->mColor);
				anEndX = (float)cos((double)aBolt.mEndAngle) * aTargetRadius + ((float)((double)aTarget->mX - (double)aGemX)) / anArtScale;
				anEndY = (float)sin((double)aBolt.mEndAngle) * aTargetRadius + ((float)((double)aTarget->mY - (double)aGemY)) / anArtScale;
			}
		}
		const bool aLine = aBolt.mCircling || aBolt.mTargeted;
		float aPreviousX = aStartX;
		float aPreviousY = aStartY;
		float aPreviousHeight = 0.0f;
		int aBaseX = (anArtRes * (aGemX + 64)) / 1200;
		int aBaseY = (anArtRes * (aGemY + 64)) / 1200;
		for (int aNode = 0; aNode <= aBolt.mPointCount; aNode++) {
			float aShare = (float)(aNode + 1) / (float)(aBolt.mPointCount + 1);
			float aHeight = aNode < aBolt.mPointCount ? aBolt.mHeight[aNode] : 0.0f;
			float aNodeX;
			float aNodeY;
			if (!aLine) {
				float anAngle = aShare * aBolt.mEndAngle + (1.0f - aShare) * aBolt.mStartAngle;
				float aRadius = GemSilhouettes::RadiusAt(anAngle, aFrame, mColor) + aHeight;
				aNodeX = (float)cos((double)anAngle) * aRadius;
				aNodeY = (float)sin((double)anAngle) * aRadius;
			}
			else {
				float aDirection = (float)atan2((double)(anEndY - aPreviousY), (double)(anEndX - aPreviousX));
				aNodeX = (1.0f - aShare) * aStartX + aShare * anEndX;
				aNodeY = (1.0f - aShare) * aStartY + aShare * anEndY;
				if (aNode < aBolt.mPointCount) {
					aNodeX = aNodeX + (float)sin((double)aDirection) * aHeight;
					aNodeY = aNodeY + (float)cos((double)aDirection) * aHeight;
				}
			}
			float aX1 = (float)aBaseX + aPreviousX * anArtScale;
			float aY1 = (float)aBaseY + aPreviousY * anArtScale;
			float aX2 = (float)aBaseX + aNodeX * anArtScale;
			float aY2 = (float)aBaseY + aNodeY * anArtScale;
			// the coloured ribbon, then the white one over it: the plain bolt thins and fades with each point's height
			int aFirstAlphaStart;
			int aFirstAlphaEnd;
			int aSecondAlphaStart;
			int aSecondAlphaEnd;
			float aFirstThickness;
			float aSecondThickness;
			if (!aLine) {
				double aStart = 255.0 * (1.0 - 0.03 * (double)aPreviousHeight);
				double anEnd = 255.0 * (1.0 - 0.03 * (double)aHeight);
				aFirstAlphaStart = Ftol(aStart < 2.0 ? 2.0 : aStart);
				aFirstAlphaEnd = Ftol(anEnd < 2.0 ? 2.0 : anEnd);
				double aSecondStart = 255.0 * (0.85 - 0.04 * (double)aPreviousHeight);
				double aSecondEnd = 255.0 * (0.85 - 0.04 * (double)aHeight);
				aSecondAlphaStart = Ftol(aSecondStart < 2.0 ? 2.0 : aSecondStart);
				aSecondAlphaEnd = Ftol(aSecondEnd < 2.0 ? 2.0 : aSecondEnd);
				aFirstThickness = (float)(anArtRes * 6) / ART_RES_BASE;
				aSecondThickness = aFirstThickness;
			}
			else {
				aFirstAlphaStart = anEffectAlpha;
				aFirstAlphaEnd = anEffectAlpha;
				aSecondAlphaStart = Ftol(0.5 * 255.0);
				aSecondAlphaEnd = aSecondAlphaStart;
				aFirstThickness = (float)(anArtRes * (aBolt.mCircling ? 9 : 8)) / ART_RES_BASE;
				aSecondThickness = (float)(anArtRes * 8) / ART_RES_BASE;
			}
			DrawRibbon(g, IMAGE_ELECTROTEX, aX1, aY1, aX2, aY2, aFirstThickness,
				Color(aGlow.mRed, aGlow.mGreen, aGlow.mBlue, Ftol((double)aFirstAlphaStart * (double)aFade)),
				Color(aGlow.mRed, aGlow.mGreen, aGlow.mBlue, Ftol((double)aFirstAlphaEnd * (double)aFade)));
			DrawRibbon(g, IMAGE_ELECTROTEX_CENTER, aX1, aY1, aX2, aY2, aSecondThickness,
				Color(255, 255, 255, Ftol((double)aSecondAlphaStart * (double)aFade)),
				Color(255, 255, 255, Ftol((double)aSecondAlphaEnd * (double)aFade)));
			aPreviousX = aNodeX;
			aPreviousY = aNodeY;
			aPreviousHeight = aHeight;
		}
	}
	if (mSeconds > 0) {
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, 255));
		Image* aNumbers = GemNumbers(mColor);
		int aPixelX = (anArtRes * aGemX) / 1200;
		int aPixelY = (anArtRes * aGemY) / 1200;
		int aCel = mSeconds / 5 - 1;
		if (aNumbers != nullptr)
			g->DrawImageCel(aNumbers, aPixelX, aPixelY, aCel);
		LightningBoard* aLightning = dynamic_cast<LightningBoard*>(aBoard);
		if (aPiece != nullptr && aLightning != nullptr && aLightning->GetTimeLeft() <= 500 && (aBoard->mGameTicks / 18) % 2 == 0) {
			g->SetColor(Color(255, 200, 200, 255));
			g->DrawImageCel(IMAGE_LIGHTNING_GEMNUMS_WHITE, aPixelX, aPixelY, aCel);
		}
	}
	g->PopState();
}

}  // namespace Sexy
