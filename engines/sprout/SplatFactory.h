#ifndef __SPROUT_SPLATFACTORY_H__
#define __SPROUT_SPLATFACTORY_H__

// The signal build's splats (Pizza Frenzy's; Iggle Pop's Mac build names Splat, SplatFactory and
// SplatFactoryDocHandler). The factory (0x14c bytes, constructor 0x474f40 (pizzafrenzy), vtable 0x5028e8 (pizzafrenzy);
// its name is "SplatFactory") is a container the scene holds. It reads the definitions of res\manifests\splats.xml and
// makes a splat from one: a short lived node (a picture, or a text in a font) that flies, grows or shrinks and fades
// over its life time, then tells whoever waits for it and goes. Word Harmony has its own text effects (TextEffects.h)
// and none of this is read in its build.

#include "Color4.h"
#include "Node.h"
#include "Signal.h"
#include "Xml.h"
#include <map>
#include <string>
#include <vector>

namespace engine
{

class SplatFactory;

// What a splat calls when it has arrived, with the argument it was made with
typedef void (*SplatCallback)(int theArg);

// A definition (0xf8 bytes, constructor 0x492480 (pizzafrenzy)); each range is a pair (x the low end, y the high end)
// that every splat draws its own value from
struct SplatDef
{
	SplatDef();  // 0x492480 (pizzafrenzy), 0x4925a0 (pizzafrenzy) for the values a "splat" element starts with

	Point mVelocityX;        // +0x00: "velocity" startX
	Point mVelocityY;        // +0x08: startY
	Point mAcceleration;     // +0x10: "acceleration" rate, along the motion
	Point mStartScale;       // +0x18: "scale" startScale
	Point mEndScale;         // +0x20: endScale
	float mScaleStart;       // +0x28: startAlpha, the share of the life time at which the scale starts to change
	float mScaleEnd;         // +0x2c: endAlpha, the share at which it has changed
	Point mUnreadScale;      // +0x30: (1, 1) when a definition starts; nothing reads it
	Point mRotation;         // +0x38: "rotation" degrees, the velocity turned by
	float mStartOpacity;     // +0x40: "opacity" startOp
	float mEndOpacity;       // +0x44: endOp
	Point mForce;            // +0x48: "extForce" force (a point, not a range)
	Point mWavePeriodX;      // +0x50: "jitter" axis X period
	Point mWaveSizeX;        // +0x58: amplitude
	Point mWavePeriodY;      // +0x60: axis Y period
	Point mWaveSizeY;        // +0x68: amplitude
	std::string mImagePath;  // +0x70: "resource" type image path
	std::string mFont;       // +0x8c: type text font
	std::string mText;       // +0xa8: text
	bool mShadow;            // +0xc4: textStyle holds "shadow"
	float mShadowOpacity;    // +0xc8: textShadowOpacity
	Point mShadowOffset;     // +0xcc: textShadowOffset
	Point mDestination;      // +0xd4: "destination" pos; the splat then flies there, at the speed of its velocity
	Point mLifeTime;         // +0xdc: "splat" lifeTime, seconds
	Color4 mColor;           // +0xe4: the text's colour
	int mCentered;           // +0xf4: the picture's pivot is its centre (pivot "center")
};

// A splat (0x1d0 bytes, constructor 0x481810 (pizzafrenzy), vtable 0x503610 (pizzafrenzy), named "Splat"). Each frame
// it takes its time from its life time and sets its scale and opacity from where it is in it; it moves on its velocity
// (sped up along its motion, pushed by its force), or from its origin to its destination in its life time.
class Splat : public Container
{
public:
	Splat();  // 0x481810 (pizzafrenzy)

	using Node::setPosition;
	void setPosition(float x, float y) override { mPosition.set(x, y); }  // 0x18: 0x4811d0 (pizzafrenzy)
	void update(UpdateInfo& theInfo) override;                              // 0x94: 0x4812f0 (pizzafrenzy)
	std::string getName() override { return "Splat"; }                      // 0x60: 0x4819b0 (pizzafrenzy)

	// 0x4811e0 (pizzafrenzy): where it starts, and it goes there
	void setOrigin(float x, float y);
	// 0x481200 (pizzafrenzy): it flies to the point instead, in the time its velocity's speed takes for the distance
	// from its origin
	void setDestination(float x, float y);
	// 0x481280 (pizzafrenzy): called when it has arrived, before the signal
	void setCallback(SplatCallback theCallback, int theArg)
	{
		mCallback = theCallback;
		mCallbackArg = theArg;
	}
	// 0x4812a0 (pizzafrenzy): a text's shadow, with the offset (a text made after it)
	void setShadow(bool theShadow, float theOpacity, const Point& theOffset);
	// 0x4815f0 (pizzafrenzy): theText in the splat's font, centred, in place of its children, kept within the
	// screen's width (nothing for an empty text)
	void setText(const std::string& theText);
	// 0x4812d0 (pizzafrenzy): the node in place of its children
	void setChild(Node* theNode);
	// 0x4817c0 (pizzafrenzy): it has arrived: the callback is called and the signal emitted
	void arrive();
	// the factory removes a splat when its time is up
	bool isFinished() const { return mTimeLeft < 0.0f; }

