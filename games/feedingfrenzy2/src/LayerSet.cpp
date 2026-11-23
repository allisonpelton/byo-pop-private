#include "LayerSet.h"

#include "Engine.h"
#include "Params.h"
#include "VectorMath.h"

using namespace engine;

namespace
{

// 0x5083da (feedingfrenzy2): a colour of four bytes
Color colorOfBytes(int theRed, int theGreen, int theBlue, int theAlpha)
{
	return Color(channelOf(theRed), channelOf(theGreen), channelOf(theBlue), channelOf(theAlpha));
}

// 0x45b6b0 (feedingfrenzy2): a string compared with a literal
bool equals(const std::string& theString, const char* theLiteral)
{
	return theString == theLiteral;
}

} // namespace

LayerInfo::LayerInfo()
	: mColor(1.0f, 1.0f, 1.0f, 1.0f), mBlur(0.0f), mPanSize(0.0f, 0.0f), mScrollRate(0.0f), mColorMode(LAYER_COLOR_NONE)
{
}

void LayerInfo::read(Params& theAttributes)
{
	mColor = theAttributes.getColor("color", colorOfBytes(0, 0, 0, 0));
	std::string aColorMode = theAttributes.getString("colorMode", "blend");
	if (equals(aColorMode, "add"))
		mColorMode = LAYER_COLOR_ADD;
	else if (equals(aColorMode, "multiply"))
		mColorMode = LAYER_COLOR_MULTIPLY;
	else
		// 0x478120 (feedingfrenzy2)
		mColorMode = equals(aColorMode, "blend") ? LAYER_COLOR_BLEND : LAYER_COLOR_NONE;
	mBlur = theAttributes.getFloat("blur", 0.0f);
	mPanSize = theAttributes.getFloatPair("panSize", Vector2f(0.0f, 0.0f));
	mScrollRate = theAttributes.getFloat("scrollRate", 0.0f);
}

void LayoutPanInfo::read(Params& theAttributes)
{
	LayerInfo::read(theAttributes);
	mLayoutPath = theAttributes.getString("layout", "");
}

Layer* LayoutPanInfo::createLayer(float theWaterLevel)
{
	return new LayoutPan(this, theWaterLevel);
}

LayoutPan::LayoutPan(LayoutPanInfo* theInfo, float theWaterLevel)
{
	mInfo = theInfo;
	mPlayers = new Container();
	mPlayers->setName("PanLayerPlayers");
	mActors = new Container();
	mActors->setName("PanLayerActors");
	mOverlays = new Container();
	mOverlays->setName("PanLayerOverlays");
	init(theWaterLevel);
}

void LayoutPan::init(float theWaterLevel)
{
	if (mLayout != nullptr)
		// 0x4f17cc (feedingfrenzy2): the layout read before has its children taken out
		mLayout->removeAllChildren();
	mLayout = new ScreenLayout();
	mLayout->load(mInfo->getLayoutPath());
	if (0.0f < mInfo->getColor().a)
	{
		// The info's colour over the water (ColorFx, 0x40e820 (feedingfrenzy2), in its colour mode, over the
		// rectangle from 100 left of the layer at the water level to 100 past the pan size (0x5668a8 and 0x566ca8),
		// added through the layout's slot 0xb0): not yet translated
		getEngine()->log("LayoutPan: the layer's colour not yet translated\n");
	}
	mLayout->addChild(mPlayers.get());
	mLayout->addChild(mActors.get());
	mLayout->addChild(mOverlays.get());
}

void LayoutPan::pan(const Vector2f& theCameraPosition)
{
	// 0x43dc80 and 0x43c1a0 (feedingfrenzy2)
	Vector2f aPosition(theCameraPosition.x * mPanFactor.x, theCameraPosition.y * mPanFactor.y);
	aPosition = Vector2f(aPosition.x * -1.0f, -1.0f * aPosition.y);
	mLayout->setPosition(aPosition);
}

LayerSet::LayerSet()
	: mBaseLayer(0), mWaterLevel(0), mSkyCeiling(0), mCameraPosition(0.0f, 0.0f), mPanPosition(0.0f, 0.0f),
	  mNightColorLight(colorOfBytes(0xff, 0xff, 0xff, 0xff)), mNightColorDark(colorOfBytes(0xff, 0xff, 0xff, 0xff)),
	  mNightColorLightPct(1.0f)
{
}

void LayerSet::load(const std::string& thePath)
{
	MasterDocHandler aMaster;
	LayerSetDocHandler* aHandler = new LayerSetDocHandler(this);
	aHandler->attach(&aMaster, "set");
	if (!getEngine()->parseXml(thePath, &aMaster))
		return;
	mLayers.resize(mInfos.size());
	for (size_t i = 0; i < mInfos.size(); i++)
	{
		Layer* aLayer = mInfos[i]->createLayer((float)mWaterLevel);
		mLayers[i] = aLayer;
		addChild(aLayer->getComponent());
	}
}

