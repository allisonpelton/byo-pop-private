#ifndef __SPROUT_IMAGENODE_H__
#define __SPROUT_IMAGENODE_H__

// A node that draws an image (0x94 bytes, constructor 0x435d50 (wordharmony), vtable 0x484858 (wordharmony))

#include "Graphics.h"
#include "Node.h"

namespace engine
{

class ImageNode : public Node
{
public:
	ImageNode(Image* theImage);  // 0x435d50 (wordharmony)
	~ImageNode() override;       // 0x435ec0 (wordharmony)

	void draw(Graphics* g) override;  // 0x6c: 0x435d20 (wordharmony)
	void layout() override;           // 0x74: 0x435c60 (wordharmony)

	Ref<Image> mImage;  // +0x88
};

} // namespace engine

#endif
