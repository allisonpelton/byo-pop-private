#ifndef __WORDHARMONY_ENGINE_IMAGENODE_H__
#define __WORDHARMONY_ENGINE_IMAGENODE_H__

// A node that draws an image (0x94 bytes, constructor 0x435d50, vtable 0x484858)

#include "Graphics.h"
#include "Node.h"

namespace engine
{

class ImageNode : public Node
{
public:
	ImageNode(Image* theImage);  // 0x435d50
	~ImageNode() override;       // 0x435ec0

	void draw(Graphics* g) override;  // 0x6c: 0x435d20
	void layout() override;           // 0x74: 0x435c60

	Ref<Image> mImage;  // +0x88
};

} // namespace engine

#endif
