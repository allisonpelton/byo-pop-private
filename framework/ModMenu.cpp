// port: the mod menu (see ModMenu.h and docs/MOD_MENU_DESIGN.md 5)

#include "ModMenu.h"
#include "BuiltinFont.h"
#include "SexyAppBase.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "MTRand.h"
#include "../platform/ModManager.h"
#include "../platform/Text.h"

#include <SDL.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

using namespace Sexy;
using Platform::JsonValue;
using Platform::ModManager;
using Platform::ModSettingDef;

#ifdef __EMSCRIPTEN__
// The web page's live region for screen readers (Module.onModMenuFocus): what the focus is on
EM_JS(void, modmenu_announce, (const char* theText), {
	if (typeof Module['onModMenuFocus'] === 'function')
		Module['onModMenuFocus'](UTF8ToString(theText));
});

// The page's Mods panel (web/modpanel.js), which adds and removes mods: the list page offers it when the page has
// one. Opened after this event, not inside it: the panel reads the mods model as it opens.
EM_JS(int, modmenu_has_panel, (), {
	return (typeof Module['onModPanel'] === 'function') ? 1 : 0;
});

EM_JS(void, modmenu_open_panel, (), {
	setTimeout(function() {
		if (typeof Module['onModPanel'] === 'function')
			Module['onModPanel']();
	}, 0);
});
#endif

namespace
{

// The look: one for every game (docs/MOD_MENU_DESIGN.md 5.3), drawn with rectangles and the built-in font
const Color kBackdrop(0, 0, 0, 160);
const Color kPanel(0x1e, 0x22, 0x30);
const Color kBorder(0x8a, 0x93, 0xb8);
const Color kText(0xff, 0xff, 0xff);
const Color kDim(0xb7, 0xbd, 0xd2);
const Color kDisabled(0x6c, 0x72, 0x88);
const Color kFocus(0xff, 0xd3, 0x4d);
const Color kError(0xff, 0x6b, 0x6b);
const Color kWarn(0xff, 0xb3, 0x47);
const Color kScores(0x8c, 0xc8, 0xff);
const Color kFeatures(0x9b, 0xe3, 0x9b);
const Color kButton(0x2c, 0x32, 0x46);
const Color kButtonHover(0x3a, 0x42, 0x5c);
const Color kButtonDown(0x4a, 0x55, 0x78);
const Color kButtonBorder(0x5a, 0x63, 0x80);
const Color kField(0x14, 0x17, 0x22);
const Color kRowFocus(0x26, 0x2b, 0x3c);
const Color kTrack(0x4a, 0x50, 0x70);
const Color kThumb(0xdf, 0xe3, 0xf0);

const int PANEL_WIDTH = 560;
const int PANEL_HEIGHT = 432;
const int HEADER_HEIGHT = 34;
const int FOOTER_HEIGHT = 80;
const int LIST_ROW = 44;
const int SETTING_ROW = 30;
const int SECTION_ROW = 24;
const int LINE = 14;
const int SCROLLBAR = 12;
const int BUTTON_HEIGHT = 24;
const int LABEL_WIDTH = 230;
const int PENDING_WIDTH = 84;
const int DRAG_DISTANCE = 6;

enum
{
	ITEM_BANNER,
	ITEM_EMPTY,
	ITEM_MOD,
	ITEM_TEXT,
	ITEM_SECTION,
	ITEM_SETTING
};

enum
{
	CONFIRM_NONE,
	CONFIRM_RESTART,
	CONFIRM_RESET
};

enum
{
	TEXT_NORMAL,
	TEXT_DIM,
	TEXT_ERROR,
	TEXT_WARN,
	TEXT_ACCENT
};

BuiltinFont* gFont = NULL;
BuiltinFont* gBold = NULL;

const Color& TextColor(int theColor)
{
	switch (theColor)
	{
	case TEXT_DIM: return kDim;
	case TEXT_ERROR: return kError;
	case TEXT_WARN: return kWarn;
	case TEXT_ACCENT: return kFocus;
	default: return kText;
	}
}

// Mod texts are UTF-8; the menu draws Latin-1 (other characters become '?')
std::string L1(const std::string& theText)
{
	return Platform::Utf8ToLatin1(theText, '?');
}

std::string Str(SexyAppBase* theApp, const char* theId, const char* theDefault)
{
	return SexyStringToString(theApp->GetString(theId, theDefault));
}

std::string Replace(std::string theText, const std::string& theKey, const std::string& theValue)
{
	size_t aPos = theText.find(theKey);
	if (aPos != std::string::npos)
		theText.replace(aPos, theKey.size(), theValue);
	return theText;
}

std::string Fit(Font* theFont, const std::string& theText, int theWidth)
{
	if (theFont->StringWidth(theText) <= theWidth)
		return theText;
	std::string aText = theText;
	int aDots = theFont->StringWidth("...");
	while (!aText.empty() && (theFont->StringWidth(aText) + aDots > theWidth))
		aText.erase(aText.size() - 1);
	while (!aText.empty() && (aText[aText.size() - 1] == ' '))
		aText.erase(aText.size() - 1);
	return aText + "...";
}

// theText in lines of at most theWidth, at most theMaxLines of them (the last cut with "..." if it goes on)
std::vector<std::string> Wrap(Font* theFont, const std::string& theText, int theWidth, int theMaxLines)
{
	std::vector<std::string> aLines;
	std::string aLine;
	size_t i = 0;
	while (i < theText.size())
	{
		size_t anEnd = theText.find(' ', i);
		if (anEnd == std::string::npos)
			anEnd = theText.size();
		std::string aWord = theText.substr(i, anEnd - i);
		std::string aTry = aLine.empty() ? aWord : aLine + " " + aWord;
		if ((theFont->StringWidth(aTry) <= theWidth) || aLine.empty())
			aLine = aTry;
		else
		{
			aLines.push_back(aLine);
			aLine = aWord;
		}
		i = anEnd + 1;
	}
	if (!aLine.empty())
		aLines.push_back(aLine);
	for (std::string& aText : aLines)
		aText = Fit(theFont, aText, theWidth);
	if ((int) aLines.size() > theMaxLines)
	{
		std::string aRest;
		for (size_t j = (size_t) theMaxLines - 1; j < aLines.size(); j++)
			aRest += (aRest.empty() ? "" : " ") + aLines[j];
		aLines.resize((size_t) theMaxLines);
		std::string aLast = Fit(theFont, aRest, theWidth);
		if (aLast == aRest)
			aLast = Fit(theFont, aRest + "...", theWidth);
		aLines.back() = aLast;
	}
	return aLines;
}

void Outline(Graphics* g, const Rect& theRect, const Color& theColor, int theThickness = 1)
{
	g->SetColor(theColor);
	g->FillRect(theRect.mX, theRect.mY, theRect.mWidth, theThickness);
	g->FillRect(theRect.mX, theRect.mY + theRect.mHeight - theThickness, theRect.mWidth, theThickness);
	g->FillRect(theRect.mX, theRect.mY + theThickness, theThickness, theRect.mHeight - 2 * theThickness);
	g->FillRect(theRect.mX + theRect.mWidth - theThickness, theRect.mY + theThickness, theThickness, theRect.mHeight - 2 * theThickness);
}

void Fill(Graphics* g, const Rect& theRect, const Color& theColor)
{
	g->SetColor(theColor);
	g->FillRect(theRect);
}

void Text(Graphics* g, Font* theFont, const std::string& theText, int x, int theBaseline, const Color& theColor)
{
	g->SetFont(theFont);
	g->SetColor(theColor);
	g->DrawString(theText, x, theBaseline);
}

// The baseline that centres capitals (8 pixels) in a box
int CenterBaseline(const Rect& theRect)
{
	return theRect.mY + (theRect.mHeight + 8) / 2;
}

// Triangles for the arrow buttons: theDirection 0 up, 1 down, 2 left, 3 right, centred on x, y
void Arrow(Graphics* g, int x, int y, int theDirection, const Color& theColor)
{
	g->SetColor(theColor);
	for (int i = 0; i < 5; i++)
	{
		switch (theDirection)
		{
		case 0: g->FillRect(x - i, y - 2 + i, 2 * i + 1, 1); break;
		case 1: g->FillRect(x - i, y + 2 - i, 2 * i + 1, 1); break;
		case 2: g->FillRect(x - 2 + i, y - i, 1, 2 * i + 1); break;
		default: g->FillRect(x + 2 - i, y - i, 1, 2 * i + 1); break;
		}
	}
}

void CheckMark(Graphics* g, const Rect& theBox, const Color& theColor)
{
	static const int aPoints[][2] = { {3, 7}, {4, 8}, {5, 9}, {6, 8}, {7, 7}, {8, 6}, {9, 5}, {10, 4}, {11, 3} };
	g->SetColor(theColor);
	for (const auto& aPoint : aPoints)
		g->FillRect(theBox.mX + aPoint[0], theBox.mY + aPoint[1], 2, 2);
}

void Checkbox(Graphics* g, const Rect& theBox, bool isChecked, bool isDisabled)
{
	Fill(g, theBox, kField);
	Outline(g, theBox, isDisabled ? kDisabled : kDim);
	if (isChecked)
		CheckMark(g, theBox, isDisabled ? kDisabled : kText);
}

bool ParseColor(const std::string& theText, Color* theColor)
{
	if (((theText.size() != 7) && (theText.size() != 9)) || (theText[0] != '#'))
		return false;
	unsigned long aValue = strtoul(theText.substr(1, 6).c_str(), NULL, 16);
	*theColor = Color((int) ((aValue >> 16) & 0xFF), (int) ((aValue >> 8) & 0xFF), (int) (aValue & 0xFF));
	return true;
}

// Words of a script line; "..." is one word (without the quotes). theEnds[i]: where word i ends in theText.
std::vector<std::string> SplitWords(const std::string& theText, std::vector<size_t>* theEnds)
{
	std::vector<std::string> aWords;
	size_t i = 0;
	while (i < theText.size())
	{
		while ((i < theText.size()) && ((theText[i] == ' ') || (theText[i] == '\t')))
			i++;
		if (i >= theText.size())
			break;
		std::string aWord;
		if (theText[i] == '"')
		{
			size_t anEnd = theText.find('"', i + 1);
			if (anEnd == std::string::npos)
				anEnd = theText.size();
			aWord = theText.substr(i + 1, anEnd - i - 1);
			i = (anEnd < theText.size()) ? anEnd + 1 : anEnd;
		}
		else
		{
			size_t anEnd = theText.find_first_of(" \t", i);
			if (anEnd == std::string::npos)
				anEnd = theText.size();
			aWord = theText.substr(i, anEnd - i);
			i = anEnd;
		}
		aWords.push_back(aWord);
		if (theEnds != NULL)
			theEnds->push_back(i);
	}
	return aWords;
}

}

//////////////////////////////////////////////////////////////////////////

