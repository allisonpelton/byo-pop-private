#include "FunFacts.h"
#include "App.h"
#include "DocHandler.h"
#include "Font.h"
#include "Layout.h"
#include "Random.h"
#include "Stream.h"

using namespace engine;
using namespace feedingfrenzy;

namespace
{
// The handler's element and its attribute (0x42a9f0), and the text of a fact that has none
const char* const FACT_ELEMENT = "fact";
const char* const FACT_ATTRIBUTE = "string";
const char* const MISSING_FACT = "NO FACT";
// What getRandomFact gives when there are no facts
const char* const NO_FACTS_TEXT = "NO FACTS LOADED";
// The break of a line
const char* const LINE_BREAK = "\n";

// The handler of the file's elements (FactDocHandler, 0x42a930, vtable 0x4a5ec4): each fact is added to the list
class FactDocHandler : public BaseDocHandler
{
public:
	FactDocHandler(const std::string& theName, DocHandlerStack* theParent, FunFacts* theFacts) :
	    BaseDocHandler(theName, theParent), mFacts(theFacts)
	{
	}

	// 0x42a9f0
	void startElement(const std::string& theName, const XmlAttributes& theAttributes) override
	{
		if (theName == FACT_ELEMENT)
			mFacts->add(getStringAttribute(theAttributes, FACT_ATTRIBUTE, MISSING_FACT));
	}

private:
	FunFacts* mFacts;  // +0x14: not counted
};
}

// 0x42b7f0
FunFacts::FunFacts()
{
}

// 0x42ab70
void FunFacts::load(const std::string& theFile)
{
	Ref<DocHandlerStack> aStack = new DocHandlerStack();
	Ref<FactDocHandler> aHandler = new FactDocHandler("FunFacts", aStack, this);
	aStack->addHandler(aHandler);
	Ref<InputStream> aStream = App::instance()->openStream(theFile.c_str());
	parseXml(*aStack, aStream);
}

// 0x42b890
void FunFacts::add(const std::string& theFact)
{
	mFacts.push_back(theFact);
}

// 0x42ad90: the words are taken one at a time, and each goes on the line when the line and the word fit in the width,
// or else the line ends and the word begins the next
std::string FunFacts::getFact(int theIndex, Font* theFont, float theWidth)
{
	std::string aFact = mFacts[theIndex];
	std::string aLine;
	std::string aWord;
	std::string aText;
	for (size_t aPlace = 0; aPlace < aFact.size(); aPlace++)
	{
		if (aFact[aPlace] == ' ')
		{
			float aLineWidth = (float)theFont->getStringWidth(aLine.c_str());
			float aWordWidth = (float)theFont->getStringWidth(aWord.c_str());
			if (aWordWidth + aLineWidth <= theWidth)
			{
				aLine += aWord;
				aLine.append(1, ' ');
				aWord = "";
			}
			else
			{
				aText += aLine;
				aText += LINE_BREAK;
				aLine = aWord;
				aLine += " ";
				aWord = "";
			}
		}
		else
		{
			aWord.append(1, aFact[aPlace]);
		}
	}
	float aLineWidth = (float)theFont->getStringWidth(aLine.c_str());
	float aWordWidth = (float)theFont->getStringWidth(aWord.c_str());
	aText += aLine;
	if (theWidth < aWordWidth + aLineWidth)
		aText += LINE_BREAK;
	aText += aWord;
	return aText;
}

// 0x42b280
std::string FunFacts::getRandomFact(Font* theFont, float theWidth)
{
	if (mFacts.empty())
		return NO_FACTS_TEXT;
	return getFact(randomInt(0, (int)mFacts.size()), theFont, theWidth);
}
