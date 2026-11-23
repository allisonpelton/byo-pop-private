#include "Background.h"

using namespace engine;

namespace feedingfrenzy
{

// 0x42a340
Background::Background(int theWidth, int theHeight) :
	mDownCallback(nullptr), mDownArg(nullptr), mUpCallback(nullptr), mUpArg(nullptr), mButtonHeld(false)
{
	setFlags(NODE_MOUSE);
	setRect(0.0f, 0.0f, (float)theWidth, (float)theHeight);
}

// 0x42a270
void Background::setDownCallback(ButtonCallback theCallback, void* theArg)
{
	mDownCallback = theCallback;
	mDownArg = theArg;
}

// 0x42a290
void Background::setUpCallback(ButtonCallback theCallback, void* theArg)
{
	mUpCallback = theCallback;
	mUpArg = theArg;
}

// The callback's arguments are the place as whole numbers (0x462fa0 truncates)
void Background::callDown(int theButton)
{
	if (mDownCallback != nullptr)
		mDownCallback((int)mLastPoint.x, (int)mLastPoint.y, theButton, mDownArg);
	mButtonHeld = true;
}

void Background::callUp(int theButton)
{
	if (mUpCallback != nullptr)
		mUpCallback((int)mLastPoint.x, (int)mLastPoint.y, theButton, mUpArg);
	mButtonHeld = false;
}

// 0x42a160
void Background::onMouseDownEvent(MouseEvent& theEvent)
{
	callDown(BUTTON_LEFT);
}

// 0x42a1a0
void Background::onMouseUpEvent(MouseEvent& theEvent)
{
	callUp(BUTTON_LEFT);
}

// 0x42a0e0
void Background::onRightDownEvent(MouseEvent& theEvent)
{
	callDown(BUTTON_RIGHT);
}

// 0x42a120
void Background::onRightUpEvent(MouseEvent& theEvent)
{
	callUp(BUTTON_RIGHT);
}

// 0x42a1e0
void Background::handleMouseMoveEvent(MouseEvent& theEvent)
{
	mLastPoint = theEvent.mPosition;
}

// 0x42a250
void Background::onMouseExitEvent(MouseEvent& theEvent)
{
	if (mButtonHeld)
		callUp(BUTTON_LEFT);
}

} // namespace feedingfrenzy
