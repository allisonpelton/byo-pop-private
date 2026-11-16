#ifndef __SPROUT2_SIGSLOT_H__
#define __SPROUT2_SIGSLOT_H__

// The engine's signals and slots: Sarah Thompson's public-domain sigslot library as Sprout built it, its single-threaded
// policy only, and its has_slots class named HasSlots. A signal keeps a list of connections, each an object and one of
// its member functions; emitting calls them in the order they were connected. An object that receives signals derives
// from HasSlots, which keeps the set of signals connected to it so that either side's destruction disconnects the
// other.
//
// The original instantiates signal0, signal1<A> and signal2<A, B> (and their bases and connections) once per argument
// list; the port writes them once over a parameter pack and names the three by their arity.

#include <list>
#include <set>

namespace sigslot
{

class HasSlots;

class _signal_base
{
public:
	virtual ~_signal_base() {}
	// 0x40b670 (feedingfrenzy2) for every signal: delete the connections whose object is theDest
	virtual void slot_disconnect(HasSlots* theDest) = 0;
	// 0x52d902 (feedingfrenzy2) for signal0: copy theOld's connections to theNew
	virtual void slot_duplicate(const HasSlots* theOld, HasSlots* theNew) = 0;
};

class HasSlots
{
public:
	HasSlots() {}
	HasSlots(const HasSlots& theOther)
	{
		for (_signal_base* aSender : theOther.mSenders)
		{
			aSender->slot_duplicate(&theOther, this);
			mSenders.insert(aSender);
		}
	}
	HasSlots& operator=(const HasSlots&) = delete;
	// 0x41ad10 (feedingfrenzy2)
	virtual ~HasSlots() { disconnect_all(); }

	void signal_connect(_signal_base* theSender) { mSenders.insert(theSender); }
	void signal_disconnect(_signal_base* theSender) { mSenders.erase(theSender); }

	// 0x471f20 (feedingfrenzy2): every signal connected to this object forgets it
	void disconnect_all()
	{
		for (_signal_base* aSender : mSenders)
			aSender->slot_disconnect(this);
		mSenders.clear();
	}

private:
	std::set<_signal_base*> mSenders;
};

template <class... Args>
class _connection_base
{
public:
	virtual ~_connection_base() {}  // port: the original's connections have no destructor and are freed as the base
	virtual HasSlots* getdest() const = 0;
	virtual void emit(Args... theArgs) = 0;
	virtual _connection_base* clone() = 0;
	virtual _connection_base* duplicate(HasSlots* theNewDest) = 0;
};

template <class Dest, class... Args>
class _connection : public _connection_base<Args...>
{
public:
	typedef void (Dest::*Method)(Args...);

	_connection(Dest* theObject, Method theMethod) : mObject(theObject), mMethod(theMethod) {}

	HasSlots* getdest() const override { return mObject; }
	void emit(Args... theArgs) override { (mObject->*mMethod)(theArgs...); }
	_connection_base<Args...>* clone() override { return new _connection(*this); }
	_connection_base<Args...>* duplicate(HasSlots* theNewDest) override
	{
		return new _connection(static_cast<Dest*>(theNewDest), mMethod);
	}

private:
	Dest* mObject;
	Method mMethod;
};

template <class... Args>
class _signal_base_n : public _signal_base
{
public:
	typedef std::list<_connection_base<Args...>*> ConnectionList;

	_signal_base_n() {}
	_signal_base_n(const _signal_base_n& theOther) : _signal_base()
	{
		for (_connection_base<Args...>* aConnection : theOther.mConnected)
		{
			aConnection->getdest()->signal_connect(this);
			mConnected.push_back(aConnection->clone());
		}
	}
	_signal_base_n& operator=(const _signal_base_n&) = delete;
	~_signal_base_n() { disconnect_all(); }

	void disconnect_all()
	{
		for (_connection_base<Args...>* aConnection : mConnected)
		{
			aConnection->getdest()->signal_disconnect(this);
			delete aConnection;
		}
		mConnected.clear();
	}

	void disconnect(HasSlots* theDest)
	{
		for (typename ConnectionList::iterator it = mConnected.begin(); it != mConnected.end(); ++it)
		{
			if ((*it)->getdest() == theDest)
			{
				delete *it;
				mConnected.erase(it);
				theDest->signal_disconnect(this);
				return;
			}
		}
	}

	void slot_disconnect(HasSlots* theDest) override
	{
		typename ConnectionList::iterator it = mConnected.begin();
		while (it != mConnected.end())
		{
			typename ConnectionList::iterator aNext = it;
			++aNext;
			if ((*it)->getdest() == theDest)
			{
				delete *it;
				mConnected.erase(it);
			}
			it = aNext;
		}
	}

	void slot_duplicate(const HasSlots* theOld, HasSlots* theNew) override
	{
		for (_connection_base<Args...>* aConnection : ConnectionList(mConnected))
		{
			if (aConnection->getdest() == theOld)
				mConnected.push_back(aConnection->duplicate(theNew));
		}
	}

protected:
	ConnectionList mConnected;
};

template <class... Args>
class signal : public _signal_base_n<Args...>
{
public:
	// The connect functions (0x49f1f0 (feedingfrenzy2) Scene's to the update signal, for one): the connection goes last
	// in the list and the object learns of the signal
	template <class Dest>
	void connect(Dest* theObject, void (Dest::*theMethod)(Args...))
	{
		this->mConnected.push_back(new _connection<Dest, Args...>(theObject, theMethod));
		theObject->signal_connect(this);
	}

	// The emit functions (0x45dc20 (feedingfrenzy2) the update signal's, 0x435ab0 (feedingfrenzy2) the draw signal's):
	// each connection in turn, the next one found before the call so a slot may disconnect itself
	void emit(Args... theArgs)
	{
		typename _signal_base_n<Args...>::ConnectionList::const_iterator it = this->mConnected.begin();
		while (it != this->mConnected.end())
		{
			typename _signal_base_n<Args...>::ConnectionList::const_iterator aNext = it;
			++aNext;
			(*it)->emit(theArgs...);
			it = aNext;
		}
	}
	void operator()(Args... theArgs) { emit(theArgs...); }
};

typedef signal<> signal0;
template <class A>
using signal1 = signal<A>;
template <class A, class B>
using signal2 = signal<A, B>;

} // namespace sigslot

#endif
