#ifndef __SPROUT_SIGNAL_H__
#define __SPROUT_SIGNAL_H__

// The engine's signals and slots: Sarah Thompson's sigslot library (public domain), its single-threaded policy, which
// the engine's signal build (Pizza Frenzy's) wires its application, its scene and its screens with. A signal keeps its
// connections in the order they were made and emits to each in turn, taking the next before it calls one, so that a
// slot may disconnect itself; an object with slots (HasSlots, the library's has_slots, 0x14 bytes: a vtable and the set
// of the signals it is connected to) disconnects from them all when it goes, and a signal from its slots' objects.
//
// The original's signal (0x10 bytes) is a vtable, the list of its connections (each a vtable, the slot's object and
// its method) and the list's size; connect is 0x408790 (pizzafrenzy) and its siblings, one for each signal type. The
// port keeps each connection as the object and a call of its method.

#include <functional>
#include <iterator>
#include <list>
#include <set>

namespace engine
{

class HasSlots;

class SignalBase
{
public:
	virtual ~SignalBase() {}
	// the object went: its connections are dropped (it has already let go of this signal)
	virtual void slotDisconnect(HasSlots* theTarget) = 0;
};

class HasSlots
{
public:
	HasSlots() {}
	HasSlots(const HasSlots&) = delete;
	HasSlots& operator=(const HasSlots&) = delete;
	virtual ~HasSlots() { disconnectAll(); }

	void signalConnect(SignalBase* theSender) { mSenders.insert(theSender); }
	void signalDisconnect(SignalBase* theSender) { mSenders.erase(theSender); }

	// has_slots::disconnect_all: every signal this is connected to drops it
	void disconnectAll()
	{
		std::set<SignalBase*> aSenders;
		aSenders.swap(mSenders);
		for (SignalBase* aSender : aSenders)
			aSender->slotDisconnect(this);
	}

private:
	std::set<SignalBase*> mSenders;
};

template <class... Args>
class Signal : public SignalBase
{
public:
	Signal() {}
	Signal(const Signal&) = delete;
	Signal& operator=(const Signal&) = delete;
	~Signal() override { disconnectAll(); }

	// connect: the connection goes at the end of the list, and the object learns of the signal
	template <class T>
	void connect(T* theTarget, void (T::*theMethod)(Args...))
	{
		mConnections.push_back(
			Connection{theTarget, [theTarget, theMethod](Args... theArgs) { (theTarget->*theMethod)(theArgs...); }});
		theTarget->signalConnect(this);
	}

	// disconnect: every connection to the object goes
	void disconnect(HasSlots* theTarget)
	{
		for (auto it = mConnections.begin(); it != mConnections.end();)
		{
			if (it->mTarget == theTarget)
				it = mConnections.erase(it);
			else
				++it;
		}
		theTarget->signalDisconnect(this);
	}

	void disconnectAll()
	{
		for (Connection& aConnection : mConnections)
			aConnection.mTarget->signalDisconnect(this);
		mConnections.clear();
	}

	void slotDisconnect(HasSlots* theTarget) override
	{
		for (auto it = mConnections.begin(); it != mConnections.end();)
		{
			if (it->mTarget == theTarget)
				it = mConnections.erase(it);
			else
				++it;
		}
	}

	// emit (operator()): each connection in turn, the next one taken before the call
	void emit(Args... theArgs)
	{
		auto it = mConnections.begin();
		while (it != mConnections.end())
		{
			auto aNext = std::next(it);
			it->mCall(theArgs...);
			it = aNext;
		}
	}

	bool empty() const { return mConnections.empty(); }

private:
	struct Connection
	{
		HasSlots* mTarget;
		std::function<void(Args...)> mCall;
	};
	std::list<Connection> mConnections;
};

} // namespace engine

#endif
