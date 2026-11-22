#include "DescParser.h"
#include "PakInterface.h"

using namespace Sexy;

DescParser::DescParser()
{
	mCmdSep = CMDSEP_SEMICOLON;
}

DescParser::~DescParser()
{
}

bool DescParser::Error(const std::string& theError)
{
	mError = theError;
	return false;
}

DataElement* DescParser::Dereference(const std::string& theString)
{
	std::string aDefineName = StringToUpper(theString);

	DataElementMap::iterator anItr = mDefineMap.find(aDefineName);
	if (anItr != mDefineMap.end())
		return anItr->second;
	else
		return NULL;
}

bool DescParser::IsImmediate(const std::string& theString)
{
	return (((theString[0] >= '0') && (theString[0] <= '9')) || (theString[0] == '-') || 
		(theString[0] == '+') || (theString[0] == '\'') || (theString[0] == '"'));
}

std::string DescParser::Unquote(const std::string& theQuotedString)
{
	if ((theQuotedString[0] == '\'') || (theQuotedString[0] == '"'))
	{
		char aQuoteChar = theQuotedString[0];
		std::string aLiteralString;

		// port: as in Bookworm Adventures Volume 2's exe (it sets the format the later descriptors use): the quotes
		// at both ends are not part of the text, a doubled quote inside is one quote, and a backslash makes the next
		// character literal (ParseToList keeps the backslash for this). Before, the closing quote counted as the
		// second of a pair after an escaped quote, which only worked by luck, and a quote written '\'' came out as
		// two characters.
		bool lastWasQuote = true;
		bool escaped = false;

		for (ulong i = 1; i + 1 < theQuotedString.length(); i++)
		{
			char aChar = theQuotedString[i];

			if (escaped)
			{
				aLiteralString += aChar;
				escaped = false;
			}
			else if (aChar == aQuoteChar)
			{
				if (lastWasQuote)
					aLiteralString += aQuoteChar;

				lastWasQuote = true;
			}
			else
			{
				if (aChar == '\\')
					escaped = true;
				else
					aLiteralString += aChar;

				lastWasQuote = false;
			}
		}

		return aLiteralString;
	}
	else
		return theQuotedString;
}

bool DescParser::GetValues(ListDataElement* theSource, ListDataElement* theValues)
{
	theValues->mElementVector.clear();
	
	for (ulong aSourceNum = 0; aSourceNum < theSource->mElementVector.size(); aSourceNum++)
	{
		if (theSource->mElementVector[aSourceNum]->mIsList)
		{
			ListDataElement* aChildList = new ListDataElement();
			theValues->mElementVector.push_back(aChildList);

			if (!GetValues((ListDataElement*) theSource->mElementVector[aSourceNum], aChildList))
				return false;
		}
		else
		{
			std::string aString = ((SingleDataElement*) theSource->mElementVector[aSourceNum])->mString;

			if (aString.length() > 0)
			{				
				if ((aString[0] == '\'') || (aString[0] == '"'))
				{
					SingleDataElement* aChildData = new SingleDataElement(Unquote(aString));
					theValues->mElementVector.push_back(aChildData);					
				}
				else if (IsImmediate(aString))
				{
					theValues->mElementVector.push_back(new SingleDataElement(aString));
				}
				else
				{
					std::string aDefineName = StringToUpper(aString);

					DataElementMap::iterator anItr = mDefineMap.find(aDefineName);

					if (anItr == mDefineMap.end())
					{
						Error("Unable to Dereference \"" + aString + "\"");
						return false;
					}
					
					theValues->mElementVector.push_back(anItr->second->Duplicate());
				}
			}

			
		}
	}

	return true;
}

std::string DescParser::DataElementToString(DataElement* theDataElement)
{
	if (theDataElement->mIsList)
	{
		ListDataElement* aListDataElement = (ListDataElement*) theDataElement;

		std::string aString = "(";

		for (ulong i = 0; i < aListDataElement->mElementVector.size(); i++)
		{
			if (i != 0)
				aString += ", ";

			aString += DataElementToString(aListDataElement->mElementVector[i]);
		}

		aString += ")";

		return aString;
	}
	else
	{
		SingleDataElement* aSingleDataElement = (SingleDataElement*) theDataElement;
		return aSingleDataElement->mString;
	}
}

