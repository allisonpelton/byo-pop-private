#ifndef __FEEDINGFRENZY2_BUBBLETEXT_H__
#define __FEEDINGFRENZY2_BUBBLETEXT_H__

// A word of letters in bubbles (BubbleText, 0x128 bytes, vtable 0x555df4 (feedingfrenzy2); its constructor 0x488190
// (feedingfrenzy2)): the splash's "LOADING". Each letter that is not a space is a glyph (BubbleGlyph, 0xdc bytes,
// vtable 0x555cbc (feedingfrenzy2); its constructor 0x478cf0 (feedingfrenzy2)): a bubble image, the letter centred on
// it and an effect, rocked up and down by a wave of its own. The glyphs grow in one after another, and burst into
// their effects at the end. The glyphs a text no longer uses are kept for the next.

#include "Container.h"
#include "Image.h"
#include "ParticleSystem.h"
#include "ParticleSystemDocHandler.h"
#include "TextItem.h"

#include <string>
#include <vector>

// Where a glyph is in its life (+0xd0)
enum BubbleGlyphPhase
{
	BUBBLE_GLYPH_GROWING = 0,   // its scale growing from 0 to 1
	BUBBLE_GLYPH_WAITING = 1,   // hidden at scale 0 until its delay has passed
	BUBBLE_GLYPH_SHOWN = 2,     // at full size
	BUBBLE_GLYPH_BURST = 4      // its effect started and its bubble and letter hidden (and before it first appears)
};

class BubbleGlyph : public engine::Container
{
public:
	// 0x478cf0 (feedingfrenzy2): empty, burst
	BubbleGlyph();

	// 0xa4 (0x437dc0 (feedingfrenzy2)): the delay counted down, or the scale grown, then the children updated
	void update(engine::UpdateContext& theContext) override;

	// 0x473d30 (feedingfrenzy2): the bubble (when there is one), the letter in the colour and, above them, the
	// effect, made and added; the glyph hidden
	void init(engine::Surface* theBubble, engine::ParticleSystemDef* theEffect, const engine::Color& theColor);
	// 0x410db0 (feedingfrenzy2)
	void setLetter(char theLetter);
	// 0x414c20 (feedingfrenzy2): shown at scale 0, then grown to full size over the time once the delay has passed
	void appear(float theDelay, float theGrowTime);
	// 0x410d40 (feedingfrenzy2): unless waiting or burst already, the effect started again and the bubble and letter
	// hidden
	void burst();
	// 0x4376f0 (feedingfrenzy2), as 0x40ebe0 (feedingfrenzy2) asks it
	bool isEffectDone() { return mEffect->isDone(); }

private:
	engine::Ref<engine::ParticleSystem> mEffect;   // +0xb8
	engine::Ref<engine::TextItem> mLetter;         // +0xbc
	engine::Ref<engine::Image> mBubble;            // +0xc0: none when the text has no bubble image
	float mTimer;                                  // +0xc4: the seconds of the delay or the growth left
	float mGrowTime;                               // +0xc8
	float mDelay;                                  // +0xcc
	int mPhase;                                    // +0xd0: BubbleGlyphPhase
};

class BubbleText : public engine::Container
{
public:
	// 0x488190 (feedingfrenzy2): no bubble, effect or spacing; red letters
	BubbleText();
	// 0x487d00 (feedingfrenzy2)
	~BubbleText() override;

	// 0x418f10 (feedingfrenzy2): a glyph for each letter that is not a space, hidden, the lines (split at each "\n",
	// a backslash and an n) centred on the position one under another
	void setText(const std::string& theText);
	// 0x409dd0 (feedingfrenzy2): the glyphs grown in one after another from the start, the last starting a fifth of
	// a second before the duration ends
	void appear(float theStart, float theDuration);
	// 0x4928a0 (feedingfrenzy2): every glyph burst
	void burst();
	// 0x40ebe0 (feedingfrenzy2): every glyph's effect done
	bool isDone();

	// Set where the original writes the fields (the splash's 0x409540 (feedingfrenzy2))
	void setBubble(engine::Surface* theBubble) { mBubble = theBubble; }
	void setEffect(engine::ParticleSystemDef* theEffect) { mEffect = theEffect; }
	void setLetterColor(const engine::Color& theColor) { mLetterColor = theColor; }
	void setSpacing(float theSpacing) { mSpacing = theSpacing; }
	// 0x475e00 (feedingfrenzy2): glyphs made for the next texts, each with its wave (the stage's word makes them
	// ready as it loads)
	void addFreeGlyphs(int theCount);

private:
	// 0x4883a0 (feedingfrenzy2): the glyphs taken out and kept for the next text
	void clear();

	engine::Ref<engine::Surface> mBubble;                  // +0xb8
	engine::Ref<engine::ParticleSystemDef> mEffect;        // +0xbc
	float mSpacing;                                        // +0xc0: the pixels between two glyphs
	std::vector<engine::Ref<BubbleGlyph>> mGlyphs;         // +0xc4: the text's, in its order
	std::vector<engine::Ref<BubbleGlyph>> mFreeGlyphs;     // +0xd4: those kept for the next text
	// +0xe8: 0x28 bytes holding a buffer, which the constructor zeroes (0x421c00 (feedingfrenzy2)) and the
	// destructor frees, with no reader found: not translated
	engine::Color mLetterColor;                            // +0x110
};

#endif
