#ifndef __SPROUT_RECTANGLEITEM_H__
#define __SPROUT_RECTANGLEITEM_H__

// The signal build's filled rectangle (Pizza Frenzy's; 0x110 bytes, constructor 0x483950 (pizzafrenzy), vtable
// 0x503ab0 (pizzafrenzy); its name is "RectangleItem"): a node that fills a rectangle of its own, in the parent's
// coordinates, with the node's colour. It lays out no bounds.

#include "Font.h"
#include "Graphics.h"
#include "Node.h"

namespace engine
{

class RectangleItem : public Node
{
public:
	// 0x483950 (pizzafrenzy): the rectangle (left, top, right, bottom), and the node draws
	RectangleItem(float theLeft, float theTop, float theRight, float theBottom);

	// 0x60: 0x483a60 (pizzafrenzy)
	std::string getName() override { return "RectangleItem"; }
	// 0x90: 0x4838b0 (pizzafrenzy): the colour of the node, and the rectangle's whole pixels filled with it
	void draw(Graphics* g) override;
	// 0x98: 0x4d0470 (pizzafrenzy): nothing
	void layout() override {}

	// 0x483880 (pizzafrenzy)
	void setRect(float theLeft, float theTop, float theRight, float theBottom)
	{
		mRectangle.set(theLeft, theTop, theRight, theBottom);
	}

private:
	Rect mRectangle;  // +0x78
};

} // namespace engine

#endif
