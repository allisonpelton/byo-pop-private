#include "VisualData.h"

#include "ActorInfo.h"
#include "DrawContext.h"
#include "Engine.h"
#include "FeedingFrenzyTwo.h"
#include "PixelFx.h"
#include "Surface.h"
#include "Theme.h"

#include <cstdio>

using namespace engine;

namespace
{

// The pixel effect of a visual's colour effect, null for none: hsvAdd's hue, saturation and value shifts, rgbAdd's
// colour added (its alpha 0), rgbMultiply's multiplied (its alpha 1), rgbBlend's laid over by its own alpha (unless
// left out) and rainbow's hue and its turn per column
Ref<PixelFx> colorEffectOf(const VisualInfo* theInfo, bool withBlend)
{
	const float* aParams = theInfo->mColorParams;
	switch (theInfo->mColorEffect)
	{
	case COLOR_EFFECT_HSV_ADD:
		return new HSVShiftFx(aParams[0], aParams[1], aParams[2]);
	case COLOR_EFFECT_RGB_ADD:
		return new ColorFx(Color(aParams[0], aParams[1], aParams[2], 0.0f), COLOR_FX_ADD);
	case COLOR_EFFECT_RGB_MULTIPLY:
		return new ColorFx(Color(aParams[0], aParams[1], aParams[2], 1.0f), COLOR_FX_MULTIPLY);
	case COLOR_EFFECT_RGB_BLEND:
		if (!withBlend)
			return nullptr;
		return new ColorFx(Color(aParams[0], aParams[1], aParams[2], aParams[3]), COLOR_FX_BLEND);
	case COLOR_EFFECT_RAINBOW:
		return new RainbowShiftFx(aParams[0], aParams[1]);
	}
	return nullptr;
}

// 0x566760 (feedingfrenzy2), a double
const double HALF = 0.5;
// The danger icon's picture darkened (0x54ddac (feedingfrenzy2)), and where its overlay goes (0x566c80 and
// 0x566c84 (feedingfrenzy2))
const float DANGER_SHADE = 0.3f;
const float DANGER_OVERLAY_X = 43.0f;
const float DANGER_OVERLAY_Y = 37.0f;

// 0x43c1a0 (feedingfrenzy2): a point times a number, each coordinate rounded to a float
Vector2f scaled(const Vector2f& thePoint, float theScale)
{
	return Vector2f((float)((double)thePoint.x * (double)theScale), (float)((double)theScale * (double)thePoint.y));
}

// 0x43aa30 (feedingfrenzy2): the offset's coordinates added to the point's
Vector2f moved(const Vector2f& thePoint, const Vector2f& theOffset)
{
	return Vector2f((float)((double)theOffset.x + (double)thePoint.x),
	                (float)((double)theOffset.y + (double)thePoint.y));
}

// The shares of a body's width and height a fish's points sit from its centre, toward its upper left: floats the
// x87 reads as doubles (0x566900, 0x566b38, 0x566b30 and 0x566b28 (feedingfrenzy2))
const float HALO_WIDTH_SHARE = 0.15f;
const float HALO_HEIGHT_SHARE = 0.57f;
const float IDEA_WIDTH_SHARE = 0.45f;
const float IDEA_HEIGHT_SHARE = 0.55f;

} // namespace

void VisualData::setup(VisualInfo* theInfo, const std::string& theAnimBase, float theSize)
{
	mInfo = theInfo;
	ModelInfo* aModel = mInfo->mModel;
	if (theSize <= 0.0f || aModel->mSize < 1)
		mScale = 1.0f;
	else
		mScale = (float)((double)theSize / (double)aModel->mSize);
	if (!aModel->mSource.empty())
	{
		if (!aModel->mAnimated)
		{
			addFrame(getEngine()->getImage(aModel->mSource));
			addAnimation("still", 0, 1, 1.0f);
		}
		else
		{
			char aPath[600];
			snprintf(aPath, sizeof(aPath), "%s/%s/animation.xml", theAnimBase.c_str(), aModel->mSource.c_str());
			load(aPath);
		}
	}
	finish();
}

