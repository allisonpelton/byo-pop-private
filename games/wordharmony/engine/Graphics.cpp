#include "Graphics.h"
#include "Font.h"

using namespace engine;

// The bit fields of RenderState::mFlags (the masks and shifts at 0x497200 and 0x497210)
static const unsigned int HALIGN_MASK = 0x03, HALIGN_SHIFT = 0;
static const unsigned int VALIGN_MASK = 0x0c, VALIGN_SHIFT = 2;
static const unsigned int SNAP_MASK = 0xc0, SNAP_SHIFT = 6;

// 0x43d0a0
Graphics::Graphics()
{
	mStates.push_back(RenderState());
}

// 0x43cb10
Graphics::~Graphics()
{
}

// 0x45ddf0
void Graphics::drawImage(Image* theImage, float x, float y)
{
	int aHeight = theImage->getHeight();
	int aWidth = theImage->getWidth();
	drawImage(theImage, IntRect(0, 0, aWidth, aHeight), x, y);
}

// 0x45dd70
void Graphics::drawImage(Image* theImage)
{
	int aHeight = theImage->getHeight();
	int aWidth = theImage->getWidth();
	drawImage(theImage, IntRect(0, 0, aWidth, aHeight), 0, 0);
}

// 0x43c310
void Graphics::drawChar(char theChar)
{
	char aString[2] = {theChar, 0};
	drawString(aString);
}

// 0x43d120
void Graphics::pushState()
{
	mStates.push_back(mStates.back());
}

// 0x43ce10
void Graphics::popState()
{
	if (!mStates.empty())
		mStates.pop_back();
}

// 0x43c610
void Graphics::translate(float x, float y)
{
	state().mTranslation.x = x + state().mTranslation.x;
	state().mTranslation.y = y + state().mTranslation.y;
}

// 0x43c690
void Graphics::scale(float sx, float sy)
{
	state().mScale.x = sx * state().mScale.x;
	state().mScale.y = sy * state().mScale.y;
}

float Graphics::getScaleX() { return state().mScale.x; }  // 0x43c6e0
float Graphics::getScaleY() { return state().mScale.y; }  // 0x43c710

// 0x43c740
void Graphics::rotate(float theAngle)
{
	state().mRotation = theAngle + state().mRotation;
}

float Graphics::getRotation() { return state().mRotation; }  // 0x43c770

void Graphics::setOpacity(float theOpacity) { state().mOpacity = theOpacity; }  // 0x43c7a0

// 0x43c7d0
void Graphics::multiplyOpacity(float theOpacity)
{
	state().mOpacity = theOpacity * state().mOpacity;
}

float Graphics::getOpacity() { return state().mOpacity; }  // 0x43c800

void Graphics::setColor(float r, float g, float b) { state().mColor.set(r, g, b); }  // 0x43c830
const Color& Graphics::getColor() { return state().mColor; }                         // 0x43c860
const Color& Graphics::getColorOffset() { return state().mColorOffset; }             // 0x43c890

// 0x43c8c0
void Graphics::addColorOffset(float r, float g, float b)
{
	Color c = state().mColorOffset;
	c.r = c.r + r;
	c.g = c.g + g;
	c.b = c.b + b;
	state().mColorOffset.set(c.r, c.g, c.b);
}

// 0x43c980
void Graphics::setFont(Font* theFont)
{
	state().mFont = theFont;
}

// 0x43c9e0
void Graphics::setHAlign(int theAlign)
{
	state().mFlags = ((theAlign << HALIGN_SHIFT) & HALIGN_MASK) | (~HALIGN_MASK & state().mFlags);
}

// 0x43ca30
void Graphics::setVAlign(int theAlign)
{
	state().mFlags = ((theAlign << VALIGN_SHIFT) & VALIGN_MASK) | (~VALIGN_MASK & state().mFlags);
}

// 0x43ca80
void Graphics::setSnap(int theSnap)
{
	state().mFlags = ((theSnap << SNAP_SHIFT) & SNAP_MASK) | (~SNAP_MASK & state().mFlags);
}

int Graphics::getHAlign() { return (state().mFlags & HALIGN_MASK) >> HALIGN_SHIFT; }
int Graphics::getVAlign() { return (state().mFlags & VALIGN_MASK) >> VALIGN_SHIFT; }
int Graphics::getSnap() { return (state().mFlags & SNAP_MASK) >> SNAP_SHIFT; }  // 0x43cad0
