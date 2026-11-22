#include "BubbleText.h"

#include "DrawContext.h"
#include "Engine.h"
#include "Font.h"
#include "Modifiers.h"
#include "Random.h"

#include <cctype>

using namespace engine;

namespace
{

// The seconds a glyph takes to grow to full size (the float at 0x54ddf8 (feedingfrenzy2))
const float GLYPH_GROW_SECONDS = 0.2f;
// Each glyph's wave: up and down by 5 pixels, once in 2.5 to 3.5 seconds, started up to 2 seconds in
const float GLYPH_WAVE_AMPLITUDE = 5.0f;
const float GLYPH_WAVE_SHORTEST_PERIOD = 2.5f;
const float GLYPH_WAVE_LONGEST_PERIOD = 3.5f;
const float GLYPH_WAVE_LATEST_START = 2.0f;

} // namespace

BubbleGlyph::BubbleGlyph()
	: mTimer(0.0f), mGrowTime(0.0f), mDelay(0.0f), mPhase(BUBBLE_GLYPH_BURST)
{
}

void BubbleGlyph::init(Surface* theBubble, ParticleSystemDef* theEffect, const Color& theColor)
{
	if (theBubble != nullptr)
	{
		mBubble = new Image(theBubble);
		addChild(mBubble);
	}
	mLetter = new TextItem();
	mLetter->setColor(theColor);
	mLetter->setColorMode(COLOR_MODE_MULTIPLY);
	mLetter->setAlignment(TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE);
	addChild(mLetter);
	mEffect = new ParticleSystem();
	mEffect->init(theEffect);
	addChild(mEffect);
	setVisible(false);
}

void BubbleGlyph::setLetter(char theLetter)
{
	mLetter->setText(std::string(1, theLetter));
}

void BubbleGlyph::appear(float theDelay, float theGrowTime)
{
	addOwnState(STATE_ANIMATED);
	mDelay = theDelay;
	mGrowTime = theGrowTime;
	setScale(0.0f, 0.0f);
	setVisible(true);
	if (mBubble != nullptr)
		mBubble->setVisible(true);
	mLetter->setVisible(true);
	if (0.0f < mDelay)
	{
		mPhase = BUBBLE_GLYPH_WAITING;
		mTimer = mDelay;
	}
	else
	{
		mPhase = BUBBLE_GLYPH_GROWING;
		mTimer = mGrowTime;
	}
}

void BubbleGlyph::update(UpdateContext& theContext)
{
	if (mPhase == BUBBLE_GLYPH_GROWING)
	{
		mTimer = (float)((double)mTimer - theContext.mElapsed);
		if (mTimer < 0.0f)
			mTimer = 0.0f;
		float aScale = (float)(1.0 - (double)mTimer / mGrowTime);
		setScale(aScale, aScale);
		if (mTimer == 0.0f)
			mPhase = BUBBLE_GLYPH_SHOWN;
	}
	else if (mPhase == BUBBLE_GLYPH_WAITING)
	{
		mTimer = (float)((double)mTimer - theContext.mElapsed);
		if (mTimer < 0.0f)
		{
			mPhase = BUBBLE_GLYPH_GROWING;
			mTimer = mGrowTime;
		}
	}
	Container::update(theContext);
}

void BubbleGlyph::burst()
{
	if (mPhase != BUBBLE_GLYPH_SHOWN && mPhase != BUBBLE_GLYPH_GROWING)
		return;
	mEffect->stop();
	mEffect->start();
	if (mBubble != nullptr)
		mBubble->setVisible(false);
	mLetter->setVisible(false);
	mPhase = BUBBLE_GLYPH_BURST;
}

BubbleText::BubbleText()
	// The colour made from (255, 0, 0) out of 255 (0x40c270 (feedingfrenzy2))
	: mSpacing(0.0f), mLetterColor(1.0f, 0.0f, 0.0f, 1.0f)
{
}

BubbleText::~BubbleText()
{
	clear();
}

void BubbleText::clear()
{
	removeAllChildren();
	mFreeGlyphs.insert(mFreeGlyphs.end(), mGlyphs.begin(), mGlyphs.end());
	mGlyphs.clear();
}

