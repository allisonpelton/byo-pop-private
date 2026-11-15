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

	Ref<Image> mImage;  // +0x88; +0x11c (pizzafrenzy)
	Rect mClipRect;     // +0x108 (pizzafrenzy)
	bool mClipped;      // +0x118 (pizzafrenzy)
};

} // namespace engine

#endif
