#ifndef __SPROUT2_DXDRAWCONTEXT_H__
#define __SPROUT2_DXDRAWCONTEXT_H__

// The Windows driver's draw context (dxdriver::DXDrawContext, vtables 0x5615bc and 0x5615c4 (feedingfrenzy2)): it
// draws into a surface in memory, for a frame the display's back buffer (0x438700 (feedingfrenzy2)), with the engine's
// own blitters, MMX code over 32-bit ARGB pixels whose colours are multiplied by their alpha.
//
// An image goes through one blitter per rectangle of the clip region (the whole target while the clip stack is
// empty), chosen by the state's colour mode and the image's blend mode. The fills, outlines, lines and circles ignore
// the clip region and stop at the target's edges.
//
// port: the first draft has the dispatch, the geometry of unrotated images and the blend of an image drawn whole
// pixels apart at its own size (0x444ec0 (feedingfrenzy2)) as the original computes them; scaled, rotated and
// sub-pixel images, and the multiply, add, pixel effect and colour key blends, are drawn by a plain sampler whose
// pixels may differ from the original's. Each such place says which blitter the looks pass is to translate.

#include "DrawContext.h"

namespace engine
{

class DXDrawContext : public DrawContext
{
public:
	// The frame's context (built in 0x438700 (feedingfrenzy2)) over the surface it draws into
	explicit DXDrawContext(Surface* theTarget);
	// 0x411c20 (feedingfrenzy2)
	~DXDrawContext() override;

	using DrawContext::drawImage;
	using DrawContext::drawRect;
	using DrawContext::fillRect;

	// 0x00 (0x43f550 (feedingfrenzy2))
	void drawImage(Surface* theImage, const RectI& theSource, float x, float y) override;
	// 0x18 (0x513cd5 (feedingfrenzy2))
	void drawRect(const RectI& theRect) override;
	// 0x20 (0x44ef30 (feedingfrenzy2))
	void fillRect(const RectI& theRect) override;
	// 0x28 (0x46c520 (feedingfrenzy2))
	void drawLine(const Vector2f& theFrom, const Vector2f& theTo) override;
	// 0x2c (0x51b5c8 (feedingfrenzy2))
	void drawCircle(const Vector2f& theCenter, float theRadius) override;
	// 0x30 (0x435990 (feedingfrenzy2))
	void darkenByAlpha() override;
	// 0x34 (0x513cb9 (feedingfrenzy2))
	void clear() override;

	Surface* getTarget() const { return mTarget; }

private:
	// What an image's pixels do to the target's
	enum ImageBlend
	{
		IMAGE_BLEND_COPY,       // 0x452450 and 0x44fc40 (feedingfrenzy2): an opaque image copied
		IMAGE_BLEND_ALPHA,      // 0x441810 and 0x442740 (feedingfrenzy2): blended by its alpha, scaled by the opacity
		IMAGE_BLEND_MULTIPLY,   // 0x440910 (feedingfrenzy2): its colours multiplied by the state's
		IMAGE_BLEND_ADD,        // 0x4609d0 (feedingfrenzy2): the state's colour added to its own
		IMAGE_BLEND_PIXEL_FX,   // 0x418190 (feedingfrenzy2): through the state's pixel effect
		IMAGE_BLEND_COLOR_KEY,  // 0x539fb7 (feedingfrenzy2): an image of blend mode 1
		IMAGE_BLEND_MODE_3      // 0x44cfd0 (feedingfrenzy2): an image of blend mode 3, with the state's pixel effect
	};

	// One image drawn, as drawImage passes it to each clip rectangle's blitter
	struct ImageDraw
	{
		Surface* mImage;
		RectI mSource;
		float mX;              // where the anchor goes on the target
		float mY;
		float mScaleX;
		float mScaleY;
		float mRotation;       // radians
		int mFilter;           // 0 placed at whole pixels, 0xf filtered (the state's filter mode)
		ImageBlend mBlend;
		int mAlphaScale;       // the opacity as the blitters scale by it (0x48ca50 (feedingfrenzy2))
		uint32_t mColor;       // the state's colour as bytes, for the multiply and add blends
	};

	// 0x441810 (feedingfrenzy2) and its siblings: the image drawn inside one rectangle of the clip region
	void blitImage(const ImageDraw& theDraw, const RectI& theClip);
	// The port's sampler for what the first draft does not draw the original's way: each target pixel inside the
	// image's transformed rectangle takes the image's pixel under its centre (or, filtered, the four around it)
	void sampleImage(const ImageDraw& theDraw, const RectI& theClip);

	Ref<Surface> mTarget;   // +0x58
};

} // namespace engine

#endif