void BubbleText::addFreeGlyphs(int theCount)
{
	for (int i = theCount; i != 0; i--)
	{
		BubbleGlyph* aGlyph = new BubbleGlyph();
		mFreeGlyphs.push_back(aGlyph);
		aGlyph->init(mBubble, mEffect, mLetterColor);
		WaveModifier* aWave = new WaveModifier(0.0f,
			(float)randomFloat(GLYPH_WAVE_SHORTEST_PERIOD, GLYPH_WAVE_LONGEST_PERIOD), 0.0f, GLYPH_WAVE_AMPLITUDE, 0.0f);
		// The wave run once for a random time, so the glyphs do not rock together
		UpdateContext aContext;
		aContext.mElapsed = (float)randomFloat(0.0f, GLYPH_WAVE_LATEST_START);
		aWave->modify(aContext, aGlyph);
		aGlyph->addModifier(aWave);
	}
}

void BubbleText::setText(const std::string& theText)
{
	clear();
	Font* aFont = getFont();
	float aGlyphWidth;
	float aGlyphHeight;
	if (mBubble != nullptr)
	{
		aGlyphWidth = (float)mBubble->getWidth();
		aGlyphHeight = (float)mBubble->getHeight();
	}
	else
	{
		// The text's widest character (spaces too), and the font's height
		aGlyphWidth = 0.0f;
		aGlyphHeight = (float)aFont->getHeight();
		for (size_t i = 0; i < theText.size(); i++)
		{
			float aCharWidth = (float)aFont->getCharWidth(theText[i]);
			if (aGlyphWidth < aCharWidth)
				aGlyphWidth = aCharWidth;
		}
	}
	size_t aSize = theText.size();
	float y = 0.0f;
	double aHalfWidth = (double)aGlyphWidth * 0.5;
	double aLineStep = (double)aGlyphHeight * 1.5;
	size_t aStart = 0;
	do
	{
		size_t anEnd = theText.find("\\n", aStart);
		if (anEnd == std::string::npos)
			anEnd = aSize;
		// The line's glyphs centred on the position, the first glyph's middle the half width from its left
		float aLineWidth = (float)(((double)aGlyphWidth + mSpacing) * (double)(unsigned int)(anEnd - aStart) - mSpacing);
		float x = (float)(aHalfWidth - (double)aLineWidth * 0.5);
		for (size_t i = aStart; i != anEnd; i++)
		{
			char aLetter = theText[i];
			if (!isspace((unsigned char)aLetter))
			{
				if (mFreeGlyphs.empty())
					addFreeGlyphs(1);
				BubbleGlyph* aGlyph = mFreeGlyphs.back();
				mGlyphs.push_back(aGlyph);
				addChild(aGlyph);
				mFreeGlyphs.pop_back();
				aGlyph->setPosition(x, y);
				aGlyph->setLetter(aLetter);
			}
			x = (float)((double)aGlyphWidth + mSpacing + x);
		}
		y = (float)(y + aLineStep);
		aStart = anEnd + 2;
	} while (aStart < aSize);
	setFilterMode(FILTER_MODE_SMOOTH);
}

void BubbleText::appear(float theStart, float theDuration)
{
	// The 0.2 subtracted is the float's value as a double (0x5669c8 (feedingfrenzy2))
	float aStep = (float)(((double)theDuration - (double)GLYPH_GROW_SECONDS) / (double)(unsigned int)mGlyphs.size());
	for (size_t i = 0; i < mGlyphs.size(); i++)
		mGlyphs[i]->appear((float)((double)(unsigned int)i * aStep + theStart), GLYPH_GROW_SECONDS);
}

void BubbleText::burst()
{
	for (size_t i = 0; i < mGlyphs.size(); i++)
		mGlyphs[i]->burst();
}

bool BubbleText::isDone()
{
	for (size_t i = 0; i < mGlyphs.size(); i++)
	{
		if (!mGlyphs[i]->isEffectDone())
			return false;
	}
	return true;
}
