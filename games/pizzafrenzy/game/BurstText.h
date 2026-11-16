#ifndef __PIZZAFRENZY_BURSTTEXT_H__
#define __PIZZAFRENZY_BURSTTEXT_H__

// The title a level's end shows over the city (ModeStates.h, BonusStates.h): a line of letters that grow in one after
// another, bob up and down, and burst when they leave, each letter with a particle system of its own that starts when
// it leaves (the tile manifest's "BurstFx").
//
// BurstLetter (0x154 bytes, constructor 0x42b230, vtable 0x4fbc20) is one letter; BurstText (0x148 bytes, constructor
// 0x42b800, vtable 0x4fbd68) is the line.

#include "Node.h"
#include "SignalParticles.h"
#include "TextItem.h"

#include <string>
#include <vector>

namespace pizzafrenzy
{

class BurstLetter : public engine::Container
{
public:
	BurstLetter();  // 0x42b230

	// slot 0x94: 0x42ab80: a frame of the letter's stage, and its children's
	void update(engine::UpdateInfo& theInfo) override;

	// 0x42b020: the letter is a text node of the character in the font, with a particle system of the definition (if
	// any), hidden until it starts; it waits the delay before it grows. The character's width in the font is returned.
	int setLetter(char theChar, engine::Font* theFont, engine::ParticleSystemDef* theBurst, float theDelay);
	// 0x42af20: the letter stands at the point, shown at nothing, and grows (after its delay); a space never shows
	void start(const engine::Point& thePosition);
	// 0x42afb0: a letter that is growing or shown bursts: its particle system starts and its text is hidden
	void leave();
	char getChar() const { return mChar; }

private:
	// The stages: the letter waits its delay, grows, and is shown until it leaves. The original's code has three more,
	// a hold after the growth and the shrink that follows it, which only a hold time sets off (+0x140), and nothing
	// sets one; they are left out.
	enum
	{
		STAGE_GROWING = 0,  // the scale goes from nothing to one over the growth's seconds
		STAGE_WAITING = 2,  // the delay before the growth
		STAGE_SHOWN = 4,
		STAGE_IDLE = 7      // not started, or gone
	};

	engine::Ref<engine::SignalParticleSystem> mBurst;  // +0x128
	engine::Ref<engine::TextItem> mText;               // +0x12c
	float mTimeLeft;                                   // +0x134: the seconds left of the stage
	float mGrowSeconds;                                // +0x138
	float mDelay;                                      // +0x13c: the seconds before it grows
	char mChar;                                        // +0x144
	bool mIsSpace;                                     // +0x145: a space never shows
	int mStage;                                        // +0x148
};

class BurstText : public engine::Container
{
public:
	BurstText();  // 0x42b800

	// 0x42b8a0: a letter for each character of the text in the font, the next letter's delay the step longer than the
	// one before, the first's the first delay, and the letters spaced apart by the spacing; with a wave size, each
	// letter bobs up and down by that much of the font's height, a tenth of a second behind the one before
	void setText(const std::string& theText, float theDelayStep, float theSpacing, engine::Font* theFont,
		engine::ParticleSystemDef* theBurst, float theFirstDelay, float theWaveSize);
	// 0x42b350: the line is centred on the point, and its letters start
	void start(const engine::Point& thePosition);
	// 0x42b460: every letter leaves
	void leave();

private:
	std::vector<BurstLetter*> mLetters;  // +0x128 (the letters are the container's children)
	float mSpacing;                      // +0x138
	float mWidth;                        // +0x13c: the letters' widths and the spacing after each
};

// The title of a level's end (0x458490) and of the bonus round's (0x45d1b0), which make it the same way: the string
// table's text in the title font, its letters a tenth of a second apart, bobbing by a twentieth of the font's height
// and bursting with the tile manifest's "BurstFx", put on the screen and started over its top centre
BurstText* makeEndTitle(engine::Container* theScreen, int theStringId);

} // namespace pizzafrenzy

#endif
