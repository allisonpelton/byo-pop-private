#ifndef __WORDHARMONY_ENGINE_TEXTEFFECTS_H__
#define __WORDHARMONY_ENGINE_TEXTEFFECTS_H__

// Floating texts ("splats") made from named templates. The factory (0xbc bytes, constructor 0x429760, vtable 0x483618,
// named "SplatFactory"; the controller's +0x70) reads the templates from a file and makes a text from one, drawing its
// motion from the template's ranges; the texts are its children until their time is up.

#include "Node.h"
#include "Xml.h"
#include <map>
#include <string>

namespace engine
{

// What a text calls when its time is up, with the argument it was made with
typedef void (*TextEffectCallback)(int theArg);

// A template (0xc8 bytes, constructor 0x430c20); the ranges are (low, high) pairs
struct TextEffectTemplate
{
	TextEffectTemplate();  // 0x430c20

	Point mVelocityX;        // +0x00: "velocity" startX, 0-10
	Point mVelocityY;        // +0x08: startY, 0-10
	Point mAcceleration;     // +0x10: "acceleration" rate, along the motion
	Point mStartScale;       // +0x18: "scale"
	Point mEndScale;         // +0x20
	Point mRotation;         // +0x30: "rotation" degrees, the velocity turned by
	float mStartOpacity;     // +0x38: "opacity" startOp, 1
	float mEndOpacity;       // +0x3c: endOp, 1
	Point mForce;            // +0x40: "extForce" force (a point, not a range)
	Point mJitterRateX;      // +0x48: "jitter" axis X rate
	Point mJitterAmountX;    // +0x50: amplitude
	Point mJitterRateY;      // +0x58: axis Y
	Point mJitterAmountY;    // +0x60
	std::string mImagePath;  // +0x68: "resource" type image path
	std::string mFont;       // +0x84: type text font
	std::string mText;       // +0xa0: text
	Point mDestination;      // +0xbc: "destination" pos; the text then moves there in its time instead
	float mLifeTime;         // +0xc4: "splat" lifeTime, 1
};

// A floating text (0x114 bytes, constructor 0x42a890, vtable 0x483a50, named "Splat")
class TextEffect : public Container
{
public:
	TextEffect();  // 0x42a890

	void setPosition(float x, float y) override;  // 0x18: 0x42a400 (the position only)
	using Node::setPosition;
	void applyTransform(Graphics* g) override;    // 0x3c: 0x42a750 (no rotation or colour offset)
	void update(UpdateInfo& theInfo) override;    // 0x70: 0x42a4b0

	// 0x42a410: where it starts
	void setOrigin(float x, float y);
	// 0x42a430
	void setDestination(float x, float y);
	// 0x42a450
	void setCallback(TextEffectCallback theCallback, int theArg);
	// 0x42a470
	bool isDone() { return mTimeLeft < 0; }
	// 0x42a490: the callback called
	void done();
	// 0x42a7a0: theText in the text's font, centred, in place of its children (nothing for an empty text)
	void setText(std::string theText);

	Point mVelocity;               // +0xa8
	float mAcceleration;           // +0xb0
	float mStartScale;             // +0xb4
	float mEndScale;               // +0xb8
	float mStartOpacity;           // +0xbc
	float mEndOpacity;             // +0xc0
	Point mForce;                  // +0xc4
	Point mOrigin;                 // +0xcc
	Point mDestination;            // +0xd4
	std::string mFontName;         // +0xdc
	float mLifeTime;               // +0xf8
	float mTimeLeft;               // +0xfc
	bool mHasDestination;          // +0x100
	TextEffectCallback mCallback;  // +0x104
	int mCallbackArg;              // +0x108
};

class TextEffects : public Container
{
public:
	TextEffects();  // 0x429760

	void update(UpdateInfo& theInfo) override;  // 0x70: 0x428940, the texts whose time is up removed

	// 0x428780: the templates of theFile added
	bool load(std::string theFile);
	// 0x4289d0: a text of the template theName at (x, y), or null; theDone is called with theArg when its time is up
	TextEffect* create(const std::string& theName, float x, float y, TextEffectCallback theDone, int theArg);
	// 0x429740
	void addTemplate(std::string theName, TextEffectTemplate* theTemplate);
	// 0x4288a0: whether every text's time is up
	bool allDone();
	// 0x4288e0: every text removed, its callback called
	void clear();

	std::map<std::string, TextEffectTemplate*> mTemplates;  // +0xa8 (the templates are never freed)
};

// The template file's reader (0x40 bytes, constructor 0x430ab0, vtable 0x484288)
class TextEffectHandler : public XmlHandler
{
public:
	TextEffectHandler() : mTemplate(nullptr), mFactory(nullptr) {}  // 0x430ab0

	void setFactory(TextEffects* theFactory) { mFactory = theFactory; }  // 0x4337f0

	void startElement(const std::string& theName, const XmlAttributes& theAttributes) override;  // 0x430ee0
	void endElement(const std::string& theName) override;                                       // 0x430a70

	// 0x20: 0x430d20, a "splat" element: a new template
	virtual void startTemplate(const std::string& theName, XmlAttributes theAttributes);

private:
	TextEffectTemplate* mTemplate;  // +0x14
	TextEffects* mFactory;          // +0x18
	std::string mName;              // +0x1c
};

} // namespace engine

#endif
