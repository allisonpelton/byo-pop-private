// WidgetMover.h: slides widgets on to the screen and off it (the original's Sexy::WidgetMover). A screen or dialog is
// scrolled on from one edge; once it is on, the mover has nothing more to do with it until it is scrolled off.

#ifndef __WIDGETMOVER_H__
#define __WIDGETMOVER_H__

#include <list>

namespace Sexy
{

class Widget;

// The edge a widget comes in from or leaves to
enum WidgetScroll
{
	SCROLL_NONE = 0,
	SCROLL_LEFT,  // the widget moves left (comes in from the right edge, leaves by the left)
	SCROLL_RIGHT,
	SCROLL_DOWN,
	SCROLL_UP
};

class WidgetMover
{
public:
	WidgetMover() {} // 0x40d390
	virtual ~WidgetMover() {}

	bool IsMoving() const { return !mEntries.empty(); }	// 0x408550
	void Update();										// 0x40c670: one step of every move
	void ScrollOn(Widget* theWidget, WidgetScroll theFrom);					// 0x421e40: moves it from the edge to where it is
	void ScrollOff(Widget* theWidget, WidgetScroll theTo, bool theRemoveWhenDone); // 0x421f80: from where it is to the edge
	bool WillRemove(Widget* theWidget);					// 0x4220d0: is it scrolling off to be removed
	void Forget(Widget* theWidget);						// 0x40c7a0: drops its move, if it has one

private:
	// A move in progress (0x1c bytes in the original)
	struct Entry
	{
		Widget* mWidget = nullptr;
		int mStepsLeft = 0;			 // +4: counts down from SCROLL_STEPS
		bool mRemoveWhenDone = false; // +8: the widget is taken out of the manager and deleted at the end
		float mStartX = 0;			 // +0xc
		float mStartY = 0;
		float mEndX = 0;
		float mEndY = 0;

		bool Step(); // 0x406010: false when the move is done
	};

	static const int SCROLL_STEPS = 20; // how many updates a move takes

	Entry* GetEntry(Widget* theWidget, bool theCreate);	// 0x421d90

	std::list<Entry> mEntries; // +8
};

} // namespace Sexy

#endif
