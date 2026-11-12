// The level select screen's quotes (0x418c30-0x4191e0).

#include "Quotes.h"

#include "Profiles.h"
#include "SceneLoader.h"
#include "SceneObject.h"
#include "SceneTag.h"
#include "TextObject.h"
#include "VeniceGlobals.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace
{
// a quote is its lines (the data file's line1, line2 and so on)
typedef std::vector<std::string> Quote;

std::vector<Quote> gJourneyQuotes; // 0x5ba3dc
std::vector<Quote> gFloodQuotes; // 0x5ba3e8
bool gQuotesLoaded = false; // 0x5ba3d8

const int QuoteLineWidth = 50;
const int QuoteLineHeight = 26;
const float QuoteFirstLineY = -204.0f;

// 0x418c30: the lines of one quote tag, from line1 on until one is missing
Quote ReadQuote(const SceneTag& theTag)
{
	Quote aQuote;
	for (int aLine = 1;; aLine++) {
		char aName[32];
		sprintf(aName, "line%d", aLine);
		if (!theTag.Has(aName))
			break;
		aQuote.push_back(theTag.GetString(aName));
	}
	return aQuote;
}

// 0x418ff0: a quotes file is a run of quote tags up to the closing quotes tag
void ReadQuotesFile(const char* theFileName, std::vector<Quote>* theQuotes)
{
	std::string aFile;
	if (!ReadGameFile(theFileName, &aFile))
		return;
	const char* aText = aFile.c_str();
	SceneTag aTag;
	aText = ReadTag(aText, &aTag);
	while (aTag.mName != "/quotes" && *aText != '\0') {
		if (aTag.mName == "quote")
			theQuotes->push_back(ReadQuote(aTag));
		aText = ReadTag(aText, &aTag);
	}
}

// 0x418cc0: each line of the quote comes in a line of text objects (the line cut to the width), the lines of the quote all
// starting from the same height, and each text fades in after a short wait
void ShowQuote(const Quote& theQuote)
{
	for (const std::string& aQuoteLine : theQuote) {
		float aY = QuoteFirstLineY;
		std::string aText = aQuoteLine;
		int aTextLength = (int)aText.size();
		int aLineStart = 0;
		while (aLineStart < aTextLength) {
			std::string aLine(aText.size() + 1, '\0');
			aLineStart = CutTextLine(&aText[0], aLineStart, QuoteLineWidth, &aLine[0]);
			TextObject* aLineText = new TextObject();
			gScenes.mTail->NewChild(aLineText);
			aLineText->SetText(aLine.c_str(), "gfx\\font2.png", -2.0f, 0, JustifyCenter);
			aLineText->AddStage(600.0f, 0, 0, 0, nullptr);
			aLineText->AddStage(400.0f, 0, 0, 1, nullptr);
			aLineText->SetAttribute(AttributeX, 0.0f, 0.0f);
			aLineText->SetAttribute(AttributeY, aY, aY);
			aLineText->SetAttribute(AttributeAlpha, 0.0f, 1.0f);
			aLineText->SetAttribute(AttributeRed, 0.25f, 0.25f);
			aLineText->SetAttribute(AttributeGreen, 0.25f, 0.25f);
			aLineText->SetAttribute(AttributeBlue, 0.25f, 0.25f);
			aLineText->AddStage(1000.0f, 1, 1, 1, nullptr);
			aY = (float)QuoteLineHeight + aY;
		}
	}
}
}

// The quotes come one for each three levels (and one more at the last level, the 72nd); a mode's list that is shorter than the
// level asks for shows none
void ShowLevelQuote()
{
	if (!gQuotesLoaded) {
		ReadQuotesFile("levels\\journey_quotes.xml", &gJourneyQuotes);
		ReadQuotesFile("levels\\flood_quotes.xml", &gFloodQuotes);
		gQuotesLoaded = true;
	}
	Profile& aProfile = CurrentProfile();
	int aQuoteNumber = aProfile.mLevelIndex[aProfile.mMode] / 3;
	if (aProfile.mLevelIndex[aProfile.mMode] == 71)
		aQuoteNumber++;
	const std::vector<Quote>& aQuotes = aProfile.mMode == GameModeJourney ? gJourneyQuotes : gFloodQuotes;
	if (aQuoteNumber >= 0 && aQuoteNumber < (int)aQuotes.size()) {
		ShowQuote(aQuotes[aQuoteNumber]);
		aProfile.mTipCycle++;
		if (aProfile.mTipCycle > 2)
			aProfile.mTipCycle = 0;
	}
}