void VisualData::addFrame(Surface* theFrame)
{
	Ref<Surface> aFrame = theFrame;
	Ref<PixelFx> anEffect = colorEffectOf(mInfo, true);
	if (!aFrame)
	{
		// No frame (the bonuses' pop's last, which shows nothing) is kept as it is when nothing is to be done to it;
		// with an effect, a scale or a tint the original goes on to use it and fails, and the port keeps nothing
		if (!anEffect && mScale == 1.0f && !(0.0f < mTint.a))
			AnimationData::addFrame(nullptr);
		return;
	}
	// 0x4267e0 and 0x426990 (feedingfrenzy2): the visual's colour effect put on a copy before it is scaled (the
	// surface's slot 0x34); without one, a scaled copy unless the scale is 1 and there is no tint
	if (anEffect)
		aFrame = aFrame->pixelFxScaledCopy(mScale, *anEffect);
	else if (mScale != 1.0f || 0.0f < mTint.a)
		aFrame = aFrame->scaledCopy(mScale);
	if (0.0f < mTint.a)
		aFrame->applyPixelFx(ColorFx(mTint, COLOR_FX_BLEND));
	AnimationData::addFrame(aFrame);
}

void VisualData::finish()
{
	setupShapes(1.0f);
}

Vector2f VisualData::getAnchor(const std::string& theRole) const
{
	std::map<std::string, Vector2f>::const_iterator anIt = mAnchors.find(theRole);
	if (anIt == mAnchors.end())
		return Vector2f(0.0f, 0.0f);
	return anIt->second;
}

Shape* VisualData::getShape(const std::string& theRole) const
{
	std::map<std::string, Ref<Shape>>::const_iterator anIt = mShapes.find(theRole);
	if (anIt == mShapes.end())
		return nullptr;
	return anIt->second;
}

void VisualData::setupShapes(float theShapeScale)
{
	shape::Box* aBox = new shape::Box();
	Vector2f anOffset(0.0f, 0.0f);
	if (mFrames.empty())
	{
		aBox->mCenter = Vector2f(1.0f, 1.0f);
		aBox->mDiagonal = Vector2f(1.0f, 1.0f);
	}
	else
	{
		Surface* aFrame = mFrames.front();
		int aHeight = aFrame->getHeight();
		int aWidth = aFrame->getWidth();
		// 0x566850 (feedingfrenzy2): -1
		anOffset = scaled(aFrame->getAnchor(), -1.0f);
		aBox->mCenter = Vector2f((float)(0.5 * (double)aWidth), (float)((double)aHeight * 0.5));
		aBox->mDiagonal = aBox->mCenter;
		aBox->translate(anOffset);
		aBox->scale(1.0f);
	}
	mShapes["visual"] = aBox;
	ModelInfo* aModel = mInfo->mModel;
	for (std::map<std::string, Ref<Shape>>::iterator anIt = aModel->mShapes.begin(); anIt != aModel->mShapes.end();
	     ++anIt)
	{
		if (!anIt->second)
			continue;
		Shape* aShape = anIt->second->clone();
		aShape->scale(theShapeScale);
		aShape->translate(anOffset);
		mShapes[anIt->first] = aShape;
	}
	for (std::map<std::string, Vector2f>::iterator anIt = aModel->mAnchors.begin(); anIt != aModel->mAnchors.end();
	     ++anIt)
		mAnchors[anIt->first] = moved(scaled(anIt->second, theShapeScale), anOffset);
	if (mAnchors.find("body") == mAnchors.end())
	{
		Shape* aBody = getShape("body");
		mAnchors["body"] = aBody ? aBody->getCenter() : Vector2f(0.0f, 0.0f);
	}
	if (mAnchors.find("mouth") == mAnchors.end())
	{
		Shape* aMouth = getShape("mouth");
		mAnchors["mouth"] = aMouth ? aMouth->getCenter() : Vector2f(0.0f, 0.0f);
	}
}

