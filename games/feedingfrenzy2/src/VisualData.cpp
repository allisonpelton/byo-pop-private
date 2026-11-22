#include "VisualData.h"

#include "ActorInfo.h"
#include "Engine.h"
#include "PixelFx.h"
#include "Surface.h"

#include <cstdio>

using namespace engine;

namespace
{

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
	// The original goes on with a frame that did not load; the port keeps nothing for it
	if (!aFrame)
		return;
	// The visual's colour effect, put on a copy before it is scaled (the surface's slot 0x34, 0x426740
	// (feedingfrenzy2)): the colour ones (0x481ec0 and 0x40e820 (feedingfrenzy2): rgbAdd in mode 1, rgbMultiply in
	// mode 2 at an alpha of 1, rgbBlend in mode 3), HSVShiftFx (0x412db0 (feedingfrenzy2)) for hsvAdd and
	// RainbowShiftFx (0x408c80 (feedingfrenzy2)) for rainbow
	int anEffect = mInfo->mColorEffect;
	if (anEffect == COLOR_EFFECT_RGB_BLEND)
	{
		const float* aParams = mInfo->mColorParams;
		ColorFx anFx(Color(aParams[0], aParams[1], aParams[2], aParams[3]), COLOR_FX_BLEND);
		Ref<Surface> aCopy = aFrame->clone();
		aCopy->applyColorFx(anFx);
		aCopy->scale(mScale, mScale);
		aFrame = aCopy;
	}
	else if (anEffect >= COLOR_EFFECT_HSV_ADD && anEffect <= COLOR_EFFECT_RAINBOW)
	{
		getEngine()->log("VisualData: %s's colour effect %d not yet translated\n", mInfo->mName.c_str(), anEffect);
		Ref<Surface> aCopy = aFrame->clone();
		aCopy->scale(mScale, mScale);
		aFrame = aCopy;
	}
	// 0x426990 (feedingfrenzy2): without an effect, a scaled copy unless the scale is 1 and there is no tint
	else if (mScale != 1.0f || 0.0f < mTint.a)
		aFrame = aFrame->scaledCopy(mScale);
	if (0.0f < mTint.a)
		aFrame->applyColorFx(ColorFx(mTint, COLOR_FX_BLEND));
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
	// 0x427490 (feedingfrenzy2), for a model with a menu picture: "<menu>.png" through the visual's effect, and the
	// HUD's icons drawn from it with "<menu>_shadow.png" and the hudDangerOverlay (+0x84 to +0x8c): not yet translated
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
