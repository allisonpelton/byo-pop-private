// The input state (see Input.h).

#include "Input.h"
#include "Graphics.h"
#include "Host.h"
#include "Platform.h"

namespace bonnies
{

Input* gInput = nullptr;

// 0x42b610
Input::Input()
{
	mPinnedX = 0;
	mPinnedY = 0;
	mSpareFirst = 0;
	mSpareSecond = 0;
	mSpareThird = 0;
	mDownTime = 0;
	mDownX = 0;
	mDownY = 0;
	mPinned = 0;
	mSkipMotion = 0;
	for (int i = 0; i < 0x13d; i++)
		mSpareBlock[i] = 0;
}

// 0x42b640
bool Input::leftButtonDown()
{
	return (host()->getMouseState(nullptr, nullptr) & 1) != 0;
}

// 0x42b650
void Input::getMousePos(float* theX, float* theY)
{
	if (mPinned != 0)
	{
		*theX = mPinnedX;
		*theY = mPinnedY;
		return;
	}
	int x;
	int y;
	if (getCursorPos(&x, &y) == 0)
		host()->getMouseState(&x, &y);
	float aX = (float)x;
	*theX = (float)((double)aX / screenWidth());
	float aY = (float)y;
	*theY = (float)((double)aY / screenHeight());
}

// 0x42b6f0
bool Input::isKeyDown(int theKey)
{
	const unsigned char* aKeys = host()->getKeyState();
	if (theKey >= 0 && theKey < SDLK_LAST)
		return aKeys[theKey] != 0;
	return false;
}

// 0x42b730
bool Input::anyKeyDown()
{
	const unsigned char* aKeys = host()->getKeyState();
	int i = 0;
	if (SDLK_LAST > 0)
	{
		do
		{
			if (aKeys[i] != 0)
				return true;
			i++;
		} while (i < SDLK_LAST);
	}
	return false;
}

// 0x42b760
float Input::mouseX()
{
	float x;
	float y;
	getMousePos(&x, &y);
	return x;
}

// 0x42b780
float Input::mouseY()
{
	float x;
	float y;
	getMousePos(&x, &y);
	return y;
}

} // namespace bonnies
