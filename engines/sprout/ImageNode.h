#ifndef __SPROUT_IMAGENODE_H__
#define __SPROUT_IMAGENODE_H__

// A node that draws an image (0x94 bytes, constructor 0x435d50 (wordharmony), vtable 0x484858 (wordharmony); 0x128
// bytes, constructor 0x470fd0 (pizzafrenzy), vtable 0x5022d8 (pizzafrenzy)). The signal build's can draw a part of its
// image (a clip) and tells the image's size.

#include "Font.h"
#include "Graphics.h"
#include "Node.h"

namespace engine
{

class ImageNode : public Node
{
public:
	ImageNode(Image* theImage);  // 0x435d50 (wordharmony), 0x470fd0 (pizzafrenzy)
	~ImageNode() override;       // 0x435ec0 (wordharmony)

	void draw(Graphics* g) override;  // 0x6c: 0x435d20 (wordharmony); 0x90: 0x470f20 (pizzafrenzy)
	void layout() override;           // 0x74: 0x435c60 (wordharmony); 0x98: 0x470e40 (pizzafrenzy)

	// The signal build's (Pizza Frenzy's)
	// 0x470ce0 (pizzafrenzy): whether the node draws only the clip rectangle of its image (the original's setClip)
	void setClipped(bool theClipped) { mClipped = theClipped; }
	// 0x470cf0 (pizzafrenzy): the part of the image the clip draws (whole pixels when it draws)
	void setClipRect(const Rect& theRect) { mClipRect = theRect; }
	// 0x470f80 (pizzafrenzy), 0x470fa0 (pizzafrenzy): the image's size, 0 for no image
	int getImageWidth() { return mImage != nullptr ? mImage->getWidth() : 0; }
	int getImageHeight() { return mImage != nullptr ? mImage->getHeight() : 0; }
	// Slot 0x50 (0x467be0 (pizzafrenzy)) of the signal build's node, which keeps it at +0x64 and has the scene apply it
	// to the render state when it draws (0x467fd0 (pizzafrenzy), 0x478190 (pizzafrenzy)): 1 smooths the image when it is
	// scaled. It belongs to the node (Node.h); until that applies it, the image node keeps it.
	void setSmoothType(int theSmoothType) { mSmoothType = theSmoothType; }
	int getSmoothType() const { return mSmoothType; }
	// Slot 0x48 (0x467bd0 (pizzafrenzy), +0x60) of the signal build's node, like the text's: 1 adds the colour offset to
	// the image's colours (the lightened over and down images of a button), 2 sets the colour
	void setColorMode(int theMode) { mColorMode = theMode; }

	Ref<Image> mImage;  // +0x88; +0x11c (pizzafrenzy)
	Rect mClipRect;     // +0x108 (pizzafrenzy)
	bool mClipped;      // +0x118 (pizzafrenzy)
	int mSmoothType;    // +0x64 of the signal build's node
	int mColorMode;     // +0x60 of the signal build's node
};

} // namespace engine

#endif
