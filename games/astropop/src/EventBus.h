#ifndef __ASTROPOP_EVENTBUS_H__
#define __ASTROPOP_EVENTBUS_H__

// The game's event bus: 43 lists of listeners, one per event (the application keeps them at +0x62c, 12 bytes each: a
// list head and its size; every game object and the Board point at them). A listener registers itself with an event
// (0x424270, appended without a check for a duplicate), asks whether it is registered (0x40d7e0), removes itself from
// one list (0x40dfa0, the first occurrence) or from all of them (0x40e000, the first occurrence in each list). An
// object that fires an event (0x40cde0, and 0x40d470 for the events 0x1f .. 0x2a, which carry a number) calls the
// listener's handler for that event with itself as the source, for each listener in the order of registration. The
// handler of the next listener is found before the current one is called, so a handler may remove itself.
//
// The original calls a virtual method of the listener's vtable for each event (a table maps the event to the slot);
// the port has one virtual method, OnEvent, and each class switches on the event. docs/GAME_NOTES.md section 4b has
// the table of events and the slots they were.

#include <list>

namespace Sexy
{

class EventListener;

enum
{
	NUM_EVENT_LISTS = 43
};

class EventBus
{
public:
	std::list<EventListener*> mLists[NUM_EVENT_LISTS];

	void Add(EventListener* theListener, int theEvent)                     // 0x424270
	{
		mLists[theEvent].push_back(theListener);
	}

	bool Has(EventListener* theListener, int theEvent) const               // 0x40d7e0
	{
		for (std::list<EventListener*>::const_iterator i = mLists[theEvent].begin(); i != mLists[theEvent].end(); ++i)
			if (*i == theListener)
				return true;
		return false;
	}

	void Remove(EventListener* theListener, int theEvent)                  // 0x40dfa0
	{
		for (std::list<EventListener*>::iterator i = mLists[theEvent].begin(); i != mLists[theEvent].end(); ++i)
		{
			if (*i == theListener)
			{
				mLists[theEvent].erase(i);
				return;
			}
		}
	}

	void RemoveEverywhere(EventListener* theListener)                      // 0x40e000
	{
		for (int anEvent = 0; anEvent < NUM_EVENT_LISTS; anEvent++)
			Remove(theListener, anEvent);
	}

	void Fire(EventListener* theSource, int theEvent, int theArg = 0);     // 0x40cde0, 0x40d470

	// An event for one listener only (0x40d820, and 0x40d9f0 for the events that carry a number): the bus looks the
	// target up in the event's list and calls its handler; a target that is not in the list gets nothing (the
	// original would call through whatever the list's head holds).
	void FireTo(EventListener* theSource, EventListener* theTarget, int theEvent, int theArg = 0);
};

// Anything that fires events or listens to them (original: an object whose +4 points at the bus; a listener part is
// a second subobject of the game objects, at +0xc, with the bus again at +0x10)
class EventListener
{
public:
	EventBus*	mBus;

	EventListener(EventBus* theBus = NULL) : mBus(theBus) { }
	virtual ~EventListener() { }

	// The handler of an event. theArg is the number the events 0x1f .. 0x2a carry.
	virtual void	OnEvent(int theEvent, EventListener* theSource, int theArg)
	{
		(void) theEvent; (void) theSource; (void) theArg;
	}

	void	Listen(int theEvent)                { mBus->Add(this, theEvent); }               // 0x425290
	bool	IsListening(int theEvent) const     { return mBus->Has(const_cast<EventListener*>(this), theEvent); } // 0x40ddc0
	void	StopListening(int theEvent)         { mBus->Remove(this, theEvent); }            // 0x40e7c0
	void	StopListeningToAll()                { mBus->RemoveEverywhere(this); }            // 0x40e7e0
	void	Fire(int theEvent)                  { mBus->Fire(this, theEvent); }              // 0x40dd40
	void	Fire(int theEvent, int theArg)      { mBus->Fire(this, theEvent, theArg); }      // 0x40dd60
	void	FireTo(EventListener* theTarget, int theEvent, int theArg = 0)                   // 0x40dd80, 0x40dda0
	{
		mBus->FireTo(this, theTarget, theEvent, theArg);
	}
};

inline void EventBus::Fire(EventListener* theSource, int theEvent, int theArg)
{
	std::list<EventListener*>& aList = mLists[theEvent];
	std::list<EventListener*>::iterator i = aList.begin();
	while (i != aList.end())
	{
		EventListener* aListener = *i;
		++i;
		aListener->OnEvent(theEvent, theSource, theArg);
	}
}

inline void EventBus::FireTo(EventListener* theSource, EventListener* theTarget, int theEvent, int theArg)
{
	std::list<EventListener*>& aList = mLists[theEvent];
	for (std::list<EventListener*>::iterator i = aList.begin(); i != aList.end(); ++i)
	{
		if (*i == theTarget)
		{
			theTarget->OnEvent(theEvent, theSource, theArg);
			return;
		}
	}
}

}

#endif
