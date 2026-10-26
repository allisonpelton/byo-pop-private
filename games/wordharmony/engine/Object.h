#ifndef __WORDHARMONY_ENGINE_OBJECT_H__
#define __WORDHARMONY_ENGINE_OBJECT_H__

// Sprout Games' engine (the first generation, shared by Word Harmony, Pizza Frenzy and Iggle Pop): the reference-counted
// base of every engine class (games/wordharmony/docs/ENGINE.md, "Objects and references").
//
// In the original, engine::Interface is a virtual base with three slots and engine::Object implements them over a
// count; every class derives virtually from them, and every reference kept in a field or a container is counted. The
// port keeps that: classes derive from Object, interfaces (listeners) derive virtually from Interface, and Ref<T>
// counts the references the original counted by hand.
//
// Addresses in the comments are those of the game exe inside WordHarmony.exe, linked on 2006-02-22.

#include <cstddef>

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
	Object() : mRefCount(0) {}  // 0x435c10
	virtual ~Object() {}

	// 0x435bd0
	void addRef() override { ++mRefCount; }
	// 0x435be0: deletes the object when the last reference goes
	void release() override
	{
		int aCount = --mRefCount;
		if (aCount < 1)
			delete this;
	}
	// 0x435c00
	int refCount() const override { return mRefCount; }

private:
	int mRefCount;
};

// A counted reference (the original's inlined add-reference and release pairs)
template <class T>
class Ref
{
public:
	Ref() : mPtr(nullptr) {}
	Ref(T* thePtr) : mPtr(thePtr) { if (mPtr) mPtr->addRef(); }
	Ref(const Ref& theOther) : mPtr(theOther.mPtr) { if (mPtr) mPtr->addRef(); }
	template <class U>
	Ref(const Ref<U>& theOther) : mPtr(theOther.get()) { if (mPtr) mPtr->addRef(); }
	~Ref() { if (mPtr) mPtr->release(); }

	Ref& operator=(T* thePtr)
	{
		// the original adds the new reference before it releases the old one
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
