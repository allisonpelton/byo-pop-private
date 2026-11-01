// The game's texts (see Language.h).

#include "Language.h"
#include "File.h"
#include "Platform.h"

#include <cstdlib>
#include <vector>

namespace bonnies
{

namespace
{
int gTextCount = 0;              // 0x4819a0
std::vector<Str> gTexts;         // 0x4819a4
std::vector<uint8_t> gHasText;   // 0x4819a8
int gLanguageLoaded = 0;         // 0x4819ac
}

// 0x428f20
void freeLanguage()
{
	gTexts.clear();
	gHasText.clear();
	gLanguageLoaded = 0;
}

// 0x428f70
const char* getText(int theId)
{
	if (theId >= 0 && theId < gTextCount)
		return gTexts[theId].c_str();
	// The original builds "String # <id> not found." here and returns an empty string instead
	return "";
}

// 0x429040
void loadLanguage()
{
	if (gLanguageLoaded != 0)
		return;
	gLanguageLoaded = 1;
	Str aText;
	int aFile = fileOpen(dataDir(true) + "Language.txt", false, false);
	if (aFile == -1)
		return;
	aText = fileReadText(aFile, 5000000);
	fileClose(aFile);
	int aLength = (int)aText.size();
	int aMaxId = 0;
	int aPass = 0;
	do
	{
		if (aPass == 1)
		{
			gTextCount = aMaxId + 1;
			gTexts.assign(gTextCount, Str());
			gHasText.assign(gTextCount, 0);
		}
		int i = 0;
		if (aLength > 0)
		{
			do
			{
				// The line runs to the first control character that is not a tab; the next starts after the run of
				// control characters (tabs among them)
				int aStart = i;
				while (i < aLength && ((unsigned char)aText[i] > 0xd || aText[i] == 9))
					i++;
				int anEnd = i - 1;
				while (i < aLength && (unsigned char)aText[i] <= 0xd)
					i++;
				int k = aStart;
				if (k < anEnd - 1)
				{
					do
					{
						if (aText[k] == '*' && k + 1 < aLength && aText[k + 1] == '*')
						{
							anEnd = k - 1;
							break;
						}
						k++;
					} while (k < anEnd - 1);
				}
				if (anEnd - aStart < 1)
					continue;
				int anId = (int)std::atol(substr(aText, aStart, anEnd + 1).c_str());
				if (aText[aStart] == '*')
					continue;
				if (anId < 1)
				{
					trim(substr(aText, aStart, anEnd + 1), false);
					continue;
				}
				if (aPass == 0)
				{
					if (aMaxId <= anId)
						aMaxId = anId;
					continue;
				}
				int aFirst = -1;
				for (int j = aStart; j <= anEnd; j++)
				{
					if (aText[j] == '"')
					{
						aFirst = j;
						break;
					}
				}
				int aLast = -1;
				for (int j = anEnd; j >= aStart; j--)
				{
					if (aText[j] == '"')
					{
						aLast = j;
						break;
					}
				}
				if (aFirst == -1 || aLast == -1 || aLast - aFirst < 1)
					continue;
				if (aLast - aFirst > 1999)
					aLast = aFirst + 2000;
				char aBuffer[2108];
				int n = 0;
				bool anEscape = false;
				int aStop = aLast - 1;
				for (int j = aFirst + 1; j <= aStop; j++)
				{
					char c = (char)at(aText, j);
					if (c == '\\' && j < aLength)
					{
						if (anEscape)
						{
							aBuffer[n++] = '\\';
							anEscape = false;
						}
						else
							anEscape = true;
					}
					else if (anEscape)
					{
						switch (toUpper((unsigned char)c))
						{
						case '"':
							aBuffer[n++] = '"';
							break;
						case 'N':
						case 'R':
							aBuffer[n++] = '\n';
							break;
						case 'T':
							aBuffer[n++] = '\t';
							break;
						default:
							aBuffer[n++] = c;
							break;
						}
						anEscape = false;
					}
					else
						aBuffer[n++] = c;
				}
				aBuffer[n] = 0;
				gTexts[anId] += aBuffer;
				gHasText[anId] = 1;
			} while (i < aLength);
		}
		aPass++;
	} while (aPass < 2);
}

} // namespace bonnies
