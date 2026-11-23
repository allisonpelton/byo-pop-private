#include "TextButton.h"
#include "TextItem.h"

using namespace engine;

// 0x452e30 (feedingfrenzy): the original leaves the text node unset until setFonts makes it
TextButton::TextButton()
{
	mText = nullptr;
}

// 0x45bb50 (feedingfrenzy)
void TextButton::setText(std::string theText)
{
	mText->setText(theText);
}

// 0x45b9f0 (feedingfrenzy)
void TextButton::setHAlign(int theAlign)
{
	mText->setHAlign(theAlign);
}

// 0x45ba00 (feedingfrenzy)
void TextButton::setVAlign(int theAlign)
{
	mText->setVAlign(theAlign);
}

// 0x45ba10 (feedingfrenzy)
void TextButton::setStyle(unsigned int theStyle)
{
	mText->mStyle = theStyle;
}

// 0x45bbf0 (feedingfrenzy)
std::string TextButton::getText()
{
	return mText->mText;
}

// 0x45bc20 (feedingfrenzy)
bool TextButton::setFonts(const std::string& theFont, const std::string& theOverFont, const std::string& theActiveFont)
{
	if (theFont.size() == 0 || theOverFont.size() == 0)
		return false;
	mFont = theFont;
	mOverFont = theOverFont;
	mActiveFont = theActiveFont;
	mText = new TextItem();
	addChild(mText);
	mText->setFont(mFont);
	addOwnFlags(NODE_MOUSE);
	addOwnFlags(NODE_LISTENERS);
	layout();
	setName("TextButton");
	mUpAddColor.set(0, 0, 0);
	mOverAddColor.set(0, 0, 0);
	mActiveAddColor.set(0, 0, 0);
	return true;
}

// 0x45baf0 (feedingfrenzy)
void TextButton::setAddColors(const Color& theUpColor, const Color& theOverColor, const Color& theActiveColor)
{
	mUpAddColor = theUpColor;
	mOverAddColor = theOverColor;
	mActiveAddColor = theActiveColor;
}

// 0x45ba30 (feedingfrenzy): the press first, then the active font (the colour stays the over colour)
void TextButton::onMouseDown(Point& thePoint)
{
	Button::onMouseDown(thePoint);
	mText->setFont(mActiveFont);
}

// 0x45ba50 (feedingfrenzy): the active font and colour when the mouse comes back to a pressed button, else the over ones
void TextButton::onMouseEnter(Point& thePoint)
{
	if (mPressed)
	{
		mText->setFont(mActiveFont);
		mText->mColorOffset = mActiveAddColor;
	}
	else
	{
		mText->setFont(mOverFont);
		mText->mColorOffset = mOverAddColor;
	}
	Button::onMouseEnter(thePoint);
}

// 0x45bac0 (feedingfrenzy)
void TextButton::onMouseExit(Point& thePoint)
{
	mText->setFont(mFont);
	mText->mColorOffset = mUpAddColor;
	Button::onMouseExit(thePoint);
}
