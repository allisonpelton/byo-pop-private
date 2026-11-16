#ifndef __SPROUT2_OBJECT_H__
#define __SPROUT2_OBJECT_H__

// Sprout Games' second-generation engine (Feeding Frenzy 2 and Talismania; docs/ENGINES.md, "Sprout's second
// generation"): the reference-counted base of every engine class.
//
// In the original, engine::Interface is a virtual base with three slots (add a reference, release one, read the
// count), and engine::Object implements them over an interlocked count; every class derives virtually from Interface
// through Object, and an interface class (a listener such as Eater) derives from Interface too, so an object has one
// count. Every reference kept in a field or a container is added and released by hand. The port keeps the model:
// classes derive from Object, interface classes derive virtually from Interface, and Ref<T> counts the references the
// original counted by hand. The port runs one thread, so the count needs no interlocking.
//
// Each address in the comments names the game exe it belongs to, by the game's folder name.

namespace engine
{

class Interface
{
public:
	virtual ~Interface() {}
	virtual void addRef() = 0;
	virtual void release() = 0;
	virtual int refCount() const = 0;
};

class Object : public virtual Interface
{
public:
	Object() : mRefCount(0) {}
	virtual ~Object() {}

	// 0x4a9800 (feedingfrenzy2): InterlockedIncrement
	void addRef() override { ++mRefCount; }
	// 0x45e9c0 (feedingfrenzy2): InterlockedDecrement; the object deletes itself when the count reaches 0
	void release() override
	{
		if (--mRefCount <= 0)
			delete this;
	}
	// 0x4ff7ab (feedingfrenzy2)
	int refCount() const override { return mRefCount; }

private:
	int mRefCount;
};

// A counted reference: the original's inlined add-reference and release pairs, the new reference added before the old
// one is released
template <class T>
class Ref
{
public:
	Ref() : mPtr(nullptr) {}
	Ref(T* thePtr) : mPtr(thePtr)
	{
		if (mPtr)
			mPtr->addRef();
	}
	Ref(const Ref& theOther) : mPtr(theOther.mPtr)
	{
		if (mPtr)
			mPtr->addRef();
	}
	template <class U>
	Ref(const Ref<U>& theOther) : mPtr(theOther.get())
	{
		if (mPtr)
			mPtr->addRef();
	}
	~Ref()
	{
		if (mPtr)
			mPtr->release();
	}

	Ref& operator=(T* thePtr)
	{
		if (thePtr)
			thePtr->addRef();
		T* anOld = mPtr;
		mPtr = thePtr;
		if (anOld)
			anOld->release();
		return *this;
	}
	Ref& operator=(const Ref& theOther) { return *this = theOther.mPtr; }

	T* get() const { return mPtr; }
	T* operator->() const { return mPtr; }
	T& operator*() const { return *mPtr; }
	operator T*() const { return mPtr; }

private:
	T* mPtr;
};

} // namespace engine

#endif
