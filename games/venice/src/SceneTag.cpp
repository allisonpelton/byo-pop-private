// The engine's tag reader (0x4359d0 and the tag getters).

#include "SceneTag.h"

#include <cstdlib>
#include <cstring>

static bool IsBlank(char theChar) { return theChar == ' ' || theChar == '\n' || theChar == '\r' || theChar == '\t'; }

// 0x411ce0
void SceneTag::Reset()
{
	mAttributes.clear();
	mName.clear();
	mContent = nullptr;
	mContentLength = 0;
	mIsClosing = false;
	mIsSelfClosing = false;
}

// 0x409c40
bool SceneTag::Has(const char* theName) const
{
	for (const TagAttribute& anAttribute : mAttributes)
		if (anAttribute.mName == theName)
			return true;
	return false;
}

// 0x405790
const char* SceneTag::GetString(const char* theName) const
{
	for (const TagAttribute& anAttribute : mAttributes)
		if (anAttribute.mName == theName)
			return anAttribute.mValue.c_str();
	return nullptr;
}

// 0x409b50
int SceneTag::GetInt(const char* theName) const
{
	const char* aValue = GetString(theName);
	return aValue == nullptr ? 0 : (int)strtol(aValue, nullptr, 10);
}

// 0x409bc0
double SceneTag::GetFloat(const char* theName) const
{
	const char* aValue = GetString(theName);
	return aValue == nullptr ? 0.0 : atof(aValue);
}

// 0x4359d0
const char* ReadTag(const char* theText, SceneTag* theTag)
{
	theTag->Reset();
	const char* aText = theText;
	while (*aText != '<' && *aText != '\0')
		aText++;
	if (*aText == '\0')
		return aText;
	aText++;
	while (IsBlank(*aText))
		aText++;
	while (*aText != ' ' && *aText != '>' && *aText != '\n' && *aText != '\r' && *aText != '\t' && *aText != '\0')
		theTag->mName += *aText++;
	if (!theTag->mName.empty() && theTag->mName[0] == '/')
		theTag->mIsClosing = true;
	while (IsBlank(*aText))
		aText++;
	while (*aText != '/' && *aText != '>' && *aText != '\0') {
		TagAttribute anAttribute;
		while (*aText != ' ' && *aText != '=' && *aText != '\n' && *aText != '\r' && *aText != '\t' && *aText != '\0')
			anAttribute.mName += *aText++;
		while (IsBlank(*aText))
			aText++;
		if (*aText == '=')
			aText++;
		while (IsBlank(*aText))
			aText++;
		if (*aText == '"')
			aText++;
		while (*aText != '"' && *aText != '\0')
			anAttribute.mValue += *aText++;
		if (*aText == '"')
			aText++;
		while (IsBlank(*aText))
			aText++;
		theTag->mAttributes.push_back(anAttribute);
	}
	if (*aText == '/') {
		theTag->mIsSelfClosing = true;
		aText++;
	}
	if (*aText == '>')
		aText++;
	if (!theTag->mIsClosing && !theTag->mIsSelfClosing) {
		theTag->mContent = aText;
		theTag->mContentLength = 0;
		while (aText[theTag->mContentLength] != '<' && aText[theTag->mContentLength] != '\0')
			theTag->mContentLength++;
	}
	return aText;
}