ModMenu::ModMenu(SexyAppBase* theApp)
{
	mApp = theApp;
	mZOrder = 1 << 30;  // above every widget a game adds while it is open
	mWantsFocus = true;
	mHasTransparencies = true;

	// The fonts are made when the menu first opens, and kept
	if (gFont == NULL)
		gFont = new BuiltinFont(false);
	if (gBold == NULL)
		gBold = new BuiltinFont(true);
	mFont = gFont;
	mBold = gBold;

	mSerial = 0;
	mHasModel = false;
	mNoMods = false;
	mScroll = 0;
	mLayoutDirty = true;
	mFocusKind = T_NONE;
	mFocusItem = -1;
	mHoverKind = T_NONE;
	mHoverItem = -1;
	mPressKind = T_NONE;
	mPressItem = -1;
	mPressX = 0;
	mPressY = 0;
	mPressScroll = 0;
	mPressInContent = false;
	mDragScroll = false;
	mDragThumb = false;
	mDragSlider = -1;
	mEditItem = -1;
	mEditCaret = 0;
	mConfirm = CONFIRM_NONE;
	mRestartAction = 0;
	mOldCursor = theApp->GetCursor();
#ifdef __EMSCRIPTEN__
	mHasModPanel = modmenu_has_panel() != 0;
#else
	mHasModPanel = false;
#endif

	int aWidth = std::min(PANEL_WIDTH, theApp->mWidth - 16);
	int aHeight = std::min(PANEL_HEIGHT, theApp->mHeight - 16);
	mPanel = Rect((theApp->mWidth - aWidth) / 2, (theApp->mHeight - aHeight) / 2, aWidth, aHeight);

	// What the files say now: they may have been edited by hand, by the web page or by another copy of the game
	ModManager::Get().ScanDesired();
	Refresh();
	ShowList();
}

ModMenu::~ModMenu()
{
}

void ModMenu::FreeResources()
{
	delete gFont;
	gFont = NULL;
	delete gBold;
	gBold = NULL;
	BuiltinFont::FreeImage();
}

bool ModMenu::WantsFocus()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////
// The model

void ModMenu::Refresh()
{
	ModManager& aManager = ModManager::Get();
	JsonValue aState;
	Platform::ParseJson(aManager.GetStateJson(), aState);
	const std::vector<Platform::ModInfo>& aDesired = aManager.GetDesiredMods();
	mSerial = aManager.GetChangeSerial();
	mHasModel = true;

	mNoMods = aState["nomods"].AsBool(false);
	mSettingsFile = aState["settingsFile"].AsString("missing");
	mScoreKey = aState["scoreKey"].AsString("");
	mNextScoreKey = aState["nextScoreKey"].AsString("");
	mPendingRestart.clear();
	mPendingNextGame.clear();
	for (const JsonValue& aText : aState["pending"]["restart"].Items())
		mPendingRestart.push_back(L1(aText.AsString("")));
	for (const JsonValue& aText : aState["pending"]["nextGame"].Items())
		mPendingNextGame.push_back(L1(aText.AsString("")));

	std::vector<ModMessage> aMessages;
	mApp->GetModMessages(&aMessages);
	mGameMessages.clear();
	for (const ModMessage& aMessage : aMessages)
		if (aMessage.mModId.empty())
			mGameMessages.push_back(L1(aMessage.mText));

	mMods.clear();
	const std::vector<JsonValue>& aMods = aState["mods"].Items();
	for (size_t i = 0; (i < aMods.size()) && (i < aDesired.size()); i++)
	{
		const JsonValue& aJson = aMods[i];
		const Platform::ModInfo& anInfo = aDesired[i];
		ModView aView;
		aView.mId = aJson["id"].AsString("");
		aView.mName = L1(aJson["name"].AsString(""));
		aView.mVersion = L1(aJson["version"].AsString(""));
		aView.mAuthor = L1(aJson["author"].AsString(""));
		aView.mDescription = L1(aJson["description"].AsString(""));
		aView.mEnabled = aJson["enabled"].AsBool(true);
		aView.mScores = aJson["scores"].AsBool(false);
		aView.mFeatures = aJson["features"].AsBool(false);
		aView.mHasSettings = aJson["hasSettings"].AsBool(false);
		aView.mDuplicate = anInfo.mDuplicate;
		aView.mCanUp = !anInfo.mDuplicate && aManager.CanMove(aView.mId, -1);
		aView.mCanDown = !anInfo.mDuplicate && aManager.CanMove(aView.mId, 1);
		aView.mPending = aJson["pending"].AsString("");
		for (const JsonValue& aText : aJson["errors"].Items())
			aView.mErrors.push_back(L1(aText.AsString("")));
		for (const JsonValue& aText : aJson["warnings"].Items())
			aView.mWarnings.push_back(L1(aText.AsString("")));
		for (const ModMessage& aMessage : aMessages)
		{
			if (!aMessage.mModId.empty() && (aMessage.mModId == aView.mId))
				(aMessage.mIsError ? aView.mErrors : aView.mWarnings).push_back(L1(aMessage.mText));
		}

		const std::vector<JsonValue>& aSettings = aJson["settings"].Items();
		for (size_t j = 0; (j < aSettings.size()) && (j < anInfo.mSettings.mDefs.size()); j++)
		{
			SettingView aSetting;
			aSetting.mDef = anInfo.mSettings.mDefs[j];
			aSetting.mValue = aSettings[j]["value"];
			aSetting.mLabel = L1(aSetting.mDef.mLabel);
			aSetting.mHelp = L1(aSetting.mDef.mHelp);
			aSetting.mEnabled = aSettings[j]["enabled"].AsBool(true);
			aSetting.mPending = aSettings[j]["pending"].AsString("");
			aView.mSettings.push_back(aSetting);
		}
		mMods.push_back(aView);
	}

	if (!mPageMod.empty() && (PageModView() == NULL))
		ShowList();
	if (mEditItem >= 0)
	{
		SettingView* aSetting = PageSetting(mEditItem);
		if ((aSetting == NULL) || !aSetting->mDef.IsValue())
			mEditItem = -1;
	}
	mLayoutDirty = true;
	MarkDirty();
}

ModMenu::ModView* ModMenu::PageModView()
{
	if (mPageMod.empty())
		return NULL;
	for (ModView& aView : mMods)
		if ((aView.mId == mPageMod) && !aView.mDuplicate)
			return &aView;
	return NULL;
}

ModMenu::SettingView* ModMenu::PageSetting(int theItem)
{
	ModView* aView = PageModView();
	if ((aView == NULL) || (theItem < 0) || (theItem >= (int) aView->mSettings.size()))
		return NULL;
	return &aView->mSettings[(size_t) theItem];
}

//////////////////////////////////////////////////////////////////////////
// Pages and layout

void ModMenu::ShowList()
{
	CommitEdit();
	std::string aFrom = mPageMod;
	mPageMod.clear();
	mScroll = 0;
	mConfirm = CONFIRM_NONE;
	mNote.clear();
	Layout();
	mFocusKind = T_NONE;
	for (size_t i = 0; i < mMods.size(); i++)
		if (mMods[i].mId == aFrom)
			SetFocusTarget(T_ROW, (int) i);
	if (mFocusKind == T_NONE)
		SetFocusTarget(mMods.empty() ? (mHasModPanel ? T_FILES : T_CLOSE) : T_ROW, mMods.empty() ? -1 : 0);
	MarkDirty();
}

bool ModMenu::ShowMod(const std::string& theModId)
{
	CommitEdit();
	for (const ModView& aView : mMods)
	{
		if ((aView.mId != theModId) || aView.mDuplicate)
			continue;
		mPageMod = theModId;
		mScroll = 0;
		mConfirm = CONFIRM_NONE;
		mNote.clear();
		Layout();
		mFocusKind = T_NONE;
		for (size_t i = 0; i < aView.mSettings.size(); i++)
		{
			if (aView.mSettings[i].mDef.IsValue())
			{
				SetFocusTarget(T_VALUE, (int) i);
				break;
			}
		}
		if (mFocusKind == T_NONE)
			SetFocusTarget(T_BACK, -1);
		MarkDirty();
		return true;
	}
	return false;
}

Rect ModMenu::ContentRect() const
{
	return Rect(mPanel.mX + 8, mPanel.mY + HEADER_HEIGHT, mPanel.mWidth - 16, mPanel.mHeight - HEADER_HEIGHT - FOOTER_HEIGHT);
}

int ModMenu::ContentHeight() const
{
	return mItems.empty() ? 0 : mItems.back().mY + mItems.back().mHeight;
}

void ModMenu::AddTarget(int theKind, int theItem, const Rect& theRect, bool isDisabled, bool isInContent)
{
	Target aTarget;
	aTarget.mKind = theKind;
	aTarget.mItem = theItem;
	aTarget.mRect = theRect;
	aTarget.mDisabled = isDisabled;
	aTarget.mInContent = isInContent;
	mTargets.push_back(aTarget);
}