	// the splat's end, which whatever waits for it connects to: the signal sends mArrivedValue. A method of no
	// arguments ignores it.
	template <class T>
	void connectArrived(T* theTarget, void (T::*theMethod)())
	{
		mArrived.connectCall(theTarget, [theTarget, theMethod](int) { (theTarget->*theMethod)(); });
	}
	template <class T>
	void connectArrived(T* theTarget, void (T::*theMethod)(int))
	{
		mArrived.connect(theTarget, theMethod);
	}
	Signal<int> mArrived;  // +0x128
	Point mVelocity;    // +0x138
	float mAcceleration;       // +0x140: along the velocity
	float mStartScale;         // +0x144
	float mEndScale;           // +0x148
	float mScaleStart;         // +0x14c: the share of the life time at which the scale starts to change
	float mScaleEnd;           // +0x150
	float mStartOpacity;       // +0x154
	float mEndOpacity;         // +0x158
	Point mForce;              // +0x15c: added to the velocity each second
	Point mOrigin;             // +0x164
	Point mDestination;        // +0x16c
	std::string mFontName;     // +0x174
	Color4 mColor;             // +0x190: the text's
	float mLifeTime;           // +0x1a0 (seconds)
	float mTimeLeft;           // +0x1a4: the factory removes the splat when it goes below 0
	bool mHasDestination;      // +0x1a8
	SplatCallback mCallback;   // +0x1ac
	int mCallbackArg;          // +0x1b0
	int mArrivedValue;         // +0x1b4: what the signal sends
	bool mShadow;              // +0x1b8
	float mShadowOpacity;      // +0x1bc
	Point mShadowOffset;       // +0x1c0
};

class SplatFactory : public Container
{
public:
	SplatFactory();            // 0x474f40 (pizzafrenzy)
	~SplatFactory() override;  // 0x4748c0 (pizzafrenzy)

	// 0x4739f0 (pizzafrenzy): the definitions of the file, through the application's stream
	void load(const std::string& theFile);
	// 0x474a20 (pizzafrenzy): a splat of the definition named theName, starting at (x, y), or null when there is
	// none; its picture is kept within the screen's width. The splat joins the factory's children at the next update.
	// Each splat draws from the generator, in this order: the acceleration, the velocity's two numbers, the angle the
	// velocity turns by, the start and end scales, the two wave periods and (when either is not 0) the wave's two
	// sizes, and the life time.
	Splat* create(const std::string& theName, float x, float y, SplatCallback theArrived = nullptr, int theArg = 0);
	// 0x475020 (pizzafrenzy): a definition added, in place of one of the same name
	void addDef(const std::string& theName, SplatDef* theDef);
	// 0x98: 0x473aa0 (pizzafrenzy): the bounds are an empty rectangle, whatever the children are
	void layout() override;
	// 0x60: 0x4749f0 (pizzafrenzy)
	std::string getName() override { return "SplatFactory"; }
	// 0x94: 0x473b90 (pizzafrenzy): the splats made since the last frame become children; each splat whose time is not
	// up has its frame, and each other one has arrived and is removed
	void update(UpdateInfo& theInfo) override;

private:
	// 0x474670 (pizzafrenzy): a splat to become a child at the next update
	void queue(Splat* theSplat);

	std::string mFile;                                  // the definitions' file
	std::map<std::string, SplatDef*> mDefs;             // +0x128 (never freed, as in the original's destructor too)
	std::vector<Ref<Splat>> mQueued;                    // +0x134
};

// The definitions' file's reader (0x40 bytes, constructor 0x492369 (pizzafrenzy), vtable 0x50488c (pizzafrenzy)): each
// "splat" element starts a definition, and the elements inside it set its parts
class SplatFactoryDocHandler : public XmlHandler
{
public:
	SplatFactoryDocHandler() : mDef(nullptr), mFactory(nullptr) {}

	void setFactory(SplatFactory* theFactory) { mFactory = theFactory; }

	// 0x4928c0 (pizzafrenzy)
	void startElement(const std::string& theName, const XmlAttributes& theAttributes) override;
	// 0x492320 (pizzafrenzy)
	void endElement(const std::string& theName) override;

private:
	// 0x4925a0 (pizzafrenzy), a "splat" element: a new definition
	void startSplat(const XmlAttributes& theAttributes);

	SplatDef* mDef;          // +0x14: the definition being read
	SplatFactory* mFactory;  // +0x18 (via +0xc)
	std::string mName;       // +0x1c
};

} // namespace engine

#endif