bool DescParser::DataToString(DataElement* theSource, std::string* theString)
{
	*theString = "";

	if (theSource->mIsList)
		return false;

	std::string aDefName = ((SingleDataElement*) theSource)->mString;

	DataElement* aDataElement = Dereference(aDefName);
	
	if (aDataElement != NULL)
	{
		if (aDataElement->mIsList)
			return false;

		*theString = Unquote(((SingleDataElement*) aDataElement)->mString);
	}
	else
		*theString = Unquote(aDefName);				

	return true;
}

bool DescParser::DataToInt(DataElement* theSource, int* theInt)
{
	*theInt = 0;

	std::string aTempString;
	if (!DataToString(theSource, &aTempString))
		return false;
	
	if (!StringToInt(aTempString, theInt))
		return false;

	return true;
}

bool DescParser::DataToStringVector(DataElement* theSource, StringVector* theStringVector)
{
	theStringVector->clear();

	ListDataElement aStaticValues;
	ListDataElement* aValues;

	if (theSource->mIsList)
	{
		if (!GetValues((ListDataElement*) theSource, &aStaticValues))
			return false;

		aValues = &aStaticValues;
	}
	else
	{
		std::string aDefName = ((SingleDataElement*) theSource)->mString;

		DataElement* aDataElement = Dereference(aDefName);
		
		if (aDataElement == NULL)
		{
			Error("Unable to Dereference \"" + aDefName + "\"");
			return false;
		}

		if (!aDataElement->mIsList)
			return false;

		aValues = (ListDataElement*) aDataElement;
	}	

	for (ulong i = 0; i < aValues->mElementVector.size(); i++)
	{
		if (aValues->mElementVector[i]->mIsList)
		{
			theStringVector->clear();
			return false;
		}		

		SingleDataElement* aSingleDataElement = (SingleDataElement*) aValues->mElementVector[i];

		theStringVector->push_back(aSingleDataElement->mString);
	}

	return true;
}

bool DescParser::DataToList(DataElement* theSource, ListDataElement* theValues)
{
	if (theSource->mIsList)
	{
		return GetValues((ListDataElement*) theSource, theValues);		
	}

	DataElement* aDataElement = Dereference(((SingleDataElement*) theSource)->mString);
		
	if ((aDataElement == NULL) || (!aDataElement->mIsList))
		return false;

	ListDataElement* aListElement = (ListDataElement*) aDataElement;

	*theValues = *aListElement;

	return true;
}

bool DescParser::DataToIntVector(DataElement* theSource, IntVector* theIntVector)
{
	theIntVector->clear();
	
	StringVector aStringVector;
	if (!DataToStringVector(theSource, &aStringVector))
		return false;	

	for (ulong i = 0; i < aStringVector.size(); i++)
	{		
		int aIntVal;
		if (!StringToInt(aStringVector[i], &aIntVal))
			return false;

		theIntVector->push_back(aIntVal);
	}

	return true;
}

bool DescParser::DataToDoubleVector(DataElement* theSource, DoubleVector* theDoubleVector)
{
	theDoubleVector->clear();
	
	StringVector aStringVector;
	if (!DataToStringVector(theSource, &aStringVector))
		return false;	

	for (ulong i = 0; i < aStringVector.size(); i++)
	{		
		double aDoubleVal;
		if (!StringToDouble(aStringVector[i], &aDoubleVal))
			return false;

		theDoubleVector->push_back(aDoubleVal);
	}

	return true;
}