void ModMenu::Layout()
{
	mLayoutDirty = false;
	Rect aContent = ContentRect();
	bool isReadOnly = (mSettingsFile == "newer");

	for (int aPass = 0; aPass < 2; aPass++)
	{
		int aWidth = aContent.mWidth - ((aPass == 1) ? SCROLLBAR + 4 : 0);
		int aTextWidth = aWidth - 16;
		mItems.clear();
		int y = 0;
		auto addText = [&](int theKind, int theIndex, const std::string& theText, int theColor, int theMaxLines) {
			Item anItem;
			anItem.mKind = theKind;
			anItem.mIndex = theIndex;
			anItem.mY = y;
			anItem.mLines = Wrap(mFont, theText, aTextWidth, theMaxLines);
			anItem.mHeight = (int) anItem.mLines.size() * LINE + 6;
			anItem.mColor = theColor;
			y += anItem.mHeight;
			mItems.push_back(anItem);
		};
		auto addRow = [&](int theKind, int theIndex, int theHeight) {
			Item anItem;
			anItem.mKind = theKind;
			anItem.mIndex = theIndex;
			anItem.mY = y;
			anItem.mHeight = theHeight;
			anItem.mColor = TEXT_NORMAL;
			y += theHeight;
			mItems.push_back(anItem);
		};

		if (mPageMod.empty())
		{
			if (mNoMods)
				addText(ITEM_BANNER, -1, Str(mApp, "MODMENU_NOMODS", "Mods are off for this run (-nomods). Restart game turns them on."), TEXT_ACCENT, 2);
			if (mSettingsFile == "unreadable")
				addText(ITEM_BANNER, -1, Str(mApp, "MODMENU_SETTINGS_UNREADABLE", "mod-settings.json could not be read: every value is at its default."), TEXT_WARN, 2);
			else if (isReadOnly)
				addText(ITEM_BANNER, -1, Str(mApp, "MODMENU_SETTINGS_NEWER", "mod-settings.json was made by a newer version of the game: values cannot be changed here."), TEXT_WARN, 2);
			for (const std::string& aMessage : mGameMessages)
				addText(ITEM_BANNER, -1, "! " + aMessage, TEXT_WARN, 2);
			if (mMods.empty())
			{
#ifdef __EMSCRIPTEN__
				addText(ITEM_EMPTY, -1, mHasModPanel ? Str(mApp, "MODMENU_EMPTY_WEB", "No mods installed. Add some with \"Add or remove mods\" below.") :
					Str(mApp, "MODMENU_EMPTY_NOPANEL", "No mods installed."), TEXT_DIM, 3);
#else
				addText(ITEM_EMPTY, -1, Str(mApp, "MODMENU_EMPTY", "No mods installed. Mods are folders in") + " " +
					L1(ModManager::Get().GetModsDir()) + ".", TEXT_DIM, 3);
#endif
			}
			for (size_t i = 0; i < mMods.size(); i++)
				addRow(ITEM_MOD, (int) i, LIST_ROW);
		}
		else if (ModView* aView = PageModView())
		{
			if (!aView->mDescription.empty())
				addText(ITEM_TEXT, -1, aView->mDescription, TEXT_DIM, 3);
			for (const std::string& anError : aView->mErrors)
				addText(ITEM_TEXT, -1, "! " + anError, TEXT_ERROR, 2);
			for (const std::string& aWarning : aView->mWarnings)
				addText(ITEM_TEXT, -1, "! " + aWarning, TEXT_WARN, 2);
			if (isReadOnly)
				addText(ITEM_TEXT, -1, Str(mApp, "MODMENU_SETTINGS_NEWER", "mod-settings.json was made by a newer version of the game: values cannot be changed here."), TEXT_WARN, 2);
			bool hasValues = false;
			for (size_t i = 0; i < aView->mSettings.size(); i++)
			{
				if (aView->mSettings[i].mDef.IsValue())
				{
					addRow(ITEM_SETTING, (int) i, SETTING_ROW);
					hasValues = true;
				}
				else
					addRow(ITEM_SECTION, (int) i, SECTION_ROW);
			}
			if (!hasValues)
				addText(ITEM_TEXT, -1, Str(mApp, "MODMENU_NO_SETTINGS", "This mod has no settings."), TEXT_DIM, 1);
		}
		if ((aPass == 0) && (y <= aContent.mHeight))
			break;
	}

	int aMaxScroll = std::max(0, ContentHeight() - aContent.mHeight);
	mScroll = std::max(0, std::min(mScroll, aMaxScroll));
	bool hasScrollbar = ContentHeight() > aContent.mHeight;
	int aRowWidth = aContent.mWidth - (hasScrollbar ? SCROLLBAR + 4 : 0);

	// The targets: the header, the content's (in order), the footer's. Tab follows this order.
	mTargets.clear();
	if (!mPageMod.empty())
		AddTarget(T_BACK, -2, Rect(mPanel.mX + 8, mPanel.mY + 6, 64, 22), false, false);

	for (const Item& anItem : mItems)
	{
		Rect aRow(aContent.mX, aContent.mY + anItem.mY - mScroll, aRowWidth, anItem.mHeight);
		if (anItem.mKind == ITEM_MOD)
		{
			const ModView& aView = mMods[(size_t) anItem.mIndex];
			int anOpenX = aRow.mX + aRow.mWidth - 8 - 72;
			int aDownX = anOpenX - 8 - 24;
			int anUpX = aDownX - 4 - 24;
			int aButtonY = aRow.mY + (aRow.mHeight - BUTTON_HEIGHT) / 2;
			AddTarget(T_ROW, anItem.mIndex, Rect(aRow.mX, aRow.mY, anUpX - 8 - aRow.mX, aRow.mHeight), aView.mDuplicate, true);
			AddTarget(T_UP, anItem.mIndex, Rect(anUpX, aButtonY, 24, BUTTON_HEIGHT), !aView.mCanUp, true);
			AddTarget(T_DOWN, anItem.mIndex, Rect(aDownX, aButtonY, 24, BUTTON_HEIGHT), !aView.mCanDown, true);
			if (aView.mHasSettings)
				AddTarget(T_OPEN, anItem.mIndex, Rect(anOpenX, aButtonY, 72, BUTTON_HEIGHT), aView.mDuplicate, true);
		}
		else if (anItem.mKind == ITEM_SETTING)
		{
			const SettingView* aSetting = PageSetting(anItem.mIndex);
			if (aSetting == NULL)
				continue;
			bool isDisabled = isReadOnly || !aSetting->mEnabled;
			int x = aRow.mX + LABEL_WIDTH + 4;
			int aWidth = aRow.mX + aRow.mWidth - PENDING_WIDTH - x;
			int y = aRow.mY + (aRow.mHeight - BUTTON_HEIGHT) / 2;
			int i = anItem.mIndex;
			AddTarget(T_VALUE, i, Rect(x - 3, aRow.mY + 1, aWidth + 6, aRow.mHeight - 2), isDisabled, true);
			AddTarget(T_LABEL, i, Rect(aRow.mX, aRow.mY, LABEL_WIDTH, aRow.mHeight), isDisabled, true);
			switch (aSetting->mDef.mType)
			{
			case Platform::MOD_SETTING_INT:
			case Platform::MOD_SETTING_FLOAT:
				AddTarget(T_MINUS, i, Rect(x, y, 24, BUTTON_HEIGHT), isDisabled, true);
				AddTarget(T_PLUS, i, Rect(x + 92, y, 24, BUTTON_HEIGHT), isDisabled, true);
				if ((aSetting->mDef.mType == Platform::MOD_SETTING_FLOAT) || (Platform::ModSettingStepCount(aSetting->mDef) <= 200))
					AddTarget(T_SLIDER, i, Rect(x + 124, y, aWidth - 124, BUTTON_HEIGHT), isDisabled, true);
				break;
			case Platform::MOD_SETTING_CHOICE:
				AddTarget(T_PREV, i, Rect(x, y, 24, BUTTON_HEIGHT), isDisabled, true);
				AddTarget(T_NEXT, i, Rect(x + aWidth - 24, y, 24, BUTTON_HEIGHT), isDisabled, true);
				break;
			case Platform::MOD_SETTING_TEXT:
				AddTarget(T_FIELD, i, Rect(x, y, aWidth, BUTTON_HEIGHT), isDisabled, true);
				break;
			case Platform::MOD_SETTING_COLOR:
				AddTarget(T_FIELD, i, Rect(x + 30, y, 84, BUTTON_HEIGHT), isDisabled, true);
				AddTarget(T_PREV, i, Rect(x + 120, y, 24, BUTTON_HEIGHT), isDisabled, true);
				AddTarget(T_NEXT, i, Rect(x + 148, y, 24, BUTTON_HEIGHT), isDisabled, true);
				break;
			}
		}
	}
	if (hasScrollbar)
		AddTarget(T_SCROLL, -1, Rect(aContent.mX + aContent.mWidth - SCROLLBAR, aContent.mY, SCROLLBAR, aContent.mHeight), false, false);

	// The footer's buttons, right to left
	int aButtonY = mPanel.mY + mPanel.mHeight - 8 - BUTTON_HEIGHT;
	int aRight = mPanel.mX + mPanel.mWidth - 8;
	std::vector<std::pair<int, std::string> > aButtons;
	if (mConfirm == CONFIRM_RESTART)
	{
		aButtons.push_back(std::make_pair((int) T_YES, Str(mApp, "MODMENU_RESTART_YES", "Restart")));
		aButtons.push_back(std::make_pair((int) T_NO, Str(mApp, "MODMENU_CANCEL", "Cancel")));
	}
	else if (mConfirm == CONFIRM_RESET)
	{
		aButtons.push_back(std::make_pair((int) T_YES, Str(mApp, "MODMENU_RESET_YES", "Reset")));
		aButtons.push_back(std::make_pair((int) T_NO, Str(mApp, "MODMENU_CANCEL", "Cancel")));
	}
	else
	{
		aButtons.push_back(std::make_pair((int) T_RESTART, Str(mApp, "MODMENU_RESTART", "Restart game")));
		aButtons.push_back(std::make_pair(mPageMod.empty() ? (int) T_CLOSE : (int) T_BACK,
			mPageMod.empty() ? Str(mApp, "MODMENU_CLOSE", "Close") : Str(mApp, "MODMENU_BACK_BUTTON", "Back")));
	}
	std::vector<Target> aFooter;
	for (size_t i = aButtons.size(); i-- > 0;)
	{
		int aWidth = std::max(72, mBold->StringWidth(aButtons[i].second) + 20);
		aRight -= aWidth;
		Target aTarget;
		aTarget.mKind = aButtons[i].first;
		aTarget.mItem = -1;
		aTarget.mRect = Rect(aRight, aButtonY, aWidth, BUTTON_HEIGHT);
		aTarget.mDisabled = false;
		aTarget.mInContent = false;
		aFooter.insert(aFooter.begin(), aTarget);
		aRight -= 8;
	}
	if (!mPageMod.empty() && (mConfirm == CONFIRM_NONE))
	{
		const ModView* aView = PageModView();
		bool hasValues = false;
		if (aView != NULL)
			for (const SettingView& aSetting : aView->mSettings)
				hasValues = hasValues || aSetting.mDef.IsValue();
		std::string aLabel = Str(mApp, "MODMENU_RESET", "Reset to defaults");
		AddTarget(T_RESET, -1, Rect(mPanel.mX + 8, aButtonY, std::max(72, mBold->StringWidth(aLabel) + 20), BUTTON_HEIGHT),
			!hasValues || isReadOnly, false);
	}
	if (mPageMod.empty() && (mConfirm == CONFIRM_NONE) && mHasModPanel)
	{
		std::string aLabel = Str(mApp, "MODMENU_ADD_REMOVE", "Add or remove mods");
		AddTarget(T_FILES, -1, Rect(mPanel.mX + 8, aButtonY, std::max(72, mBold->StringWidth(aLabel) + 20), BUTTON_HEIGHT), false, false);
	}
	for (const Target& aTarget : aFooter)
		mTargets.push_back(aTarget);
}

const ModMenu::Target* ModMenu::FindTarget(int theKind, int theItem) const
{
	for (const Target& aTarget : mTargets)
		if ((aTarget.mKind == theKind) && (aTarget.mItem == theItem))
			return &aTarget;
	return NULL;
}

const ModMenu::Target* ModMenu::TargetAt(int x, int y) const
{
	Rect aContent = ContentRect();
	const Target* aFound = NULL;
	for (const Target& aTarget : mTargets)
	{
		if (!aTarget.mRect.Contains(x, y))
			continue;
		if (aTarget.mInContent && !aContent.Contains(x, y))
			continue;
		// a control's parts win over the whole control
		if ((aFound == NULL) || (aFound->mKind == T_VALUE))
			aFound = &aTarget;
	}
	return aFound;
}

bool ModMenu::IsFocusable(const Target& theTarget) const
{
	switch (theTarget.mKind)
	{
	case T_VALUE:
	case T_ROW:
		return true;  // a greyed setting can still be read (its help)
	case T_UP:
	case T_DOWN:
	case T_OPEN:
	case T_BACK:
	case T_RESTART:
	case T_CLOSE:
	case T_RESET:
	case T_YES:
	case T_NO:
	case T_FILES:
		return !theTarget.mDisabled;
	default:
		return false;
	}
}

void ModMenu::SetFocusTarget(int theKind, int theItem)
{
	if (mLayoutDirty)
		Layout();
	const Target* aTarget = FindTarget(theKind, theItem);
	if ((aTarget == NULL) && (theKind == T_BACK))
		aTarget = FindTarget(T_BACK, -2);
	if ((aTarget == NULL) || !IsFocusable(*aTarget))
		return;
	mFocusKind = aTarget->mKind;
	mFocusItem = aTarget->mItem;
	ScrollToFocus();
	AnnounceFocus();
	MarkDirty();
}

void ModMenu::ScrollToFocus()
{
	for (const Item& anItem : mItems)
	{
		bool isFocused = ((anItem.mKind == ITEM_MOD) || (anItem.mKind == ITEM_SETTING)) && (anItem.mIndex == mFocusItem) &&
			((mFocusKind == T_ROW) || (mFocusKind == T_UP) || (mFocusKind == T_DOWN) || (mFocusKind == T_OPEN) || (mFocusKind == T_VALUE));
		if (!isFocused)
			continue;
		int aHeight = ContentRect().mHeight;
		int aTop = anItem.mY;
		// a setting's section heading comes into view with the first setting under it
		if (anItem.mKind == ITEM_SETTING)
		{
			for (size_t i = 0; i < mItems.size(); i++)
				if ((&mItems[i] == &anItem) && (i > 0) && (mItems[i - 1].mKind == ITEM_SECTION))
					aTop = mItems[i - 1].mY;
		}
		int aNewScroll = mScroll;
		if (aTop < aNewScroll)
			aNewScroll = aTop;
		if (anItem.mY + anItem.mHeight > aNewScroll + aHeight)
			aNewScroll = anItem.mY + anItem.mHeight - aHeight;
		// the first row shows whatever is above it (banners, the description)
		bool isFirst = true;
		for (const Item& anOther : mItems)
			if (((anOther.mKind == ITEM_MOD) || (anOther.mKind == ITEM_SETTING)) && (anOther.mY < anItem.mY))
				isFirst = false;
		if (isFirst && (anItem.mY + anItem.mHeight <= aHeight))
			aNewScroll = 0;
		if (aNewScroll != mScroll)
		{
			mScroll = aNewScroll;
			Layout();
		}
		return;
	}
}

