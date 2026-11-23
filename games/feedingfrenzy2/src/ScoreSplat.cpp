#include "ScoreSplat.h"

#include "DrawContext.h"
#include "Engine.h"
#include "FeedingFrenzyTwo.h"
#include "GameHud.h"
#include "Params.h"
#include "Random.h"
#include "Theme.h"
#include "VectorMath.h"

#include <cstdio>

using namespace engine;

namespace
{

// The jump before the flight: up to 60 pixels either side, and 80 up (0x4302f0 (feedingfrenzy2), its two draws)
const float JUMP_ASIDE = 60.0f;
const float JUMP_UP = -80.0f;
// The jump ends this share into the flight (0x4302f0 (feedingfrenzy2))
const float JUMP_TIME = 0.15f;
// Of the flight after the jump, the share that runs straight before it slows to the end
const float FLIGHT_STRAIGHT_SHARE = 0.5f;
// The text's scale at the start, at the jump's top and at the end
const float START_SCALE = 0.5f;
const float JUMP_SCALE = 1.0f;
const float END_SCALE = 0.5f;
// The points to which the small font is used, and from which the large one is
const int SMALL_FONT_MOST_POINTS = 50;
const int LARGE_FONT_LEAST_POINTS = 1000;
// A rising text's (0x416020 (feedingfrenzy2)): its rise, the share of its time the rise takes, its scale at the
// start, its seconds, and its shadow's opacity and offset
const float RISE_HEIGHT = -90.0f;
const float RISE_TIME = 0.25f;
const float RISE_START_SCALE = 0.5f;
const float RISE_SECONDS = 1.5f;
const float RISE_SHADOW_OPACITY = 0.9f;
const float RISE_SHADOW_OFFSET = 1.0f;
// The waits between two texts of a queue (0x4194c0 and 0x4301d0 (feedingfrenzy2))
const float POINTS_WAIT = 0.09f;
const float OTHERS_WAIT = 0.3f;
// A points bubble's speed in pixels a second (0x427250 (feedingfrenzy2))
const float POINTS_BUBBLE_SPEED = 400.0f;
// Its colour mode (the text's slot 0x50)
const int POINTS_BUBBLE_COLOR_MODE = 2;

// The points bubbles' colours by the frenzy's multiplier, from 1 (the static initializer 0x4d73c0 (feedingfrenzy2):
// 0x5acbc0 to 0x5acc10, read from 0x5acbb0 by the multiplier)
const int POINTS_COLOR_COUNT = 6;
const int POINTS_COLOR_BYTES[POINTS_COLOR_COUNT][3] = {
	{0xff, 0xff, 0xff}, {0x3e, 0xeb, 0x0f}, {0xf7, 0xa4, 0x3f}, {0x5f, 0xe3, 0xff}, {0xea, 0xe5, 0x54}, {0xff, 0x45, 0x14},
};

// 0x5083da (feedingfrenzy2): a colour of four bytes; the bubbles' are opaque
Color pointsColor(int theMultiplier)
{
	const int* aBytes = POINTS_COLOR_BYTES[theMultiplier - 1];
	return Color(channelOf(aBytes[0]), channelOf(aBytes[1]), channelOf(aBytes[2]), channelOf(0xff));
}

// A key of a straight line
Vector2Key pathKey(float theTime, const Vector2f& thePoint)
{
	Vector2Key aKey;
	aKey.mTime = theTime;
	aKey.mValue = thePoint;
	return aKey;
}

FloatKey floatKey(float theTime, float theValue)
{
	FloatKey aKey;
	aKey.mTime = theTime;
	aKey.mValue = theValue;
	return aKey;
}

} // namespace

ScoreSplat::ScoreSplat() : mContainer(nullptr), mPoints(0)
{
}

void ScoreSplat::update(UpdateContext& theContext)
{
	mFlight.advance(theContext.mElapsed);
	place();
	if (mFlight.getDirection() != TIME_STOPPED)
		return;
	mLandedSignal(mPoints);
	removeOwnState(STATE_ANIMATED);
	addOwnState(STATE_DELETE);
}

void ScoreSplat::setup(const std::string& theText, Font* theMeasureFont, float theSpeed, int thePoints,
	const Vector2f& theFrom, const Vector2f& theTo)
{
	// The draws are made up first, then aside
	float anUp = (float)randomFloat(JUMP_UP, JUMP_UP);
	float anAside = (float)randomFloat(-JUMP_ASIDE, JUMP_ASIDE);
	Vector2f aJump(anAside, anUp);
	setText(theText);
	setAlignH(TEXT_ALIGN_CENTER);
	const char* aFontName = "scoreSplatFont";
	if (thePoints <= SMALL_FONT_MOST_POINTS)
		aFontName = "scoreSplatFontSmall";
	else if (thePoints >= LARGE_FONT_LEAST_POINTS)
		aFontName = "scoreSplatFontLarge";
	setFont(getApp()->mShellTheme->getFont(aFontName));
	float aWidth = (float)theMeasureFont->getStringWidth(theText.c_str());
	// The path starts where the points were scored; only the jump's top is reckoned from the start kept on the
	// screen, and the flight's time from it
	Vector2f aKeptFrom = theFrom;
	float aRight = (float)((double)getEngine()->getWidth() - aWidth);
	float aLeft = (float)(aWidth * 0.5);
	aKeptFrom.x = clampOf(aKeptFrom.x, aLeft, aRight);
	Vector2f aTop((float)((double)aJump.x + aKeptFrom.x), (float)((double)aJump.y + aKeptFrom.y));
	aRight = (float)((double)getEngine()->getWidth() - aWidth);
	aTop.x = clampOf(aTop.x, aLeft, aRight);
	mPath.clear();
	mPath.addKey(pathKey(0.0f, theFrom));
	mPath.addKey(pathKey(JUMP_TIME, aTop));
	mPath.addKey(pathKey(1.0f, theTo));
	mPath.easeStretch(0, false, true, 0.0f);
	mPath.easeStretch(1, true, false, FLIGHT_STRAIGHT_SHARE);
	mScaleKeys.addKey(floatKey(0.0f, START_SCALE));
	mScaleKeys.addKey(floatKey(JUMP_TIME, JUMP_SCALE));
	mScaleKeys.addKey(floatKey(1.0f, END_SCALE));
	mScaleKeys.easeStretch(0, false, true, 0.0f);
	mScaleKeys.easeStretch(1, true, false, FLIGHT_STRAIGHT_SHARE);
	mOpacityKeys.addKey(floatKey(0.0f, 1.0f));
	mOpacityKeys.addKey(floatKey(1.0f, 1.0f));
	float aDuration = (float)((double)length(difference(theTo, aKeptFrom)) / theSpeed);
	mFlight.start(aDuration, 1.0f);
	mFlight.setTime(0.0f);
	mPoints = thePoints;
}

