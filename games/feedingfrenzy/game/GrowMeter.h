#ifndef __FEEDINGFRENZY_GROWMETER_H__
#define __FEEDINGFRENZY_GROWMETER_H__

// A bar (GrowMeter, 0x12c bytes, constructor 0x433ea0, vtable 0x4a6ad0) that fills a share of its rectangle with a
// colour: the player's growth in the HUD and the time the poison has left over the player's fish.

#include "Geometry.h"
#include "Node.h"

namespace feedingfrenzy
{

class GrowMeter : public engine::Node
{
public:
	GrowMeter();  // 0x433ea0

	// 0x433e10: the colour, and the bar's filled width and height from the origin
	void draw(engine::Graphics* g) override;
	// 0x433dd0: the node has no bounds of its own
	void layout() override;

	// 0xd4: 0x433ce0: the rectangle the bar fills, which comes up empty until the value is set
	void setRect(const engine::Rect& theRect);
	// 0x433d20: the share of the rectangle's width that is filled, 0 to 1
	void setValue(float theValue);
	// 0xd8: 0x433de0: the colour of the bar
	void setBarColor(float theRed, float theGreen, float theBlue);

private:
	float mValue;       // +0xfc
	engine::Rect mRect; // +0x100
	float mFillWidth;   // +0x110
	float mFillHeight;  // +0x114
	float mRed;         // +0x118
	float mGreen;       // +0x11c
	float mBlue;        // +0x120
};

} // namespace feedingfrenzy

#endif