void ModMenu::ScrollBy(int theDelta)
{
	int aMaxScroll = std::max(0, ContentHeight() - ContentRect().mHeight);
	int aScroll = std::max(0, std::min(mScroll + theDelta, aMaxScroll));
	if (aScroll != mScroll)
	{
		mScroll = aScroll;
		Layout();
		MarkDirty();
	}
}

// Up and Down: the previous or next row (a mod, a setting)
void ModMenu::MoveFocusRow(int theDelta)
{
	std::vector<std::pair<int, int> > aRows;
	for (const Target& aTarget : mTargets)
		if ((aTarget.mKind == T_ROW) || (aTarget.mKind == T_VALUE))
			aRows.push_back(std::make_pair(aTarget.mKind, aTarget.mItem));
	if (aRows.empty())
		return;
	int aCurrent = -1;
	for (size_t i = 0; i < aRows.size(); i++)
		if (aRows[i].second == mFocusItem)
			aCurrent = (int) i;
	bool isInRow = (mFocusKind == T_ROW) || (mFocusKind == T_VALUE) || (mFocusKind == T_UP) || (mFocusKind == T_DOWN) || (mFocusKind == T_OPEN);
	int aNext;
	if ((theDelta <= -100000) || (theDelta >= 100000))
		aNext = (theDelta < 0) ? 0 : (int) aRows.size() - 1;  // Home, End
	else if (!isInRow || (aCurrent < 0))
	{
		// from the header or the footer
		if ((mFocusKind == T_BACK) && (mFocusItem == -2))
			aNext = (theDelta > 0) ? 0 : -1;
		else
			aNext = (theDelta < 0) ? (int) aRows.size() - 1 : -1;
	}
	else
		aNext = std::max(0, std::min((int) aRows.size() - 1, aCurrent + theDelta));
	if (aNext >= 0)
		SetFocusTarget(aRows[(size_t) aNext].first, aRows[(size_t) aNext].second);
}

// Tab and Shift+Tab: the next or previous control
void ModMenu::MoveFocusTab(int theDelta)
{
	std::vector<const Target*> aFocusable;
	int aCurrent = -1;
	for (const Target& aTarget : mTargets)
	{
		// a question keeps the focus on its answers
		if (!IsFocusable(aTarget) || ((mConfirm != CONFIRM_NONE) && (aTarget.mKind != T_YES) && (aTarget.mKind != T_NO)))
			continue;
		if ((aTarget.mKind == mFocusKind) && (aTarget.mItem == mFocusItem))
			aCurrent = (int) aFocusable.size();
		aFocusable.push_back(&aTarget);
	}
	if (aFocusable.empty())
		return;
	int aCount = (int) aFocusable.size();
	int aNext = (aCurrent < 0) ? 0 : ((aCurrent + theDelta) % aCount + aCount) % aCount;
	SetFocusTarget(aFocusable[(size_t) aNext]->mKind, aFocusable[(size_t) aNext]->mItem);
}

void ModMenu::AnnounceFocus()
{
	std::string aText;
	if ((mFocusKind == T_ROW) && (mFocusItem >= 0) && (mFocusItem < (int) mMods.size()))
	{
		const ModView& aView = mMods[(size_t) mFocusItem];
		aText = aView.mName + " " + aView.mVersion + ", " + (aView.mEnabled ? Str(mApp, "MODMENU_ON", "On") : Str(mApp, "MODMENU_OFF", "Off"));
	}
	else if (mFocusKind == T_VALUE)
	{
		const SettingView* aSetting = PageSetting(mFocusItem);
		if (aSetting != NULL)
			aText = aSetting->mLabel + ": " + L1(Platform::ModSettingText(aSetting->mDef, aSetting->mValue));
	}
	else if (const Target* aTarget = FindTarget(mFocusKind, mFocusItem))
	{
		switch (aTarget->mKind)
		{
		case T_UP: aText = Str(mApp, "MODMENU_MOVE_UP", "Move up"); break;
		case T_DOWN: aText = Str(mApp, "MODMENU_MOVE_DOWN", "Move down"); break;
		case T_OPEN: aText = Str(mApp, "MODMENU_SETTINGS", "Settings"); break;
		case T_BACK: aText = Str(mApp, "MODMENU_BACK_BUTTON", "Back"); break;
		case T_RESTART: aText = Str(mApp, "MODMENU_RESTART", "Restart game"); break;
		case T_CLOSE: aText = Str(mApp, "MODMENU_CLOSE", "Close"); break;
		case T_RESET: aText = Str(mApp, "MODMENU_RESET", "Reset to defaults"); break;
		case T_FILES: aText = Str(mApp, "MODMENU_ADD_REMOVE", "Add or remove mods"); break;
		case T_YES: aText = mConfirmText; break;
		case T_NO: aText = Str(mApp, "MODMENU_CANCEL", "Cancel"); break;
		}
	}
	if (aText == mAnnounced)
		return;
	mAnnounced = aText;
#ifdef __EMSCRIPTEN__
	modmenu_announce(Platform::Latin1ToUtf8(aText).c_str());
#endif
}

//////////////////////////////////////////////////////////////////////////
// Actions

void ModMenu::DoEdit(const std::string& theOp, const std::string& theModId, const std::string& theSettingId, const JsonValue& theValue)
{
	ModManager& aManager = ModManager::Get();
	std::string aKeyBefore = mNextScoreKey;
	std::string anError;
	if (theOp == "enable")
		anError = aManager.EditEnable(theModId, theValue.AsBool(true));
	else if (theOp == "move")
		anError = aManager.EditMove(theModId, (int) theValue.AsNumber(0));
	else if (theOp == "set")
		anError = aManager.EditSetting(theModId, theSettingId, theValue);
	else if (theOp == "reset")
		anError = aManager.EditReset(theModId);
	else
		anError = "unknown edit " + theOp;
	Refresh();
	if (!anError.empty())
		mNote = L1(anError);
	else if ((mNextScoreKey != aKeyBefore) && (mNextScoreKey != mScoreKey) && mApp->HasGameInProgress())
		mNote = Str(mApp, "MODMENU_KEY_NOTE", "Gameplay changes use separate high scores. Your current game stays saved with the current mods and continues when you switch back.");
	else
		mNote.clear();
	Layout();
	AnnounceFocus();
	MarkDirty();
}

void ModMenu::SetValue(int theItem, const JsonValue& theValue)
{
	const SettingView* aSetting = PageSetting(theItem);
	if ((aSetting == NULL) || !aSetting->mDef.IsValue() || !aSetting->mEnabled || (mSettingsFile == "newer"))
		return;
	if (Platform::ModSettingSame(aSetting->mDef, theValue, aSetting->mValue))
		return;
	std::string anId = aSetting->mDef.mId;
	DoEdit("set", mPageMod, anId, theValue);
}

void ModMenu::ChangeValue(int theItem, int theSteps)
{
	const SettingView* aSetting = PageSetting(theItem);
	if ((aSetting == NULL) || !aSetting->mDef.IsValue())
		return;
	SetValue(theItem, Platform::ModSettingStep(aSetting->mDef, aSetting->mValue, theSteps));
}

void ModMenu::StartEdit(int theItem)
{
	const SettingView* aSetting = PageSetting(theItem);
	if ((aSetting == NULL) || !aSetting->mEnabled || (mSettingsFile == "newer") ||
		((aSetting->mDef.mType != Platform::MOD_SETTING_TEXT) && (aSetting->mDef.mType != Platform::MOD_SETTING_COLOR)))
		return;
	mEditItem = theItem;
	mEditText = L1(aSetting->mValue.AsString(""));
	mEditCaret = (int) mEditText.size();
	SetFocusTarget(T_VALUE, theItem);
	MarkDirty();
}

void ModMenu::CommitEdit()
{
	if (mEditItem < 0)
		return;
	int anItem = mEditItem;
	mEditItem = -1;
	SetValue(anItem, JsonValue(Platform::Latin1ToUtf8(mEditText)));
	MarkDirty();
}

void ModMenu::CancelEdit()
{
	mEditItem = -1;
	MarkDirty();
}

void ModMenu::Closing()
{
	CommitEdit();
	mApp->SetCursor(mOldCursor);
}

void ModMenu::Back()
{
	if (mPageMod.empty())
		Close();
	else
		ShowList();
}

void ModMenu::Close()
{
	// Removes this widget (deleted later, SafeDeleteWidget): nothing else may happen here after it
	mApp->CloseModMenu();
}

void ModMenu::AskConfirm(int theWhat, const std::string& theText)
{
	mConfirm = theWhat;
	mConfirmText = theText;
	mNote.clear();
	Layout();
	SetFocusTarget(T_YES, -1);
	MarkDirty();
}

void ModMenu::PressRestart(bool isConfirmed)
{
	CommitEdit();
	mRestartAction = isConfirmed ? 2 : 1;
}

void ModMenu::Activate(int theKind, int theItem, bool isSpace)
{
	switch (theKind)
	{
	case T_ROW:
		if ((theItem >= 0) && (theItem < (int) mMods.size()) && !mMods[(size_t) theItem].mDuplicate)
		{
			const ModView& aView = mMods[(size_t) theItem];
			if (!isSpace && aView.mHasSettings)
				ShowMod(aView.mId);
			else
			{
				std::string anId = aView.mId;
				DoEdit("enable", anId, "", JsonValue(!aView.mEnabled));
			}
		}
		break;
	case T_UP:
	case T_DOWN:
		if ((theItem >= 0) && (theItem < (int) mMods.size()))
		{
			int aDelta = (theKind == T_UP) ? -1 : 1;
			std::string anId = mMods[(size_t) theItem].mId;
			DoEdit("move", anId, "", JsonValue((double) aDelta));
			// the focus stays on the mod that moved
			for (size_t i = 0; i < mMods.size(); i++)
			{
				if (mMods[i].mId != anId)
					continue;
				const Target* aTarget = FindTarget(theKind, (int) i);
				SetFocusTarget(((aTarget != NULL) && !aTarget->mDisabled) ? theKind : T_ROW, (int) i);
			}
		}
		break;
	case T_OPEN:
		if ((theItem >= 0) && (theItem < (int) mMods.size()))
			ShowMod(mMods[(size_t) theItem].mId);
		break;
	case T_BACK:
		ShowList();
		break;
	case T_RESTART:
		PressRestart(false);
		break;
	case T_FILES:
#ifdef __EMSCRIPTEN__
		CommitEdit();
		modmenu_open_panel();
#endif
		break;
	case T_CLOSE:
		Close();
		return;
	case T_RESET:
		if (const ModView* aView = PageModView())
			AskConfirm(CONFIRM_RESET, Replace(Str(mApp, "MODMENU_RESET_ASK", "Reset every setting of {mod} to its default?"), "{mod}", aView->mName));
		break;
	case T_YES:
	{
		int aWhat = mConfirm;
		mConfirm = CONFIRM_NONE;
		Layout();
		if (aWhat == CONFIRM_RESTART)
		{
			SetFocusTarget(T_RESTART, -1);
			PressRestart(true);
		}
		else if (aWhat == CONFIRM_RESET)
		{
			SetFocusTarget(T_RESET, -1);
			std::string anId = mPageMod;
			DoEdit("reset", anId, "", JsonValue());
		}
		break;
	}
	case T_NO:
	{
		int aWhat = mConfirm;
		mConfirm = CONFIRM_NONE;
		Layout();
		SetFocusTarget((aWhat == CONFIRM_RESET) ? T_RESET : T_RESTART, -1);
		break;
	}
	case T_VALUE:
	{
		const SettingView* aSetting = PageSetting(theItem);
		if (aSetting == NULL)
			break;
		switch (aSetting->mDef.mType)
		{
		case Platform::MOD_SETTING_BOOL:
		case Platform::MOD_SETTING_CHOICE:
			ChangeValue(theItem, 1);
			break;
		case Platform::MOD_SETTING_TEXT:
		case Platform::MOD_SETTING_COLOR:
			StartEdit(theItem);
			break;
		}
		break;
	}
	case T_MINUS:
	case T_PREV:
		ChangeValue(theItem, -1);
		break;
	case T_PLUS:
	case T_NEXT:
		ChangeValue(theItem, 1);
		break;
	case T_FIELD:
		StartEdit(theItem);
		break;
	case T_LABEL:
		if (const SettingView* aSetting = PageSetting(theItem))
			if (aSetting->mDef.mType == Platform::MOD_SETTING_BOOL)
				ChangeValue(theItem, 1);
		break;
	}
	MarkDirty();
}