bool DescParser::ParseToList(const std::string& theString, ListDataElement* theList, bool expectListEnd, int* theStringPos)
{
	bool inSingleQuotes = false;
	bool inDoubleQuotes = false;
	bool escaped = false;	

	SingleDataElement* aCurSingleDataElement = NULL;	

	int aStringPos = 0;
	
	if (theStringPos == NULL)
		theStringPos = &aStringPos;

	while (*theStringPos < (int) theString.length())
	{
		bool addSingleChar = false;
		bool addBackslash = false;
		char aChar = theString[(*theStringPos)++];

		bool isSeperator = (aChar == ' ') || (aChar == '\t') || (aChar == '\n') || (aChar == ',');
		
		if (escaped)
		{
			// port: the backslash stays in the text for Unquote, as in Bookworm Adventures Volume 2's exe
			addSingleChar = true;
			addBackslash = true;
			escaped = false;
		}
		else
		{
			if ((aChar == '\'') && (!inDoubleQuotes))
				inSingleQuotes = !inSingleQuotes;
			else if ((aChar == '"') && (!inSingleQuotes))
				inDoubleQuotes = !inDoubleQuotes;

			if (aChar == '\\')
			{
				escaped = true;
			}
			else if ((!inSingleQuotes) && (!inDoubleQuotes))
			{
				if (aChar == ')')
				{
					if (expectListEnd)
						return true;
					else
					{
						Error("Unexpected List End");
						return false;
					}
				}
				else if (aChar == '(') 
				{
					if (aCurSingleDataElement != NULL)
					{
						Error("Unexpected List Start");
						return false;
					}
					else
					{
						ListDataElement* aChildList = new ListDataElement();

						if (!ParseToList(theString, aChildList, true, theStringPos))
							return false;

						theList->mElementVector.push_back(aChildList);
					}
				}				
				else if (isSeperator)
				{
					if (aCurSingleDataElement != NULL)
						aCurSingleDataElement = NULL;					
				}
				else
					addSingleChar = true;
			}
			else
				addSingleChar = true;						
		}

		if (addSingleChar)
		{
			if (aCurSingleDataElement == NULL)
			{
				aCurSingleDataElement = new SingleDataElement();
				theList->mElementVector.push_back(aCurSingleDataElement);
			}

			if (addBackslash)
				aCurSingleDataElement->mString += '\\';

			aCurSingleDataElement->mString += aChar;
		}
	}

	if (inSingleQuotes)
	{
		Error("Unterminated Single Quotes");
		return false;
	}

	if (inDoubleQuotes)
	{
		Error("Unterminated Double Quotes");
		return false;
	}

	if (expectListEnd)
	{
		Error("Unterminated List");
		return false;
	}

	return true;
}

bool DescParser::ParseDescriptorLine(const std::string& theDescriptorLine)
{
	ListDataElement aParams;
	if (!ParseToList(theDescriptorLine, &aParams, false, NULL))
		return false;
	
	if (aParams.mElementVector.size() > 0)
	{
		if (aParams.mElementVector[0]->mIsList)
		{
			Error("Missing Command");
			return false;
		}

		if (!HandleCommand(aParams))
			return false;
	}

	return true;
}

// port: the next character of a descriptor. A file that starts with a UTF-8 byte order mark is UTF-8, as the later
// framework's EncodingParser reads it (Bejeweled 3's flaregothic32.txt, the only such font among its 69): each
// character comes back as the Latin-1 byte the narrow text uses, the euro sign as 0x80 (its place in Windows-1252,
// which the fonts without a mark are written in), and any other character above U+00FF, or a malformed sequence, is
// skipped. Other files are read byte by byte, as before.
static int ReadDescriptorChar(PFILE* theStream, bool isUTF8)
{
	for (;;)
	{
		int aChar = p_fgetc(theStream);
		if (aChar == EOF || !isUTF8 || aChar < 0x80)
			return aChar;
		int aTrailCount = aChar >= 0xF0 ? 3 : aChar >= 0xE0 ? 2 : aChar >= 0xC0 ? 1 : 0;
		int aCodePoint = aChar & (0x3F >> aTrailCount);
		for (int i = 0; i < aTrailCount; i++)
		{
			int aTrail = p_fgetc(theStream);
			if (aTrail == EOF)
				return EOF;
			aCodePoint = (aCodePoint << 6) | (aTrail & 0x3F);
		}
		if (aTrailCount > 0 && aCodePoint <= 0xFF)
			return aCodePoint;
		if (aCodePoint == 0x20AC)
			return 0x80;
	}
}

