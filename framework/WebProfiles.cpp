// port: what a game asks the web page about the site's players, and tells it (framework/WebProfiles.h; docs/WEB_PLAYER.md,
// "Player profiles"; web/profiles.js). Web only; natively this file is empty.

#ifdef __EMSCRIPTEN__
#include "WebProfiles.h"
#include "EditWidget.h"
#include "Font.h"
#include "../platform/Json.h"

#include <emscripten.h>
#include <stdlib.h>

using namespace Sexy;

EM_JS_DEPS(webprofiles, "$UTF8ToString,$stringToNewUTF8");

// The page's answer for a game, as JSON text ("{}" without a page): the caller frees it
EM_JS(char*, byopop_site_json, (const char* theGameId), {
	var out = '{}';
	try {
		if ((typeof window !== 'undefined') && window.ByopopProfiles)
			out = JSON.stringify(window.ByopopProfiles.forGame(UTF8ToString(theGameId)));
	} catch (e) {}
	return stringToNewUTF8(out);
});

// The game's report of its players to the page
EM_JS(void, byopop_site_report, (const char* theGameId, const char* theJson), {
	try {
		if ((typeof window !== 'undefined') && window.ByopopProfiles)
			window.ByopopProfiles.report(UTF8ToString(theGameId), JSON.parse(UTF8ToString(theJson)));
	} catch (e) {}
});