//////////////////////////////////////////////////////////////////////////
// Input

void ModMenu::Update()
{
	Widget::Update();

	if (mRestartAction != 0)
	{
		// Outside the no-random scope: the game's own shutdown runs from here
		bool isConfirmed = mRestartAction == 2;
		mRestartAction = 0;
		SexyString aMessage;
		int aResult = mApp->RequestRestart(isConfirmed, &aMessage);
		if (aResult == RESTART_STARTED)
			return;
		if (aResult == RESTART_CONFIRM)
			AskConfirm(CONFIRM_RESTART, aMessage);
		else
		{
			mNote = aMessage;
			MarkDirty();
		}
	}

	MTAutoDisallowRand aDisallowRand;
	if (mWidgetManager == NULL)
		return;
	// Stays on top and keeps the keyboard, whatever the game does underneath (a dialog it closes gives its
	// board the focus)
	if (!mWidgetManager->mWidgets.empty() && (mWidgetManager->mWidgets.back() != this))
		mWidgetManager->BringToFront(this);
	if (mWidgetManager->mFocusWidget != this)
		mWidgetManager->SetFocus(this);
	if (ModManager::Get().GetChangeSerial() != mSerial)
		Refresh();
	if (mLayoutDirty)
		Layout();
}

void ModMenu::KeyDown(KeyCode theKey)
{
	MTAutoDisallowRand aDisallowRand;
	if (mWidgetManager == NULL)
		return;
	bool isShift = mWidgetManager->mKeyDown[KEYCODE_SHIFT];
	bool isAlt = mWidgetManager->mKeyDown[KEYCODE_MENU];

	if (mEditItem >= 0)
	{
		switch (theKey)
		{
		case KEYCODE_LEFT: mEditCaret = std::max(0, mEditCaret - 1); break;
		case KEYCODE_RIGHT: mEditCaret = std::min((int) mEditText.size(), mEditCaret + 1); break;
		case KEYCODE_HOME: mEditCaret = 0; break;
		case KEYCODE_END: mEditCaret = (int) mEditText.size(); break;
		case KEYCODE_DELETE:
			if (mEditCaret < (int) mEditText.size())
				mEditText.erase((size_t) mEditCaret, 1);
			break;
		default: break;
		}
		MarkDirty();
		return;
	}

	bool isOnRow = (mFocusKind == T_ROW) || (mFocusKind == T_UP) || (mFocusKind == T_DOWN) || (mFocusKind == T_OPEN);
	switch (theKey)
	{
	case KEYCODE_UP:
	case KEYCODE_DOWN:
		if (isAlt && mPageMod.empty() && isOnRow && (mConfirm == CONFIRM_NONE))
		{
			int aKind = (theKey == KEYCODE_UP) ? T_UP : T_DOWN;
			const Target* aTarget = FindTarget(aKind, mFocusItem);
			if ((aTarget != NULL) && !aTarget->mDisabled)
			{
				int aFocusKind = mFocusKind;
				std::string anId = mMods[(size_t) mFocusItem].mId;
				Activate(aKind, mFocusItem, false);
				for (size_t i = 0; i < mMods.size(); i++)
					if (mMods[i].mId == anId)
						SetFocusTarget(aFocusKind, (int) i);
			}
		}
		else if (mConfirm == CONFIRM_NONE)
			MoveFocusRow((theKey == KEYCODE_UP) ? -1 : 1);
		break;
	case KEYCODE_LEFT:
	case KEYCODE_RIGHT:
	{
		int aDelta = (theKey == KEYCODE_LEFT) ? -1 : 1;
		const SettingView* aSetting = (mFocusKind == T_VALUE) ? PageSetting(mFocusItem) : NULL;
		if (aSetting != NULL)
		{
			bool isNumber = (aSetting->mDef.mType == Platform::MOD_SETTING_INT) || (aSetting->mDef.mType == Platform::MOD_SETTING_FLOAT);
			ChangeValue(mFocusItem, aDelta * ((isShift && isNumber) ? 10 : 1));
		}
		else
			MoveFocusTab(aDelta);
		break;
	}
	case KEYCODE_HOME:
	case KEYCODE_END:
		if (mConfirm == CONFIRM_NONE)
			MoveFocusRow((theKey == KEYCODE_HOME) ? -100000 : 100000);
		break;
	case KEYCODE_PRIOR:
	case KEYCODE_NEXT:
		if (mConfirm == CONFIRM_NONE)
		{
			int aRows = std::max(1, ContentRect().mHeight / (mPageMod.empty() ? LIST_ROW : SETTING_ROW) - 1);
			MoveFocusRow((theKey == KEYCODE_PRIOR) ? -aRows : aRows);
		}
		break;
	case KEYCODE_DELETE:
		if ((mFocusKind == T_VALUE) && (mConfirm == CONFIRM_NONE))
		{
			const SettingView* aSetting = PageSetting(mFocusItem);
			if (aSetting != NULL)
				SetValue(mFocusItem, aSetting->mDef.mDefault);
		}
		break;
	default:
		break;
	}
	MarkDirty();
}

void ModMenu::KeyChar(SexyChar theChar)
{
	MTAutoDisallowRand aDisallowRand;
	if (mWidgetManager == NULL)
		return;
	unsigned char c = (unsigned char) theChar;
	bool isShift = mWidgetManager->mKeyDown[KEYCODE_SHIFT];

	if (mEditItem >= 0)
	{
		const SettingView* aSetting = PageSetting(mEditItem);
		if (aSetting == NULL)
		{
			mEditItem = -1;
			return;
		}
		if (c == '\r')
			CommitEdit();
		else if (c == 0x1B)
			CancelEdit();
		else if (c == '\b')
		{
			if (mEditCaret > 0)
			{
				mEditText.erase((size_t) (mEditCaret - 1), 1);
				mEditCaret--;
			}
		}
		else if (c == '\t')
		{
			CommitEdit();
			MoveFocusTab(isShift ? -1 : 1);
		}
		else if ((c >= 0x20) && (c != 0x7F))
		{
			unsigned char aTyped = Platform::ModSettingTypedChar(aSetting->mDef, c);
			int aMax = (aSetting->mDef.mType == Platform::MOD_SETTING_COLOR) ? (aSetting->mDef.mAlpha ? 9 : 7) : aSetting->mDef.mMaxLength;
			if ((aTyped != 0) && ((int) mEditText.size() < aMax))
			{
				mEditText.insert((size_t) mEditCaret, 1, (char) aTyped);
				mEditCaret++;
			}
		}
		MarkDirty();
		return;
	}

	switch (c)
	{
	case '\r':
	case ' ':
		if (mFocusKind != T_NONE)
		{
			if (mFocusKind == T_CLOSE)
			{
				Close();
				return;
			}
			Activate(mFocusKind, mFocusItem, c == ' ');
		}
		break;
	case 0x1B:
		if (mConfirm != CONFIRM_NONE)
			Activate(T_NO, -1, false);
		else if (mPageMod.empty())
		{
			Close();
			return;
		}
		else
			ShowList();
		break;
	case '\t':
		MoveFocusTab(isShift ? -1 : 1);
		break;
	default:
		break;  // nothing else reaches the game underneath either
	}
	MarkDirty();
}

Rect ModMenu::SliderTrack(const Target& theTarget) const
{
	return Rect(theTarget.mRect.mX + 6, theTarget.mRect.mY, std::max(1, theTarget.mRect.mWidth - 12), theTarget.mRect.mHeight);
}

JsonValue ModMenu::SliderValue(int theItem, int x) const
{
	const Target* aTarget = FindTarget(T_SLIDER, theItem);
	const SettingView* aSetting = const_cast<ModMenu*>(this)->PageSetting(theItem);
	if ((aTarget == NULL) || (aSetting == NULL))
		return JsonValue();
	Rect aTrack = SliderTrack(*aTarget);
	int64_t aCount = Platform::ModSettingStepCount(aSetting->mDef);
	int aPos = std::max(0, std::min(aTrack.mWidth, x - aTrack.mX));
	int64_t anIndex = (aCount * aPos * 2 + aTrack.mWidth) / (2 * (int64_t) aTrack.mWidth);
	return Platform::ModSettingAtStep(aSetting->mDef, anIndex);
}

void ModMenu::MouseMove(int x, int y)
{
	MTAutoDisallowRand aDisallowRand;
	const Target* aTarget = TargetAt(x, y);
	int aKind = ((aTarget != NULL) && !aTarget->mDisabled) ? aTarget->mKind : T_NONE;
	int anItem = (aTarget != NULL) ? aTarget->mItem : -1;
	if ((aKind != mHoverKind) || (anItem != mHoverItem))
	{
		mHoverKind = aKind;
		mHoverItem = anItem;
		// the hand over what a click changes (a label only for a bool)
		const SettingView* aSetting = (aKind == T_LABEL) ? PageSetting(anItem) : NULL;
		bool isClickable = (aKind != T_NONE) && (aKind != T_SCROLL) &&
			((aKind != T_LABEL) || ((aSetting != NULL) && (aSetting->mDef.mType == Platform::MOD_SETTING_BOOL)));
		mApp->SetCursor(isClickable ? CURSOR_HAND : CURSOR_POINTER);
		MarkDirty();
	}
}

void ModMenu::MouseLeave()
{
	mHoverKind = T_NONE;
	mHoverItem = -1;
	MarkDirty();
}

