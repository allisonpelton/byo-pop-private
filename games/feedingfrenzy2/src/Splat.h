#ifndef __FEEDINGFRENZY2_SPLAT_H__
#define __FEEDINGFRENZY2_SPLAT_H__

// Splats: short-lived pictures or texts thrown out from a point, which slow, drift, grow and fade until their life
// runs out (the chomps' bursts on the HUD, the frenzy's words, the clock's warning, the stage's end counting the fish
// eaten). A factory reads the kinds from a file (<splats>, each <splat> a SplatDefinition), throws one out by name
// (spawn), and keeps them as its children until they die. Each splat draws its motion's ranges at its birth, nine
// draws of the C library's random numbers in a fixed order.

#include "Container.h"
#include "DocHandler.h"
#include "Font.h"
#include "Geometry.h"
#include "Surface.h"

#include <map>
#include <string>
#include <vector>

namespace engine
{
class TextItem;
}

// One kind of splat (SplatFactory::SplatDefinition, 0xf8 bytes, its constructor 0x41a710 (feedingfrenzy2)), with the
// defaults a <splat> starts from (0x479fd0 (feedingfrenzy2))
struct SplatDefinition : public engine::Object
{
	engine::FloatRange mVelocityX;             // +0x0c: <velocity startX>, pixels a second
	engine::FloatRange mVelocityY;             // +0x14: <velocity startY>
	engine::FloatRange mAcceleration;          // +0x1c: <acceleration rate>, along the velocity (below 0 it slows to 0)
	engine::FloatRange mStartScale{1.0f, 1.0f};  // +0x24: <scale startScale>
	engine::FloatRange mEndScale{1.0f, 1.0f};    // +0x2c: <scale endScale>
	float mGrowStart = 0.0f;                   // +0x34: <scale startAlpha>, the share of the life the growth starts at
	float mGrowEnd = 0.0f;                     // +0x38: <scale endAlpha>, and ends at
	// (+0x3c, a range set to 1 and 1, is read by nothing)
	engine::FloatRange mTurn;                  // +0x44: <rotation degrees>, the velocity turned by it (taken as radians)
	float mStartOpacity = 1.0f;                // +0x4c: <opacity startOp>
	float mEndOpacity = 1.0f;                  // +0x50: <opacity endOp>
	engine::Vector2f mForce;                   // +0x54: <extForce force>, added to the velocity each second
	// +0x5c and +0x6c: two of <jitter>'s four ranges, drawn at each birth; the element is not translated (the data has
	// none), so they stay 0 and the jitter (a modifier, 0x44 bytes, with +0x64 and +0x74) is never made
	engine::FloatRange mJitterX;
	engine::FloatRange mJitterY;
	engine::Ref<engine::Surface> mImage;       // +0x7c: <resource type="image" path>, its pivot by "pivot"
	engine::Ref<engine::Font> mFont;           // +0x80: <resource type="text" font>
	std::string mText;                         // +0x84: and its text
	bool mShadow = false;                      // +0xa0: its textStyle has "shadow"
	float mShadowOpacity = 0.0f;               // +0xa4: textShadowOpacity (1 by default)
	engine::Vector2f mShadowOffset;            // +0xa8: textShadowOffset (1, 1 by default)
	engine::Vector2f mDestination;             // +0xb0: <destination pos>; not translated (the data has none)
	engine::FloatRange mLifetime{1.0f, 1.0f};  // +0xb8: <splat lifeTime>, seconds
	engine::Color mColor{1.0f, 1.0f, 1.0f, 1.0f};  // +0xc0: the text's colour
	std::string mThemedImage;                  // +0xd0: <resource type="themedimage">; not translated (the data has
	                                           // none, and no factory is given a theme, its +0xb8)
};

// A splat (Splat, 0x190 bytes, vtables 0x55cf3c and 0x55d058; its constructor 0x483c60 (feedingfrenzy2)): its image
// or its text as its one child
class Splat : public engine::Container
{
	friend class SplatFactory;

public:
	// 0x483c60 (feedingfrenzy2): white text, scale and opacity 1, a life of 1, updated every frame
	Splat();

	// 0x68 (0x520617 (feedingfrenzy2))
	std::string getClassName() override { return "Splat"; }
	// 0x18 (0x5010de (feedingfrenzy2)): the position stored, the bounds left as they are
	void setPosition(float x, float y) override;
	using engine::Container::setPosition;
	// 0xa4 (0x434280 (feedingfrenzy2)): the life run down by the frame's time; a live splat's opacity eased from the
	// start's to the end's over its life (held to 0 to 1), its scale from the start's to the end's over the growth's
	// share of the life, its velocity sped up or slowed along itself (a slowing one stopping at 0, each axis on its
	// own) and pushed by the force, and it moved by the new velocity; then its children updated. A dead one is left
	void update(engine::UpdateContext& theContext) override;
	// 0xa8 (0x4230f0 (feedingfrenzy2)): the container's bounds, then the splat moved left or right to lie within
	// the window's width (by its bounds' half width either side of its position)
	void computeBounds() override;

