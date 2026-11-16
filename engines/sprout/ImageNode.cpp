#include "ImageNode.h"
#include "Font.h"
#include <cmath>

using namespace engine;

// 0x435d50 (wordharmony), 0x470fd0 (pizzafrenzy): the signal build's has no name of its own
ImageNode::ImageNode(Image* theImage)
{
	mClipped = false;
	mImage = theImage;
	addOwnFlags(NODE_DRAWABLE);
	setFlags(NODE_LAYOUT);
	if (!gSignalBuild)
		setName("Image");
}

// 0x470d20 (pizzafrenzy)
ImageNode::ImageNode()
{
	mClipped = false;
	addOwnFlags(NODE_DRAWABLE);
}

// 0x435ec0 (wordharmony), 0x470dd0 (pizzafrenzy)
ImageNode::~ImageNode()
{
}

// 0x435d20 (wordharmony), 0x470f20 (pizzafrenzy): the drawable flag is cleared after drawing, which the own flag keeps
// from having any effect. The signal build draws the clip rectangle of the image when it has one set, and nothing
// without an image.
void ImageNode::draw(Graphics* g)
{
	if (gSignalBuild)
	{
		if (isVisible() && mImage != nullptr)
		{
			if (mClipped)
				g->drawImage(mImage, IntRect((int)mClipRect.left, (int)mClipRect.top, (int)mClipRect.right,
					(int)mClipRect.bottom), 0, 0);
			else
				g->drawImage(mImage);
		}
		clearFlags(NODE_DRAWABLE);
		return;
	}
	if (mVisible)
		g->drawImage(mImage);
	clearFlags(NODE_DRAWABLE);
}

// 0x435c60 (wordharmony), 0x470e40 (pizzafrenzy): the image's rectangle less its pivot, at the position, scaled by the
// scale's size (after the offset). The signal build's has no rectangle without an image.
void ImageNode::layout()
{
	Image* anImage = mImage;
	if (gSignalBuild && anImage == nullptr)
	{
		mBounds = Rect();
		mBounds.scale(fabsf(mScale.x), fabsf(mScale.y));
		clearFlags(NODE_LAYOUT);
		return;
	}
	int aPivotX = anImage->getPivotX();
	float aBottom = ((float)anImage->getHeight() + mPosition.y) - (float)anImage->getPivotY();
	float aRight = ((float)anImage->getWidth() + mPosition.x) - (float)aPivotX;
	mBounds.set(mPosition.x - (float)aPivotX, mPosition.y - (float)anImage->getPivotY(), aRight, aBottom);
	mBounds.scale(fabsf(mScale.x), fabsf(mScale.y));
	clearFlags(NODE_LAYOUT);
}
