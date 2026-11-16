#include "SplatFactory.h"
#include "App.h"
#include "Graphics.h"
#include "ImageNode.h"
#include "Layout.h"
#include "Modifiers.h"
#include "Random.h"
#include "Stream.h"
#include "TextItem.h"
#include <cmath>

namespace engine
{

// the colour mode that multiplies the colour into the pixels
const int COLOR_MODE_MULTIPLY = 2;

// a text's lines are centred about the splat's point, across and down
const int ALIGN_CENTER = 1;

// 0x4925a0 (pizzafrenzy) and 0x492480 (pizzafrenzy): what a definition holds before its elements say
SplatDef::SplatDef() :
	mScaleStart(0),
	mScaleEnd(0),
	mUnreadScale(1.0f, 1.0f),
	mStartOpacity(1.0f),
	mEndOpacity(1.0f),
	mShadow(false),
	mShadowOpacity(0),
	mCentered(0)
{
	mStartScale = Point(1.0f, 1.0f);
	mEndScale = Point(1.0f, 1.0f);
	mLifeTime = Point(1.0f, 1.0f);
}

// 0x481810 (pizzafrenzy)
Splat::Splat() :
	mAcceleration(0),
	mStartScale(1.0f),
	mEndScale(1.0f),
	mScaleStart(0),
	mScaleEnd(0),
	mStartOpacity(1.0f),
	mEndOpacity(1.0f),
	mLifeTime(1.0f),
	mTimeLeft(1.0f),
	mHasDestination(false),
	mCallback(nullptr),
	mCallbackArg(0),
	mArrivedValue(0),
	mShadow(false),
	mShadowOpacity(0)
{
	addOwnFlags(NODE_LISTENERS);
}

// 0x4811e0 (pizzafrenzy)
void Splat::setOrigin(float x, float y)
{
	mOrigin.x = x;
	mOrigin.y = y;
	setPosition(x, y);
}

// 0x481200 (pizzafrenzy): the time is the distance over the speed, which the arithmetic takes in double precision
void Splat::setDestination(float x, float y)
{
	mDestination.y = y;
	mHasDestination = true;
	mDestination.x = x;
	float aSpeed = (float)sqrt((double)mVelocity.x * mVelocity.x + (double)mVelocity.y * mVelocity.y);
	if (aSpeed > 0.0f)
	{
		double aDx = (double)(mDestination.x - mOrigin.x);
		double aDy = (double)(mDestination.y - mOrigin.y);
		float aSeconds = (float)(sqrt(aDx * aDx + aDy * aDy) / (double)aSpeed);
		mLifeTime = aSeconds;
		mTimeLeft = aSeconds;
	}
}

// 0x4812a0 (pizzafrenzy)
void Splat::setShadow(bool theShadow, float theOpacity, const Point& theOffset)
{
	mShadow = theShadow;
	mShadowOpacity = theOpacity;
	mShadowOffset = theOffset;
}

// 0x4812d0 (pizzafrenzy)
void Splat::setChild(Node* theNode)
{
	removeAllChildren();
	addChild(theNode);
}

// 0x4815f0 (pizzafrenzy)
void Splat::setText(const std::string& theText)
{
	if (theText.size() == 0)
		return;
	TextItem* aText = new TextItem();
	aText->setFont(std::string(mFontName.c_str()));
	aText->setHAlign(ALIGN_CENTER);
	aText->setVAlign(ALIGN_CENTER);
	aText->setText(theText);
	if (mShadow)
	{
		aText->mStyle |= TEXT_SHADOW;
		aText->setShadow(mShadowOpacity, mShadowOffset);
	}
	if (mColor.r != 1.0f || mColor.g != 1.0f || mColor.b != 1.0f)
	{
		aText->setColorMode(COLOR_MODE_MULTIPLY);
		aText->setColor(mColor);
	}
	removeAllChildren();
	addChild(aText);
	layout();
	float aHalfWidth = (float)((double)getBounds()->width() * 0.5);
	float x = getPosition()->x;
	float aNewX = aHalfWidth;
	if (0.0f <= x - aHalfWidth)
	{
		aNewX = x;
		if ((float)SCREEN_WIDTH < x + aHalfWidth)
			aNewX = (float)SCREEN_WIDTH - aHalfWidth;
	}
	setPosition(aNewX, getPosition()->y);
}

// 0x4812f0 (pizzafrenzy): the scale goes from its start to its end between the two shares of the life time, the opacity
// over the whole of it
void Splat::update(UpdateInfo& theInfo)
{
	float aSeconds = theInfo.mSeconds;
	mTimeLeft = mTimeLeft - aSeconds;
	if (!(0.0f <= mTimeLeft))
		return;
	float aDone = (mLifeTime - mTimeLeft) / mLifeTime;
	float anOpacity = (mEndOpacity - mStartOpacity) * aDone + mStartOpacity;
	if (0.0f < anOpacity)
	{
		if (1.0f <= anOpacity)
			anOpacity = 1.0f;
	}
	else
	{
		anOpacity = 0.0f;
	}
	float aScaleShare;
	if (mScaleStart <= aDone)
		aScaleShare = aDone <= mScaleEnd ? (aDone - mScaleStart) / (mScaleEnd - mScaleStart) : 1.0f;
	else
		aScaleShare = 0.0f;
	float aScale = (mEndScale - mStartScale) * aScaleShare + mStartScale;
	if (aScale <= 0.0f)
		aScale = 0.0f;
	setScale(aScale, aScale);
	setOpacity(anOpacity);
	Point aPosition;
	if (mHasDestination)
	{
		aPosition = mDestination;
		aPosition.subtract(mOrigin);
		aPosition.scale(aDone);
		aPosition.add(mOrigin);
	}
	else
	{
		Point aVelocity = mVelocity;
		aPosition = Point(getX(), getY());
		Point aStep = aVelocity;
		if (sqrt((double)aStep.x * aStep.x + (double)aStep.y * aStep.y) != 0.0)
			aStep.normalize();
		aStep.scale(aSeconds * mAcceleration);
		if (0.0f <= mAcceleration)
		{
			aVelocity.add(aStep);
		}
		else
		{
			// a slowing splat stops on an axis instead of turning back
			Point aBefore = aVelocity;
			aVelocity.add(aStep);
			if (aBefore.x * aVelocity.x < 0.0f)
				aVelocity.x = 0.0f;
			if (aBefore.y * aVelocity.y < 0.0f)
				aVelocity.y = 0.0f;
		}
		aStep = mForce;
		aStep.scale(aSeconds);
		aVelocity.add(aStep);
		mVelocity = aVelocity;
		aVelocity.scale(aSeconds);
		aPosition.add(aVelocity);
	}
	setPosition(aPosition.x, aPosition.y);
	Container::update(theInfo);
}

// 0x4817c0 (pizzafrenzy)
void Splat::arrive()
{
	if (mCallback != nullptr)
		mCallback(mCallbackArg);
	mArrived.emit(mArrivedValue);
}

// 0x474f40 (pizzafrenzy)
SplatFactory::SplatFactory()
{
}

// 0x4748c0 (pizzafrenzy)
SplatFactory::~SplatFactory()
{
	for (std::map<std::string, SplatDef*>::iterator anIt = mDefs.begin(); anIt != mDefs.end(); ++anIt)
		delete anIt->second;
}

// 0x4739f0 (pizzafrenzy)
void SplatFactory::load(const std::string& theFile)
{
	mFile = theFile;
	SplatFactoryDocHandler aHandler;
	aHandler.setFactory(this);
	Ref<InputStream> aStream = App::instance()->openStream(theFile.c_str());
	parseXml(aHandler, aStream);
}

// 0x475020 (pizzafrenzy)
void SplatFactory::addDef(const std::string& theName, SplatDef* theDef)
{
	SplatDef*& aSlot = mDefs[theName];
	aSlot = theDef;
}

// 0x473aa0 (pizzafrenzy)
void SplatFactory::layout()
{
	mRect = Rect();
	setFlags(NODE_LAYOUT);
}

// 0x474670 (pizzafrenzy)
void SplatFactory::queue(Splat* theSplat)
{
	mQueued.push_back(Ref<Splat>(theSplat));
	addOwnFlags(NODE_LISTENERS);
}

// 0x473b90 (pizzafrenzy)
void SplatFactory::update(UpdateInfo& theInfo)
{
	while (!mQueued.empty())
	{
		Ref<Splat> aSplat = mQueued.front();
		addChild(aSplat);
		mQueued.erase(mQueued.begin());
	}
	removeOwnFlags(NODE_LISTENERS);
	std::vector<Node*>::iterator anIt = mChildren.begin();
	while (anIt != mChildren.end())
	{
		Splat* aSplat = static_cast<Splat*>(*anIt);
		if (0.0f <= aSplat->mTimeLeft)
		{
			aSplat->update(theInfo);
			++anIt;
		}
		else
		{
			aSplat->arrive();
			aSplat->clearParent(this);
			anIt = mChildren.erase(anIt);
		}
	}
}

// 0x474a20 (pizzafrenzy)
Splat* SplatFactory::create(const std::string& theName, float x, float y, SplatCallback theArrived, int theArg)
{
	std::map<std::string, SplatDef*>::iterator aDefIt = mDefs.find(theName);
	if (aDefIt == mDefs.end())
		return nullptr;
	SplatDef* aDef = aDefIt->second;
	Splat* aSplat = new Splat();
	aSplat->setName(theName);
	aSplat->mAcceleration = randomFloat(aDef->mAcceleration);
	float aVelocityX = randomFloat(aDef->mVelocityX);
	float aVelocityY = randomFloat(aDef->mVelocityY);
	aSplat->mVelocity = Point(aVelocityX, aVelocityY);
	aSplat->mVelocity.rotate(randomFloat(aDef->mRotation));
	aSplat->mStartScale = randomFloat(aDef->mStartScale);
	aSplat->mEndScale = randomFloat(aDef->mEndScale);
	aSplat->mScaleStart = aDef->mScaleStart;
	aSplat->mScaleEnd = aDef->mScaleEnd;
	aSplat->mStartOpacity = aDef->mStartOpacity;
	aSplat->mEndOpacity = aDef->mEndOpacity;
	aSplat->mForce = aDef->mForce;
	if (aDef->mDestination.x != 0.0f && aDef->mDestination.y != 0.0f)
		aSplat->setDestination(aDef->mDestination.x, aDef->mDestination.y);
	aSplat->setCallback(theArrived, theArg);
	if (aDef->mImagePath.size() != 0)
	{
		Image* anImage = App::instance()->createImage(aDef->mImagePath.c_str(), false);
		anImage->setPivot(aDef->mCentered);
		aSplat->addChild(new ImageNode(anImage));
		aSplat->layout();
	}
	if (aDef->mFont.size() != 0)
	{
		aSplat->mFontName = aDef->mFont;
		aSplat->mColor = aDef->mColor;
		aSplat->setShadow(aDef->mShadow, aDef->mShadowOpacity, aDef->mShadowOffset);
		aSplat->setText(aDef->mText);
	}
	// the picture is kept within the screen's width, at its start
	float aHalfWidth = (float)((double)aSplat->getBounds()->width() * 0.5);
	Point aParentOrigin;
	if (mParent != nullptr)
		aParentOrigin = *mParent->getAbsPosition();
	if (0.0f <= (float)((double)aParentOrigin.x + x - aHalfWidth))
	{
		float anEdge = (float)((double)aParentOrigin.x + aHalfWidth + x);
		if ((float)SCREEN_WIDTH < anEdge)
			x = (float)((double)SCREEN_WIDTH - aHalfWidth - aParentOrigin.x);
	}
	else
	{
		x = aHalfWidth;
	}
	aSplat->setOrigin(x, y);
	aSplat->setScale(aSplat->mStartScale, aSplat->mStartScale);
	aSplat->setOpacity(aSplat->mStartOpacity);
	float aWavePeriodX = randomFloat(aDef->mWavePeriodX);
	float aWavePeriodY = randomFloat(aDef->mWavePeriodY);
	if (aWavePeriodX != 0.0f || aWavePeriodY != 0.0f)
	{
		aSplat->removeAllListeners();
		float aWaveSizeY = randomFloat(aDef->mWaveSizeY);
		float aWaveSizeX = randomFloat(aDef->mWaveSizeX);
		aSplat->addListener(new WaveModifier(aWavePeriodX, aWavePeriodY, aWaveSizeX, aWaveSizeY, 0.0f));
	}
	aSplat->layout();
	float aLifeTime = randomFloat(aDef->mLifeTime);
	aSplat->mTimeLeft = aLifeTime;
	aSplat->mLifeTime = aLifeTime;
	queue(aSplat);
	return aSplat;
}

// 0x4925a0 (pizzafrenzy), a "splat" element: the life time is read here, and the rest left as a definition starts
void SplatFactoryDocHandler::startSplat(const XmlAttributes& theAttributes)
{
	mDef = new SplatDef();
	mDef->mLifeTime = getScaleAttribute(theAttributes, "lifeTime", Point(1.0f, 1.0f));
	mName = getStringAttribute(theAttributes, "name", "");
}

// 0x492320 (pizzafrenzy)
void SplatFactoryDocHandler::endElement(const std::string& theName)
{
	if (theName.compare("splat") == 0)
	{
		mFactory->addDef(mName, mDef);
		mDef = nullptr;
	}
}

// 0x4928c0 (pizzafrenzy): an element outside a definition is passed over
void SplatFactoryDocHandler::startElement(const std::string& theName, const XmlAttributes& theAttributes)
{
	if (theName.compare("splat") == 0)
	{
		startSplat(theAttributes);
	}
	else if (theName.compare("velocity") == 0 && mDef != nullptr)
	{
		mDef->mVelocityX = getScaleAttribute(theAttributes, "startX", Point(0, 10.0f));
		mDef->mVelocityY = getScaleAttribute(theAttributes, "startY", Point(0, 10.0f));
	}
	else if (theName.compare("acceleration") == 0 && mDef != nullptr)
	{
		mDef->mAcceleration = getScaleAttribute(theAttributes, "rate", Point(0, 0));
	}
	else if (theName.compare("extForce") == 0 && mDef != nullptr)
	{
		mDef->mForce = getPointAttribute(theAttributes, "force", Point(0, 0));
	}
	else if (theName.compare("scale") == 0 && mDef != nullptr)
	{
		mDef->mScaleStart = getFloatAttribute(theAttributes, "startAlpha", 0.0f);
		mDef->mScaleEnd = getFloatAttribute(theAttributes, "endAlpha", 1.0f);
		mDef->mStartScale = getScaleAttribute(theAttributes, "startScale", Point(1.0f, 1.0f));
		mDef->mEndScale = getScaleAttribute(theAttributes, "endScale", Point(1.0f, 1.0f));
	}
	else if (theName.compare("rotation") == 0 && mDef != nullptr)
	{
		mDef->mRotation = getScaleAttribute(theAttributes, "degrees", Point(0, 0));
	}
	else if (theName.compare("opacity") == 0 && mDef != nullptr)
	{
		mDef->mStartOpacity = getFloatAttribute(theAttributes, "startOp", 1.0f);
		mDef->mEndOpacity = getFloatAttribute(theAttributes, "endOp", 1.0f);
	}
	else if (theName.compare("resource") == 0 && mDef != nullptr)
	{
		std::string aType = getStringAttribute(theAttributes, "type", "");
		if (aType.compare("image") == 0)
		{
			mDef->mImagePath = getStringAttribute(theAttributes, "path", "INVALID_RES_PATH");
			mDef->mCentered = getStringAttribute(theAttributes, "pivot", "center").compare("center") == 0 ? 1 : 0;
		}
		else if (aType.compare("text") == 0)
		{
			mDef->mFont = getStringAttribute(theAttributes, "font", "INVALID_FONT_PATH");
			mDef->mText = getStringAttribute(theAttributes, "text", "");
			mDef->mColor = getColorAttribute(theAttributes, "color", Color4::fromBytes(255, 255, 255));
			std::string aStyle = getStringAttribute(theAttributes, "textStyle", "");
			if (aStyle.find("shadow") != std::string::npos)
			{
				mDef->mShadow = true;
				mDef->mShadowOffset = getPointAttribute(theAttributes, "textShadowOffset", Point(1.0f, 1.0f));
				mDef->mShadowOpacity = getFloatAttribute(theAttributes, "textShadowOpacity", 1.0f);
			}
			else
			{
				mDef->mShadow = false;
			}
		}
	}
	else if (theName.compare("destination") == 0 && mDef != nullptr)
	{
		mDef->mDestination = getPointAttribute(theAttributes, "pos", Point(0, 0));
	}
	else if (theName.compare("jitter") == 0 && mDef != nullptr)
	{
		std::string anAxis = getStringAttribute(theAttributes, "axis", "");
		if (anAxis.compare("X") == 0)
		{
			mDef->mWavePeriodX = getScaleAttribute(theAttributes, "period", Point(1.0f, 1.0f));
			mDef->mWaveSizeX = getScaleAttribute(theAttributes, "amplitude", Point(0, 0));
		}
		else if (anAxis.compare("Y") == 0)
		{
			mDef->mWavePeriodY = getScaleAttribute(theAttributes, "period", Point(1.0f, 1.0f));
			mDef->mWaveSizeY = getScaleAttribute(theAttributes, "amplitude", Point(0, 0));
		}
	}
}

} // namespace engine
