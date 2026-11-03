#ifndef __BONNIES_EVENTS_H__
#define __BONNIES_EVENTS_H__

// The event queue (0x42e980-0x42f090, the object 0x481ab8): SDL's events turned into the engine's, and the events the
// widgets send, handed one at a time to the app (the platform's slot 4) once SDL has none left.
//
// The engine's event types:
//  1 the mouse moved; 2, 3 the left button down, up; 4, 5 the middle; 6, 7 the right
//  8, 9 a key down, up (mA the keysym, mB its character)
//  0xb quit
//  0xc, 0xd a button pressed, released (mA its id); 0xe a check box toggled; 0xf a slider moved
//  0x10, 0x11, 0x12, 0x13 a text field's Enter, Escape, Tab and its editing begun
//  0x14 a list's selection changed; 0x15 a list's item double-clicked

#include <cstdint>

namespace bonnies
{

struct Event
{
	int mType;       // +0x00
	int mA;          // +0x04
	int mB;          // +0x08
	float mX;        // +0x0c: the mouse when it was made
	float mY;        // +0x10
	bool mShift;     // +0x14: the modifier keys then
	bool mCtrl;      // +0x15
	bool mAlt;       // +0x16
	bool mMeta;      // +0x17
	bool mDone;      // +0x18: handed to the app (freed by the next pump)
	Event* mNext;    // +0x1c
};

class EventQueue
{
public:
	// 0x42e980
	EventQueue();
	// 0x42e990
	~EventQueue();
	// 0x42eac0: adds an event at the end
	void add(int theType, int theA, int theB);
	// 0x42eb00: adds a mouse button's event (also noting when and where in the input state)
	void addButton(int theType, float theX, float theY);
	// 0x42ec70: adds an event at the front, handled next
	void push(int theType, int theA, int theB);
	// 0x42ecb0: frees the handled events, reads SDL's and hands the rest to the app
	void pump();

	Event* mHead;  // +0x00
	Event* mTail;  // +0x04
};

// 0x42e9b0: a new event with the modifier keys and the mouse as they are
Event* newEvent(int theType, int theA, int theB);

extern EventQueue* gEvents;  // 0x481ab8

} // namespace bonnies

#endif
