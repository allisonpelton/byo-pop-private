#ifndef __SPROUT2_IMAGEBUTTON_H__
#define __SPROUT2_IMAGEBUTTON_H__

// A button drawn with three images, normal, high (the mouse over it) and down (ImageButton, vtable 0x55d0b4
// (feedingfrenzy2) with 88 slots, 0x118 bytes; its constructor 0x482ad0 (feedingfrenzy2)): a screen's <imageButton>.
// The images are a selector's children; a missing high image is the normal one brightened, and a missing down image
// the high one, or the normal one brightened more. Its flash fades between the normal and the high image.

#include "ButtonBase.h"

namespace engine
{

class Selector;
class Surface;

class ImageButton : public ButtonBase
{
public:
	// 0x482ad0 (feedingfrenzy2): no images
	ImageButton() : mImages(nullptr) {}

	// 0x68 (0x5205fb (feedingfrenzy2))
	std::string getClassName() override { return "ImageButton"; }
	// 0x13c (0x40e9d0 (feedingfrenzy2)): the state's image, at once
	void updateLook() override;
	// 0x154 (0x501077 (feedingfrenzy2)): at a half's end, a fifth of a second's fade to the normal image or the high
	void applyFlash(float theFlash) override;
	// (0x158 (0x50106f (feedingfrenzy2)), the state's look again, is not called in feedingfrenzy2)

	// 0x15c (0x42ff90 (feedingfrenzy2)): the images, each surface's anchor put at the button's pivot; the button's
	// bounds are the normal image's as they are made. False without a normal image
	virtual bool setImages(Surface* theNormal, Surface* theHigh, Surface* theDown);

private:
	Selector* mImages;  // +0x10c: a child, made by the first setImages
};

} // namespace engine

#endif
