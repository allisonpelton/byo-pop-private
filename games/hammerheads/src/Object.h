#ifndef __HAMMERHEADS_OBJECT_H__
#define __HAMMERHEADS_OBJECT_H__

// The game's own object system: every thing on a level (the Avatar, gnomes, cubbies, stickers, floating text, the
// HUD) is an Object that an ObjectManager updates in fixed 0.01 steps and draws by layer. Objects find each other by
// name through ObjectRefs.

#include <string>
#include <typeinfo>
#include <vector>
#include <map>

#include "Rect.h"

namespace Sexy
{

class Graphics;
class Object;
class Cubby;

// Draw layers: an object is drawn when the layer being drawn is in its layer mask (bits 1, 2, 4 ... 0x800, in order).
// Layer 0x20 is drawn from the manager's list of cubbies sorted by their depth value instead.
enum
{
	LAYER_CUBBIES = 0x20
};

// A name that points at the live object of that name. The manager keeps every ref's pointer up to date as objects are
// added and removed, so a ref to an object that is gone reads as null.
class ObjectRef
{
public:
	std::string		mName;
	Object*			mObject;

public:
	ObjectRef();											// 0x402f50
	explicit ObjectRef(const Object* theObject);			// 0x402eb0
	ObjectRef(const ObjectRef& theRef);
	~ObjectRef();											// 0x402e40

	ObjectRef&		operator=(const ObjectRef& theRef);		// 0x402f80
	void			Set(const Object* theObject);			// 0x402eb0 + 0x402f80
	void			SetName(const std::string& theName);

	bool			IsValid() const;						// 0x402dc0: points at an object that is not dead
	Object*			Get() const;							// 0x402de0
	void			Kill() const;							// 0x402e00
};

class Object
{
public:
	bool			mDead;				// set by Kill; the manager deletes a dead object at its next pass
	int				mLayers;			// layer mask
	std::string		mName;
	bool			mActive;			// an inactive object is neither updated nor drawn
	int				mTickCount;			// fixed steps taken; the first one adopts the children
	double			mAge;				// in hundredths of the time passed to Update
	ObjectRef		mParent;			// set by the parent's first fixed step
	std::vector<ObjectRef> mChildren;	// killed along with this object

public:
	explicit Object(int theLayers);							// 0x402ae0
	virtual ~Object();										// 0x402b70

	virtual void	SetName(const std::string& theName);	// 0x402c90: a blank name is made up from the type's name and a count
	virtual void	FixedUpdate();							// 0x402a00: one 0.01 step
	virtual void	Update(float theFrac);					// 0x402950
	virtual void	Kill();									// 0x402970
	virtual void	Draw(Graphics* g);						// 0x41ee00: nothing
	virtual const char* TypeName() const = 0;				// the original's typeid(*this).name(), "class Sexy::Cubby"
};

#define OBJECT_TYPE(theName) virtual const char* TypeName() const override { return "class Sexy::" #theName; }

// An Object with a position and a size (0x405c20). The original's fields at +0x80 to +0x8f are a rect, 0x90 and 0x98 the
// position as doubles.
class SimpleObject : public Object
{
public:
	Rect			mBounds;
	double			mX;
	double			mY;

public:
	explicit SimpleObject(int theLayers);					// 0x405c20
	virtual ~SimpleObject();								// 0x405c00
};

// 0x4042d0: owns the objects between Add and the pass that deletes them.
class ObjectManager
{
public:
	std::vector<Object*>		mObjects;
	std::map<std::string, Object*> mByName;
	int							mCurrentLayer;
	int							mFixedStepCount;
	double						mAccumulator;			// hundredths of a time unit not yet used by a fixed step
	std::vector<ObjectRef*>		mRefs;
	std::vector<Cubby*>		mCubbiesByDepth;

public:
	ObjectManager();										// 0x4042d0
	~ObjectManager();
	static ObjectManager* Get();							// 0x404380: made on first use

	void			Add(Object* theObject);					// 0x4044a0
	void			Remove(Object* theObject);				// 0x4039f0 (what Object's destructor calls)
	void			Clear();								// 0x403d70
	void			Update(float theFrac);					// 0x403160
	void			FixedUpdate();							// 0x403100
	void			Draw(Graphics* g);						// 0x403200
	Object*			FindByName(const std::string& theName);	// 0x403640
	void			AddRef(ObjectRef* theRef);				// 0x403500
	void			RemoveRef(ObjectRef* theRef);			// 0x403410
	void			ActivateAll();							// 0x403340
	void			DeactivateAll();						// 0x403310
	void			RemoveByActive(bool theActive);			// 0x403370: deletes every object whose active flag is theActive

	template<typename T> T* FindByType(int* theStart = nullptr);	// 0x403290

	static ObjectManager* Existing();						// null before the first use and after the manager is gone
};

template<typename T> T* ObjectManager::FindByType(int* theStart)
{
	for (int i = theStart ? *theStart : 0; i < (int)mObjects.size(); i++)
	{
		Object* anObject = mObjects[i];
		if (!anObject->mDead && typeid(*anObject) == typeid(T))
		{
			if (theStart)
				*theStart = i;
			return static_cast<T*>(anObject);
		}
	}
	return nullptr;
}

}

#endif