void ModMenu::MouseDown(int x, int y, int theClickCount)
{
	MTAutoDisallowRand aDisallowRand;
	if (theClickCount < 0)
		return;  // the right button does nothing here
	const Target* aTarget = TargetAt(x, y);
	if ((mEditItem >= 0) && ((aTarget == NULL) || (aTarget->mItem != mEditItem) || ((aTarget->mKind != T_FIELD) && (aTarget->mKind != T_VALUE))))
		CommitEdit();
	// A click anywhere but on a question's answers says no to it
	if ((mConfirm != CONFIRM_NONE) && ((aTarget == NULL) || ((aTarget->mKind != T_YES) && (aTarget->mKind != T_NO))))
	{
		Activate(T_NO, -1, false);
		mPressKind = T_NONE;
		mPressItem = -1;
		mPressInContent = false;
		return;
	}
	aTarget = TargetAt(x, y);  // the layout may have changed

	mPressX = x;
	mPressY = y;
	mPressScroll = mScroll;
	mPressInContent = ContentRect().Contains(x, y) && ((aTarget == NULL) || (aTarget->mKind != T_SCROLL));
	mDragScroll = false;
	mDragThumb = false;
	mDragSlider = -1;
	mPressKind = T_NONE;
	mPressItem = -1;
	if ((aTarget == NULL) || aTarget->mDisabled)
	{
		MarkDirty();
		return;
	}
	mPressKind = aTarget->mKind;
	mPressItem = aTarget->mItem;

	if (aTarget->mKind == T_SCROLL)
	{
		Rect aContent = ContentRect();
		int aHeight = ContentHeight();
		int aThumbHeight = std::max(20, aContent.mHeight * aContent.mHeight / std::max(1, aHeight));
		int aThumbY = aContent.mY + (aContent.mHeight - aThumbHeight) * mScroll / std::max(1, aHeight - aContent.mHeight);
		if ((y >= aThumbY) && (y < aThumbY + aThumbHeight))
			mDragThumb = true;
		else
			ScrollBy((y < aThumbY) ? -aContent.mHeight : aContent.mHeight);
		mPressKind = T_NONE;
	}
	else if (aTarget->mKind == T_SLIDER)
	{
		mDragSlider = aTarget->mItem;
		mDragValue = SliderValue(aTarget->mItem, x);
		mPressInContent = false;
	}

	if (IsFocusable(*aTarget))
		SetFocusTarget(aTarget->mKind, aTarget->mItem);
	else if (aTarget->mInContent && !mPageMod.empty())
		SetFocusTarget(T_VALUE, aTarget->mItem);
	MarkDirty();
}

void ModMenu::MouseDrag(int x, int y)
{
	MTAutoDisallowRand aDisallowRand;
	if (mDragSlider >= 0)
	{
		mDragValue = SliderValue(mDragSlider, x);
		MarkDirty();
		return;
	}
	if (mDragThumb)
	{
		Rect aContent = ContentRect();
		int aHeight = ContentHeight();
		int aThumbHeight = std::max(20, aContent.mHeight * aContent.mHeight / std::max(1, aHeight));
		int aTravel = std::max(1, aContent.mHeight - aThumbHeight);
		ScrollBy(mPressScroll + (y - mPressY) * (aHeight - aContent.mHeight) / aTravel - mScroll);
		return;
	}
	// A press on the list that moves scrolls it instead of clicking
	if (mPressInContent && !mDragScroll && (std::abs(y - mPressY) > DRAG_DISTANCE))
	{
		mDragScroll = true;
		mPressKind = T_NONE;
	}
	if (mDragScroll)
		ScrollBy(mPressScroll - (y - mPressY) - mScroll);
}

void ModMenu::MouseUp(int x, int y, int theClickCount)
{
	MTAutoDisallowRand aDisallowRand;
	if (theClickCount < 0)
		return;
	if (mDragSlider >= 0)
	{
		int anItem = mDragSlider;
		mDragSlider = -1;
		SetValue(anItem, mDragValue);
		MarkDirty();
		return;
	}
	mDragThumb = false;
	if (mDragScroll)
	{
		mDragScroll = false;
		mPressKind = T_NONE;
		return;
	}
	const Target* aTarget = TargetAt(x, y);
	int aKind = mPressKind;
	int anItem = mPressItem;
	mPressKind = T_NONE;
	mPressItem = -1;
	if ((aTarget == NULL) || aTarget->mDisabled || (aTarget->mKind != aKind) || (aTarget->mItem != anItem))
	{
		MarkDirty();
		return;
	}
	if (aKind == T_CLOSE)
	{
		Close();
		return;
	}
	// A click on a mod's row switches it, as a label does its checkbox
	Activate(aKind, anItem, aKind == T_ROW);
}

void ModMenu::MouseWheel(int theDelta)
{
	MTAutoDisallowRand aDisallowRand;
	ScrollBy(-theDelta * 40);
}

//////////////////////////////////////////////////////////////////////////
// Drawing

std::string ModMenu::PendingText() const
{
	std::string aText;
	if (!mPendingRestart.empty())
	{
		aText = Str(mApp, "MODMENU_PENDING_RESTART", "Restart to apply:") + " ";
		for (size_t i = 0; i < mPendingRestart.size(); i++)
			aText += (i ? ", " : "") + mPendingRestart[i];
		aText += ".";
	}
	if (!mPendingNextGame.empty())
	{
		aText += aText.empty() ? "" : " ";
		aText += Str(mApp, "MODMENU_PENDING_NEXT_GAME", "Applies to your next game:") + " ";
		for (size_t i = 0; i < mPendingNextGame.size(); i++)
			aText += (i ? ", " : "") + mPendingNextGame[i];
		aText += ".";
	}
	return aText;
}

std::string ModMenu::FooterHelp()
{
	if ((mFocusKind == T_VALUE) && !mPageMod.empty())
	{
		const SettingView* aSetting = PageSetting(mFocusItem);
		if (aSetting != NULL)
			return aSetting->mHelp.empty() ? aSetting->mLabel : aSetting->mHelp;
	}
	if (mPageMod.empty() && (mFocusItem >= 0) && (mFocusItem < (int) mMods.size()) &&
		((mFocusKind == T_ROW) || (mFocusKind == T_UP) || (mFocusKind == T_DOWN) || (mFocusKind == T_OPEN)))
	{
		const ModView& aView = mMods[(size_t) mFocusItem];
		if (!aView.mErrors.empty())
			return "! " + aView.mErrors[0];
		if (!aView.mWarnings.empty())
			return "! " + aView.mWarnings[0];
		return aView.mDescription.empty() ? aView.mName + " " + aView.mVersion : aView.mDescription;
	}
	return "";
}

void ModMenu::DrawButton(Graphics* g, const Rect& theRect, const std::string& theLabel, int theKind, int theItem, bool isDisabled)
{
	bool isHover = (mHoverKind == theKind) && (mHoverItem == theItem);
	bool isDown = isHover && (mPressKind == theKind) && (mPressItem == theItem);
	Fill(g, theRect, isDisabled ? kPanel : isDown ? kButtonDown : isHover ? kButtonHover : kButton);
	Outline(g, theRect, isDisabled ? kDisabled : kButtonBorder);
	const Color& aColor = isDisabled ? kDisabled : kText;
	int cx = theRect.mX + theRect.mWidth / 2;
	int cy = theRect.mY + theRect.mHeight / 2;
	switch (theKind)
	{
	case T_UP: Arrow(g, cx, cy, 0, aColor); return;
	case T_DOWN: Arrow(g, cx, cy, 1, aColor); return;
	case T_PREV: Arrow(g, cx, cy, 2, aColor); return;
	case T_NEXT: Arrow(g, cx, cy, 3, aColor); return;
	case T_MINUS:
		g->SetColor(aColor);
		g->FillRect(cx - 4, cy - 1, 8, 2);
		return;
	case T_PLUS:
		g->SetColor(aColor);
		g->FillRect(cx - 4, cy - 1, 8, 2);
		g->FillRect(cx - 1, cy - 4, 2, 8);
		return;
	}
	std::string aLabel = Fit(mBold, theLabel, theRect.mWidth - 8);
	Text(g, mBold, aLabel, cx - mBold->StringWidth(aLabel) / 2, CenterBaseline(theRect), aColor);
}

void ModMenu::DrawBadge(Graphics* g, int x, int y, const std::string& theText, const Color& theColor, int* theWidth)
{
	int aWidth = mFont->StringWidth(theText) + 6;
	Outline(g, Rect(x, y, aWidth, 14), theColor);
	Text(g, mFont, theText, x + 3, y + 11, theColor);
	if (theWidth != NULL)
		*theWidth = aWidth;
}

void ModMenu::DrawListRow(Graphics* g, const Item& theItem)
{
	const ModView& aView = mMods[(size_t) theItem.mIndex];
	const Target* aRowTarget = FindTarget(T_ROW, theItem.mIndex);
	if (aRowTarget == NULL)
		return;
	Rect aRow = aRowTarget->mRect;
	bool isFocused = (mFocusItem == theItem.mIndex) && ((mFocusKind == T_ROW) || (mFocusKind == T_UP) || (mFocusKind == T_DOWN) || (mFocusKind == T_OPEN));
	if (isFocused || ((mHoverKind == T_ROW) && (mHoverItem == theItem.mIndex)))
	{
		const Target* aLast = FindTarget(aView.mHasSettings ? T_OPEN : T_DOWN, theItem.mIndex);
		int aRight = (aLast != NULL) ? aLast->mRect.mX + aLast->mRect.mWidth + 4 : aRow.mX + aRow.mWidth;
		Fill(g, Rect(aRow.mX, aRow.mY + 1, aRight - aRow.mX, aRow.mHeight - 2), kRowFocus);
	}

	Checkbox(g, Rect(aRow.mX + 8, aRow.mY + 7, 16, 16), aView.mEnabled, aView.mDuplicate);

	// Badges after the name: separate high scores, features, an error, a change that waits
	std::vector<std::pair<std::string, Color> > aBadges;
	if (aView.mScores)
		aBadges.push_back(std::make_pair(Str(mApp, "MODMENU_BADGE_SCORES", "SCORES"), kScores));
	if (aView.mFeatures)
		aBadges.push_back(std::make_pair(Str(mApp, "MODMENU_BADGE_FEATURES", "FEATURES"), kFeatures));
	if (!aView.mErrors.empty())
		aBadges.push_back(std::make_pair(Str(mApp, "MODMENU_BADGE_ERROR", "ERROR"), kError));
	if (aView.mPending == "restart")
		aBadges.push_back(std::make_pair(Str(mApp, "MODMENU_BADGE_RESTART", "RESTART"), kFocus));
	else if (aView.mPending == "nextGame")
		aBadges.push_back(std::make_pair(Str(mApp, "MODMENU_BADGE_NEXT_GAME", "NEXT GAME"), kFocus));
	int aBadgesWidth = 0;
	for (const auto& aBadge : aBadges)
		aBadgesWidth += mFont->StringWidth(aBadge.first) + 6 + 4;

	int x = aRow.mX + 32;
	int aWidth = aRow.mX + aRow.mWidth - x - 4;
	std::string aName = Fit(mBold, aView.mName + (aView.mVersion.empty() ? "" : " " + aView.mVersion), std::max(40, aWidth - aBadgesWidth - 6));
	const Color& aNameColor = aView.mDuplicate ? kDisabled : kText;
	Text(g, mBold, aName, x, aRow.mY + 19, aNameColor);
	int aBadgeX = x + mBold->StringWidth(aName) + 6;
	for (const auto& aBadge : aBadges)
	{
		int aBadgeWidth = 0;
		if (aBadgeX + mFont->StringWidth(aBadge.first) + 6 > aRow.mX + aRow.mWidth)
			break;
		DrawBadge(g, aBadgeX, aRow.mY + 8, aBadge.first, aBadge.second, &aBadgeWidth);
		aBadgeX += aBadgeWidth + 4;
	}

	std::string aLine;
	const Color* aColor = &kDim;
	if (!aView.mErrors.empty())
	{
		aLine = "! " + aView.mErrors[0];
		aColor = &kError;
	}
	else if (!aView.mWarnings.empty())
	{
		aLine = "! " + aView.mWarnings[0];
		aColor = &kWarn;
	}
	else
	{
		if (!aView.mAuthor.empty())
			aLine = Str(mApp, "MODMENU_BY", "by") + " " + aView.mAuthor;
		if (!aView.mDescription.empty())
			aLine += (aLine.empty() ? "" : " - ") + aView.mDescription;
	}
	Text(g, mFont, Fit(mFont, aLine, aWidth), x, aRow.mY + 36, *aColor);
}

