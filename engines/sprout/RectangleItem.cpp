#include "RectangleItem.h"

using namespace engine;

// 0x483950 (pizzafrenzy)
RectangleItem::RectangleItem(float theLeft, float theTop, float theRight, float theBottom) :
	mRectangle(theLeft, theTop, theRight, theBottom)
{
	addOwnFlags(NODE_DRAWABLE);
}

// 0x4838b0 (pizzafrenzy)
void RectangleItem::draw(Graphics* g)
{
	g->setColor(mColor.r, mColor.g, mColor.b, mColor.a);
	g->fillRect(Rect((float)(int)mRectangle.left, (float)(int)mRectangle.top, (float)(int)mRectangle.right,
		(float)(int)mRectangle.bottom));
}
