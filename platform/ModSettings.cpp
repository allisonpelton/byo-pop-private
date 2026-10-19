#include "ModSettings.h"
#include "Text.h"
#include "VFS.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace Platform
{

static const int kMaxSettings = 64;
static const size_t kMaxChoices = 32;
static const int64_t kMaxSteps = 100000;
static const int kMaxPlaceholderDepth = 8;
static const int kMaxScale = 9;
static const size_t kMaxLabel = 40;
static const size_t kMaxHelp = 200;
static const size_t kMaxUnit = 8;
static const int64_t kIntDefaultMin = -1000000;
static const int64_t kIntDefaultMax = 1000000;
static const double kMaxUnits = 9.0e15;  // |units| below 2^53, so units and their doubles are exact

//////////////////////////////////////////////////////////////////////////
// Small helpers

static std::string IntText(int64_t theValue)
{
	// No printf: the same digits on every C library
	char aBuffer[24];
	int aPos = (int) sizeof(aBuffer);
	aBuffer[--aPos] = 0;
	uint64_t aMagnitude = theValue < 0 ? (uint64_t) 0 - (uint64_t) theValue : (uint64_t) theValue;
	do
	{
		aBuffer[--aPos] = (char) ('0' + (int) (aMagnitude % 10));
		aMagnitude /= 10;
	} while (aMagnitude != 0);
	if (theValue < 0)
		aBuffer[--aPos] = '-';
	return std::string(aBuffer + aPos);
}

static double Pow10(int theExponent)
{
	double aResult = 1;
	for (int i = 0; i < theExponent; i++)
		aResult *= 10;  // exact up to 10^22
	return aResult;
}

// A double that is a whole number in int64 range (and below 2^53, so exact)
static bool IsWholeNumber(const JsonValue& theValue, int64_t* theOut)
{
	if (!theValue.IsNumber())
		return false;
	double d = theValue.AsNumber();
	if (!(std::fabs(d) < kMaxUnits) || (std::floor(d) != d))
		return false;
	*theOut = (int64_t) d;
	return true;
}

// The fewest decimals (0..9) that write theValue exactly as its JSON text did, or -1
static int DecimalsOf(double theValue)
{
	for (int d = 0; d <= kMaxScale; d++)
	{
		double aScaled = theValue * Pow10(d);
		if (!(std::fabs(aScaled) < kMaxUnits))
			return -1;
		if (std::round(aScaled) / Pow10(d) == theValue)
			return d;
	}
	return -1;
}

// theValue in units of 10^-theScale, rounded half away from zero and limited to the exact range
static int64_t UnitsOf(double theValue, int theScale)
{
	double aScaled = theValue * Pow10(theScale);
	if (!(aScaled < kMaxUnits))
		return aScaled != aScaled ? 0 : (int64_t) kMaxUnits;
	if (!(aScaled > -kMaxUnits))
		return -(int64_t) kMaxUnits;
	return (int64_t) std::round(aScaled);
}

// A division of two exact doubles is correctly rounded: the same double as strtod of the decimal text
static double FromUnits(int64_t theUnits, int theScale)
{
	return (double) theUnits / Pow10(theScale);
}

static bool IsSettingId(const std::string& theId)
{
	if (theId.empty() || (theId.size() > 32) || !isalpha((unsigned char) theId[0]))
		return false;
	for (char c : theId)
		if (!isalnum((unsigned char) c) && (c != '_'))
			return false;
	return true;
}

// A choice value that is a string: an id ([A-Za-z0-9_.-], 1..32), so it cannot forge score key entries
static bool IsChoiceId(const std::string& theId)
{
	if (theId.empty() || (theId.size() > 32))
		return false;
	for (char c : theId)
		if (!isalnum((unsigned char) c) && (c != '_') && (c != '.') && (c != '-'))
			return false;
	return true;
}

static size_t Utf8Length(const std::string& theText)
{
	return DecodeUtf8(theText).size();
}

static std::string Utf8Cut(const std::string& theText, size_t theLength)
{
	std::vector<uint32_t> aCodePoints = DecodeUtf8(theText);
	if (aCodePoints.size() <= theLength)
		return theText;
	std::string aResult;
	for (size_t i = 0; i < theLength; i++)
		AppendUtf8(&aResult, aCodePoints[i]);
	return aResult;
}

static std::string Lower(const std::string& theText)
{
	std::string aResult = theText;
	for (char& c : aResult)
		c = (char) tolower((unsigned char) c);
	return aResult;
}

// A string, a whole number or a bool as text; false for anything else
static bool ScalarText(const JsonValue& theValue, std::string* theText)
{
	int64_t aWhole;
	if (theValue.IsString())
		*theText = theValue.AsString();
	else if (theValue.IsBool())
		*theText = theValue.AsBool() ? "true" : "false";
	else if (IsWholeNumber(theValue, &aWhole))
		*theText = IntText(aWhole);
	else
		return false;
	return true;
}

static bool IsPlaceholder(const JsonValue& theValue)
{
	return theValue.IsObject() && theValue.Has("$setting");
}

static bool IsOmit(const JsonValue& theValue)
{
	return theValue.IsObject() && (theValue.Members().size() == 1) && (theValue.Members()[0].first == "$omit") &&
		theValue.Members()[0].second.IsBool() && theValue.Members()[0].second.AsBool();
}

static int ParseLevel(const std::string& theName)
{
	if (theName == "live")
		return MOD_LEVEL_LIVE;
	if (theName == "nextGame")
		return MOD_LEVEL_NEXT_GAME;
	if (theName == "restart")
		return MOD_LEVEL_RESTART;
	return -1;
}

const char* ModSettingTypeName(int theType)
{
	switch (theType)
	{
	case MOD_SETTING_BOOL: return "bool";
	case MOD_SETTING_INT: return "int";
	case MOD_SETTING_FLOAT: return "float";
	case MOD_SETTING_CHOICE: return "choice";
	case MOD_SETTING_TEXT: return "text";
	case MOD_SETTING_COLOR: return "color";
	default: return "section";
	}
}

const char* ModSettingLevelName(int theLevel)
{
	switch (theLevel)
	{
	case MOD_LEVEL_NEXT_GAME: return "nextGame";
	case MOD_LEVEL_RESTART: return "restart";
	default: return "live";
	}
}

static const char* CharsetName(int theCharset)
{
	switch (theCharset)
	{
	case MOD_CHARSET_UPPER: return "upper";
	case MOD_CHARSET_DIGITS: return "digits";
	case MOD_CHARSET_NAME: return "name";
	default: return "any";
	}
}

static int ParseType(const std::string& theName)
{
	static const char* const aNames[] = {"section", "bool", "int", "float", "choice", "text", "color"};
	for (int i = 0; i < 7; i++)
		if (theName == aNames[i])
			return i;
	return -1;
}

// One Latin-1 character for a text setting, or '?' when its charset does not allow it
static unsigned char CharsetChar(uint32_t theCodePoint, int theCharset, bool theNoPercent)
{
	if (theCodePoint > 0xFF)
		return '?';
	unsigned char c = (unsigned char) theCodePoint;
	bool isPrintable = ((c >= 0x20) && (c <= 0x7E)) || (c >= 0xA0);
	if (!isPrintable || (theNoPercent && (c == '%')))
		return '?';
	switch (theCharset)
	{
	case MOD_CHARSET_UPPER:
		if ((c >= 'a') && (c <= 'z'))
			return (unsigned char) (c - 0x20);
		if ((c >= 0xE0) && (c <= 0xFE) && (c != 0xF7))
			return (unsigned char) (c - 0x20);
		if ((c == 0xDF) || (c == 0xFF) || (c == 0xB5))  // no Latin-1 capital
			return '?';
		return c;
	case MOD_CHARSET_DIGITS:
		return ((c >= '0') && (c <= '9')) ? c : '?';
	case MOD_CHARSET_NAME:
		return (isalnum(c) && (c < 0x80)) || (c == ' ') || (c == '-') || (c == '_') ? c : '?';
	default:
		return c;
	}
}

static bool ParseColor(const std::string& theText, bool theAlpha, std::string* theNormal)
{
	if ((theText.size() != 7) && (theText.size() != 9))
		return false;
	if (theText[0] != '#')
		return false;
	std::string aHex = Lower(theText.substr(1));
	for (char c : aHex)
		if (!isxdigit((unsigned char) c))
			return false;
	if (aHex.size() == 8 && !theAlpha)
		return false;
	if (aHex.size() == 6 && theAlpha)
		aHex += "ff";
	*theNormal = "#" + aHex;
	return true;
}

static int HexByte(const std::string& theText, size_t thePos)
{
	return (int) strtol(theText.substr(thePos, 2).c_str(), nullptr, 16);
}

//////////////////////////////////////////////////////////////////////////
// Values

std::string ModSettingCanonical(const ModSettingDef& theDef, const JsonValue& theValue)
{
	switch (theDef.mType)
	{
	case MOD_SETTING_BOOL:
		return theValue.AsBool() ? "true" : "false";
	case MOD_SETTING_INT:
	{
		int64_t aWhole = 0;
		IsWholeNumber(theValue, &aWhole);
		return IntText(aWhole);
	}
	case MOD_SETTING_FLOAT:
	{
		double aValue = FromUnits(UnitsOf(theValue.AsNumber(), theDef.mScale), theDef.mScale);
		uint64_t aBits;
		memcpy(&aBits, &aValue, sizeof(aBits));
		static const char aHex[] = "0123456789abcdef";
		std::string aText(16, '0');
		for (int i = 15; i >= 0; i--, aBits >>= 4)
			aText[(size_t) i] = aHex[aBits & 15];
		return aText;
	}
	case MOD_SETTING_CHOICE:
	{
		std::string aText;
		ScalarText(theValue, &aText);
		return aText;
	}
	case MOD_SETTING_TEXT:
	{
		static const char aHex[] = "0123456789ABCDEF";
		std::string aLatin1 = Utf8ToLatin1(theValue.AsString(), '?');
		std::string aText;
		for (unsigned char c : aLatin1)
		{
			if ((c < 0x20) || (c >= 0x7F) || strchr("%&=?;", c))
			{
				aText += '%';
				aText += aHex[c >> 4];
				aText += aHex[c & 15];
			}
			else
				aText += (char) c;
		}
		return aText;
	}
	case MOD_SETTING_COLOR:
		return theValue.AsString();
	default:
		return "";
	}
}

std::string ModSettingText(const ModSettingDef& theDef, const JsonValue& theValue)
{
	switch (theDef.mType)
	{
	case MOD_SETTING_FLOAT:
	{
		// units at mScale, shown with mDecimals digits (rounded half away from zero)
		int64_t aUnits = UnitsOf(theValue.AsNumber(), theDef.mScale);
		int aDecimals = theDef.mDecimals;
		if (aDecimals >= theDef.mScale)
		{
			for (int i = theDef.mScale; i < aDecimals; i++)
				aUnits *= 10;
		}
		else
		{
			int64_t aDivisor = 1;
			for (int i = aDecimals; i < theDef.mScale; i++)
				aDivisor *= 10;
			aUnits = (aUnits >= 0) ? (aUnits + aDivisor / 2) / aDivisor : -((-aUnits + aDivisor / 2) / aDivisor);
		}
		bool isNegative = aUnits < 0;
		std::string aDigits = IntText(isNegative ? -aUnits : aUnits);
		if (aDecimals > 0)
		{
			if ((int) aDigits.size() <= aDecimals)
				aDigits.insert(0, (size_t) (aDecimals + 1 - (int) aDigits.size()), '0');
			aDigits.insert(aDigits.size() - (size_t) aDecimals, ".");
		}
		return (isNegative ? "-" : "") + aDigits;
	}
	case MOD_SETTING_TEXT:
	case MOD_SETTING_COLOR:
		return theValue.AsString();
	default:
		return ModSettingCanonical(theDef, theValue);
	}
}

bool ModSettingSame(const ModSettingDef& theDef, const JsonValue& theA, const JsonValue& theB)
{
	if (theA.GetType() != theB.GetType())
	{
		// a choice value read as "1" and 1 is the same choice
		if (theDef.mType != MOD_SETTING_CHOICE)
			return false;
	}
	return ModSettingCanonical(theDef, theA) == ModSettingCanonical(theDef, theB);
}

int ModSettings::Normalize(const ModSettingDef& theDef, const JsonValue& theStored, JsonValue* theValue,
	std::string* theWarning) const
{
	theWarning->clear();
	*theValue = theDef.mDefault;
	switch (theDef.mType)
	{
	case MOD_SETTING_BOOL:
		if (!theStored.IsBool())
		{
			*theWarning = "not true or false";
			return MOD_VALUE_WRONG;
		}
		*theValue = theStored;
		return MOD_VALUE_OK;

	case MOD_SETTING_INT:
	{
		int64_t aValue;
		if (!IsWholeNumber(theStored, &aValue))
		{
			*theWarning = "not a whole number";
			return MOD_VALUE_WRONG;
		}
		int aResult = MOD_VALUE_OK;
		if ((aValue < theDef.mMin) || (aValue > theDef.mMax))
		{
			*theWarning = IntText(aValue) + " is outside " + IntText(theDef.mMin) + ".." + IntText(theDef.mMax);
			aValue = std::max(theDef.mMin, std::min(theDef.mMax, aValue));
			aResult = MOD_VALUE_ADJUSTED;
		}
		int64_t aOffset = aValue - theDef.mMin;
		int64_t k = (2 * aOffset + theDef.mStep) / (2 * theDef.mStep);
		if (theDef.mMin + k * theDef.mStep > theDef.mMax)
			k--;
		if (theDef.mMin + k * theDef.mStep != aValue)
		{
			aValue = theDef.mMin + k * theDef.mStep;
			aResult = MOD_VALUE_ADJUSTED;
		}
		*theValue = JsonValue((double) aValue);
		return aResult;
	}

	case MOD_SETTING_FLOAT:
	{
		if (!theStored.IsNumber())
		{
			*theWarning = "not a number";
			return MOD_VALUE_WRONG;
		}
		int aResult = MOD_VALUE_OK;
		int64_t aUnits = UnitsOf(theStored.AsNumber(), theDef.mScale);
		int64_t aTop = theDef.mMin + ((theDef.mMax - theDef.mMin) / theDef.mStep) * theDef.mStep;
		if ((aUnits < theDef.mMin) || (aUnits > theDef.mMax))
		{
			*theWarning = "outside " + ModSettingText(theDef, JsonValue(FromUnits(theDef.mMin, theDef.mScale))) + ".." +
				ModSettingText(theDef, JsonValue(FromUnits(theDef.mMax, theDef.mScale)));
			aResult = MOD_VALUE_ADJUSTED;
		}
		aUnits = std::max(theDef.mMin, std::min(aTop, aUnits));
		int64_t aOffset = aUnits - theDef.mMin;
		int64_t k = (2 * aOffset + theDef.mStep) / (2 * theDef.mStep);
		if (theDef.mMin + k * theDef.mStep > aTop)
			k--;
		aUnits = theDef.mMin + k * theDef.mStep;
		double aValue = FromUnits(aUnits, theDef.mScale);
		if (aValue != theStored.AsNumber())
			aResult = MOD_VALUE_ADJUSTED;
		*theValue = JsonValue(aValue);
		return aResult;
	}

	case MOD_SETTING_CHOICE:
	{
		std::string aText;
		if (ScalarText(theStored, &aText))
		{
			for (const ModSettingChoice& aChoice : theDef.mChoices)
			{
				if (aChoice.mCanonical == aText)
				{
					*theValue = aChoice.mValue;
					return theStored.GetType() == aChoice.mValue.GetType() ? MOD_VALUE_OK : MOD_VALUE_ADJUSTED;
				}
			}
		}
		*theWarning = "not one of the choices";
		return MOD_VALUE_WRONG;
	}

	case MOD_SETTING_TEXT:
	{
		if (!theStored.IsString())
		{
			*theWarning = "not text";
			return MOD_VALUE_WRONG;
		}
		std::vector<uint32_t> aCodePoints = DecodeUtf8(theStored.AsString());
		std::string aLatin1;
		for (size_t i = 0; (i < aCodePoints.size()) && ((int) aLatin1.size() < theDef.mMaxLength); i++)
			aLatin1 += (char) CharsetChar(aCodePoints[i], theDef.mCharset, theDef.mNoPercent);
		std::string aText = Latin1ToUtf8(aLatin1);
		*theValue = JsonValue(aText);
		return aText == theStored.AsString() ? MOD_VALUE_OK : MOD_VALUE_ADJUSTED;
	}

	case MOD_SETTING_COLOR:
	{
		std::string aColor;
		if (!theStored.IsString() || !ParseColor(theStored.AsString(), theDef.mAlpha, &aColor))
		{
			*theWarning = theDef.mAlpha ? "not a colour #rrggbbaa" : "not a colour #rrggbb";
			return MOD_VALUE_WRONG;
		}
		*theValue = JsonValue(aColor);
		return aColor == theStored.AsString() ? MOD_VALUE_OK : MOD_VALUE_ADJUSTED;
	}

	default:
		*theValue = JsonValue();
		return MOD_VALUE_OK;
	}
}

void ModSettings::ReadValues(const JsonValue& theStored, bool theFillDefaults, ModSettingValues* theValues,
	std::vector<std::string>* theWarnings) const
{
	theValues->clear();
	for (const ModSettingDef& aDef : mDefs)
	{
		if (!aDef.IsValue())
			continue;
		if (theStored.IsObject() && theStored.Has(aDef.mId))
		{
			JsonValue aValue;
			std::string aWarning;
			Normalize(aDef, theStored[aDef.mId], &aValue, &aWarning);
			if (!aWarning.empty() && theWarnings)
				theWarnings->push_back("setting \"" + aDef.mId + "\": stored value " + theStored[aDef.mId].Dump(0) + " " + aWarning +
					(ModSettingSame(aDef, aValue, aDef.mDefault) ? ", using the default" : ", using " + ModSettingText(aDef, aValue)));
			(*theValues)[aDef.mId] = aValue;
		}
		else if (theFillDefaults)
			(*theValues)[aDef.mId] = aDef.mDefault;
	}
}

const ModSettingDef* ModSettings::Find(const std::string& theId) const
{
	for (const ModSettingDef& aDef : mDefs)
		if (aDef.IsValue() && (aDef.mId == theId))
			return &aDef;
	return nullptr;
}

ModSettingDef* ModSettings::FindForAnalysis(const std::string& theId)
{
	return const_cast<ModSettingDef*>(Find(theId));
}

bool ModSettings::HasValues() const
{
	for (const ModSettingDef& aDef : mDefs)
		if (aDef.IsValue())
			return true;
	return false;
}

JsonValue ModSettings::GetValue(const ModSettingValues& theValues, const ModSettingDef& theDef) const
{
	auto it = theValues.find(theDef.mId);
	return it == theValues.end() ? theDef.mDefault : it->second;
}

bool ModSettings::IsActive(const ModSettingValues& theValues, const ModSettingDef& theDef) const
{
	if (theDef.mIfSetting.empty())
		return true;
	const ModSettingDef* aCondition = Find(theDef.mIfSetting);
	if (aCondition == nullptr)
		return true;
	bool isSame = ModSettingCanonical(*aCondition, EffectiveValue(theValues, *aCondition)) == theDef.mIfCanonical;
	return isSame == theDef.mIfEquals;
}

JsonValue ModSettings::EffectiveValue(const ModSettingValues& theValues, const ModSettingDef& theDef) const
{
	return IsActive(theValues, theDef) ? GetValue(theValues, theDef) : theDef.mDefault;
}

//////////////////////////////////////////////////////////////////////////
// Schema

static std::string Where(const ModSettingDef& theDef, size_t theIndex)
{
	if (!theDef.mId.empty())
		return "setting \"" + theDef.mId + "\": ";
	return "settings[" + IntText((int64_t) theIndex) + "]: ";
}

// A text key cut to theMax characters; false (with an error) if it is there but not a string
static bool ReadText(const JsonValue& theEntry, const char* theKey, size_t theMax, bool theRequired, std::string* theOut,
	const std::string& theWhere, std::vector<std::string>* theErrors, std::vector<std::string>* theWarnings)
{
	if (!theEntry.Has(theKey))
	{
		if (theRequired)
			theErrors->push_back(theWhere + "\"" + theKey + "\" is missing");
		return !theRequired;
	}
	const JsonValue& aValue = theEntry[theKey];
	if (!aValue.IsString() || (theRequired && aValue.AsString().empty()))
	{
		theErrors->push_back(theWhere + "\"" + theKey + "\" must be " + (theRequired ? "non-empty text" : "text"));
		return false;
	}
	*theOut = aValue.AsString();
	if (Utf8Length(*theOut) > theMax)
	{
		theWarnings->push_back(theWhere + "\"" + theKey + "\" is cut to " + IntText((int64_t) theMax) + " characters");
		*theOut = Utf8Cut(*theOut, theMax);
	}
	return true;
}

void ModSettings::Parse(const JsonValue& theManifest)
{
	*this = ModSettings();
	if (!theManifest.Has("settings"))
		return;
	mPresent = true;
	const JsonValue& aSettings = theManifest["settings"];
	if (!aSettings.IsArray())
	{
		mErrors.push_back("\"settings\" must be an array");
		mSchemaErrors = mErrors.size();
		return;
	}

	int aCount = 0;
	bool isFullReported = false;
	for (size_t i = 0; i < aSettings.Size(); i++)
	{
		const JsonValue& anEntry = aSettings[i];
		ModSettingDef aDef;
		std::string aWhere = Where(aDef, i);
		if (!anEntry.IsObject())
		{
			mErrors.push_back(aWhere + "must be an object");
			continue;
		}
		int aType = anEntry["type"].IsString() ? ParseType(anEntry["type"].AsString()) : -1;
		if (aType == MOD_SETTING_SECTION)
		{
			if (ReadText(anEntry, "label", kMaxLabel, true, &aDef.mLabel, aWhere, &mErrors, &mWarnings))
				mDefs.push_back(aDef);
			continue;
		}

		if (!anEntry["id"].IsString() || !IsSettingId(anEntry["id"].AsString()))
		{
			mErrors.push_back(aWhere + "\"id\" must be a letter and up to 31 letters, digits or _");
			continue;
		}
		aDef.mId = anEntry["id"].AsString();
		aWhere = Where(aDef, i);
		if (Find(aDef.mId) != nullptr)
		{
			mErrors.push_back(aWhere + "the id is used twice");
			continue;
		}
		if (aType < 0)
		{
			mErrors.push_back(aWhere + "\"type\" must be bool, int, float, choice, text, color or section");
			continue;
		}
		if (aCount >= kMaxSettings)
		{
			if (!isFullReported)
				mErrors.push_back(aWhere + "a mod has at most 64 settings: this one and the rest are left out");
			isFullReported = true;
			continue;
		}
		aDef.mType = aType;
		if (!ReadText(anEntry, "label", kMaxLabel, true, &aDef.mLabel, aWhere, &mErrors, &mWarnings) ||
			!ReadText(anEntry, "help", kMaxHelp, false, &aDef.mHelp, aWhere, &mErrors, &mWarnings))
			continue;
		if (!anEntry.Has("default"))
		{
			mErrors.push_back(aWhere + "\"default\" is missing");
			continue;
		}
		const JsonValue& aDefault = anEntry["default"];

		bool isValid = true;
		switch (aType)
		{
		case MOD_SETTING_BOOL:
			isValid = aDefault.IsBool();
			if (!isValid)
				mErrors.push_back(aWhere + "\"default\" must be true or false");
			aDef.mDefault = aDefault;
			break;

		case MOD_SETTING_INT:
		{
			aDef.mMin = kIntDefaultMin;
			aDef.mMax = kIntDefaultMax;
			aDef.mStep = 1;
			const char* const aKeys[3] = {"min", "max", "step"};
			int64_t* const aOuts[3] = {&aDef.mMin, &aDef.mMax, &aDef.mStep};
			for (int k = 0; k < 3 && isValid; k++)
			{
				if (!anEntry.Has(aKeys[k]))
					continue;
				int64_t aWhole;
				if (!IsWholeNumber(anEntry[aKeys[k]], &aWhole) || (aWhole < INT32_MIN) || (aWhole > INT32_MAX))
				{
					mErrors.push_back(aWhere + "\"" + aKeys[k] + "\" must be a whole number from -2147483648 to 2147483647");
					isValid = false;
				}
				else
					*aOuts[k] = aWhole;
			}
			if (!isValid)
				break;
			if ((aDef.mMin > aDef.mMax) || (aDef.mStep <= 0))
			{
				mErrors.push_back(aWhere + "needs min <= max and a step above 0");
				isValid = false;
				break;
			}
			if (!ReadText(anEntry, "unit", kMaxUnit, false, &aDef.mUnit, aWhere, &mErrors, &mWarnings))
			{
				isValid = false;
				break;
			}
			JsonValue aValue;
			std::string aWarning;
			if (Normalize(aDef, aDefault, &aValue, &aWarning) != MOD_VALUE_OK)
			{
				mErrors.push_back(aWhere + "\"default\" must be a whole number from min to max on a step");
				isValid = false;
			}
			aDef.mDefault = aValue;
			break;
		}

		case MOD_SETTING_FLOAT:
		{
			if (!anEntry["min"].IsNumber() || !anEntry["max"].IsNumber() || !anEntry["step"].IsNumber() || !aDefault.IsNumber())
			{
				mErrors.push_back(aWhere + "a float needs the numbers \"min\", \"max\", \"step\" and \"default\"");
				isValid = false;
				break;
			}
			double aMin = anEntry["min"].AsNumber(), aMax = anEntry["max"].AsNumber(), aStep = anEntry["step"].AsNumber();
			int aScale = 0;
			for (double aNumber : {aMin, aMax, aStep, aDefault.AsNumber()})
			{
				int d = DecimalsOf(aNumber);
				if (d < 0)
				{
					aScale = -1;
					break;
				}
				aScale = std::max(aScale, d);
			}
			if (aScale < 0)
			{
				mErrors.push_back(aWhere + "min, max, step and default must have at most 9 decimals and stay below 9000000");
				isValid = false;
				break;
			}
			aDef.mScale = aScale;
			aDef.mMin = UnitsOf(aMin, aScale);
			aDef.mMax = UnitsOf(aMax, aScale);
			aDef.mStep = UnitsOf(aStep, aScale);
			if ((aDef.mMin > aDef.mMax) || (aDef.mStep <= 0))
			{
				mErrors.push_back(aWhere + "needs min <= max and a step above 0");
				isValid = false;
				break;
			}
			if ((aDef.mMax - aDef.mMin) / aDef.mStep > kMaxSteps)
			{
				mErrors.push_back(aWhere + "has more than 100000 steps");
				isValid = false;
				break;
			}
			aDef.mDecimals = DecimalsOf(aStep);
			if (anEntry.Has("decimals"))
			{
				int64_t aDecimals;
				if (IsWholeNumber(anEntry["decimals"], &aDecimals) && (aDecimals >= 0) && (aDecimals <= kMaxScale))
					aDef.mDecimals = (int) aDecimals;
				else
					mWarnings.push_back(aWhere + "\"decimals\" must be 0..9; it is ignored");
			}
			if (!ReadText(anEntry, "unit", kMaxUnit, false, &aDef.mUnit, aWhere, &mErrors, &mWarnings))
			{
				isValid = false;
				break;
			}
			JsonValue aValue;
			std::string aWarning;
			if (Normalize(aDef, aDefault, &aValue, &aWarning) != MOD_VALUE_OK)
			{
				mErrors.push_back(aWhere + "\"default\" must be from min to max on a step");
				isValid = false;
			}
			aDef.mDefault = aValue;
			break;
		}

		case MOD_SETTING_CHOICE:
		{
			const JsonValue& aChoices = anEntry["choices"];
			if (!aChoices.IsArray() || (aChoices.Size() < 2) || (aChoices.Size() > kMaxChoices))
			{
				mErrors.push_back(aWhere + "\"choices\" must be an array of 2 to 32 entries");
				isValid = false;
				break;
			}
			for (size_t c = 0; c < aChoices.Size() && isValid; c++)
			{
				const JsonValue& aChoice = aChoices[c];
				ModSettingChoice anOption;
				const JsonValue& aValue = aChoice["value"];
				int64_t aWhole;
				bool isScalar = (aValue.IsString() && IsChoiceId(aValue.AsString())) || aValue.IsBool() ||
					(IsWholeNumber(aValue, &aWhole) && (aWhole >= INT32_MIN) && (aWhole <= INT32_MAX));
				if (!aChoice.IsObject() || !isScalar)
				{
					mErrors.push_back(aWhere + "choice " + IntText((int64_t) c + 1) +
						": \"value\" must be an id (letters, digits, _ . -), a whole number or true/false");
					isValid = false;
					break;
				}
				anOption.mValue = aValue;
				ScalarText(aValue, &anOption.mCanonical);
				for (const ModSettingChoice& anOther : aDef.mChoices)
				{
					if (anOther.mCanonical == anOption.mCanonical)
					{
						mErrors.push_back(aWhere + "the choice " + anOption.mCanonical + " is listed twice");
						isValid = false;
					}
				}
				if (isValid && !ReadText(aChoice, "label", kMaxLabel, true, &anOption.mLabel, aWhere + "choice " + anOption.mCanonical + ": ",
					&mErrors, &mWarnings))
					isValid = false;
				aDef.mChoices.push_back(anOption);
			}
			if (!isValid)
				break;
			JsonValue aValue;
			std::string aWarning;
			if (Normalize(aDef, aDefault, &aValue, &aWarning) != MOD_VALUE_OK)
			{
				mErrors.push_back(aWhere + "\"default\" must be one of the choices");
				isValid = false;
			}
			aDef.mDefault = aValue;
			break;
		}

		case MOD_SETTING_TEXT:
		{
			if (anEntry.Has("maxLength"))
			{
				int64_t aLength;
				if (!IsWholeNumber(anEntry["maxLength"], &aLength) || (aLength < 1) || (aLength > 64))
				{
					mErrors.push_back(aWhere + "\"maxLength\" must be 1..64");
					isValid = false;
					break;
				}
				aDef.mMaxLength = (int) aLength;
			}
			if (anEntry.Has("charset"))
			{
				static const char* const aNames[] = {"any", "upper", "digits", "name"};
				std::string aName = anEntry["charset"].AsString("");
				aDef.mCharset = -1;
				for (int c = 0; c < 4; c++)
					if (aName == aNames[c])
						aDef.mCharset = c;
				if (aDef.mCharset < 0)
				{
					mErrors.push_back(aWhere + "\"charset\" must be any, upper, digits or name");
					isValid = false;
					break;
				}
			}
			JsonValue aValue;
			std::string aWarning;
			if (Normalize(aDef, aDefault, &aValue, &aWarning) != MOD_VALUE_OK)
			{
				mErrors.push_back(aWhere + "\"default\" must be text that fits maxLength and the charset");
				isValid = false;
			}
			aDef.mDefault = aValue;
			break;
		}

		case MOD_SETTING_COLOR:
		{
			if (anEntry.Has("alpha"))
			{
				if (!anEntry["alpha"].IsBool())
				{
					mErrors.push_back(aWhere + "\"alpha\" must be true or false");
					isValid = false;
					break;
				}
				aDef.mAlpha = anEntry["alpha"].AsBool();
			}
			JsonValue aValue;
			std::string aWarning;
			if (Normalize(aDef, aDefault, &aValue, &aWarning) == MOD_VALUE_WRONG)
			{
				mErrors.push_back(aWhere + std::string("\"default\" must be a colour ") + (aDef.mAlpha ? "#rrggbbaa" : "#rrggbb"));
				isValid = false;
			}
			aDef.mDefault = aValue;
			break;
		}
		}
		if (!isValid)
			continue;

		if (anEntry.Has("apply"))
		{
			aDef.mApply = ParseLevel(anEntry["apply"].AsString(""));
			if (aDef.mApply < 0)
				mWarnings.push_back(aWhere + "\"apply\" must be live, nextGame or restart; it is ignored");
		}
		if (anEntry.Has("affectsScores"))
		{
			if (!anEntry["affectsScores"].IsBool())
				mWarnings.push_back(aWhere + "\"affectsScores\" must be true or false; it is ignored");
			aDef.mAffectsScores = anEntry["affectsScores"].AsBool(false);
		}
		if (anEntry.Has("enabledIf"))
		{
			const JsonValue& anIf = anEntry["enabledIf"];
			const ModSettingDef* aCondition = anIf["setting"].IsString() ? Find(anIf["setting"].AsString()) : nullptr;
			bool hasEquals = anIf.Has("equals"), hasNotEquals = anIf.Has("notEquals");
			JsonValue aValue;
			std::string aWarning;
			if (!anIf.IsObject() || (aCondition == nullptr) ||
				((aCondition->mType != MOD_SETTING_BOOL) && (aCondition->mType != MOD_SETTING_CHOICE)) || (hasEquals == hasNotEquals) ||
				(Normalize(*aCondition, anIf[hasEquals ? "equals" : "notEquals"], &aValue, &aWarning) != MOD_VALUE_OK))
				mWarnings.push_back(aWhere + "\"enabledIf\" must name an earlier bool or choice setting and one of its values with "
					"\"equals\" or \"notEquals\"; it is ignored");
			else
			{
				aDef.mIfSetting = aCondition->mId;
				aDef.mIfCanonical = ModSettingCanonical(*aCondition, aValue);
				aDef.mIfEquals = hasEquals;
			}
		}
		mDefs.push_back(aDef);
		aCount++;
	}
	mSchemaErrors = mErrors.size();
	mSchemaWarnings = mWarnings.size();
}

//////////////////////////////////////////////////////////////////////////
// Placeholders: where they may be and what they use

static bool IsDataKey(const std::string& theKey)
{
	return (theKey == "tunables") || (theKey == "rules") || (theKey == "features");
}

// May a placeholder stand at this depth of a top-level key (0: the key's value itself)?
static bool IsPlaceholderAllowed(const std::string& theTop, int theDepth)
{
	if (IsDataKey(theTop))
		return true;
	if (theTop == "imageScale")
		return theDepth == 0;
	if ((theTop == "files") || (theTop == "images") || (theTop == "strings") || (theTop == "imagePatches") || (theTop == "imageOps"))
		return theDepth >= 1;
	return false;
}

// Keys whose placeholders are replaced (all others are copied as they are)
static bool IsResolvedKey(const std::string& theTop)
{
	return IsPlaceholderAllowed(theTop, 0) || IsPlaceholderAllowed(theTop, 1);
}

// Keys whose values name files of the mod: text and colour values may not be put there directly
static bool IsPathKey(const std::string& theTop, bool theStringsIsArray)
{
	return (theTop == "files") || (theTop == "images") || (theTop == "imagePatches") || (theTop == "imageOps") ||
		((theTop == "strings") && theStringsIsArray);
}

namespace
{
struct Use
{
	std::string mId;
	int mLevel;
	bool mScores;
};

struct Analyzer
{
	ModSettings* mSettings;
	const NextGameFilter* mFilter;
	const std::vector<std::string>* mGameplayFiles;
	bool mStringsIsArray;
	std::vector<Use> mUses;

	void Bad(const std::string& theTop, const std::string& theText)
	{
		std::string aText = theTop + ": " + theText;
		if (std::find(mSettings->mErrors.begin(), mSettings->mErrors.end(), aText) == mSettings->mErrors.end())
			mSettings->mErrors.push_back(aText);
		mSettings->mBadKeys.insert(theTop);
	}

	// The data paths a placeholder's results can change (the next-game filter checks each). $omit sets nothing.
	static void CollectLeaves(const JsonValue& theValue, const std::string& thePath, std::vector<std::string>* theLeaves)
	{
		if (IsOmit(theValue))
			return;
		if (IsPlaceholder(theValue))
		{
			if (theValue["map"].IsObject() && !theValue["map"].Members().empty())
			{
				for (const auto& kv : theValue["map"].Members())
					CollectLeaves(kv.second, thePath, theLeaves);
			}
			else
				theLeaves->push_back(thePath);
		}
		else if (theValue.IsObject() && !theValue.Members().empty())
		{
			for (const auto& kv : theValue.Members())
				CollectLeaves(kv.second, thePath + "." + kv.first, theLeaves);
		}
		else
			theLeaves->push_back(thePath);
	}

	bool IsGameplayFile(const std::string& theKey) const
	{
		std::string aKey = Lower(VFS::Normalize(theKey));
		for (const std::string& aFile : *mGameplayFiles)
			if (Lower(VFS::Normalize(aFile)) == aKey)
				return true;
		return false;
	}

	void Placeholder(const JsonValue& theValue, const std::string& theTop, const std::string& theMember, const std::string& thePath,
		int theDepth, int theNesting)
	{
		if (theNesting >= kMaxPlaceholderDepth)
		{
			Bad(theTop, "placeholders are nested deeper than 8 levels");
			return;
		}
		if (!IsPlaceholderAllowed(theTop, theDepth))
		{
			Bad(theTop, IsResolvedKey(theTop) ? "a placeholder cannot stand for the whole key, only for its members" :
				"placeholders are not allowed in this key");
			return;
		}
		for (const auto& kv : theValue.Members())
		{
			if ((kv.first != "$setting") && (kv.first != "map") && (kv.first != "as"))
			{
				Bad(theTop, "unknown key \"" + kv.first + "\" in a placeholder");
				return;
			}
		}
		std::string anId = theValue["$setting"].AsString("");
		const ModSettingDef* aDef = mSettings->Find(anId);
		if (aDef == nullptr)
		{
			Bad(theTop, "setting \"" + anId + "\" is not declared");
			return;
		}
		bool hasMap = theValue.Has("map"), hasAs = theValue.Has("as");
		if (hasMap && hasAs)
		{
			Bad(theTop, "\"" + anId + "\": \"map\" and \"as\" cannot be used together");
			return;
		}
		if (hasMap)
		{
			const JsonValue& aMap = theValue["map"];
			if (!aMap.IsObject())
			{
				Bad(theTop, "\"" + anId + "\": \"map\" must be an object");
				return;
			}
			if ((aDef->mType != MOD_SETTING_BOOL) && (aDef->mType != MOD_SETTING_INT) && (aDef->mType != MOD_SETTING_CHOICE))
			{
				Bad(theTop, "\"" + anId + "\": only bool, int and choice settings can have a \"map\"");
				return;
			}
			for (const auto& kv : aMap.Members())
			{
				if (kv.first == "*")
					continue;
				bool canMatch = false;
				if (aDef->mType == MOD_SETTING_BOOL)
					canMatch = (kv.first == "true") || (kv.first == "false");
				else if (aDef->mType == MOD_SETTING_CHOICE)
				{
					for (const ModSettingChoice& aChoice : aDef->mChoices)
						canMatch = canMatch || (aChoice.mCanonical == kv.first);
				}
				else
				{
					// an int on a step from min to max, written as its canonical text
					char* anEnd = nullptr;
					long long aNumber = strtoll(kv.first.c_str(), &anEnd, 10);
					canMatch = !kv.first.empty() && (*anEnd == 0) && (IntText(aNumber) == kv.first) && (aNumber >= aDef->mMin) &&
						(aNumber <= aDef->mMax) && ((aNumber - aDef->mMin) % aDef->mStep == 0);
				}
				if (!canMatch)
				{
					Bad(theTop, "\"" + anId + "\": the map key \"" + kv.first + "\" can never match");
					return;
				}
			}
		}
		if (hasAs)
		{
			std::string anAs = theValue["as"].AsString("");
			bool isValid = ((anAs == "rgb" || anAs == "rgba" || anAs == "argb") && (aDef->mType == MOD_SETTING_COLOR)) ||
				((anAs == "int") && ((aDef->mType == MOD_SETTING_INT) || (aDef->mType == MOD_SETTING_FLOAT))) || (anAs == "string");
			if (!isValid)
			{
				Bad(theTop, "\"" + anId + "\": \"as\": \"" + anAs + "\" does not fit a " + ModSettingTypeName(aDef->mType) + " setting");
				return;
			}
		}
		if (!hasMap && IsPathKey(theTop, mStringsIsArray) && ((aDef->mType == MOD_SETTING_TEXT) || (aDef->mType == MOD_SETTING_COLOR)))
		{
			Bad(theTop, "\"" + anId + "\": a text or colour value cannot name a file; use a choice and a \"map\"");
			return;
		}

		Use aUse;
		aUse.mId = anId;
		aUse.mScores = (theTop == "tunables") || (theTop == "rules") || ((theTop == "files") && IsGameplayFile(theMember));
		aUse.mLevel = MOD_LEVEL_RESTART;
		if (IsDataKey(theTop) && *mFilter)
		{
			std::vector<std::string> aLeaves;
			CollectLeaves(theValue, thePath, &aLeaves);
			bool isNextGame = true;
			for (const std::string& aLeaf : aLeaves)
				isNextGame = isNextGame && (*mFilter)(aLeaf);
			if (isNextGame)
				aUse.mLevel = MOD_LEVEL_NEXT_GAME;
		}
		mUses.push_back(aUse);
		if (theTop == "strings")
			mSettings->FindForAnalysis(anId)->mNoPercent = true;

		// A map entry stands where the placeholder stood
		if (hasMap)
			for (const auto& kv : theValue["map"].Members())
				Walk(kv.second, theTop, theMember, thePath, theDepth, theNesting + 1);
	}

	void Walk(const JsonValue& theValue, const std::string& theTop, const std::string& theMember, const std::string& thePath,
		int theDepth, int theNesting)
	{
		if (IsPlaceholder(theValue))
			Placeholder(theValue, theTop, theMember, thePath, theDepth, theNesting);
		else if (theValue.IsObject())
		{
			for (const auto& kv : theValue.Members())
				Walk(kv.second, theTop, theDepth == 0 ? kv.first : theMember, thePath + "." + kv.first, theDepth + 1, theNesting);
		}
		else if (theValue.IsArray())
		{
			for (const JsonValue& anItem : theValue.Items())
				Walk(anItem, theTop, theMember, thePath, theDepth + 1, theNesting);
		}
	}
};
}

void ModSettings::Analyze(const JsonValue& theData, const NextGameFilter& theFilter, const std::vector<std::string>& theGameplayFiles)
{
	// Errors from an earlier analysis go; the schema's stay
	mErrors.resize(mSchemaErrors);
	mWarnings.resize(mSchemaWarnings);
	mBadKeys.clear();
	for (ModSettingDef& aDef : mDefs)
	{
		aDef.mLevel = MOD_LEVEL_LIVE;
		aDef.mScores = aDef.mAffectsScores;
		aDef.mUsed = false;
		aDef.mNoPercent = false;
	}
	if (!mPresent)
		return;

	Analyzer anAnalyzer;
	anAnalyzer.mSettings = this;
	anAnalyzer.mFilter = &theFilter;
	anAnalyzer.mGameplayFiles = &theGameplayFiles;
	anAnalyzer.mStringsIsArray = theData["strings"].IsArray();
	for (const auto& kv : theData.Members())
	{
		if (kv.first == "settings")
			continue;
		anAnalyzer.Walk(kv.second, kv.first, "", kv.first, 0, 0);
	}

	std::map<std::string, int> aLevels;
	for (const Use& aUse : anAnalyzer.mUses)
	{
		ModSettingDef* aDef = FindForAnalysis(aUse.mId);
		aDef->mUsed = true;
		aDef->mScores = aDef->mScores || aUse.mScores;
		aLevels[aUse.mId] = std::max(aLevels.count(aUse.mId) ? aLevels[aUse.mId] : MOD_LEVEL_LIVE, aUse.mLevel);
	}
	for (ModSettingDef& aDef : mDefs)
	{
		if (!aDef.IsValue())
			continue;
		std::string aWhere = "setting \"" + aDef.mId + "\": ";
		int aLevel = aDef.mUsed ? aLevels[aDef.mId] : MOD_LEVEL_LIVE;
		if (aDef.mApply > aLevel)
			aLevel = aDef.mApply;
		else if (aDef.mUsed && (aDef.mApply >= 0) && (aDef.mApply < aLevel))
			mWarnings.push_back(aWhere + "\"apply\": \"" + ModSettingLevelName(aDef.mApply) + "\" is earlier than its data allows; it applies at " +
				(aLevel == MOD_LEVEL_RESTART ? "a restart" : "the next game"));
		// A value that splits the scores never changes mid-game (the save names, high-score tables and replays
		// all follow the key)
		if (aDef.mAffectsScores && (aLevel == MOD_LEVEL_LIVE))
			aLevel = MOD_LEVEL_NEXT_GAME;
		if ((aLevel == MOD_LEVEL_NEXT_GAME) && !theFilter)
			aLevel = MOD_LEVEL_RESTART;
		aDef.mLevel = aLevel;
		if ((aDef.mType == MOD_SETTING_TEXT) && aDef.mNoPercent && (aDef.mDefault.AsString().find('%') != std::string::npos))
		{
			mErrors.push_back("strings: setting \"" + aDef.mId + "\": text that goes into \"strings\" cannot contain %");
			mBadKeys.insert("strings");
		}
	}
	// A setting that another one depends on (enabledIf) changes that one's effective value: it applies no
	// earlier and counts for the scores when that one does. Later settings depend on earlier ones only.
	for (size_t i = mDefs.size(); i-- > 0;)
	{
		const ModSettingDef& aDef = mDefs[i];
		if (!aDef.IsValue() || aDef.mIfSetting.empty())
			continue;
		ModSettingDef* aCondition = FindForAnalysis(aDef.mIfSetting);
		aCondition->mLevel = std::max(aCondition->mLevel, aDef.mLevel);
		aCondition->mScores = aCondition->mScores || aDef.mScores;
	}
}

//////////////////////////////////////////////////////////////////////////
// Resolution

namespace
{
struct Resolver
{
	const ModSettings* mSettings;
	const ModSettingValues* mValues;
	std::string mError;

	bool Convert(const ModSettingDef& theDef, const JsonValue& theValue, const std::string& theAs, JsonValue* theOut)
	{
		if (theAs == "string")
			*theOut = JsonValue(ModSettingText(theDef, theValue));
		else if (theAs == "int")
		{
			// round half away from zero, within int32
			double aRounded = std::round(theValue.AsNumber());
			*theOut = JsonValue(std::max(-2147483648.0, std::min(2147483647.0, aRounded)));
		}
		else
		{
			const std::string& aColor = theValue.AsString();
			int r = HexByte(aColor, 1), g = HexByte(aColor, 3), b = HexByte(aColor, 5);
			int a = aColor.size() >= 9 ? HexByte(aColor, 7) : 255;
			if (theAs == "argb")
			{
				// one signed 32-bit integer (AsInt reads it without overflow)
				uint32_t anArgb = ((uint32_t) a << 24) | ((uint32_t) r << 16) | ((uint32_t) g << 8) | (uint32_t) b;
				*theOut = JsonValue((double) (int32_t) anArgb);
			}
			else
			{
				JsonValue anArray = JsonValue::MakeArray();
				anArray.Push(JsonValue((double) r));
				anArray.Push(JsonValue((double) g));
				anArray.Push(JsonValue((double) b));
				if (theAs == "rgba")
					anArray.Push(JsonValue((double) a));
				*theOut = anArray;
			}
		}
		return true;
	}

	// theValue at theDepth of theTop, resolved into *theOut; *theOmit when it is to be removed, *theFromPlaceholder
	// when a placeholder made it
	bool Node(const JsonValue& theValue, const std::string& theTop, int theDepth, int theNesting, JsonValue* theOut, bool* theOmit,
		bool* theFromPlaceholder)
	{
		*theOmit = false;
		*theFromPlaceholder = false;
		if (IsOmit(theValue))
		{
			*theOmit = true;
			return true;
		}
		if (IsPlaceholder(theValue))
		{
			*theFromPlaceholder = true;
			if (theNesting >= kMaxPlaceholderDepth)
			{
				mError = "placeholders are nested deeper than 8 levels";
				return false;
			}
			std::string anId = theValue["$setting"].AsString("");
			const ModSettingDef* aDef = mSettings->Find(anId);
			if (aDef == nullptr)
			{
				mError = "setting \"" + anId + "\" is not declared";
				return false;
			}
			JsonValue aValue = mSettings->EffectiveValue(*mValues, *aDef);
			if (theValue.Has("map"))
			{
				const JsonValue& aMap = theValue["map"];
				std::string aKey = ModSettingCanonical(*aDef, aValue);
				if (!aMap.Has(aKey) && !aMap.Has("*"))
				{
					mError = "\"" + anId + "\": the map has no entry for " + aKey + " and no \"*\"";
					return false;
				}
				bool isFromPlaceholder;
				if (!Node(aMap.Has(aKey) ? aMap[aKey] : aMap["*"], theTop, theDepth, theNesting + 1, theOut, theOmit, &isFromPlaceholder))
					return false;
				return true;
			}
			if (theValue.Has("as"))
				return Convert(*aDef, aValue, theValue["as"].AsString(""), theOut);
			*theOut = aValue;
			// strings are text: a number or a bool is written as it is shown
			if ((theTop == "strings") && (theDepth == 1) && (aValue.IsNumber() || aValue.IsBool()))
				*theOut = JsonValue(ModSettingText(*aDef, aValue));
			return true;
		}
		if (theValue.IsObject())
		{
			JsonValue anObject = JsonValue::MakeObject();
			bool isMemberRemovable = (theDepth == 0) && ((theTop == "files") || (theTop == "images") || (theTop == "strings"));
			for (const auto& kv : theValue.Members())
			{
				JsonValue aResult;
				bool isOmitted, isFromPlaceholder;
				if (!Node(kv.second, theTop, theDepth + 1, theNesting, &aResult, &isOmitted, &isFromPlaceholder))
					return false;
				if (isOmitted || (isMemberRemovable && isFromPlaceholder && aResult.IsNull()))
					continue;
				anObject.Set(kv.first, aResult);
			}
			*theOut = anObject;
			return true;
		}
		if (theValue.IsArray())
		{
			JsonValue anArray = JsonValue::MakeArray();
			bool isElementRemovable = (theDepth == 0) && ((theTop == "imagePatches") || (theTop == "imageOps"));
			for (const JsonValue& anItem : theValue.Items())
			{
				JsonValue aResult;
				bool isOmitted, isFromPlaceholder;
				if (!Node(anItem, theTop, theDepth + 1, theNesting, &aResult, &isOmitted, &isFromPlaceholder))
					return false;
				if (isOmitted || (isElementRemovable && isFromPlaceholder && aResult.IsNull()))
					continue;
				anArray.Push(aResult);
			}
			*theOut = anArray;
			return true;
		}
		*theOut = theValue;
		return true;
	}
};
}

void ModSettings::Resolve(const JsonValue& theData, const ModSettingValues& theValues, JsonValue* theEffective,
	std::vector<ModSettingsError>* theErrors) const
{
	if (!mPresent)
	{
		*theEffective = theData;
		return;
	}
	JsonValue anEffective = JsonValue::MakeObject();
	for (const auto& kv : theData.Members())
	{
		if (kv.first == "settings")
		{
			anEffective.Set(kv.first, kv.second);
			continue;
		}
		if (mBadKeys.count(kv.first))
		{
			// Analyze reported why
			std::string aText = "left out: a placeholder in it is wrong";
			for (const std::string& anError : mErrors)
				if (anError.compare(0, kv.first.size() + 2, kv.first + ": ") == 0)
				{
					aText = anError.substr(kv.first.size() + 2);
					break;
				}
			theErrors->push_back({kv.first, aText});
			continue;
		}
		if (!IsResolvedKey(kv.first))
		{
			// id, name, ..., and keys nobody reads: never resolved (Analyze leaves out those with placeholders)
			anEffective.Set(kv.first, kv.second);
			continue;
		}
		Resolver aResolver;
		aResolver.mSettings = this;
		aResolver.mValues = &theValues;
		JsonValue aResult;
		bool isOmitted, isFromPlaceholder;
		if (!aResolver.Node(kv.second, kv.first, 0, 0, &aResult, &isOmitted, &isFromPlaceholder))
		{
			theErrors->push_back({kv.first, aResolver.mError});
			continue;
		}
		if (!isOmitted)
			anEffective.Set(kv.first, aResult);
	}
	*theEffective = anEffective;
}

std::string ModSettings::ScoreKeyPairs(const ModSettingValues& theValues) const
{
	std::vector<const ModSettingDef*> aDefs;
	for (const ModSettingDef& aDef : mDefs)
		if (aDef.IsValue() && aDef.mScores)
			aDefs.push_back(&aDef);
	std::sort(aDefs.begin(), aDefs.end(), [](const ModSettingDef* a, const ModSettingDef* b) { return a->mId < b->mId; });
	std::string aPairs;
	for (const ModSettingDef* aDef : aDefs)
	{
		JsonValue aValue = EffectiveValue(theValues, *aDef);
		if (ModSettingSame(*aDef, aValue, aDef->mDefault))
			continue;
		aPairs += (aPairs.empty() ? "" : "&") + aDef->mId + "=" + ModSettingCanonical(*aDef, aValue);
	}
	return aPairs;
}

JsonValue ModSettingSchemaJson(const ModSettingDef& theDef)
{
	JsonValue aJson = JsonValue::MakeObject();
	if (!theDef.IsValue())
	{
		aJson.Set("type", JsonValue(std::string("section")));
		aJson.Set("label", JsonValue(theDef.mLabel));
		return aJson;
	}
	aJson.Set("id", JsonValue(theDef.mId));
	aJson.Set("label", JsonValue(theDef.mLabel));
	aJson.Set("type", JsonValue(std::string(ModSettingTypeName(theDef.mType))));
	if (!theDef.mHelp.empty())
		aJson.Set("help", JsonValue(theDef.mHelp));
	aJson.Set("default", theDef.mDefault);
	if (theDef.mType == MOD_SETTING_INT)
	{
		aJson.Set("min", JsonValue((double) theDef.mMin));
		aJson.Set("max", JsonValue((double) theDef.mMax));
		aJson.Set("step", JsonValue((double) theDef.mStep));
	}
	else if (theDef.mType == MOD_SETTING_FLOAT)
	{
		aJson.Set("min", JsonValue(FromUnits(theDef.mMin, theDef.mScale)));
		aJson.Set("max", JsonValue(FromUnits(theDef.mMax, theDef.mScale)));
		aJson.Set("step", JsonValue(FromUnits(theDef.mStep, theDef.mScale)));
		aJson.Set("decimals", JsonValue((double) theDef.mDecimals));
	}
	if (!theDef.mUnit.empty())
		aJson.Set("unit", JsonValue(theDef.mUnit));
	if (theDef.mType == MOD_SETTING_CHOICE)
	{
		JsonValue aChoices = JsonValue::MakeArray();
		for (const ModSettingChoice& aChoice : theDef.mChoices)
		{
			JsonValue anEntry = JsonValue::MakeObject();
			anEntry.Set("value", aChoice.mValue);
			anEntry.Set("label", JsonValue(aChoice.mLabel));
			aChoices.Push(anEntry);
		}
		aJson.Set("choices", aChoices);
	}
	if (theDef.mType == MOD_SETTING_TEXT)
	{
		aJson.Set("maxLength", JsonValue((double) theDef.mMaxLength));
		aJson.Set("charset", JsonValue(std::string(CharsetName(theDef.mCharset))));
		if (theDef.mNoPercent)
			aJson.Set("noPercent", JsonValue(true));
	}
	if (theDef.mType == MOD_SETTING_COLOR)
		aJson.Set("alpha", JsonValue(theDef.mAlpha));
	aJson.Set("level", JsonValue(std::string(ModSettingLevelName(theDef.mLevel))));
	aJson.Set("scores", JsonValue(theDef.mScores));
	if (!theDef.mIfSetting.empty())
	{
		JsonValue anIf = JsonValue::MakeObject();
		anIf.Set("setting", JsonValue(theDef.mIfSetting));
		anIf.Set(theDef.mIfEquals ? "equals" : "notEquals", JsonValue(theDef.mIfCanonical));
		aJson.Set("enabledIf", anIf);
	}
	return aJson;
}

}
