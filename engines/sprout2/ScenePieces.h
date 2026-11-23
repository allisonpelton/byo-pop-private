#ifndef __TALISMANIA_SCENEPIECES_H__
#define __TALISMANIA_SCENEPIECES_H__

// Two components of the engine that Talismania's scene builds its layers from and Feeding Frenzy 2 does not use: the
// clipping container (engine::ClipPane, 0xe4 bytes, constructor 0x64d7b0) and the filled rectangle (RectangleItem,
// 0xb4 bytes, constructor 0x61f0d0). Shared in engines/sprout2 (Talismania's addresses; Feeding Frenzy 2 has none for either and does not use them).

#include "Container.h"
#include "DrawContext.h"

namespace engine
{

// A container whose children are cut to a rectangle
class ClipPane : public Container
{
public:
	// 0x64d7b0: the rectangle empty; the component is animated-state flagged (0x4) as the constructor sets
	ClipPane() : mClip(0, 0, 0, 0) {}

	// The rectangle (the layer's clip), left, top, right, bottom in the parent's pixels (vf 0xec)
	void setClip(const RectF& theClip) { mClip = theClip; }
	// 0x64d910 (slot 0x6c): the transform applied as a component's, then the rectangle, its edges truncated, cut into
	// the clip region and the state's clip count set to the stack's size
	void applyTransform(DrawContext* theContext) override;
	std::string getClassName() override { return "Container"; }

private:
	RectF mClip;  // +0xcc
};

// A rectangle filled with the component's colour
class RectangleItem : public Component
{
public:
	// 0x61f0d0: the rectangle left, top, right and bottom; a component that paints is drawn (STATE_VISIBLE)
	explicit RectangleItem(const RectF& theRect) : mRect(theRect) { addOwnState(STATE_VISIBLE); }

	// 0x61f070
	std::string getClassName() override { return "RectangleItem"; }
	// 0x61f230 (slot 0x8c): the state's colour set to the component's, the rectangle (edges truncated) filled
	void paint(DrawContext* theContext) override;
	void computeBounds() override { mBounds = mRect; }

private:
	RectF mRect;  // +0x7c
};

} // namespace engine

#endif
