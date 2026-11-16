#ifndef __SPROUT_TEXTBUTTON_H__
#define __SPROUT_TEXTBUTTON_H__

// A text button (0x1f4 bytes, constructor 0x452e30 (feedingfrenzy), vtable 0x4a9170 (feedingfrenzy)): a button whose
// face is a text node, a child of its own. The text shows in the up font while the mouse is away, in the over font
// while it is over the button and in the active font while it is pressed, and its colour offset follows the same three
// states (each colour is added to the text's own). The layout reader makes it from the textButton element.

#include "Button.h"
#include "Geometry.h"
#include <string>

namespace engine
{

class TextItem;

class TextButton : public Button
{
public:
	TextButton();  // 0x452e30 (feedingfrenzy)

	// 0x104: 0x45bb50 (feedingfrenzy)
	virtual void setText(std::string theText);
	// 0x108: 0x45b9f0 (feedingfrenzy), 0 left, 1 centre, 2 right
	virtual void setHAlign(int theAlign);
	// 0x10c: 0x45ba00 (feedingfrenzy), 0 top, 1 middle, 2 bottom
	virtual void setVAlign(int theAlign);
	// 0x110: 0x45ba10 (feedingfrenzy), TEXT_* (TextItem.h)
	virtual void setStyle(unsigned int theStyle);
	// 0x114: 0x45bbf0 (feedingfrenzy)
	virtual std::string getText();
	// 0x118: 0x45bc20 (feedingfrenzy), the fonts by their names in the archive; the text node is made here, so the
	// button needs the first two (the active font may be empty). The three colour offsets start at none.
	virtual bool setFonts(const std::string& theFont, const std::string& theOverFont, const std::string& theActiveFont);
	// 0x11c: 0x45baf0 (feedingfrenzy), the colour offsets of the three states; the text takes the up one only when the
	// mouse leaves the button
	virtual void setAddColors(const Color& theUpColor, const Color& theOverColor, const Color& theActiveColor);

protected:
	void onMouseDown(Point& thePoint) override;   // 0x8c: 0x45ba30 (feedingfrenzy)
	void onMouseEnter(Point& thePoint) override;  // 0xb0: 0x45ba50 (feedingfrenzy)
	void onMouseExit(Point& thePoint) override;   // 0xb8: 0x45bac0 (feedingfrenzy)

public:
	std::string mFont;           // +0x170 (feedingfrenzy)
	std::string mOverFont;       // +0x18c (feedingfrenzy)
	std::string mActiveFont;     // +0x1a8 (feedingfrenzy)
	TextItem* mText;             // +0x1c4 (feedingfrenzy): a child
	Color mUpAddColor;           // +0x1c8 (feedingfrenzy)
	Color mOverAddColor;         // +0x1d4 (feedingfrenzy)
	Color mActiveAddColor;       // +0x1e0 (feedingfrenzy)
};

} // namespace engine

#endif
