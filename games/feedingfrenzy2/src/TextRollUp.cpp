#include "TextRollUp.h"

#include "NumberText.h"

#include <cstdio>

using namespace engine;

namespace
{

// The step tried first when the largest step is more (0x49c500 (feedingfrenzy2)), and each next step's factor
const int FIRST_TRIED_STEP = 10;
const int STEP_FACTOR = 10;

} // namespace

TextRollUp::TextRollUp()
    : mStart(0), mTarget(0), mValue(0), mLargestStep(0), mText(nullptr), mStepWait(0.0f), mForm(FORM_LOCALE),
      mWait(0.0f)
{
}

void TextRollUp::start(int theStart, int theTarget, TextItem* theText, Form theForm, int theLargestStep,
                       float theWait)
{
	mTarget = theTarget;
	mStepWait = theWait;
	mText = theText;
	mWait = 0.0f;
	mForm = theForm;
	mStart = theStart;
	mLargestStep = theLargestStep;
	mValue = theStart;
	draw();
}

void TextRollUp::setAffixes(const std::string& thePrefix, const std::string& theSuffix)
{
	mPrefix = thePrefix;
	mSuffix = theSuffix;
}

bool TextRollUp::update(UpdateContext* theContext)
{
	if (!(mValue < mTarget))
		return true;
	mWait = (float)((double)mWait - theContext->mElapsed);
	if (mWait < 0.0f)
	{
		step();
		draw();
		mWait = mStepWait;
	}
	return mTarget <= mValue;
}

void TextRollUp::step()
{
	int aCandidate = mLargestStep;
	if (FIRST_TRIED_STEP - 1 < mLargestStep)
		aCandidate = FIRST_TRIED_STEP;
	int aStep = 1;
	if (1 < mLargestStep)
	{
		int aTried;
		do
		{
			aTried = aCandidate;
			if (mTarget < mValue + aTried)
				break;
			aCandidate = aTried * STEP_FACTOR;
			if (mLargestStep < aTried * STEP_FACTOR)
				aCandidate = mLargestStep;
			aStep = aTried;
		} while (aTried < mLargestStep);
	}
	mValue = mValue + aStep;
	// The shell theme's sound countUpClick at full volume: not yet translated, as the sounds are not
}

void TextRollUp::draw()
{
	switch (mForm)
	{
	case FORM_LOCALE:
		mText->setText(localeNumber(mValue));
		break;
	case FORM_GROUPED:
		mText->setText(groupedNumber(mValue));
		break;
	case FORM_PADDED:
		mText->setText(groupedNumber(mValue, true));
		break;
	case FORM_CLOCK:
		mText->setText(stageClock(mValue));
		break;
	case FORM_AFFIXED:
	{
		char aNumber[16];
		snprintf(aNumber, sizeof(aNumber), "%d", mValue);
		mText->setText(mPrefix + aNumber + mSuffix);
		break;
	}
	}
}
