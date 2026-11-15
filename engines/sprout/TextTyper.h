#ifndef __SPROUT_TEXTTYPER_H__
#define __SPROUT_TEXTTYPER_H__

// The signal build's text that types itself out (0x17c bytes, constructor 0x482330 (pizzafrenzy), vtable
// 0x503920 (pizzafrenzy)): a text item whose lines show a letter at a time, a number of letters each second, and wait
// at the end of a sentence (a full stop) and of a line. It updates only while it types.

#include "TextItem.h"

namespace engine
{

class TextTyper : public TextItem
{
public:
	TextTyper();  // 0x482330 (pizzafrenzy)

	// 0x10c: 0x4822b0 (pizzafrenzy): the width the lines break at, the letters each second, and the seconds it waits
	// after a sentence
	void setWrapAndTiming(float theWrap, float theRate, float theSentencePause);
	// 0x110: 0x4822e0 (pizzafrenzy): how many lines show at a time (0 for all), and the seconds it waits at a line's
	// end
	void setLines(int theVisibleLines, float theLinePause);
	// 0x114: 0x482290 (pizzafrenzy): the text, with nothing typed yet
	void setTypedText(const std::string& theText);
	// 0x118: 0x482300 (pizzafrenzy): nothing typed, and no updates
	void reset();
	// 0x11c: 0x482480 (pizzafrenzy): types, when there is anything left to type
	void start();
	// 0x120: 0x482320 (pizzafrenzy)
	void stop();
	// 0x124: 0x4824f0 (pizzafrenzy): all of it typed
	void skipToEnd();
	// 0x128: 0x482430 (pizzafrenzy)
	bool isFinished();

	// 0x70: 0x482540 (pizzafrenzy)
	void update(UpdateInfo& theInfo) override;
	// 0x6c: 0x4827f0 (pizzafrenzy), in the place of the item's drawing of its lines
	void draw(Graphics* g) override;

private:
	// 0x12c: 0x482730 (pizzafrenzy): when a full stop is in the text from (theFromLine, theFromChar) up to (theToLine,
	// theToChar), the typing is at it
	bool findPause(int theFromLine, int theFromChar, int theToLine, int theToChar);
	void drawTypedLines(Graphics* g);

	int mLine;             // +0x158: the line being typed
	int mChar;             // +0x15c: the last letter typed of it
	float mRate;           // +0x160: the letters each second
	float mSentencePause;  // +0x164
	float mTimer;          // +0x168: the seconds until the next letters
	int mVisibleLines;     // +0x16c: 0 shows every line
	float mLinePause;      // +0x170
};

} // namespace engine

#endif