void ScoreSplat::setupRising(const std::string& theText, Font* theFont, const Vector2f& thePoint)
{
	setText(theText);
	setAlignH(TEXT_ALIGN_CENTER);
	addFlags(TEXT_ITEM_SHADOW);
	setShadow(RISE_SHADOW_OPACITY, Vector2f(RISE_SHADOW_OFFSET, RISE_SHADOW_OFFSET));
	setFont(theFont);
	// The text is measured and the point's x clamped between half the width and the screen's width less it, as the
	// points bubbles' start is; the clamped point is not used again (the path starts from the point as given)
	Vector2f aTop = moved(thePoint, 0.0f, RISE_HEIGHT);
	mPath.clear();
	mPath.addKey(pathKey(0.0f, thePoint));
	mPath.addKey(pathKey(RISE_TIME, aTop));
	mPath.addKey(pathKey(1.0f, aTop));
	mPath.easeStretch(0, true, false, 0.0f);
	mScaleKeys.addKey(floatKey(0.0f, RISE_START_SCALE));
	mScaleKeys.addKey(floatKey(RISE_TIME, 1.0f));
	mScaleKeys.addKey(floatKey(1.0f, 1.0f));
	mScaleKeys.easeStretch(0, true, false, 0.0f);
	mOpacityKeys.addKey(floatKey(0.0f, 1.0f));
	mOpacityKeys.addKey(floatKey(RISE_TIME, 1.0f));
	mOpacityKeys.addKey(floatKey(1.0f, 0.0f));
	mOpacityKeys.easeStretch(1, true, false, 0.0f);
	mFlight.start(RISE_SECONDS, 1.0f);
	mFlight.setTime(0.0f);
	mPoints = 0;
}

void ScoreSplat::show()
{
	mContainer->addChild(this);
	place();
	addOwnState(STATE_ANIMATED);
}

void ScoreSplat::place()
{
	setPosition(mPath.getValue((float)mFlight.getValue()));
	setScale((float)mScaleKeys.getValue((float)mFlight.getValue()));
	setOpacity((float)mOpacityKeys.getValue((float)mFlight.getValue()));
}

void ScoreSplatQueue::push(ScoreSplat* theSplat)
{
	if (0 < theSplat->getPoints())
	{
		if (mPoints.empty() && mPointsWait <= 0.0f)
		{
			theSplat->show();
			mPointsWait = POINTS_WAIT;
		}
		else
			mPoints.push_back(theSplat);
		return;
	}
	if (mOthers.empty() && mOthersWait <= 0.0f)
	{
		theSplat->show();
		mOthersWait = OTHERS_WAIT;
	}
	else
		mOthers.push_back(theSplat);
}

void ScoreSplatQueue::update(float theSeconds)
{
	if (0.0f < mPointsWait)
		mPointsWait = mPointsWait - theSeconds;
	if (!(0.0f < mPointsWait) && !mPoints.empty())
	{
		mPoints.front()->show();
		mPoints.pop_front();
		mPointsWait = POINTS_WAIT;
	}
	if (0.0f < mOthersWait)
		mOthersWait = mOthersWait - theSeconds;
	if (!(0.0f < mOthersWait) && !mOthers.empty())
	{
		mOthers.front()->show();
		mOthers.pop_front();
		mOthersWait = OTHERS_WAIT;
	}
}

void ScoreSplatQueue::clear()
{
	mPoints.clear();
	mOthers.clear();
}

ScoreSplatQueue& scoreSplatQueue()
{
	static ScoreSplatQueue aQueue;
	return aQueue;
}

void addPointsBubble(const Vector2f& theFrom, GameHud* theHud, int thePoints, int theMultiplier)
{
	// Negative points ("%d", in red, the cold path 0x4f0a85 (feedingfrenzy2)): not translated, as the player's points,
	// which alone come here, are always more than 0
	Ref<ScoreSplat> aSplat = new ScoreSplat();
	char aText[32];
	snprintf(aText, sizeof(aText), "+%d", thePoints);
	Vector2f aTo = theHud->getScoreText()->getPosition();
	Font* aFont = getApp()->mShellTheme->getFont("scoreSplatFont");
	aSplat->setup(aText, aFont, POINTS_BUBBLE_SPEED, thePoints, theFrom, aTo);
	aSplat->setColor(pointsColor(theMultiplier));
	aSplat->setColorMode(POINTS_BUBBLE_COLOR_MODE);
	aSplat->mLandedSignal.connect(theHud, &GameHud::addShownScore);
	aSplat->setContainer(theHud->getPointsBubbles());
	scoreSplatQueue().push(aSplat.get());
}
