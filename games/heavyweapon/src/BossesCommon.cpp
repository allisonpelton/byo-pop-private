// BossesCommon.cpp: what the ten bosses (Bosses.h) share: their data from data\bosses.xml.
// Original: each boss's constructor calls a loader of its own (0x438400 Ape, 0x439120 Battleship, 0x43b350 Eye,
// 0x43d480 Final, 0x43e7d0 Head, 0x43f910 Helicopter, 0x441450 Rainer, 0x4426c0 Robot, 0x444a60 Worm, 0x446c30 Wrecker),
// which opens the file, finds the boss's element and its block for the encounter (Level1 the first time the player
// meets the boss, Level2 the second) and reads that block's attributes into the boss's fields as it goes.

#include "Bosses.h"
#include "HeavyWeaponApp.h"
#include "XMLParser.h"

#include <stdlib.h>

using namespace Sexy;

// port: the loaders' common part. The original reads each attribute at the moment its element comes up in a walk
// that stops at the end of the block; here the block's elements are kept first and read after (an element or attribute
// the block does not have reads as 0, as the original's empty-string atol and atof do).
BossData::BossData(const char* theBoss, const char* theBlock)
{
	XMLParser* aParser = new XMLParser();
	aParser->OpenFile("data\\bosses.xml");
	XMLElement anElement;
	bool isInBoss = false;
	bool hasElement = aParser->NextElement(&anElement);
	while (hasElement)
	{
		if (anElement.mType == XMLElement::TYPE_START)
		{
			if (!isInBoss)
				isInBoss = anElement.mValue == theBoss;
			else if (anElement.mValue == theBlock)
			{
				mElements[""] = anElement.mAttributes;
				hasElement = aParser->NextElement(&anElement);
				while (hasElement)
				{
					if (anElement.mType == XMLElement::TYPE_START)
						mElements[anElement.mValue] = anElement.mAttributes;
					else if (anElement.mType == XMLElement::TYPE_END && anElement.mValue == theBlock)
						break;
					hasElement = aParser->NextElement(&anElement);
				}
				break;
			}
		}
		else if (anElement.mType == XMLElement::TYPE_END && anElement.mValue == theBoss)
			isInBoss = false;
		hasElement = aParser->NextElement(&anElement);
	}
	delete aParser;
}

// port: one attribute of an element of the boss's data, as the original reads each from its parser's map
const std::string& BossData::Attribute(const char* theElement, const char* theName)
{
	return mElements[theElement][theName];
}

// port: an attribute as a number (the original's atol)
int BossData::Int(const char* theElement, const char* theName)
{
	return (int)atol(Attribute(theElement, theName).c_str());
}

// port: an attribute as a double (the original's atof)
double BossData::Double(const char* theElement, const char* theName)
{
	return atof(Attribute(theElement, theName).c_str());
}

// port: the encounter's block name, "Level" and the number of the boss's encounter: the first nine levels meet
// each boss for the first time, the next nine for the second
void Sexy::GetBossBlockName(HeavyWeaponApp* theApp, char* theName)
{
	sprintf(theName, "Level%i", theApp->mLevel / BOSS_ENCOUNTER_LEVELS + 1);
}
