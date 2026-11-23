#include "Splat.h"

#include "DrawContext.h"
#include "Draws.h"
#include "Engine.h"
#include "Image.h"
#include "Params.h"
#include "TextItem.h"
#include "VectorMath.h"

using namespace engine;

namespace
{

// 0x54ddb0 (feedingfrenzy2)
const float HALF = 0.5f;
// A <velocity>'s range when startX or startY is missing (0x467940 (feedingfrenzy2))
const FloatRange MISSING_VELOCITY(0.0f, 10.0f);

} // namespace

Splat::Splat()
    : mAcceleration(0.0f), mStartScale(1.0f), mEndScale(1.0f), mGrowStart(0.0f), mGrowEnd(0.0f),
      mStartOpacity(1.0f), mEndOpacity(1.0f), mTextColor(1.0f, 1.0f, 1.0f, 1.0f), mLifetime(1.0f), mLife(1.0f),
      mTextShadow(false), mTextShadowOpacity(0.0f)
{
	addOwnState(STATE_ANIMATED);
}

void Splat::setPosition(float x, float y)
{
	mPosition.x = x;
	mPosition.y = y;
}

void Splat::place(float x, float y)
{
	mBirthPosition.x = x;
	mBirthPosition.y = y;
	setPosition(x, y);
}

void Splat::update(UpdateContext& theContext)
{
	float anElapsed = theContext.mElapsed;
	double aLife = (double)mLife - (double)anElapsed;
	mLife = (float)aLife;
	if (aLife < 0.0)
		return;

	float aShare = (float)(((double)mLifetime - aLife) / (double)mLifetime);
	// 0x428090 and 0x46d360 (feedingfrenzy2)
	float anOpacity = (float)(((double)mEndOpacity - (double)mStartOpacity) * (double)aShare + (double)mStartOpacity);
	anOpacity = minOf(1.0f, maxOf(0.0f, anOpacity));
	double aGrowth;
	if (aShare < mGrowStart)
		aGrowth = 0.0;
	else if (mGrowEnd < aShare)
		aGrowth = 1.0;
	else
		aGrowth = ((double)aShare - (double)mGrowStart) / ((double)mGrowEnd - (double)mGrowStart);
	float aScale = maxOf(0.0f, (float)(((double)mEndScale - (double)mStartScale) * aGrowth + (double)mStartScale));
	setScale(aScale, aScale);
	setOpacity(anOpacity);

	// (A splat with a destination, its +0x11c, follows its interpolator instead: never, as none is given one)
	Vector2f aVelocity = mVelocity;
	float aX = getX();
	float aY = getY();
	// The velocity's direction (0x43cde0 and 0x4345f0 (feedingfrenzy2)) times the acceleration's step
	Vector2f aPush = aVelocity;
	normalize(aPush);
	double aStep = (double)mAcceleration * (double)anElapsed;
	aPush.x = (float)((double)aPush.x * aStep);
	aPush.y = (float)(aStep * (double)aPush.y);
	double aNewX = (double)aPush.x + (double)aVelocity.x;
	double aNewY = (double)aPush.y + (double)aVelocity.y;
	if (mAcceleration < 0.0f)
	{
		// Slowing never turns an axis about: one that would change its sign stops
		if (0.0 > (double)aVelocity.x * aNewX)
			aNewX = 0.0;
		if ((double)aVelocity.y * aNewY < 0.0)
			aNewY = 0.0;
	}
	float aForceX = (float)((double)mForce.x * (double)anElapsed);
	float aForceY = (float)((double)mForce.y * (double)anElapsed);
	mVelocity.x = (float)(aNewX + (double)aForceX);
	mVelocity.y = (float)(aNewY + (double)aForceY);
	float aMoveX = (float)((double)mVelocity.x * (double)anElapsed);
	float aMoveY = (float)((double)anElapsed * (double)mVelocity.y);
	setPosition((float)((double)aX + (double)aMoveX), (float)((double)aY + (double)aMoveY));
	Container::update(theContext);
}

void Splat::computeBounds()
{
	Container::computeBounds();
	const RectF& aBounds = getBounds();
	float aHalfWidth = (float)(((double)aBounds.right - (double)aBounds.left) * (double)HALF);
	float aX = getPosition().x;
	if ((double)aX - (double)aHalfWidth < 0.0)
		aX = aHalfWidth;
	else
	{
		float aRight = (float)((double)aX + (double)aHalfWidth);
		int aWidth = getEngine()->getWidth();
		if ((double)aWidth < (double)aRight)
			aX = (float)((double)aWidth - (double)aHalfWidth);
	}
	setPosition(aX, getPosition().y);
}

void Splat::setText(const std::string& theText)
{
	if (theText.empty())
		return;
	TextItem* anItem = makeTextItem();
	anItem->setText(theText);
	computeBounds();
}

