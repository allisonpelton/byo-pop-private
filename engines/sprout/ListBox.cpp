#include "ListBox.h"
#include "App.h"
#include "Sound.h"

using namespace engine;

// 0x491ea0 (pizzafrenzy): black background, near-black text (the colours are bytes over 255, and the original gives 1
// where it means to give white)
ListBox::ListBox()
{
	mBackgroundColor = Color4::fromBytes(0, 0, 0);
	mTextColor = Color4::fromBytes(1, 1, 1);
	mHighlightColor = Color4::fromBytes(1, 1, 0);
	mSelectionBackgroundColor = Color4::fromBytes(1, 1, 1);
	mSelectionTextColor = Color4::fromBytes(0, 0, 0);
	mSelectionHighlightColor = Color4::fromBytes(0, 0, 1);
	mIndent = 0;
	mScrollOffset = 0;
	mSelectedIndex = -1;
	mHoverIndex = -1;
	mEnterSound = nullptr;
	mPressSound = nullptr;
	addOwnFlags(NODE_DRAWABLE | NODE_MOUSE);
}

// 0x491760 (pizzafrenzy): each sound is loaded under its own name, then a voice is made of it (like the button's)
void ListBox::setSounds(const std::string& theEnterSound, const std::string& thePressSound)
{
	if (theEnterSound.size() != 0 && App::getSoundSystem() != nullptr)
	{
		App::getSoundSystem()->load(theEnterSound.c_str(), theEnterSound.c_str());
		mEnterSound = App::getSoundSystem()->createSound(theEnterSound.c_str(), 1, 0);
	}
	if (thePressSound.size() != 0 && App::getSoundSystem() != nullptr)
	{
		App::getSoundSystem()->load(thePressSound.c_str(), thePressSound.c_str());
		mPressSound = App::getSoundSystem()->createSound(thePressSound.c_str(), 1, 0);
	}
}
