#ifndef __SPROUT2_IMAGE_H__
#define __SPROUT2_IMAGE_H__

// A component that draws a surface, whole or a part of it (engine::Image, vtable 0x56020c (feedingfrenzy2), 0xb8
// bytes; its constructor 0x453980 (feedingfrenzy2)). The surface's anchor is the image's pivot: the point drawn at the
// component's position.

#include "Component.h"
#include "Surface.h"

namespace engine
{

class Image : public Component
{
public:
	// 0x453980 (feedingfrenzy2): named after the surface, visible, its bounds to be computed
	explicit Image(Surface* theSurface);

	// 0x68 (0x51fea2 (feedingfrenzy2))
	std::string getClassName() override { return "Image"; }
	// 0xa0 (0x4437e0 (feedingfrenzy2)): the part, or the whole surface, at the state's origin
	void paint(DrawContext* theContext) override;
	// 0xa8 (0x4472c0 (feedingfrenzy2)): the surface's rectangle around the anchor from the position, all of it times
	// the scale's size (the position too)
	void computeBounds() override;
	// (0xb0, 0x4899f0 (feedingfrenzy2), a part of the surface drawn through the surface's slot 0x50, is left out until
	// a caller of a component's paintRegion is translated)

	Surface* getSurface() const { return mSurface; }
	// 0x40ca20 and 0x475d80 (feedingfrenzy2): the surface's size, 0 without one
	int getWidth() const { return mSurface != nullptr ? mSurface->getWidth() : 0; }
	int getHeight() const { return mSurface != nullptr ? mSurface->getHeight() : 0; }
	// Inlined where the original uses it (the splash's load bar): only the part drawn
	void setSource(const RectI& theSource)
	{
		mUseSource = true;
		mSource = theSource;
	}

private:
	RectI mSource;             // +0x98: the part of the surface drawn when mUseSource is set
	bool mUseSource;           // +0xa8
	Ref<Surface> mSurface;     // +0xac
};

} // namespace engine

#endif