void ModMenu::DrawSettingRow(Graphics* g, const Item& theItem)
{
	const SettingView* aSetting = PageSetting(theItem.mIndex);
	const Target* aValueTarget = FindTarget(T_VALUE, theItem.mIndex);
	if ((aSetting == NULL) || (aValueTarget == NULL))
		return;
	const ModSettingDef& aDef = aSetting->mDef;
	bool isDisabled = aValueTarget->mDisabled;
	Rect aContent = ContentRect();
	int aRowX = aContent.mX;
	int aRowY = aValueTarget->mRect.mY - 1;
	int aRowRight = aValueTarget->mRect.mX + aValueTarget->mRect.mWidth - 3 + PENDING_WIDTH;
	const Color& aTextColor = isDisabled ? kDisabled : kText;

	if ((mFocusKind == T_VALUE) && (mFocusItem == theItem.mIndex))
		Fill(g, Rect(aRowX, aRowY + 1, aRowRight - aRowX, SETTING_ROW - 2), kRowFocus);

	// The label, and SCORES when the value counts for the high scores
	int aScoresWidth = aDef.mScores ? mFont->StringWidth(Str(mApp, "MODMENU_BADGE_SCORES", "SCORES")) + 10 : 0;
	std::string aLabel = Fit(mFont, aSetting->mLabel, LABEL_WIDTH - 12 - aScoresWidth);
	Text(g, mFont, aLabel, aRowX + 8, aRowY + 19, aTextColor);
	if (aDef.mScores)
		DrawBadge(g, aRowX + 8 + mFont->StringWidth(aLabel) + 6, aRowY + 8, Str(mApp, "MODMENU_BADGE_SCORES", "SCORES"), isDisabled ? kDisabled : kScores, NULL);

	int x = aRowX + LABEL_WIDTH + 4;
	int aWidth = aValueTarget->mRect.mWidth - 6;
	int y = aRowY + (SETTING_ROW - BUTTON_HEIGHT) / 2;
	Rect aBox(x, y, aWidth, BUTTON_HEIGHT);
	JsonValue aValue = (mDragSlider == theItem.mIndex) ? mDragValue : aSetting->mValue;
	std::string aUnit = L1(aDef.mUnit);

	switch (aDef.mType)
	{
	case Platform::MOD_SETTING_BOOL:
	{
		bool isOn = aValue.AsBool(false);
		Checkbox(g, Rect(x + 4, y + 4, 16, 16), isOn, isDisabled);
		Text(g, mFont, isOn ? Str(mApp, "MODMENU_ON", "On") : Str(mApp, "MODMENU_OFF", "Off"), x + 28, CenterBaseline(aBox), aTextColor);
		break;
	}
	case Platform::MOD_SETTING_INT:
	case Platform::MOD_SETTING_FLOAT:
	{
		if (const Target* aTarget = FindTarget(T_MINUS, theItem.mIndex))
			DrawButton(g, aTarget->mRect, "", T_MINUS, theItem.mIndex, isDisabled);
		if (const Target* aTarget = FindTarget(T_PLUS, theItem.mIndex))
			DrawButton(g, aTarget->mRect, "", T_PLUS, theItem.mIndex, isDisabled);
		std::string aText = Fit(mFont, L1(Platform::ModSettingText(aDef, aValue)) + (aUnit.empty() ? "" : " " + aUnit), 60);
		Text(g, mFont, aText, x + 58 - mFont->StringWidth(aText) / 2, CenterBaseline(aBox), aTextColor);
		if (const Target* aSlider = FindTarget(T_SLIDER, theItem.mIndex))
		{
			Rect aTrack = SliderTrack(*aSlider);
			int64_t aCount = std::max((int64_t) 1, Platform::ModSettingStepCount(aDef));
			int64_t anIndex = Platform::ModSettingStepIndex(aDef, aValue);
			int aThumbX = aTrack.mX + (int) (anIndex * aTrack.mWidth / aCount);
			int aMid = aTrack.mY + aTrack.mHeight / 2;
			Fill(g, Rect(aTrack.mX, aMid - 2, aTrack.mWidth, 4), kTrack);
			Fill(g, Rect(aTrack.mX, aMid - 2, aThumbX - aTrack.mX, 4), isDisabled ? kDisabled : kBorder);
			Fill(g, Rect(aThumbX - 4, aMid - 8, 8, 16), isDisabled ? kDisabled : kThumb);
		}
		break;
	}
	case Platform::MOD_SETTING_CHOICE:
	{
		if (const Target* aTarget = FindTarget(T_PREV, theItem.mIndex))
			DrawButton(g, aTarget->mRect, "", T_PREV, theItem.mIndex, isDisabled);
		if (const Target* aTarget = FindTarget(T_NEXT, theItem.mIndex))
			DrawButton(g, aTarget->mRect, "", T_NEXT, theItem.mIndex, isDisabled);
		std::string aText;
		int64_t anIndex = Platform::ModSettingStepIndex(aDef, aValue);
		if ((anIndex >= 0) && (anIndex < (int64_t) aDef.mChoices.size()))
			aText = L1(aDef.mChoices[(size_t) anIndex].mLabel);
		aText = Fit(mFont, aText, aWidth - 56);
		Text(g, mFont, aText, x + aWidth / 2 - mFont->StringWidth(aText) / 2, CenterBaseline(aBox), aTextColor);
		break;
	}
	case Platform::MOD_SETTING_TEXT:
	case Platform::MOD_SETTING_COLOR:
	{
		const Target* aField = FindTarget(T_FIELD, theItem.mIndex);
		if (aField == NULL)
			break;
		bool isEditing = mEditItem == theItem.mIndex;
		Fill(g, aField->mRect, kField);
		Outline(g, aField->mRect, isEditing ? kFocus : isDisabled ? kDisabled : kButtonBorder);
		std::string aText = isEditing ? mEditText : L1(aValue.AsString(""));
		int aTextX = aField->mRect.mX + 5;
		int aRoom = aField->mRect.mWidth - 10;
		if (isEditing)
		{
			// The part of the text around the caret
			size_t aStart = 0;
			while ((aStart < (size_t) mEditCaret) && (mFont->StringWidth(aText.substr(aStart, (size_t) mEditCaret - aStart)) > aRoom - 2))
				aStart++;
			std::string aShown = aText.substr(aStart);
			while (!aShown.empty() && (mFont->StringWidth(aShown) > aRoom))
				aShown.erase(aShown.size() - 1);
			Text(g, mFont, aShown, aTextX, CenterBaseline(aField->mRect), kText);
			int aCaretX = aTextX + mFont->StringWidth(aText.substr(aStart, (size_t) mEditCaret - aStart));
			Fill(g, Rect(aCaretX, aField->mRect.mY + 5, 1, aField->mRect.mHeight - 10), kFocus);
		}
		else
			Text(g, mFont, Fit(mFont, aText, aRoom), aTextX, CenterBaseline(aField->mRect), aTextColor);
		if (aDef.mType == Platform::MOD_SETTING_COLOR)
		{
			Color aColor;
			Rect aSwatch(x, y, 24, BUTTON_HEIGHT);
			if (ParseColor(aValue.AsString(""), &aColor))
				Fill(g, aSwatch, aColor);
			Outline(g, aSwatch, isDisabled ? kDisabled : kDim);
			if (const Target* aTarget = FindTarget(T_PREV, theItem.mIndex))
				DrawButton(g, aTarget->mRect, "", T_PREV, theItem.mIndex, isDisabled);
			if (const Target* aTarget = FindTarget(T_NEXT, theItem.mIndex))
				DrawButton(g, aTarget->mRect, "", T_NEXT, theItem.mIndex, isDisabled);
		}
		break;
	}
	}

	// What the change waits for
	if (!aSetting->mPending.empty())
	{
		std::string aBadge = (aSetting->mPending == "nextGame") ? Str(mApp, "MODMENU_BADGE_NEXT_GAME", "NEXT GAME") : Str(mApp, "MODMENU_BADGE_RESTART", "RESTART");
		DrawBadge(g, aRowRight - 4 - (mFont->StringWidth(aBadge) + 6), aRowY + 8, aBadge, kFocus, NULL);
	}
}

