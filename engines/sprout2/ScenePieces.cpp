#include "ScenePieces.h"

namespace engine
{

void ClipPane::applyTransform(DrawContext* theContext)
{
	Container::applyTransform(theContext);
	RectI aRect((int)mClip.left, (int)mClip.top, (int)mClip.right, (int)mClip.bottom);
	theContext->pushClip(aRect, CLIP_INTERSECT);
	theContext->back().mClipCount = theContext->getClipCount();
}

void RectangleItem::paint(DrawContext* theContext)
{
	theContext->back().mColor = mColor;
	RectI aRect((int)mRect.left, (int)mRect.top, (int)mRect.right, (int)mRect.bottom);
	theContext->fillRect(aRect);
}

} // namespace engine