TextItem* Splat::makeTextItem()
{
	TextItem* anItem = new TextItem();
	anItem->setFont(getFont());
	anItem->setAlignH(TEXT_ALIGN_CENTER);
	anItem->setAlignV(TEXT_ALIGN_MIDDLE);
	if (mTextShadow)
	{
		anItem->addFlags(TEXT_ITEM_SHADOW);
		anItem->setShadow(mTextShadowOpacity, mTextShadowOffset);
	}
	// The alpha is not looked at
	if (mTextColor.r != 1.0f || mTextColor.g != 1.0f || mTextColor.b != 1.0f)
	{
		anItem->setColorMode(COLOR_MODE_MULTIPLY);
		anItem->setColor(mTextColor);
	}
	removeAllChildren();
	addChild(anItem);
	anItem->computeBounds();
	return anItem;
}

void SplatFactory::update(UpdateContext& theContext)
{
	for (size_t i = 0; i < mWaiting.size(); i++)
		addChild(mWaiting[i]);
	mWaiting.clear();
	removeOwnState(STATE_ANIMATED);

	size_t i = 0;
	while (i < mChildren.size())
	{
		Splat* aSplat = static_cast<Splat*>(mChildren[i]);
		if (aSplat->getLife() < 0.0f)
		{
			// 0x4130b0 (feedingfrenzy2): the splat's function called first, when it has one (none has); the
			// splat taken out by itself, without the container's bookkeeping of a removed child
			mChildren.erase(mChildren.begin() + i);
			aSplat->clearParent();
		}
		else
		{
			aSplat->update(theContext);
			i++;
		}
	}
}

void SplatFactory::load(const std::string& thePath)
{
	SplatFactoryDocHandler aHandler(this);
	getEngine()->parseXml(thePath, &aHandler);
}

void SplatFactory::addDefinition(const std::string& theName, SplatDefinition* theDefinition)
{
	mDefinitions[theName] = theDefinition;
}

void SplatFactory::addSplat(Splat* theSplat)
{
	mWaiting.push_back(theSplat);
	addOwnState(STATE_ANIMATED);
}

Splat* SplatFactory::spawn(const std::string& theName, float x, float y)
{
	std::map<std::string, Ref<SplatDefinition>>::iterator anIt = mDefinitions.find(theName);
	if (anIt == mDefinitions.end())
	{
		getEngine()->log("Error: Cannot find splat definition.\n");
		return nullptr;
	}
	SplatDefinition* aKind = anIt->second;
	Ref<Splat> aSplat = new Splat();
	aSplat->setName(theName);
	if (aKind->mImage)
	{
		aSplat->addChild(new Image(aKind->mImage));
		aSplat->computeBounds();
	}
	else if (!aKind->mThemedImage.empty())
		getEngine()->log("SplatFactory: the themed image %s not yet translated\n", aKind->mThemedImage.c_str());
	if (aKind->mFont)
	{
		aSplat->setFont(aKind->mFont);
		aSplat->mTextColor = aKind->mColor;
		aSplat->mTextShadow = aKind->mShadow;
		aSplat->mTextShadowOpacity = aKind->mShadowOpacity;
		aSplat->mTextShadowOffset = aKind->mShadowOffset;
		aSplat->setText(aKind->mText);
	}
	aSplat->computeBounds();

	// Kept within the window: the parent's place (the original reads a stale stack value without a parent) plus the
	// point, by half the splat's width either side
	const RectF& aBounds = aSplat->getBounds();
	float aHalfWidth = (float)(((double)aBounds.right - (double)aBounds.left) * (double)HALF);
	float aParentX = 0.0f;
	if (mParent != nullptr)
		aParentX = mParent->getAbsolutePosition().x;
	double aLeft = (double)aParentX + (double)x - (double)aHalfWidth;
	if (aLeft < 0.0)
		x = (float)((double)x - aLeft);
	else
	{
		float aRight = (float)((double)aParentX + (double)aHalfWidth + (double)x);
		int aWidth = getEngine()->getWidth();
		if ((double)aWidth < (double)aRight)
			x = (float)((double)x - ((double)aRight - (double)aWidth));
	}

	// The draws, in the original's order
	aSplat->mAcceleration = drawIn(aKind->mAcceleration);
	float aVelocityX = drawIn(aKind->mVelocityX);
	float aVelocityY = drawIn(aKind->mVelocityY);
	aSplat->mVelocity = Vector2f(aVelocityX, aVelocityY);
	float aTurn = drawIn(aKind->mTurn);
	aSplat->mVelocity = rotated(aSplat->mVelocity, aTurn);
	aSplat->mForce = aKind->mForce;
	aSplat->mStartScale = drawIn(aKind->mStartScale);
	aSplat->mEndScale = drawIn(aKind->mEndScale);
	aSplat->mGrowStart = aKind->mGrowStart;
	aSplat->mGrowEnd = aKind->mGrowEnd;
	aSplat->setScale(aSplat->mStartScale, aSplat->mStartScale);
	aSplat->mStartOpacity = aKind->mStartOpacity;
	aSplat->mEndOpacity = aKind->mEndOpacity;
	aSplat->setOpacity(aSplat->mStartOpacity);
	aSplat->place(x, y);
	float aLifetime = drawIn(aKind->mLifetime);
	aSplat->mLife = aLifetime;
	aSplat->mLifetime = aLifetime;
	if (aKind->mDestination.x != 0.0f && aKind->mDestination.y != 0.0f)
		getEngine()->log("SplatFactory: %s's destination not yet translated\n", theName.c_str());
	float aJitterX = drawIn(aKind->mJitterX);
	float aJitterY = drawIn(aKind->mJitterY);
	if (aJitterX != 0.0f || aJitterY != 0.0f)
		getEngine()->log("SplatFactory: %s's jitter not yet translated\n", theName.c_str());
	addSplat(aSplat);
	return aSplat;
}

