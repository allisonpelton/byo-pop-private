#ifndef __PIZZAFRENZY_TOPPINGBUTTON_H__
#define __PIZZAFRENZY_TOPPINGBUTTON_H__

// A topping's button (0x144 bytes, constructor 0x427210, vtable 0x4fb9c0): a card with the topping's small picture
// and, under it, a number (a level of the topping, or nothing when the card stands for the topping alone). The
// screen it is made for hears it under the topping's name.

#include "Button.h"
#include "ImageNode.h"
#include "ScreenLayout.h"
#include "TextItem.h"
#include "TileEntries.h"

namespace pizzafrenzy
{

class ToppingButton : public engine::Container
{
public:
	// 0x427210: the card for the topping, its number (-1 for none) and the screen that listens to it
	ToppingButton(Topping* theTopping, int theNumber, engine::ScreenLayout* theScreen);

	using engine::Container::setScale;
	// 0x20: 0x427150: the card, the picture and the number take the scale and keep their places on it
	void setScale(float theScale) override;
	// 0x80: 0x4271e0: the node and the card
	void setEnabled(bool theEnabled) override;

	// 0x12c: 0x4271d0: the text under the picture
	void setCaption(const std::string& theCaption) { mCaption->setText(theCaption); }
	// 0x130: 0x427100: the button takes the mouse, in its own colour
	void showPlain();
	// 0x134: 0x427120: the button does not take the mouse, and is darkened
	void showDimmed();

	Topping* getTopping() { return mTopping; }  // +0x130

private:
	engine::Ref<engine::ImageButton> mButton;  // +0x128: the card
	engine::Ref<engine::ImageNode> mPicture;   // +0x12c: the topping's small picture
	engine::Ref<Topping> mTopping;             // +0x130
	engine::Ref<engine::TextItem> mCaption;    // +0x134: the number
	int mNumber;                               // +0x138
};

} // namespace pizzafrenzy

#endif
