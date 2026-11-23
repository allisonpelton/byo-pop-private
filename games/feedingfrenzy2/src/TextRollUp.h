#ifndef __FEEDINGFRENZY2_TEXTROLLUP_H__
#define __FEEDINGFRENZY2_TEXTROLLUP_H__

// A number counting up in a text (TextRollUp, 0x70 bytes, its constructor 0x49c4b0 (feedingfrenzy2)), used by the bonus
// stage's end: from where it starts to its target, a step at the end of each wait, each step the largest power of ten
// up to its largest step that does not pass the target, and the text written in one of the number's forms each time.

#include "TextItem.h"

#include <string>

class TextRollUp
{
public:
	// The forms the number is written in (0x48c7b0 (feedingfrenzy2))
	enum Form
	{
		FORM_LOCALE = 0,   // the locale's number
		FORM_GROUPED = 1,  // grouped by commas
		FORM_PADDED = 2,   // grouped by commas, six digits at least
		FORM_CLOCK = 3,    // the stage's clock
		FORM_AFFIXED = 4   // between the prefix and the suffix
	};

	TextRollUp();

	// 0x49f480 (feedingfrenzy2): counting from the start to the target in the text, steps of up to the largest step
	// one wait apart, and the text written at once
	void start(int theStart, int theTarget, engine::TextItem* theText, Form theForm, int theLargestStep,
	           float theWait);
	// 0x49b670 (feedingfrenzy2): the text before and after the number in FORM_AFFIXED
	void setAffixes(const std::string& thePrefix, const std::string& theSuffix);
	// 0x49c570 (feedingfrenzy2): the wait run down, and at its end a step taken and the text written; whether the
	// target is reached
	bool update(engine::UpdateContext* theContext);

private:
	// 0x49c500 (feedingfrenzy2): the step taken, with the shell theme's sound countUpClick (not yet translated, as the
	// sounds are not)
	void step();
	// 0x48c7b0 (feedingfrenzy2): the number written in the text in its form
	void draw();

	int mStart;                  // +0x0c
	int mTarget;                 // +0x10
	int mValue;                  // +0x14
	int mLargestStep;            // +0x18
	std::string mPrefix;         // +0x1c
	std::string mSuffix;         // +0x38
	engine::TextItem* mText;     // +0x54
	float mStepWait;             // +0x58
	                             // (+0x5c, set by start, is read by nothing)
	Form mForm;                  // +0x60
	float mWait;                 // +0x64
};

#endif