bool DescParser::LoadDescriptor(const std::string& theFileName)
{
	mCurrentLineNum = 0;
	int aLineCount = 0;
	bool hasErrors = false;

	//Apparently VC6 doesn't have a clear() function for basic_strings
	//mError.clear();
	mError.erase();
	// port: dropped a following mError.erase(mError.begin()) (undefined on the now empty string)

	PFILE *aStream = p_fopen(theFileName.c_str(),"r");
	if (aStream==NULL)
		return false;

	// port: a UTF-8 byte order mark (ReadDescriptorChar)
	bool isUTF8 = p_fgetc(aStream) == 0xEF && p_fgetc(aStream) == 0xBB && p_fgetc(aStream) == 0xBF;
	if (!isUTF8)
		p_fseek(aStream, 0, SEEK_SET);

	char aBuffChar = 0;

	while (!p_feof(aStream))
	{		
		int aChar;
						
		bool skipLine = false;
		bool atLineStart = true;
		bool inSingleQuotes = false;
		bool inDoubleQuotes = false;
		bool escaped = false; 
		bool isIndented = false;

		for (;;)
		{
			if (aBuffChar != 0)
			{
				aChar = aBuffChar;
				aBuffChar = 0;
			}
			else
			{
				aChar = ReadDescriptorChar(aStream, isUTF8);
				if (aChar==EOF)
					break;
			}
			
			if (aChar != '\r')
			{
				if (aChar == '\n')
					aLineCount++;

				if (((aChar == ' ') || (aChar == '\t')) && (atLineStart))
					isIndented = true;

				if ((!atLineStart) || ((aChar != ' ') && (aChar != '\t') && (aChar != '\n')))
				{
					if (atLineStart)
					{
						if ((mCmdSep & CMDSEP_NO_INDENT) && (!isIndented) && (mCurrentLine.size() > 0))
						{
							// Start a new non-indented line
							aBuffChar = aChar;
							break;
						}

						if (aChar == '#')
							skipLine = true;

						atLineStart = false;
					}					

					if (aChar == '\n')		
					{
						isIndented = false;
						atLineStart = true;				
					}

					if ((aChar == '\n') && (skipLine))
					{
						skipLine = false;						
					}
					else if (!skipLine)
					{
						if (aChar == '\\' && (inSingleQuotes || inDoubleQuotes) && !escaped)
							escaped = true;
						else
						{
							if ((aChar == '\'') && (!inDoubleQuotes) && (!escaped))
								inSingleQuotes = !inSingleQuotes;

							if ((aChar == '"') && (!inSingleQuotes) && (!escaped))
								inDoubleQuotes = !inDoubleQuotes;
							
							if ((aChar == ';') && (mCmdSep & CMDSEP_SEMICOLON) && (!inSingleQuotes) && (!inDoubleQuotes))
								break;
							
							if(escaped) // stay escaped for when this is actually parsed
							{
								mCurrentLine += '\\';
								escaped = false;
							}

							if (mCurrentLine.size() == 0)
								mCurrentLineNum = aLineCount + 1;

							mCurrentLine += aChar;
						}
					}
				}
			}
		}

		if (mCurrentLine.length() > 0)
		{
			if (!ParseDescriptorLine(mCurrentLine))
			{
				hasErrors = true;
				break;
			}

			//Apparently VC6 doesn't have a clear() function for basic_strings
			//mCurrentLine.clear();
			mCurrentLine.erase();
		}
	}

	//Apparently VC6 doesn't have a clear() function for basic_strings
	//mCurrentLine.clear();
	mCurrentLine.erase();
	mCurrentLineNum = 0;

	p_fclose(aStream);
	return !hasErrors;
}
