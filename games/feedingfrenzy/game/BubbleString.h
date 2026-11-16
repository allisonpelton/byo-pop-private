#ifndef __FEEDINGFRENZY_BUBBLESTRING_H__
#define __FEEDINGFRENZY_BUBBLESTRING_H__

// The banner a stage ends with (BubbleString, 0x138 bytes, constructor 0x43ea90, vtable 0x4a7920): a line of letters,
// each a bubble that grows in turn and bobs on its own wave.

#include "ImageNode.h"
#include "Node.h"
#include "Particles.h"
#include "TextItem.h"

#include <string>
#include <vector>

namespace feedingfrenzy
{

// What a bubble letter is doing (+0x138)
enum BubbleLetterState
{
	LETTER_GROWING = 0,  // the bubble grows to its full size
	LETTER_WAITING = 1,  // it waits its turn to grow
	LETTER_SHOWN = 2,    // it shows at its full size
	LETTER_HIDDEN = 4    // it is not shown
};

// One letter of the banner, in a bubble (BubbleLetter, 0x144 bytes, constructor 0x43e4d9, vtable 0x4a77f0)
class BubbleLetter : public engine::Container
{
public:
	BubbleLetter();  // 0x43e4d9

	// 0x43e260: the letter's pictures (the bubble, the letter and the particles that burst when it pops), and how long it
	// waits before it grows
	void init(char theLetter, float theDelay);
	void start(const engine::Point& thePlace);  // 0x43e030: the letter shows and begins to grow
	void stop();                                // 0x43e1f0: the bubble bursts
	void update(engine::UpdateInfo& theInfo) override;  // 0x7c: 0x43e120

private:
	engine::Ref<engine::ParticleSystem> mBurstFx;  // +0x11c
	engine::Ref<engine::TextItem> mText;           // +0x120
	engine::Ref<engine::ImageNode> mBubble;        // +0x124
	float mCountdown;                              // +0x128: the seconds the state has left
	float mGrowSeconds;                            // +0x12c: how long the bubble takes to grow to its full size
	float mDelay;                                  // +0x130: the seconds it waits before it grows
	bool mIsSpace;                                 // +0x134: a space has no bubble
	int mState;                                    // +0x138
};

class BubbleString : public engine::Container
{
public:
	BubbleString();  // 0x43ea90

	// 0x43eb20: the letters; each waits the start delay and theLetterDelay for each letter before it before it grows;
	// they are theSpacing apart
	void setText(const std::string& theText, float theLetterDelay, float theSpacing, float theStartDelay);
	float getWidth();                           // 0x43e580
	void show(const engine::Point& thePlace);   // 0x43e5f0
	void hide();                                // 0x43e6c0

private:
	std::vector<BubbleLetter*> mLetters;  // +0x11c: the letters (the banner's children keep them)
	float mSpacing;                                    // +0x12c: how far apart the letters are
};

} // namespace feedingfrenzy

#endif
