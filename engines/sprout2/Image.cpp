#include "Image.h"

#include "DrawContext.h"

#include <cmath>

namespace engine
{

Image::Image(Surface* theSurface) : mUseSource(false), mSurface(theSurface)
{
	setName(theSurface->getName());
	addState(STATE_BOUNDS_DIRTY);
	mOwnStates |= STATE_VISIBLE;
	addState(STATE_VISIBLE);
}

void Image::paint(DrawContext* theContext)
{
	if (isVisible() && mSurface)
	{
		if (mUseSource)
		{
			theContext->drawImage(mSurface, mSource, 0.0f, 0.0f);
			removeState(STATE_VISIBLE);
			return;
		}
		theContext->drawImage(mSurface);
	}
	removeState(STATE_VISIBLE);
}

void Image::computeBounds()
{
	if (!mSurface)
	{
		mBounds.left = 0;
		mBounds.top = 0;
		mBounds.right = 0;
		mBounds.bottom = 0;
	}
	else
	{
		const Vector2f& anAnchor = mSurface->getAnchor();
		mBounds.left = mPosition.x - anAnchor.x;
		mBounds.top = mPosition.y - anAnchor.y;
		// The x87 keeps the sums at double precision until they are stored
		mBounds.right = (float)(((double)mSurface->getWidth() + mPosition.x) - anAnchor.x);
		mBounds.bottom = (float)(((double)mSurface->getHeight() + mPosition.y) - anAnchor.y);
	}
	float aScaleX = fabsf(mScale.x);
	float aScaleY = fabsf(mScale.y);
	mBounds.left = mBounds.left * aScaleX;
	mBounds.right = mBounds.right * aScaleX;
	mBounds.top = mBounds.top * aScaleY;
	mBounds.bottom = mBounds.bottom * aScaleY;
	removeState(STATE_BOUNDS_DIRTY);
}

} // namespace engine