namespace Byopop
{

namespace
{

bool sInStart = false;	// SiteStart is running: the changes it makes are reported once, at its end

// UTF-8 (the page's text) to the game's text, Latin-1: what the game cannot show is left out. A name from the page has no
// control characters, so those are dropped too.
std::string ToGame(const std::string& theText)
{
	std::string aResult;
	for (size_t i = 0; i < theText.size();)
	{
		unsigned char c = (unsigned char) theText[i];
		unsigned int aCode = 0;
		int aLength = 1;
		if (c < 0x80)
			aCode = c;
		else if ((c & 0xe0) == 0xc0)
		{
			aLength = 2;
			aCode = c & 0x1f;
		}
		else if ((c & 0xf0) == 0xe0)
		{
			aLength = 3;
			aCode = c & 0x0f;
		}
		else if ((c & 0xf8) == 0xf0)
		{
			aLength = 4;
			aCode = c & 0x07;
		}
		bool aValid = (aLength == 1) ? (c < 0x80) : (i + aLength <= theText.size());
		for (int j = 1; aValid && (j < aLength); j++)
		{
			unsigned char d = (unsigned char) theText[i + j];
			if ((d & 0xc0) != 0x80)
				aValid = false;
			else
				aCode = (aCode << 6) | (d & 0x3f);
		}
		i += aValid ? aLength : 1;
		if (aValid && (aCode >= 0x20) && (aCode <= 0xff) && (aCode != 0x7f))
			aResult += (char) aCode;
	}
	return aResult;
}

// The game's text, Latin-1, as UTF-8 for the page
std::string ToPage(const std::string& theText)
{
	std::string aResult;
	for (size_t i = 0; i < theText.size(); i++)
	{
		unsigned char c = (unsigned char) theText[i];
		if (c < 0x80)
			aResult += (char) c;
		else
		{
			aResult += (char) (0xc0 | (c >> 6));
			aResult += (char) (0x80 | (c & 0x3f));
		}
	}
	return aResult;
}

// One answer of the page, as the parse of its JSON
bool GetInfo(const char* theGameId, SiteInfo& theInfo)
{
	char* aText = byopop_site_json(theGameId);
	if (aText == NULL)
		return false;
	std::string aJson(aText);
	free(aText);

	Platform::JsonValue aRoot;
	if (!Platform::ParseJson(aJson, aRoot) || !aRoot.IsObject())
		return false;
	std::string aMode = aRoot["mode"].AsString("game");
	theInfo.mMode = (aMode == "create") ? SITE_CREATE : (aMode == "fill") ? SITE_FILL : SITE_GAME;
	theInfo.mName = ToGame(aRoot["name"].AsString(""));
	const Platform::JsonValue& aProfiles = aRoot["profiles"];
	for (size_t i = 0; aProfiles.IsArray() && (i < aProfiles.Size()); i++)
		theInfo.mProfiles.push_back(ToGame(aProfiles[i].AsString("")));
	const Platform::JsonValue& anOps = aRoot["ops"];
	for (size_t i = 0; anOps.IsArray() && (i < anOps.Size()); i++)
	{
		SiteOp anOp;
		anOp.mId = anOps[i]["id"].AsInt(0);
		anOp.mOp = anOps[i]["op"].AsString("");
		anOp.mFrom = ToGame(anOps[i]["from"].AsString(""));
		anOp.mTo = ToGame(anOps[i]["to"].AsString(""));
		anOp.mName = ToGame(anOps[i]["name"].AsString(""));
		if (anOp.mId > 0)
			theInfo.mOps.push_back(anOp);
	}
	return true;
}

Platform::JsonValue Result(const SiteOp& theOp, bool theDone, const std::string& theNote)
{
	Platform::JsonValue aResult = Platform::JsonValue::MakeObject();
	aResult.Set("id", Platform::JsonValue((double) theOp.mId));
	aResult.Set("status", Platform::JsonValue(std::string(theDone ? "done" : "skipped")));
	aResult.Set("note", Platform::JsonValue(ToPage(theNote)));
	return aResult;
}

// theOp against the game, with its own functions. A rename to a name the game already has is skipped: two players' progress
// is never merged.
Platform::JsonValue Apply(SiteProfiles& theGame, const SiteOp& theOp)
{
	if (theOp.mOp == "rename")
	{
		std::string aFrom = theGame.Find(theOp.mFrom);
		if (aFrom.empty())
			return Result(theOp, false, "The game has no player called \"" + theOp.mFrom + "\".");
		if (!theGame.CanRename())
			return Result(theOp, false, "This game cannot rename a player.");
		std::string aTo = theGame.Fit(theOp.mTo);
		if (aTo.empty())
			return Result(theOp, false, "The game\xe2\x80\x99s name rules leave nothing of \"" + theOp.mTo + "\".");
		std::string anExisting = theGame.Find(aTo);
		if (!anExisting.empty() && (anExisting != aFrom))
			return Result(theOp, false, "The game already has a player called \"" + anExisting + "\". Players are never merged.");
		if (aTo == aFrom)
			return Result(theOp, true, "Already called that.");
		std::string aWhy;
		if (!theGame.Rename(aFrom, aTo, &aWhy))
			return Result(theOp, false, aWhy.empty() ? std::string("The game would not rename the player.") : aWhy);
		return Result(theOp, true, (aTo != theOp.mTo) ? "Renamed to \"" + aTo + "\": the game\xe2\x80\x99s name rules changed the name." : "");
	}
	if (theOp.mOp == "delete")
	{
		std::string aName = theGame.Find(theOp.mName);
		if (aName.empty())
			return Result(theOp, false, "The game has no player called \"" + theOp.mName + "\".");
		if (!theGame.CanDelete())
			return Result(theOp, false, "This game cannot delete a player.");
		std::string aWhy;
		if (!theGame.Delete(aName, &aWhy))
			return Result(theOp, false, aWhy.empty() ? std::string("The game would not delete the player.") : aWhy);
		return Result(theOp, true, "");
	}
	return Result(theOp, false, "The game does not know this operation.");
}

// The report: the game's players, how each site player's name fits the game, and what became of the operations
void Report(const char* theGameId, SiteProfiles& theGame, const SiteInfo& theInfo, const Platform::JsonValue* theResults)
{
	Platform::JsonValue aReport = Platform::JsonValue::MakeObject();
	aReport.Set("kind", Platform::JsonValue(std::string(theGame.IsScores() ? "scores" : "profiles")));
	Platform::JsonValue aNames = Platform::JsonValue::MakeArray();
	std::vector<std::string> aList = theGame.Names();
	for (size_t i = 0; i < aList.size(); i++)
		aNames.Push(Platform::JsonValue(ToPage(aList[i])));
	aReport.Set("names", aNames);
	aReport.Set("current", Platform::JsonValue(ToPage(theGame.Current())));
	Platform::JsonValue aFits = Platform::JsonValue::MakeObject();
	for (size_t i = 0; i < theInfo.mProfiles.size(); i++)
		aFits.Set(ToPage(theInfo.mProfiles[i]), Platform::JsonValue(ToPage(theGame.Fit(theInfo.mProfiles[i]))));
	aReport.Set("fits", aFits);
	aReport.Set("rules", Platform::JsonValue(std::string(theGame.Rules())));
	aReport.Set("deletes", Platform::JsonValue(std::string(theGame.DeleteNote())));
	aReport.Set("max", Platform::JsonValue((double) theGame.MaxPlayers()));
	aReport.Set("canRename", Platform::JsonValue(theGame.CanRename()));
	aReport.Set("canDelete", Platform::JsonValue(theGame.CanDelete()));
	if (theResults != NULL)
		aReport.Set("results", *theResults);
	byopop_site_report(theGameId, aReport.Dump(0).c_str());
}

}

SiteInfo SiteGet(const char* theGameId)
{
	SiteInfo anInfo;
	if (!GetInfo(theGameId, anInfo))
		return SiteInfo();
	return anInfo;
}

void SiteStart(const char* theGameId, SiteProfiles& theGame)
{
	SiteInfo anInfo = SiteGet(theGameId);
	sInStart = true;

	Platform::JsonValue aResults = Platform::JsonValue::MakeArray();
	for (size_t i = 0; i < anInfo.mOps.size(); i++)
		aResults.Push(Apply(theGame, anInfo.mOps[i]));

	// "create": the game has the site's player and has chosen it, so it does not ask. Not for a game that has no players
	// (it only asks for a name at each high score): there the name is only filled in.
	if ((anInfo.mMode == SITE_CREATE) && !anInfo.mName.empty() && !theGame.IsScores())
	{
		std::string aName = theGame.Find(anInfo.mName);
		if (aName.empty())
		{
			std::string aFitted = theGame.Fit(anInfo.mName);
			if (!aFitted.empty())
			{
				aName = theGame.Find(aFitted);
				if (aName.empty())
				{
					std::string aWhy;
					if (theGame.Create(aFitted, &aWhy))
						aName = aFitted;
				}
			}
		}
		if (!aName.empty() && (theGame.Current() != aName))
			theGame.Select(aName);
	}

	sInStart = false;
	Report(theGameId, theGame, anInfo, &aResults);
}

void SiteChanged(const char* theGameId, SiteProfiles& theGame)
{
	if (sInStart)
		return;
	SiteInfo anInfo = SiteGet(theGameId);
	Report(theGameId, theGame, anInfo, NULL);
}

std::string SiteNameFor(const char* theGameId)
{
	SiteInfo anInfo = SiteGet(theGameId);
	if ((anInfo.mMode == SITE_GAME) || anInfo.mName.empty())
		return "";
	return anInfo.mName;
}

bool SiteType(const char* theGameId, EditWidget* theEdit, bool theLeftPosToZero)
{
	if (theEdit == NULL)
		return false;
	std::string aName = SiteNameFor(theGameId);
	if (aName.empty())
		return false;
	std::string aBefore = theEdit->mString;
	theEdit->SetText(_S(""), true);
	for (size_t i = 0; i < aName.size(); i++)
		theEdit->KeyChar((SexyChar) aName[i]);
	if (theEdit->mString.empty())
	{
		// nothing of the name can be typed in this box: it keeps what the game put in
		theEdit->SetText(aBefore, theLeftPosToZero);
		return false;
	}
	theEdit->SetText(theEdit->mString, theLeftPosToZero);
	return true;
}

std::string SiteFitTyped(const std::string& theName, Font* theFont, int theMaxChars, int theMaxPixels, EditListener* theListener)
{
	EditListener aDefault;
	EditListener* aListener = (theListener != NULL) ? theListener : &aDefault;
	if (theFont == NULL)
	{
		// no font yet: the characters the box takes and the count
		std::string aResult;
		for (size_t i = 0; i < theName.size(); i++)
		{
			unsigned char c = (unsigned char) theName[i];
			if ((c >= 32) && (c <= 127) && aListener->AllowChar(0, (SexyChar) c))
				aResult += (char) c;
			if ((theMaxChars != -1) && ((int) aResult.size() > theMaxChars))
				aResult.resize(theMaxChars);
		}
		return aResult;
	}
	EditWidget anEdit(0, aListener);
	anEdit.SetFont(theFont, NULL);
	anEdit.mMaxChars = theMaxChars;
	anEdit.mMaxPixels = theMaxPixels;
	anEdit.SetText(_S(""), true);
	for (size_t i = 0; i < theName.size(); i++)
		anEdit.KeyChar((SexyChar) theName[i]);
	return anEdit.mString;
}

}
#endif
