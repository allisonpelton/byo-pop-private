#include "TextEffects.h"
#include "App.h"
#include "Font.h"
#include "Graphics.h"
#include "ImageNode.h"
#include "Layout.h"
#include "Particles.h"
#include "Random.h"
#include "Stream.h"
#include "TextItem.h"

namespace engine
{

// 0x430c20: the opacities and the life time are left to the "splat" element; the port clears them too
TextEffectTemplate::TextEffectTemplate() : mStartOpacity(0), mEndOpacity(0), mLifeTime(0)
{
}

// 0x42a890
TextEffect::TextEffect()
{
	mAcceleration = 0;
	mStartScale = 1.0f;
	mEndScale = 1.0f;
	mStartOpacity = 1.0f;
	mEndOpacity = 1.0f;
	mLifeTime = 1.0f;
	mTimeLeft = 1.0f;
	mHasDestination = false;
	mCallback = nullptr;
	mCallbackArg = 0;
	addOwnFlags(NODE_LISTENERS);
	setName("Splat");
}

// 0x42a400
void TextEffect::setPosition(float x, float y)
{
	mPosition.set(x, y);
}

// 0x42a410
void TextEffect::setOrigin(float x, float y)
{
	mOrigin.x = x;
	mOrigin.y = y;
	setPosition(x, y);
}

// 0x42a430
void TextEffect::setDestination(float x, float y)
{
	mHasDestination = true;
	mDestination.x = x;
	mDestination.y = y;
}

// 0x42a450
void TextEffect::setCallback(TextEffectCallback theCallback, int theArg)
{
	mCallback = theCallback;
	mCallbackArg = theArg;
}

// 0x42a490
void TextEffect::done()
{
	if (mCallback != nullptr)
		mCallback(mCallbackArg);
}

// 0x42a4b0: the scale and the opacity go from their start to their end over the life time; the text flies on its
// velocity (sped up along its motion and pushed by the force), or slides from its origin to its destination
void TextEffect::update(UpdateInfo& theInfo)
{
	float aSeconds = theInfo.mSeconds;
	mTimeLeft = mTimeLeft - aSeconds;
	if (0 <= mTimeLeft)
	{
		float aDone = (mLifeTime - mTimeLeft) / mLifeTime;
		float aScale = (mEndScale - mStartScale) * aDone + mStartScale;
		float anOpacity = (mEndOpacity - mStartOpacity) * aDone + mStartOpacity;
		if (anOpacity <= 0.0f)
			anOpacity = 0.0f;
		if (aScale <= 0.0f)
			aScale = 0.0f;
		setScale(aScale, aScale);
		setOpacity(anOpacity);
		if (!mHasDestination)
		{
			Point aVelocity = mVelocity;
			Point aPosition(getX(), getY());
			Point aStep = aVelocity;
			aStep.normalize();
			aStep.scale(aSeconds * mAcceleration);
			aVelocity.add(aStep);
			aStep = mForce;
			aStep.scale(aSeconds);
			aVelocity.add(aStep);
			mVelocity = aVelocity;
			aVelocity.scale(aSeconds);
			aPosition.add(aVelocity);
			setPosition(aPosition.x, aPosition.y);
		}
		else
		{
			Point aPosition = mDestination;
			aPosition.subtract(mOrigin);
			aPosition.scale(aDone);
			aPosition.add(mOrigin);
			setPosition(aPosition.x, aPosition.y);
		}
		Container::update(theInfo);
	}
}

// 0x42a750: the opacity is multiplied in, the rotation and the colour offset left out
void TextEffect::applyTransform(Graphics* g)
{
	g->translate(mPosition.x, mPosition.y);
	g->scale(mScale.x, mScale.y);
	g->multiplyOpacity(mOpacity);
	if (mFont != nullptr)
		g->setFont(mFont);
}

// 0x42a7a0
void TextEffect::setText(std::string theText)
{
	if (theText.size() != 0)
	{
		TextItem* aText = new TextItem();
		aText->setFont(std::string(mFontName.c_str()));
		aText->setHAlign(1);
		aText->setText(theText);
		removeAllChildren();
		addChild(aText);
		layout();
	}
}

// 0x429760
TextEffects::TextEffects()
{
	setName("SplatFactory");
}

// 0x428780
bool TextEffects::load(std::string theFile)
{
	TextEffectHandler aHandler;
	aHandler.setFactory(this);
	Ref<InputStream> aStream = App::instance()->openStream(theFile.c_str());
	parseXml(aHandler, aStream);
	return true;
}

// 0x4288a0
bool TextEffects::allDone()
{
	std::vector<Node*>::iterator anIt = mChildren.begin();
	if (anIt != mChildren.end())
	{
		do
		{
			if (!static_cast<TextEffect*>(*anIt)->isDone())
				return false;
			++anIt;
		} while (anIt != mChildren.end());
	}
	return true;
}

// 0x4288e0
void TextEffects::clear()
{
	std::vector<Node*>::iterator anIt = mChildren.begin();
	if (anIt != mChildren.end())
	{
		do
		{
			TextEffect* anEffect = static_cast<TextEffect*>(*anIt);
			anEffect->done();
			anEffect->clearParent(this);
			anIt = mChildren.erase(anIt);
		} while (anIt != mChildren.end());
	}
}

// 0x428940: Container's update is not called
void TextEffects::update(UpdateInfo& theInfo)
{
	std::vector<Node*>::iterator anIt = mChildren.begin();
	if (anIt != mChildren.end())
	{
		do
		{
			TextEffect* anEffect = static_cast<TextEffect*>(*anIt);
			if (0 <= anEffect->mTimeLeft)
			{
				anEffect->update(theInfo);
				++anIt;
			}
			else
			{
				anEffect->done();
				anEffect->clearParent(this);
				anIt = mChildren.erase(anIt);
			}
		} while (anIt != mChildren.end());
	}
}

// 0x4289d0: the ranges are drawn in this order (the jitter's amounts y before x)
TextEffect* TextEffects::create(const std::string& theName, float x, float y, TextEffectCallback theDone, int theArg)
{
	std::map<std::string, TextEffectTemplate*>::iterator anIt = mTemplates.find(theName);
	if (anIt == mTemplates.end())
		return nullptr;
	TextEffectTemplate* aTemplate = anIt->second;
	TextEffect* anEffect = new TextEffect();
	anEffect->setName(theName);
	anEffect->mAcceleration = randomFloat(aTemplate->mAcceleration);
	float aVelocityX = randomFloat(aTemplate->mVelocityX);
	float aVelocityY = randomFloat(aTemplate->mVelocityY);
	anEffect->mVelocity = Point(aVelocityX, aVelocityY);
	anEffect->mVelocity.rotate(randomFloat(aTemplate->mRotation));
	anEffect->mStartScale = randomFloat(aTemplate->mStartScale);
	anEffect->mEndScale = randomFloat(aTemplate->mEndScale);
	anEffect->mStartOpacity = aTemplate->mStartOpacity;
	anEffect->mEndOpacity = aTemplate->mEndOpacity;
	anEffect->mForce = aTemplate->mForce;
	if ((aTemplate->mDestination.x != 0) && (aTemplate->mDestination.y != 0))
		anEffect->setDestination(aTemplate->mDestination.x, aTemplate->mDestination.y);
	anEffect->setOrigin(x, y);
	anEffect->setCallback(theDone, theArg);
	if (aTemplate->mImagePath.size() != 0)
	{
		Image* anImage = App::instance()->getImage(aTemplate->mImagePath.c_str());
		anImage->setPivot(1);
		anEffect->addChild(new ImageNode(anImage));
	}
	if (aTemplate->mFont.size() != 0)
	{
		anEffect->mFontName = aTemplate->mFont;
		anEffect->setText(aTemplate->mText);
	}
	float aRateX = randomFloat(aTemplate->mJitterRateX);
	float aRateY = randomFloat(aTemplate->mJitterRateY);
	if ((aRateX != 0) || (aRateY != 0))
	{
		anEffect->removeAllListeners();
		float anAmountY = randomFloat(aTemplate->mJitterAmountY);
		float anAmountX = randomFloat(aTemplate->mJitterAmountX);
		anEffect->addListener(new JitterListener(aRateX, aRateY, anAmountX, anAmountY));
	}
	anEffect->layout();
	anEffect->mTimeLeft = aTemplate->mLifeTime;
	anEffect->mLifeTime = aTemplate->mLifeTime;
	addChild(anEffect);
	return anEffect;
}

// 0x429740
void TextEffects::addTemplate(std::string theName, TextEffectTemplate* theTemplate)
{
	mTemplates[theName] = theTemplate;
}

// 0x430a70
void TextEffectHandler::endElement(const std::string& theName)
{
	if (theName.compare("splat") == 0)
	{
		mFactory->addTemplate(mName, mTemplate);
		mTemplate = nullptr;
	}
}

// 0x430d20
void TextEffectHandler::startTemplate(const std::string& theName, XmlAttributes theAttributes)
{
	mTemplate = new TextEffectTemplate();
	mTemplate->mLifeTime = getFloatAttribute(theAttributes, "lifeTime", 1.0f);
	mTemplate->mEndOpacity = 1.0f;
	mTemplate->mStartOpacity = 1.0f;
	mName = getStringAttribute(theAttributes, "name", "");
}

// 0x430ee0: an element outside a template is passed over
void TextEffectHandler::startElement(const std::string& theName, const XmlAttributes& theAttributes)
{
	if (theName.compare("splat") == 0)
	{
		startTemplate(theName, theAttributes);
	}
	else if (theName.compare("velocity") == 0 && mTemplate != nullptr)
	{
		mTemplate->mVelocityX = getScaleAttribute(theAttributes, "startX", Point(0, 10.0f));
		mTemplate->mVelocityY = getScaleAttribute(theAttributes, "startY", Point(0, 10.0f));
	}
	else if (theName.compare("acceleration") == 0 && mTemplate != nullptr)
	{
		mTemplate->mAcceleration = getScaleAttribute(theAttributes, "rate", Point(0, 0));
	}
	else if (theName.compare("extForce") == 0 && mTemplate != nullptr)
	{
		mTemplate->mForce = getPointAttribute(theAttributes, "force", Point(0, 0));
	}
	else if (theName.compare("scale") == 0 && mTemplate != nullptr)
	{
		mTemplate->mStartScale = getScaleAttribute(theAttributes, "startScale", Point(1.0f, 1.0f));
		mTemplate->mEndScale = getScaleAttribute(theAttributes, "endScale", Point(1.0f, 1.0f));
	}
	else if (theName.compare("rotation") == 0 && mTemplate != nullptr)
	{
		mTemplate->mRotation = getScaleAttribute(theAttributes, "degrees", Point(0, 0));
	}
	else if (theName.compare("opacity") == 0 && mTemplate != nullptr)
	{
		mTemplate->mStartOpacity = getFloatAttribute(theAttributes, "startOp", 1.0f);
		mTemplate->mEndOpacity = getFloatAttribute(theAttributes, "endOp", 1.0f);
	}
	else if (theName.compare("resource") == 0 && mTemplate != nullptr)
	{
		std::string aType = getStringAttribute(theAttributes, "type", "");
		if (aType.compare("image") == 0)
		{
			mTemplate->mImagePath = getStringAttribute(theAttributes, "path", "INVALID_RES_PATH");
		}
		else if (aType.compare("text") == 0)
		{
			mTemplate->mFont = getStringAttribute(theAttributes, "font", "INVALID_FONT_PATH");
			mTemplate->mText = getStringAttribute(theAttributes, "text", "");
		}
	}
	else if (theName.compare("destination") == 0 && mTemplate != nullptr)
	{
		mTemplate->mDestination = getPointAttribute(theAttributes, "pos", Point(0, 0));
	}
	else if (theName.compare("jitter") == 0 && mTemplate != nullptr)
	{
		std::string anAxis = getStringAttribute(theAttributes, "axis", "");
		if (anAxis.compare("X") == 0)
		{
			mTemplate->mJitterRateX = getScaleAttribute(theAttributes, "rate", Point(0, 0));
			mTemplate->mJitterAmountX = getScaleAttribute(theAttributes, "amplitude", Point(0, 0));
		}
		else if (anAxis.compare("Y") == 0)
		{
			mTemplate->mJitterRateY = getScaleAttribute(theAttributes, "rate", Point(0, 0));
			mTemplate->mJitterAmountY = getScaleAttribute(theAttributes, "amplitude", Point(0, 0));
		}
	}
}

} // namespace engine