void FishVisualData::setup(FishVisualInfo* theInfo, const std::string& theAnimBase, float theSize,
                           const Color& theTint, bool inAir)
{
	mFishInfo = theInfo;
	mTint = theTint;
	mInAir = inAir;
	VisualData::setup(theInfo, theAnimBase, theSize);
	fitAnimations();
	FishModelInfo* aModel = theInfo->mFishModel;
	if (!aModel->mMenu.empty())
		setupPictures(aModel->mMenu);
}

void FishVisualData::setupPictures(const std::string& theMenu)
{
	Engine* anEngine = getEngine();
	char aPath[600];
	snprintf(aPath, sizeof(aPath), "%s.png", theMenu.c_str());
	Surface* aPicture = anEngine->getImage(aPath);
	// 0x4118d0, 0x484060 and 0x40e820 (feedingfrenzy2) with the info's values; rgbBlend leaves the picture as it is
	Ref<PixelFx> anEffect = colorEffectOf(mFishInfo, false);
	if (anEffect)
		mMenuPicture = aPicture->pixelFxCopy(*anEffect);
	else
		mMenuPicture = aPicture;
	if (!mMenuPicture)
	{
		mHudIcon = nullptr;
		mHudDangerIcon = nullptr;
		return;
	}

	// The icons are each the picture's size, cleared, anchored at their centres and drawn blended
	mHudIcon = anEngine->createSurface(mMenuPicture->getWidth(), mMenuPicture->getHeight());
	mHudIcon->fill(Color(0.0f, 0.0f, 0.0f, 0.0f));
	mHudIcon->setPivot(PIVOT_CENTER);
	mHudIcon->setBlendMode(BLEND_ALPHA, 0xff);
	Ref<DrawContext> aContext = anEngine->createDrawContext(mHudIcon);
	float aCenterX = (float)((double)mMenuPicture->getWidth() * HALF);
	float aCenterY = (float)((double)mMenuPicture->getHeight() * HALF);
	mMenuPicture->setPivot(PIVOT_CENTER);
	snprintf(aPath, sizeof(aPath), "%s_shadow.png", theMenu.c_str());
	Surface* aShadow = anEngine->getImage(aPath);
	aShadow->setPivot(PIVOT_CENTER);
	aContext->drawImage(aShadow, aCenterX, aCenterY);
	aContext->drawImage(mMenuPicture, aCenterX, aCenterY);

	mHudDangerIcon = anEngine->createSurface(mMenuPicture->getWidth(), mMenuPicture->getHeight());
	aContext = anEngine->createDrawContext(mHudDangerIcon);
	mHudDangerIcon->fill(Color(0.0f, 0.0f, 0.0f, 0.0f));
	mHudDangerIcon->setPivot(PIVOT_CENTER);
	mHudDangerIcon->setBlendMode(BLEND_ALPHA, 0xff);
	Surface* anOverlay = getApp()->mShellTheme->getImage("hudDangerOverlay");
	aContext->drawImage(aShadow, aCenterX, aCenterY);
	// 0x435510, 0x40ae20 and 0x41a660 (feedingfrenzy2): a state multiplying by the shade, opaque
	aContext->pushState();
	aContext->back().mColorMode = COLOR_MODE_MULTIPLY;
	aContext->back().mColor = Color(DANGER_SHADE, DANGER_SHADE, DANGER_SHADE, 1.0f);
	aContext->drawImage(mMenuPicture, aCenterX, aCenterY);
	aContext->popState();
	aContext->drawImage(anOverlay, DANGER_OVERLAY_X, DANGER_OVERLAY_Y);
}

