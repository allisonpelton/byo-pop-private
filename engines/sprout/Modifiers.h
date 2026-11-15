#ifndef __SPROUT_MODIFIERS_H__
#define __SPROUT_MODIFIERS_H__

// The signal build's node modifiers (Pizza Frenzy's; Iggle Pop's Mac build names ScaleModifier, OpacityModifier and
// ColorFlashModifier). Each is one of a node's listeners while it is wanted: every frame it adds the seconds to its own
// time and sets a value of the node from a sine of that time, which swings between a smallest and a largest value
// once in each period.

#include "Color4.h"
#include "Node.h"

namespace engine
{

// The node's scale, across and down, each between its own smallest and largest (0x44 bytes, constructor
// 0x483d10 (pizzafrenzy))
class ScaleModifier : public Object, public NodeListener
{
public:
	// 0x483d10 (pizzafrenzy)
	ScaleModifier(float thePeriodX, float theMinX, float theMaxX, float thePeriodY, float theMinY, float theMaxY,
	              float theTime);

	// 0x00: 0x483de0 (pizzafrenzy)
	void update(UpdateInfo& theInfo, Node* theNode) override;
	// 0x483cd0 (pizzafrenzy)
	void setHorizontal(float thePeriod, float theMin, float theMax)
	{
		mPeriodX = thePeriod;
		mMinX = theMin;
		mMaxX = theMax;
	}
	// 0x483cf0 (pizzafrenzy)
	void setVertical(float thePeriod, float theMin, float theMax)
	{
		mPeriodY = thePeriod;
		mMinY = theMin;
		mMaxY = theMax;
	}

private:
	float mPeriodX;  // +0x08
	float mPeriodY;  // +0x0c
	float mMinX;     // +0x10
	float mMaxX;     // +0x14
	float mMinY;     // +0x18
	float mMaxY;     // +0x1c
	float mTime;     // +0x20
};

// The node's opacity (0x38 bytes, constructor 0x481a90 (pizzafrenzy))
class OpacityModifier : public Object, public NodeListener
{
public:
	// 0x481a90 (pizzafrenzy)
	OpacityModifier(float thePeriod, float theMin, float theMax, float theTime);

	// 0x00: 0x481b50 (pizzafrenzy)
	void update(UpdateInfo& theInfo, Node* theNode) override;
	// 0x47b110 (pizzafrenzy)
	void setRange(float thePeriod, float theMin, float theMax)
	{
		mPeriod = thePeriod;
		mMin = theMin;
		mMax = theMax;
	}

private:
	float mPeriod;  // +0x08
	float mMin;     // +0x0c
	float mMax;     // +0x10
	float mTime;    // +0x14
};

// The node's colour, which the colour mode adds to the picture (0x44 bytes, constructor 0x483ac0 (pizzafrenzy)): the
// colour given, scaled from nothing to all of it
class ColorFlashModifier : public Object, public NodeListener
{
public:
	// 0x483ac0 (pizzafrenzy)
	ColorFlashModifier(float thePeriod, const Color4& theColor, float theTime, int theColorMode);

	// 0x00: 0x483bc0 (pizzafrenzy)
	void update(UpdateInfo& theInfo, Node* theNode) override;
	// 0x483a90 (pizzafrenzy)
	void setColor(float thePeriod, const Color4& theColor)
	{
		mPeriod = thePeriod;
		mColor = theColor;
	}

private:
	float mPeriod;     // +0x08
	Color4 mColor;     // +0x0c
	int mColorMode;    // +0x1c
	float mTime;       // +0x20
};

// The node's position, moved along a wave: across and down, each with its own period and size (0x44 bytes,
// constructor 0x47f450 (pizzafrenzy); Iggle Pop's Mac build names it WaveModifier). Each frame the node moves by the
// change of the offset since the frame before, so the node's own motion goes on beside it.
class WaveModifier : public Object, public NodeListener
{
public:
	// 0x47f450 (pizzafrenzy): a period of nothing is a period of one
	WaveModifier(float thePeriodX, float thePeriodY, float theSizeX, float theSizeY, float theTime);

	// 0x47f550 (pizzafrenzy)
	void update(UpdateInfo& theInfo, Node* theNode) override;

private:
	float mPeriodX;  // +0x08
	float mPeriodY;  // +0x0c
	float mSizeX;    // +0x10: how far across the node moves at most
	float mSizeY;    // +0x14
	float mTime;     // +0x18
	float mOffsetX;  // +0x1c: the offset the node was last moved to
	float mOffsetY;  // +0x20
};

// The node's visibility, which switches on and off (0x30 bytes, constructor 0x47aff0 (pizzafrenzy)): each time its
// countdown runs out the node is hidden when it shows and shown when it is hidden, and the countdown starts over with
// the seconds the new state lasts. It starts at nothing, so the first frame switches.
class BlinkModifier : public Object, public NodeListener
{
public:
	// 0x47aff0 (pizzafrenzy)
	BlinkModifier(float theShownSeconds, float theHiddenSeconds);

	// 0x00: 0x47af90 (pizzafrenzy)
	void update(UpdateInfo& theInfo, Node* theNode) override;

private:
	float mShownSeconds;   // +0x04: how long the node shows
	float mHiddenSeconds;  // +0x08: how long it is hidden
	float mTime;           // +0x0c: the seconds left of the state it is in
};

} // namespace engine

#endif