void LayerSet::fit(float theMarginX, float theMarginY, const RectF& theBounds)
{
	mCameraLimits.left = theBounds.left + theMarginX;
	mCameraLimits.right = theBounds.right - theMarginX;
	mCameraLimits.top = theBounds.top + theMarginY;
	mCameraLimits.bottom = theBounds.bottom - theMarginY;
	mCameraPosition = Vector2f(0.0f, 0.0f);
	const Vector2f& aBaseSize = mInfos[mBaseLayer]->getPanSize();
	// 0x459e00 and 0x459670 (feedingfrenzy2): the bounds' width and height
	float aViewWidth = (float)((double)theBounds.right - theBounds.left);
	float aViewHeight = (float)((double)theBounds.bottom - theBounds.top);
	float aRangeX = (float)((double)aBaseSize.x - aViewWidth);
	float aRangeY = (float)((double)aBaseSize.y - aViewHeight);
	mPanRange = RectF(0.0f, 0.0f, aRangeX, aRangeY);
	for (size_t i = 0; i < mLayers.size(); i++)
	{
		const Vector2f& aSize = mInfos[i]->getPanSize();
		float aFactorX = aRangeX == 0.0f ? 1.0f : (float)(((double)aSize.x - aViewWidth) / aRangeX);
		float aFactorY = aRangeY == 0.0f ? 1.0f : (float)(((double)aSize.y - aViewHeight) / aRangeY);
		mLayers[i]->setPanFactor(Vector2f(aFactorX, aFactorY));
	}
}

void LayerSet::setCamera(const Vector2f& thePosition)
{
	// 0x42b890 (feedingfrenzy2): each coordinate held between the range's ends, the low end first
	float aY = thePosition.y;
	if (mPanRange.top > aY)
		aY = mPanRange.top;
	else if (mPanRange.bottom < aY)
		aY = mPanRange.bottom;
	float aX = thePosition.x;
	if (mPanRange.left > aX)
		aX = mPanRange.left;
	else if (mPanRange.right < aX)
		aX = mPanRange.right;
	mPanPosition = Vector2f(aX, aY);
	for (size_t i = 0; i < mLayers.size(); i++)
		mLayers[i]->pan(mPanPosition);
}

void LayerSet::followCamera(const Vector2f& theTarget, float theElapsed)
{
	// The target in the scene's space less the camera's position (0x43c1a0, 0x43c1c0 and 0x43aa30 (feedingfrenzy2))
	Vector2f aDistance = scaled(theTarget, 1.0f);
	aDistance = difference(aDistance, mPanPosition);
	Vector2f anOrigin = getAbsolutePosition();
	aDistance.x += anOrigin.x;
	aDistance.y += anOrigin.y;
	// Only what lies outside the limits moves the camera
	Vector2f aMove;
	if (mCameraLimits.left > aDistance.x)
		aMove.x = aDistance.x - mCameraLimits.left;
	else if (mCameraLimits.right < aDistance.x)
		aMove.x = aDistance.x - mCameraLimits.right;
	if (mCameraLimits.top > aDistance.y)
		aMove.y = aDistance.y - mCameraLimits.top;
	else if (mCameraLimits.bottom < aDistance.y)
		aMove.y = aDistance.y - mCameraLimits.bottom;
	float aSpeed = (float)((double)length(aMove) * FOLLOW_SPEED_PER_DISTANCE + FOLLOW_SPEED);
	float aMaxStep = aSpeed * theElapsed;
	clampLength(aMove, aMaxStep);
	// The base layer must be one of the layers (else the cold path 0x4e05fc (feedingfrenzy2), an assertion)
	if (mBaseLayer >= (int)mLayers.size())
		return;
	mPanPosition.x = clampOf(mPanPosition.x + aMove.x, mPanRange.left, mPanRange.right);
	mPanPosition.y = clampOf(mPanPosition.y + aMove.y, mPanRange.top, mPanRange.bottom);
	for (size_t i = 0; i < mLayers.size(); i++)
		mLayers[i]->pan(mPanPosition);
}

void LayerSetDocHandler::begin(Params& theAttributes)
{
	int aBaseLayer = theAttributes.getInt("baseLayer", 0);
	int aWaterLevel = theAttributes.getInt("waterLevel", 0);
	int aSkyCeiling = theAttributes.getInt("skyCeiling", 0);
	Color aDark = theAttributes.getColor("nightColorDark", colorOfBytes(0x80, 0x80, 0x80, 0xff));
	Color aLight = theAttributes.getColor("nightColorLight", colorOfBytes(0xff, 0xff, 0xff, 0xff));
	mSet->mNightColorLightPct = theAttributes.getFloat("nightColorLightPct", 1.0f);
	mSet->mNightColorDark = aDark;
	mSet->mNightColorLight = aLight;
	mSet->mBaseLayer = aBaseLayer;
	mSet->mWaterLevel = aWaterLevel;
	mSet->mSkyCeiling = aSkyCeiling;
}

void LayerSetDocHandler::startElement(const std::string& theName, Params& theAttributes)
{
	if (!equals(theName, "layer"))
		return;
	std::string aType = theAttributes.getString("type", "layoutPan");
	LayerInfo* anInfo = nullptr;
	if (equals(aType, "layoutPan"))
		anInfo = new LayoutPanInfo();
	else
	{
		// panelScroll and propScroll (PanelScrollInfo 0x51d044 and PropScrollInfo 0x52ca41 (feedingfrenzy2), with
		// their handlers): not yet translated; any other type is skipped as the original skips it
		if (equals(aType, "panelScroll") || equals(aType, "propScroll"))
			getEngine()->log("LayerSet: the layer type %s not yet translated\n", aType.c_str());
		return;
	}
	LayerInfoDocHandler* aHandler = new LayerInfoDocHandler(anInfo);
	// 0x405e60 (feedingfrenzy2)
	mSet->mInfos.push_back(anInfo);
	// 0x4194a0 (feedingfrenzy2)
	aHandler->attach(mMaster, theName);
	aHandler->begin(theAttributes);
}

Vector2f LayerSet::getLayerOffset(int theLayer)
{
	Vector2f aPosition = mLayers[mBaseLayer + theLayer]->getPosition();
	Vector2f aBase = mLayers[mBaseLayer]->getPosition();
	return Vector2f((float)((double)aPosition.x - aBase.x), (float)((double)aPosition.y - aBase.y));
}
