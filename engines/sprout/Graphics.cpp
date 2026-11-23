#include "Graphics.h"
#include "Font.h"

using namespace engine;

// The bit fields of RenderState::mFlags (the masks and shifts at 0x497200 (wordharmony) and 0x497210 (wordharmony))
static const unsigned int HALIGN_MASK = 0x03, HALIGN_SHIFT = 0;
static const unsigned int VALIGN_MASK = 0x0c, VALIGN_SHIFT = 2;
static const unsigned int SNAP_MASK = 0xc0, SNAP_SHIFT = 6;

// 0x43d0a0 (wordharmony)
Graphics::Graphics()
{
	mStates.push_back(RenderState());
}

// 0x43cb10 (wordharmony)
Graphics::~Graphics()
{
}

// 0x45dd70 (wordharmony)
void Graphics::drawImage(Image* theImage)
{
	int aHeight = theImage->getHeight();
	int aWidth = theImage->getWidth();
	drawImage(theImage, IntRect(0, 0, aWidth, aHeight), 0, 0);
}

// 0x43c310 (wordharmony)
void Graphics::drawChar(char theChar)
{
	char aString[2] = {theChar, 0};
	drawString(aString);
}

// 0x43d120 (wordharmony)
void Graphics::pushState()
{
	mStates.push_back(mStates.back());
}

// 0x43ce10 (wordharmony)
void Graphics::popState()
{
	if (!mStates.empty())
		mStates.pop_back();
}

// 0x43c610 (wordharmony)
void Graphics::translate(float x, float y)
{
	state().mTranslation.x = x + state().mTranslation.x;
	state().mTranslation.y = y + state().mTranslation.y;
}

// 0x43c690 (wordharmony)
void Graphics::scale(float sx, float sy)
{
	state().mScale.x = sx * state().mScale.x;
	state().mScale.y = sy * state().mScale.y;
}

float Graphics::getScaleX() { return state().mScale.x; }  // 0x43c6e0 (wordharmony)
float Graphics::getScaleY() { return state().mScale.y; }  // 0x43c710 (wordharmony)

// 0x43c740 (wordharmony)
void Graphics::rotate(float theAngle)
{
	state().mRotation = theAngle + state().mRotation;
}

float Graphics::getRotation() { return state().mRotation; }  // 0x43c770 (wordharmony)

void Graphics::setOpacity(float theOpacity) { state().mOpacity = theOpacity; }  // 0x43c7a0 (wordharmony)

// 0x43c7d0 (wordharmony)
void Graphics::multiplyOpacity(float theOpacity)
{
	state().mOpacity = theOpacity * state().mOpacity;
}

float Graphics::getOpacity() { return state().mOpacity; }  // 0x43c800 (wordharmony)

void Graphics::setColor(float r, float g, float b) { state().mColor.set(r, g, b); }  // 0x43c830 (wordharmony)
const Color& Graphics::getColorOffset() { return state().mColorOffset; }             // 0x43c890 (wordharmony)

// 0x43c8c0 (wordharmony)
void Graphics::addColorOffset(float r, float g, float b)
{
	Color c = state().mColorOffset;
	c.r = c.r + r;
	c.g = c.g + g;
	c.b = c.b + b;
	state().mColorOffset.set(c.r, c.g, c.b);
}

// 0x43c980 (wordharmony)
void Graphics::setFont(Font* theFont)
{
	state().mFont = theFont;
}

// 0x43c9e0 (wordharmony)
void Graphics::setHAlign(int theAlign)
{
	state().mFlags = ((theAlign << HALIGN_SHIFT) & HALIGN_MASK) | (~HALIGN_MASK & state().mFlags);
}

// 0x43ca30 (wordharmony)
void Graphics::setVAlign(int theAlign)
{
	state().mFlags = ((theAlign << VALIGN_SHIFT) & VALIGN_MASK) | (~VALIGN_MASK & state().mFlags);
}

// 0x43ca80 (wordharmony)
void Graphics::setSnap(int theSnap)
{
	state().mFlags = ((theSnap << SNAP_SHIFT) & SNAP_MASK) | (~SNAP_MASK & state().mFlags);
}

// port: the state's alignment bits, which the original's text drawing reads inline
int Graphics::getHAlign() { return (state().mFlags & HALIGN_MASK) >> HALIGN_SHIFT; }
// port: as getHAlign
int Graphics::getVAlign() { return (state().mFlags & VALIGN_MASK) >> VALIGN_SHIFT; }
int Graphics::getSnap() { return (state().mFlags & SNAP_MASK) >> SNAP_SHIFT; }  // 0x43cad0 (wordharmony)

// 0x477f50 (pizzafrenzy)
void Graphics::setColorMode(int theMode)
{
	state().mColorMode = theMode;
}

// 0x478190 (pizzafrenzy)
void Graphics::setSmoothType(int theSmoothType)
{
	state().mSmoothType = theSmoothType;
}

// 0x478060 (pizzafrenzy): the state holds a reference
void Graphics::setBltFx(BltFx* theBltFx)
{
	state().mBltFx = theBltFx;
}

// 0x4bfc20 (pizzafrenzy): port: each pixel row of the rectangle is a line of the state's colour. The original blends
// the fill by the colour's alpha and the opacity (a colour with an alpha of 0, like the score table's background,
// leaves the screen as it was); the port's line takes the two as its opacity.
void Graphics::fillRect(const Rect& theRect)
{
	float anAlpha = state().mColorAlpha;
	if (anAlpha >= 1.0f)
	{
		for (int aRow = (int)theRect.top; aRow < (int)theRect.bottom; aRow++)
			drawLine(Point(theRect.left, (float)aRow), Point(theRect.right - 1, (float)aRow));
		return;
	}
	pushState();
	state().mOpacity = state().mOpacity * anAlpha;
	for (int aRow = (int)theRect.top; aRow < (int)theRect.bottom; aRow++)
		drawLine(Point(theRect.left, (float)aRow), Point(theRect.right - 1, (float)aRow));
	popState();
}

// 0x478320 (pizzafrenzy): port: the translation moves the text's origin to (x, y)
void Graphics::drawString(const char* theString, float x, float y)
{
	pushState();
	translate(x, y);
	drawString(theString);
	popState();
}
