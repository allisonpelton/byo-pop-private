#include "ImageNode.h"
#include "Font.h"
#include <cmath>

using namespace engine;

// 0x435d50
ImageNode::ImageNode(Image* theImage)
{
	mImage = theImage;
	addOwnFlags(NODE_DRAWABLE);
	setFlags(NODE_LAYOUT);
	setName("Image");
}

// 0x435ec0
ImageNode::~ImageNode()
{
}

// 0x435d20: the drawable flag is cleared after drawing, which the own flag keeps from having any effect
void ImageNode::draw(Graphics* g)
{
	if (mVisible)
		g->drawImage(mImage);
	clearFlags(NODE_DRAWABLE);
}

// 0x435c60: the image's rectangle less its pivot, at the position, scaled by the scale's size (after the offset)
void ImageNode::layout()
{
	Image* anImage = mImage;
	int aPivotX = anImage->getPivotX();
	float aBottom = ((float)anImage->getHeight() + mPosition.y) - (float)anImage->getPivotY();
	float aRight = ((float)anImage->getWidth() + mPosition.x) - (float)aPivotX;
	mBounds.set(mPosition.x - (float)aPivotX, mPosition.y - (float)anImage->getPivotY(), aRight, aBottom);
	mBounds.scale(fabsf(mScale.x), fabsf(mScale.y));
	clearFlags(NODE_LAYOUT);
}