	// 0x4089d0 (feedingfrenzy2): unless the text is empty, a text item made for it (0x40c850) and the bounds
	// recomputed
	void setText(const std::string& theText);
	float getLife() const { return mLife; }

private:
	// 0x40c850 (feedingfrenzy2): the splat's one child, a text item in its font, centred both ways, with the splat's
	// text shadow, and multiplied by its text colour unless that is white
	engine::TextItem* makeTextItem();
	// 0x413140 (feedingfrenzy2): where it was born, and its position
	void place(float x, float y);

	engine::Vector2f mVelocity;          // +0xc8
	float mAcceleration;                 // +0xd0
	float mStartScale;                   // +0xd4
	float mEndScale;                     // +0xd8
	float mGrowStart;                    // +0xdc
	float mGrowEnd;                      // +0xe0
	float mStartOpacity;                 // +0xe4
	float mEndOpacity;                   // +0xe8
	engine::Vector2f mForce;             // +0xec
	engine::Vector2f mBirthPosition;     // +0xf4
	// (+0xfc, the destination's point, and +0x11c, its mode, with the interpolator at +0x13c are left out: nothing
	// gives a splat a destination)
	engine::Color mTextColor;            // +0x104
	float mLifetime;                     // +0x114
	float mLife;                         // +0x118: below 0 the factory takes it out
	// (+0x120 and +0x124, a function and its argument called as the factory takes the splat out, and +0x128, are
	// cleared at each birth and set by nothing)
	bool mTextShadow;                    // +0x12c
	float mTextShadowOpacity;            // +0x130
	engine::Vector2f mTextShadowOffset;  // +0x134
};

// The splats of a file and the live ones (SplatFactory, 0xe0 bytes, vtables 0x55cdfc and 0x55cf18; its constructor
// 0x491cc0 (feedingfrenzy2)). A splat thrown out waits until the factory's next update to become its child.
class SplatFactory : public engine::Container
{
public:
	SplatFactory() {}

	// 0x68 (0x520714 (feedingfrenzy2))
	std::string getClassName() override { return "SplatFactory"; }
	// 0xa4 (0x433cb0 (feedingfrenzy2)): the waiting splats added, then each dead one (its life below 0) taken out
	// and each live one updated, in order. Neither the factory's own modifiers nor its position are applied
	void update(engine::UpdateContext& theContext) override;

	// 0x490b70 (feedingfrenzy2): the file's <splat>s added (SplatFactoryDocHandler)
	void load(const std::string& thePath);
	// 0x410620 (feedingfrenzy2), as the handler closes a <splat>
	void addDefinition(const std::string& theName, SplatDefinition* theDefinition);
	// 0x422ce0 (feedingfrenzy2): a splat of the kind born at the point in the factory's parent's space (moved left or
	// right to lie within the window), its ranges drawn; null, with a log line, for an unknown kind
	Splat* spawn(const std::string& theName, float x, float y);

private:
	// 0x4130e0 (feedingfrenzy2): the splat kept to be added at the next update
	void addSplat(Splat* theSplat);

	// (+0xb8, the theme of themed images, is never set)
	std::map<std::string, engine::Ref<SplatDefinition>> mDefinitions;  // +0xbc
	std::vector<engine::Ref<Splat>> mWaiting;                           // +0xc8
};

// The handler of a splats file (SplatFactoryDocHandler, 0x6c bytes, its constructor 0x48ea00 (feedingfrenzy2))
class SplatFactoryDocHandler : public engine::AbstractDocHandler
{
public:
	explicit SplatFactoryDocHandler(SplatFactory* theFactory) : mFactory(theFactory) {}

	// 0x00 (0x467940 (feedingfrenzy2)): <splat> begun (0x479fd0) and its elements read into it; <rotation> ends
	// the element's checks
	void startElement(const std::string& theName, engine::Params& theAttributes) override;
	// 0x04 (0x47a200 (feedingfrenzy2)): </splat> adds the kind under its name
	void endElement(const std::string& theName) override;

private:
	SplatFactory* mFactory;                      // +0x2c
	engine::Ref<SplatDefinition> mDefinition;   // +0x30
	std::string mName;                           // the <splat>'s name
};

#endif