void ModMenu::Draw(Graphics* g)
{
	MTAutoDisallowRand aDisallowRand;
	if (mLayoutDirty)
		Layout();

	Fill(g, Rect(0, 0, mWidth, mHeight), kBackdrop);
	Fill(g, mPanel, kPanel);
	Outline(g, mPanel, kBorder);

	// The header
	int aTitleBaseline = mPanel.mY + 22;
	int aRight = mPanel.mX + mPanel.mWidth - 12;
	std::string aHint = mPageMod.empty() ? Str(mApp, "MODMENU_HINT", "F2 or Esc: close") : Str(mApp, "MODMENU_PAGE_HINT", "Esc: back");
	Text(g, mFont, aHint, aRight - mFont->StringWidth(aHint), aTitleBaseline, kDim);
	int aTitleRoom = aRight - mFont->StringWidth(aHint) - 16;
	if (mPageMod.empty())
		Text(g, mBold, Str(mApp, "MODMENU_TITLE", "MODS"), mPanel.mX + 12, aTitleBaseline, kText);
	else if (const ModView* aView = PageModView())
	{
		if (const Target* aBack = FindTarget(T_BACK, -2))
			DrawButton(g, aBack->mRect, Str(mApp, "MODMENU_BACK", "< Back"), T_BACK, -2, false);
		int x = mPanel.mX + 84;
		std::string aTitle = Fit(mBold, aView->mName + (aView->mVersion.empty() ? "" : " " + aView->mVersion), aTitleRoom - x);
		Text(g, mBold, aTitle, x, aTitleBaseline, kText);
		x += mBold->StringWidth(aTitle) + 8;
		if (!aView->mAuthor.empty() && (x < aTitleRoom))
			Text(g, mFont, Fit(mFont, Str(mApp, "MODMENU_BY", "by") + " " + aView->mAuthor, aTitleRoom - x), x, aTitleBaseline, kDim);
	}

	// The content
	Rect aContent = ContentRect();
	g->PushState();
	g->ClipRect(aContent);
	for (const Item& anItem : mItems)
	{
		int aTop = aContent.mY + anItem.mY - mScroll;
		if ((aTop + anItem.mHeight <= aContent.mY) || (aTop >= aContent.mY + aContent.mHeight))
			continue;
		switch (anItem.mKind)
		{
		case ITEM_MOD:
			DrawListRow(g, anItem);
			break;
		case ITEM_SETTING:
			DrawSettingRow(g, anItem);
			break;
		case ITEM_SECTION:
			if (const SettingView* aSection = PageSetting(anItem.mIndex))
			{
				std::string aLabel = aSection->mLabel;
				for (char& c : aLabel)
					c = (char) toupper((unsigned char) c);
				Text(g, mBold, Fit(mBold, aLabel, aContent.mWidth - 16), aContent.mX + 8, aTop + 17, kDim);
				Fill(g, Rect(aContent.mX + 8, aTop + anItem.mHeight - 3, aContent.mWidth - 16 - ((ContentHeight() > aContent.mHeight) ? SCROLLBAR + 4 : 0), 1), kButtonBorder);
			}
			break;
		default:
			for (size_t i = 0; i < anItem.mLines.size(); i++)
				Text(g, mFont, anItem.mLines[i], aContent.mX + 8, aTop + 13 + (int) i * LINE, TextColor(anItem.mColor));
			break;
		}
	}
	g->PopState();

	if (const Target* aScroll = FindTarget(T_SCROLL, -1))
	{
		int aHeight = ContentHeight();
		int aThumbHeight = std::max(20, aContent.mHeight * aContent.mHeight / std::max(1, aHeight));
		int aThumbY = aContent.mY + (aContent.mHeight - aThumbHeight) * mScroll / std::max(1, aHeight - aContent.mHeight);
		Fill(g, aScroll->mRect, kField);
		Fill(g, Rect(aScroll->mRect.mX + 2, aThumbY, aScroll->mRect.mWidth - 4, aThumbHeight), mDragThumb ? kBorder : kButtonBorder);
	}

	// The buttons of the content (drawn over the rows, clipped to the content)
	g->PushState();
	g->ClipRect(aContent);
	for (const Target& aTarget : mTargets)
	{
		if (!aTarget.mInContent)
			continue;
		if ((aTarget.mKind == T_UP) || (aTarget.mKind == T_DOWN))
			DrawButton(g, aTarget.mRect, "", aTarget.mKind, aTarget.mItem, aTarget.mDisabled);
		else if (aTarget.mKind == T_OPEN)
			DrawButton(g, aTarget.mRect, Str(mApp, "MODMENU_SETTINGS", "Settings"), T_OPEN, aTarget.mItem, aTarget.mDisabled);
	}
	g->PopState();

	// The footer: help and what is pending, or a question or a note, then the buttons
	int aSeparatorY = mPanel.mY + mPanel.mHeight - FOOTER_HEIGHT;
	Fill(g, Rect(mPanel.mX + 8, aSeparatorY, mPanel.mWidth - 16, 1), kButtonBorder);
	int aTextWidth = mPanel.mWidth - 24;
	std::vector<std::string> aLines;
	std::vector<const Color*> aColors;
	if (mConfirm != CONFIRM_NONE)
	{
		aLines = Wrap(mFont, mConfirmText, aTextWidth, 2);
		aColors.assign(aLines.size(), &kText);
	}
	else if (!mNote.empty())
	{
		aLines = Wrap(mFont, mNote, aTextWidth, 2);
		aColors.assign(aLines.size(), &kWarn);
	}
	else
	{
		aLines.push_back(Fit(mFont, FooterHelp(), aTextWidth));
		aColors.push_back(&kDim);
		aLines.push_back(Fit(mFont, PendingText(), aTextWidth));
		aColors.push_back(&kFocus);
	}
	for (size_t i = 0; i < aLines.size(); i++)
		Text(g, mFont, aLines[i], mPanel.mX + 12, aSeparatorY + 18 + (int) i * LINE, *aColors[i]);
	for (const Target& aTarget : mTargets)
	{
		if (aTarget.mInContent || (aTarget.mKind == T_SCROLL) || (aTarget.mItem == -2))
			continue;
		std::string aLabel;
		switch (aTarget.mKind)
		{
		case T_RESTART: aLabel = Str(mApp, "MODMENU_RESTART", "Restart game"); break;
		case T_CLOSE: aLabel = Str(mApp, "MODMENU_CLOSE", "Close"); break;
		case T_BACK: aLabel = Str(mApp, "MODMENU_BACK_BUTTON", "Back"); break;
		case T_RESET: aLabel = Str(mApp, "MODMENU_RESET", "Reset to defaults"); break;
		case T_FILES: aLabel = Str(mApp, "MODMENU_ADD_REMOVE", "Add or remove mods"); break;
		case T_YES: aLabel = Str(mApp, (mConfirm == CONFIRM_RESET) ? "MODMENU_RESET_YES" : "MODMENU_RESTART_YES", (mConfirm == CONFIRM_RESET) ? "Reset" : "Restart"); break;
		case T_NO: aLabel = Str(mApp, "MODMENU_CANCEL", "Cancel"); break;
		}
		DrawButton(g, aTarget.mRect, aLabel, aTarget.mKind, aTarget.mItem, aTarget.mDisabled);
	}

	// The focus ring
	if (const Target* aFocus = FindTarget(mFocusKind, mFocusItem))
	{
		g->PushState();
		if (aFocus->mInContent)
			g->ClipRect(aContent);
		Rect aRing(aFocus->mRect.mX - 2, aFocus->mRect.mY - 2, aFocus->mRect.mWidth + 4, aFocus->mRect.mHeight + 4);
		if (aFocus->mKind == T_ROW)
			aRing = Rect(aFocus->mRect.mX + 1, aFocus->mRect.mY + 1, aFocus->mRect.mWidth - 2, aFocus->mRect.mHeight - 2);
		Outline(g, aRing, kFocus, 2);
		g->PopState();
	}
}

//////////////////////////////////////////////////////////////////////////
// The script command "modmenu ..."

void ModMenu::RunScriptCommand(SexyAppBase* theApp, const ScriptCommand& theCommand)
{
	std::vector<size_t> anEnds;
	std::vector<std::string> aWords = SplitWords(theCommand.mArgs, &anEnds);
	std::string aWhat = aWords.empty() ? "" : aWords[0];
	std::string anError;
	ModManager& aManager = ModManager::Get();
	ModMenu* aMenu = theApp->mModMenu;

	if (aWhat == "open")
		theApp->OpenModMenu();
	else if (aWhat == "close")
		theApp->CloseModMenu();
	else if ((aWhat == "page") && (aWords.size() == 2))
	{
		if (theApp->mModMenu == NULL)
			theApp->OpenModMenu();
		aMenu = theApp->mModMenu;
		if (aMenu == NULL)
			anError = "the menu cannot open now";
		else if (aWords[1] == "list")
			aMenu->ShowList();
		else if (!aMenu->ShowMod(aWords[1]))
			anError = "there is no mod \"" + aWords[1] + "\"";
	}
	else if (((aWhat == "enable") || (aWhat == "move")) && (aWords.size() == 3))
	{
		JsonValue aValue((double) atoi(aWords[2].c_str()));
		if (aWhat == "enable")
			aValue = JsonValue(atoi(aWords[2].c_str()) != 0);
		if (aMenu != NULL)
			aMenu->DoEdit(aWhat, aWords[1], "", aValue);
		else
			anError = (aWhat == "enable") ? aManager.EditEnable(aWords[1], aValue.AsBool()) : aManager.EditMove(aWords[1], (int) aValue.AsNumber());
	}
	else if ((aWhat == "set") && (aWords.size() >= 4))
	{
		// The value is the rest of the line, as JSON: 3, true, "diamond", "#ff8800"
		std::string aText = theCommand.mArgs.substr(anEnds[2]);
		JsonValue aValue;
		std::string aParseError;
		if (!Platform::ParseJson(aText, aValue, &aParseError))
			anError = "the value is not JSON: " + aParseError;
		else if (aMenu != NULL)
			aMenu->DoEdit("set", aWords[1], aWords[2], aValue);
		else
			anError = aManager.EditSetting(aWords[1], aWords[2], aValue);
	}
	else if ((aWhat == "reset") && (aWords.size() == 2))
	{
		if (aMenu != NULL)
			aMenu->DoEdit("reset", aWords[1], "", JsonValue());
		else
			anError = aManager.EditReset(aWords[1]);
	}
	else if ((aWhat == "restart") && ((aWords.size() == 1) || ((aWords.size() == 2) && (aWords[1] == "confirm"))))
	{
		bool isConfirmed = aWords.size() == 2;
		if (aMenu != NULL)
			aMenu->PressRestart(isConfirmed);
		else
		{
			SexyString aMessage;
			int aResult = theApp->RequestRestart(isConfirmed, &aMessage);
			if (aResult == RESTART_CONFIRM)
				anError = "asks \"" + aMessage + "\" (modmenu restart confirm says yes)";
			else if (aResult == RESTART_BLOCKED)
				anError = "not now: " + aMessage;
		}
	}
	else if ((aWhat == "dump") && (aWords.size() == 2))
	{
		std::string aText = aManager.GetStateJson() + "\n";
		FILE* aFile = fopen(aWords[1].c_str(), "wb");
		bool isWritten = (aFile != NULL) && (fwrite(aText.data(), 1, aText.size(), aFile) == aText.size());
		if ((aFile != NULL) && (fclose(aFile) != 0))
			isWritten = false;
		if (!isWritten)
			anError = "cannot write " + aWords[1];
	}
	else
		anError = "expected open, close, page <mod>|list, enable <mod> 0|1, move <mod> -1|1, set <mod> <setting> <value>, "
			"reset <mod>, restart [confirm] or dump <file>";

	// An edit the menu made shows in its footer; the log says it too
	if (anError.empty() && (aMenu != NULL) && (aMenu == theApp->mModMenu) && !aMenu->mNote.empty() &&
		((aWhat == "enable") || (aWhat == "move") || (aWhat == "set") || (aWhat == "reset")))
		anError = aMenu->mNote;
	if (!anError.empty())
		SDL_Log("script: line %d: modmenu %s: %s", theCommand.mLine, aWhat.c_str(), anError.c_str());
}

//////////////////////////////////////////////////////////////////////////
// The web page's calls into the running game (docs/MOD_MENU_DESIGN.md 4.5; cmake/ByopopWeb.cmake exports them,
// web/modpanel.js calls them). They run between two of the game's frames, as SDL's input events do.

#ifdef __EMSCRIPTEN__
extern "C"
{

// The page's Mods button: theOn 1 opens the mod menu, 0 closes it, -1 toggles it. Returns 1 when it is open
// afterwards, 0 when not, -1 when no game runs (before callMain, after it quit).
EMSCRIPTEN_KEEPALIVE int byopop_menu(int theOn)
{
	SexyAppBase* anApp = gSexyAppBase;
	if ((anApp == NULL) || (!anApp->mInitialized) || (anApp->mShutdown))
		return -1;
	bool isOpen = anApp->IsModMenuOpen();
	bool wantOpen = (theOn < 0) ? !isOpen : (theOn != 0);
	if (wantOpen && !isOpen)
		anApp->OpenModMenu();
	else if (!wantOpen && isOpen)
	{
		anApp->mModMenu->CommitEdit();
		anApp->CloseModMenu();
	}
	return anApp->IsModMenuOpen() ? 1 : 0;
}

// The panel's Restart: SexyAppBase::RequestRestart. Returns {"result": "started"|"confirm"|"blocked"|"none",
// "message": "..."}: "confirm" asks theMessage first (a game is in progress; call again with isConfirmed),
// "blocked" says why not now, "none" when no game runs. Once started, the game shuts down and Module.onGameExit
// reloads the page.
EMSCRIPTEN_KEEPALIVE const char* byopop_restart(int isConfirmed)
{
	static std::string sResult;
	SexyAppBase* anApp = gSexyAppBase;
	JsonValue aResult = JsonValue::MakeObject();
	if ((anApp == NULL) || (!anApp->mInitialized) || (anApp->mShutdown))
		aResult.Set("result", JsonValue(std::string("none")));
	else
	{
		// a text being typed in the open menu counts, as it does for the menu's own Restart
		if (anApp->mModMenu != NULL)
			anApp->mModMenu->CommitEdit();
		SexyString aMessage;
		int aStatus = anApp->RequestRestart(isConfirmed != 0, &aMessage);
		const char* aName = (aStatus == RESTART_STARTED) ? "started" : (aStatus == RESTART_CONFIRM) ? "confirm" : "blocked";
		aResult.Set("result", JsonValue(std::string(aName)));
		aResult.Set("message", JsonValue(Platform::Latin1ToUtf8(SexyStringToString(aMessage))));
	}
	sResult = aResult.Dump(0);
	return sResult.c_str();
}

}
#endif