void FishVisualData::loadFrames(const std::string& thePath)
{
	if (!mInAir)
	{
		std::map<std::string, int>::iterator anIdle = mAnimationIndex.find("idle");
		if (anIdle != mAnimationIndex.end())
		{
			// 0x417ec0 (feedingfrenzy2): the runs after it numbered one lower
			unsigned int anIndex = (unsigned int)anIdle->second;
			for (std::map<std::string, int>::iterator anIt = mAnimationIndex.begin(); anIt != mAnimationIndex.end();
			     ++anIt)
			{
				if (anIndex < (unsigned int)anIt->second)
					anIt->second--;
			}
			mAnimations.erase(mAnimations.begin() + anIndex);
			mAnimationIndex.erase(anIdle);
		}
	}
	AnimationData::loadFrames(thePath);
}

void FishVisualData::fitAnimations()
{
	FishModelInfo* aModel = mFishInfo->mFishModel;
	// 0x46f820 (feedingfrenzy2): a run by its index, none past the end (and so for -1)
	unsigned int anEat = (unsigned int)findAnimation("eat");
	if (anEat < mAnimations.size())
	{
		float aRate = (float)(1.0 / (double)mAnimations[anEat].mFrameTime);
		int aStart = (int)mAnimations[anEat].mStart;
		int anEnd = (int)mAnimations[anEat].mEnd;
		int anIntroEnd = aModel->mSuckFrame + aStart + 1;
		addAnimation("SuckIntro", (unsigned int)aStart, (unsigned int)anIntroEnd, aRate);
		int anOutroStart = anEnd - 1;
		if (anIntroEnd < anOutroStart)
			anOutroStart = anIntroEnd;
		addAnimation("SuckOutro", (unsigned int)anOutroStart, (unsigned int)anEnd, aRate);
	}
	unsigned int aSwim = (unsigned int)findAnimation("swim");
	if (aSwim < mAnimations.size() && 0 < aModel->mSwimShuttle)
		mAnimations[aSwim].mEnd = mAnimations[aSwim].mStart + aModel->mSwimShuttle;
	unsigned int anIdle = (unsigned int)findAnimation("idle");
	if (anIdle < mAnimations.size() && 0 < aModel->mIdleShuttle)
		mAnimations[anIdle].mEnd = mAnimations[anIdle].mStart + aModel->mIdleShuttle;
}

void FishVisualData::finish()
{
	setupShapes(mScale);
	Shape* aBody = getShape("body");
	if (mAnchors.find("halo") == mAnchors.end())
	{
		Vector2f aHalo(0.0f, 0.0f);
		if (aBody)
		{
			Vector2f aCenter = aBody->getCenter();
			aHalo.x = (float)((double)aCenter.x - (double)aBody->getWidth() * (double)HALO_WIDTH_SHARE);
			aHalo.y = (float)((double)aCenter.y - (double)aBody->getHeight() * (double)HALO_HEIGHT_SHARE);
		}
		mAnchors["halo"] = aHalo;
	}
	if (mAnchors.find("idea") == mAnchors.end())
	{
		Vector2f anIdea(0.0f, 0.0f);
		if (aBody)
		{
			Vector2f aCenter = aBody->getCenter();
			anIdea.x = (float)((double)aCenter.x - (double)aBody->getWidth() * (double)IDEA_WIDTH_SHARE);
			anIdea.y = (float)((double)aCenter.y - (double)aBody->getHeight() * (double)IDEA_HEIGHT_SHARE);
		}
		mAnchors["idea"] = anIdea;
	}
	if (mAnchors.find("tail") == mAnchors.end())
	{
		Vector2f aTail(0.0f, 0.0f);
		Shape* aTailShape = getShape("tail");
		Shape* aVisual = getShape("visual");
		if (aTailShape)
			aTail = aTailShape->getCenter();
		else if (aVisual)
			aTail = aVisual->getRight();
		mAnchors["tail"] = aTail;
	}
	mMouth = getAnchor("mouth");
}
