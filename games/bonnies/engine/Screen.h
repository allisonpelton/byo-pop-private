#ifndef __BONNIES_SCREEN_H__
#define __BONNIES_SCREEN_H__

// A screen (0x429560-0x429a70): a rectangle of the screen with widgets, in the app's list of screens. The game's
// screens derive from it; the base has no vtable of its own (each derived one fills the seven slots, calling these).

#include <cstdint>

namespace bonnies
{

struct Event;
class Widget;

class Screen
{
public:
	Screen() {}
	virtual ~Screen() {}

	// slot 0, 0x429560
	virtual void init();
	// slot 1 (each screen's, which ends with removeAll)
	virtual void shutdown();
	// slot 2, 0x4296d0
	virtual int contains(float theX, float theY);
	// slot 3, 0x4295a0: the background panel and the widgets
	virtual void draw();
	// slot 4, 0x429630
	virtual void update();
	// slot 5, 0x4297e0: the widgets from the last, then dragging and quitting; 0 when consumed
	virtual int handleEvent(Event* theEvent);
	// slot 6, 0x429650
	virtual int isVisible();

	// 0x429660: adds theWidget before the first of a higher priority
	void addWidget(Widget* theWidget);
	// 0x429730: moves the screen with the mouse, kept on the screen
	void drag();
	// 0x4298d0
	Widget* findWidget(int theId);
	// 0x429900: whether the screen is the top of the modal stack
	int isTop();
	// 0x429920: removes theWidget, shuts it down and frees it
	void removeWidget(Widget* theWidget);
	// 0x429a30
	void removeAll();

	float mX;              // +0x04
	float mY;              // +0x08
	float mWidth;          // +0x0c
	float mHeight;         // +0x10
	int mPriority;         // +0x14
	int mHidden;           // +0x18
	int mDraggable;        // +0x1c
	int mDrawsPanel;       // +0x20
	uint32_t mColor;       // +0x24
	int mLevel;            // +0x28: its level on the modal stack (0 when not modal)
	Widget* mTail;         // +0x2c
	Widget* mHead;         // +0x30
	Screen* mNext;         // +0x34
	Screen* mPrev;         // +0x38
	int mDragging;         // +0x3c
	float mDragX;          // +0x40: the mouse at the last drag
	float mDragY;          // +0x44
};

} // namespace bonnies

#endif