void SplatFactoryDocHandler::startElement(const std::string& theName, Params& theAttributes)
{
	if (theName == "splat")
	{
		// 0x479fd0 (feedingfrenzy2)
		mDefinition = new SplatDefinition();
		mDefinition->mLifetime = theAttributes.getRange("lifeTime", FloatRange(1.0f, 1.0f));
		mName = theAttributes.getString("name", "");
	}
	if (!mDefinition)
		return;
	if (theName == "velocity")
	{
		mDefinition->mVelocityX = theAttributes.getRange("startX", MISSING_VELOCITY);
		mDefinition->mVelocityY = theAttributes.getRange("startY", MISSING_VELOCITY);
	}
	if (theName == "acceleration")
		mDefinition->mAcceleration = theAttributes.getRange("rate", FloatRange());
	if (theName == "extForce")
		mDefinition->mForce = theAttributes.getFloatPair("force", Vector2f());
	if (theName == "scale")
	{
		mDefinition->mGrowStart = theAttributes.getFloat("startAlpha", 0.0f);
		mDefinition->mGrowEnd = theAttributes.getFloat("endAlpha", 1.0f);
		mDefinition->mStartScale = theAttributes.getRange("startScale", FloatRange(1.0f, 1.0f));
		mDefinition->mEndScale = theAttributes.getRange("endScale", FloatRange(1.0f, 1.0f));
	}
	if (theName == "rotation")
	{
		mDefinition->mTurn = theAttributes.getRange("degrees", FloatRange());
		return;
	}
	if (theName == "opacity")
	{
		mDefinition->mStartOpacity = theAttributes.getFloat("startOp", 1.0f);
		mDefinition->mEndOpacity = theAttributes.getFloat("endOp", 1.0f);
	}
	if (theName == "resource")
	{
		std::string aType = theAttributes.getString("type", "");
		if (aType == "themedimage")
			mDefinition->mThemedImage = theAttributes.getString("name", "");
		else if (aType == "image")
		{
			std::string aPath = theAttributes.getString("path", "INVALID_RES_PATH");
			std::string aPivot = theAttributes.getString("pivot", "center");
			mDefinition->mImage = getEngine()->getImage(aPath);
			// The image is the engine's own, shared with every other user of the file
			mDefinition->mImage->setPivot(aPivot == "center" ? PIVOT_CENTER : PIVOT_UPPER_LEFT);
		}
		else if (aType == "text")
		{
			std::string aFont = theAttributes.getString("font", "INVALID_FONT_PATH");
			mDefinition->mFont = getEngine()->getFont(aFont);
			mDefinition->mText = theAttributes.getString("text", "");
			mDefinition->mColor = theAttributes.getColor("color", Color(1.0f, 1.0f, 1.0f, 1.0f));
			std::string aStyle = theAttributes.getString("textStyle", "");
			if (aStyle.find("shadow") == std::string::npos)
				mDefinition->mShadow = false;
			else
			{
				mDefinition->mShadow = true;
				mDefinition->mShadowOffset = theAttributes.getFloatPair("textShadowOffset", Vector2f(1.0f, 1.0f));
				mDefinition->mShadowOpacity = theAttributes.getFloat("textShadowOpacity", 1.0f);
			}
		}
	}
	if (theName == "destination")
		mDefinition->mDestination = theAttributes.getFloatPair("pos", Vector2f());
	if (theName == "jitter")
		getEngine()->log("SplatFactory: <jitter> not yet translated\n");
}

void SplatFactoryDocHandler::endElement(const std::string& theName)
{
	if (theName == "splat")
	{
		mFactory->addDefinition(mName, mDefinition);
		mDefinition = nullptr;
	}
}
