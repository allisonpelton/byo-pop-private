#ifndef __SPROUT2_COMBOBUTTON_H__
#define __SPROUT2_COMBOBUTTON_H__

// A button drawn with three images and a text over them (ComboButton, vtable 0x55fc1c (feedingfrenzy2) with 98 slots,
// 0x15c bytes; its constructor 0x482770 (feedingfrenzy2)): a screen's <comboButton>. Each state has its image, its
// font and its colour for the text; the images are a selector's children, and a missing high image is the normal one
// brightened, a missing down image the high one or the normal one brightened more, as for the image button. The text
// is the selector's sibling, above it.

#include "ButtonBase.h"
#include "Font.h"

namespace engine
{

class Selector;
class Surface;
class TextItem;

class ComboButton : public ButtonBase
{
public:
	// 0x482770 (feedingfrenzy2): no images, no fonts, the colours white
	ComboButton();

	// 0x68 (0x5200e7 (feedingfrenzy2))
	std::string getClassName() override { return "ComboButton"; }
	// 0x13c (0x47b7e0 (feedingfrenzy2)): the state's font and colour, and its image at once (the normal ones for a
	// press dragged out)
	void updateLook() override;

	// 0x15c (0x47d9a0 (feedingfrenzy2)): the selector made, or emptied, and the text made, or emptied; no images and
	// no fonts
	virtual void init();
	// 0x160 (0x4759c0 (feedingfrenzy2)): the three fonts; with a normal one the text is in it, multiplied by its
	// colour, and the button takes the mouse. The three colours are then set to the byte value 1, nearly black (the
	// reader sets them again after)
	virtual void setFonts(Font* theUpFont, Font* theOverFont, Font* theActiveFont);
	// 0x164 (0x4245a0 (feedingfrenzy2)): the images, each surface's anchor put at the button's pivot; the button's
	// bounds are the normal image's as they are made, and it takes the mouse. Without a normal image nothing is shown
	virtual void setImages(Surface* theNormal, Surface* theHigh, Surface* theDown);
	// 0x168 (0x48e890 (feedingfrenzy2), shared with TextButton)
	virtual void setText(std::string theText);
	// 0x16c (0x414e10) and 0x170 (0x493190) (feedingfrenzy2), shared with TextButton: the text's alignment
	// (TextAlignH and TextAlignV)
	virtual void setAlignH(int theAlignH);
	virtual void setAlignV(int theAlignV);
	// (0x174 (0x508b09) and 0x178 (0x5200ca) (feedingfrenzy2), shared with TextButton, are not called on a combo
	// button)
	// 0x17c (0x414e00) and 0x180 (0x414df0) (feedingfrenzy2): where the text and the images are, in the button
	virtual void setTextOffset(const Vector2f& theOffset);
	virtual void setImageOffset(const Vector2f& theOffset);
	// 0x184 (0x414db0 (feedingfrenzy2)): the text's colour in each state, then the look updated
	virtual void setColors(const Color& theUpColor, const Color& theOverColor, const Color& theActiveColor);

private:
	Ref<Font> mUpFont;      // +0x10c
	Ref<Font> mOverFont;    // +0x110
	Ref<Font> mActiveFont;  // +0x114
	TextItem* mText;        // +0x118: a child, made by init
	Selector* mImages;      // +0x11c: a child, made by init
	Color mUpColor;         // +0x120
	Color mOverColor;       // +0x130
	Color mActiveColor;     // +0x140
	bool mHasImages;        // +0x150: a normal image
	bool mHasFonts;         // +0x151: a normal font
};

} // namespace engine

#endif
