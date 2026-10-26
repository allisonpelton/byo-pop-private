// port(variant): the replay browser; see ReplayBrowserDialog.h. Not part of the original game.

#include "ReplayBrowserDialog.h"
#include "WinDMApp.h"
#include "GameConfig.h"
#include "Replay.h"
#include "ReplayRecorder.h"
#include "Res.h"

#include "DialogButton.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "ListWidget.h"
#include "ScrollbarWidget.h"
#include "WidgetManager.h"
#include "Common.h"
#include "platform/Json.h"
#include "platform/UserFiles.h"
#include "platform/VFS.h"

#include <algorithm>
#include <cstdio>

using namespace Sexy;

// Background, outline, text, hilited text, selection, selected text: the game-over panel's purple and pinks
static int gReplayListColors[6][3] =
{
	{21, 0, 56}, {252, 0, 195}, {255, 100, 220}, {255, 255, 255}, {120, 0, 110}, {255, 255, 0}
};

ReplayBrowserDialog::ReplayBrowserDialog(WinDMApp* theApp) :
	Dialog(theApp->mImages[IMAGE_DIALOGBOX], theApp->mImages[IMAGE_BOXBUTTON], DIALOG_REPLAYS, true,
		theApp->GetString("REPLAYSDLG_REPLAYS", _S("REPLAYS")), _S(""), theApp->GetString("REPLAYSDLG_CLOSE", _S("Close")), BUTTONS_FOOTER)
{
	mApp = theApp;
	SetButtonFont(mApp->mMainFont);
	SetHeaderFont(mApp->mHeaderFont);
	SetLinesFont(mApp->mMainFont);

	mList = new ListWidget(LIST_ID, mApp->mMainFont, this);
	mList->SetColors(gReplayListColors, 6);
	mList->mDrawOutline = true;
	mList->mJustify = ListWidget::JUSTIFY_LEFT;
	mScrollbar = new ScrollbarWidget(SCROLLBAR_ID, mList);
	mScrollbar->SetInvisIfNoScroll(true);
	mList->mScrollbar = mScrollbar;

	mWatchButton = new DialogButton(mApp->mImages[IMAGE_BOXBUTTON], WATCH_BUTTON_ID, this);
	mWatchButton->SetFont(mApp->mMainFont);
	mWatchButton->mLabel = mApp->GetString("REPLAYSDLG_WATCH", _S("Watch"));
	mKeepButton = new DialogButton(mApp->mImages[IMAGE_BOXBUTTON], KEEP_BUTTON_ID, this);
	mKeepButton->SetFont(mApp->mMainFont);
	mDeleteButton = new DialogButton(mApp->mImages[IMAGE_BOXBUTTON], DELETE_BUTTON_ID, this);
	mDeleteButton->SetFont(mApp->mMainFont);
	mDeleteButton->mLabel = mApp->GetString("REPLAYSDLG_DELETE", _S("Delete"));

	Fill("");
}

ReplayBrowserDialog::~ReplayBrowserDialog()
{
	delete mList;
	delete mScrollbar;
	delete mWatchButton;
	delete mKeepButton;
	delete mDeleteButton;
}

// The folder's replays, newest first (by the name's time, then its copy number: "-2" came after the name it
// avoided), each read for its seed and title
void ReplayBrowserDialog::Fill(const std::string& theSelect)
{
	struct Found
	{
		std::string	mName;
		bool		mIsPinned;
		int			mCopy;
		bool operator<(const Found& o) const
		{
			int aStamp = mName.compare(0, 15, o.mName, 0, 15);
			if (aStamp != 0)
				return aStamp > 0;
			if (mCopy != o.mCopy)
				return mCopy > o.mCopy;
			return mName > o.mName;
		}
	};
	const std::string& aDir = gGameConfig.mReplayDir;
	std::vector<Found> aFound;
	std::vector<std::string> aNames = aDir.empty() ? std::vector<std::string>() : Platform::ListDirectory(aDir);
	for (size_t i = 0; i < aNames.size(); i++)
	{
		Found aFile;
		aFile.mName = aNames[i];
		aFile.mIsPinned = false;
		aFile.mCopy = 1;
		if (ReplayRecorder::IsReplayName(aNames[i], &aFile.mIsPinned, &aFile.mCopy))
			aFound.push_back(aFile);
	}
	std::sort(aFound.begin(), aFound.end());

	mList->RemoveAll();
	mRows.clear();
	int aSelect = -1;
	for (size_t i = 0; i < aFound.size(); i++)
	{
		// <yyyymmdd>-<hhmmss>-<normal|timed>-<score>...: "2026-10-25 14:03  Normal  12345"
		const std::string& aName = aFound[i].mName;
		size_t aModeEnd = aName.find('-', 16);
		size_t aScoreEnd = aName.find_first_not_of("0123456789", aModeEnd + 1);
		bool isTimed = (aName.compare(16, 5, "timed") == 0);
		SexyString aLine = SexyString(aFound[i].mIsPinned ? "* " : "  ") + aName.substr(0, 4) + "-" + aName.substr(4, 2) + "-" +
			aName.substr(6, 2) + " " + aName.substr(9, 2) + ":" + aName.substr(11, 2) + "  " +
			(isTimed ? mApp->GetString("REPLAYSDLG_TIMED", _S("Timed")) : mApp->GetString("REPLAYSDLG_NORMAL", _S("Normal"))) + "  " +
			aName.substr(aModeEnd + 1, aScoreEnd - aModeEnd - 1);
		std::string aBytes;
		Replay::Summary aSummary;
		if (Platform::ReadHostFile(aDir + "/" + aName, &aBytes) && Replay::Peek(aBytes, &aSummary))
		{
			if (aSummary.mHasSeed)
				aLine += "  " + mApp->GetString("REPLAYSDLG_SEED", _S("seed")) + StrFormat(_S(" %u"), (unsigned) aSummary.mSeed);
			Platform::JsonValue aMeta;
			if (Platform::ParseJson(aSummary.mMeta, aMeta) && aMeta["title"].IsString())
				aLine += "  " + UTF8ToLatin1(aMeta["title"].AsString(), '?');
		}
		mList->AddLine(aLine, false);
		Row aRow;
		aRow.mName = aName;
		aRow.mIsPinned = aFound[i].mIsPinned;
		mRows.push_back(aRow);
		if (aName == theSelect)
			aSelect = (int) i;
	}
	if ((aSelect < 0) && !mRows.empty())
		aSelect = 0;
	mList->SetSelect(aSelect);
	UpdateButtons();
}

void ReplayBrowserDialog::UpdateButtons()
{
	int aSelect = mList->mSelectIdx;
	bool hasRow = (aSelect >= 0) && (aSelect < (int) mRows.size());
	mKeepButton->mLabel = (hasRow && mRows[aSelect].mIsPinned) ? mApp->GetString("REPLAYSDLG_UNKEEP", _S("Unkeep")) :
		mApp->GetString("REPLAYSDLG_KEEP", _S("Keep"));
	mWatchButton->SetDisabled(!hasRow);
	mKeepButton->SetDisabled(!hasRow);
	mDeleteButton->SetDisabled(!hasRow);
	mKeepButton->MarkDirty();
	MarkDirty();
}

std::string ReplayBrowserDialog::SelectedPath()
{
	int aSelect = mList->mSelectIdx;
	if ((aSelect < 0) || (aSelect >= (int) mRows.size()))
		return "";
	return gGameConfig.mReplayDir + "/" + mRows[aSelect].mName;
}

int ReplayBrowserDialog::ListHeight()
{
	return mApp->mMainFont->GetHeight() * VISIBLE_ROWS + 8;
}

int ReplayBrowserDialog::GetPreferredHeight(int theWidth)
{
	// The header, the list, the button row, Close
	return 56 + ListHeight() + 10 + 32 + 72;
}

void ReplayBrowserDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mList);
	theWidgetManager->AddWidget(mScrollbar);
	theWidgetManager->AddWidget(mWatchButton);
	theWidgetManager->AddWidget(mKeepButton);
	theWidgetManager->AddWidget(mDeleteButton);
}

void ReplayBrowserDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mList);
	theWidgetManager->RemoveWidget(mScrollbar);
	theWidgetManager->RemoveWidget(mWatchButton);
	theWidgetManager->RemoveWidget(mKeepButton);
	theWidgetManager->RemoveWidget(mDeleteButton);
}

void ReplayBrowserDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);
	int aListHeight = ListHeight();
	mList->Resize(mX + 24, mY + 56, mWidth - 48 - 16, aListHeight);
	mScrollbar->ResizeScrollbar(mX + mWidth - 24 - 16, mY + 56, 16, aListHeight);
	// The three-button row as in Options' locked build: (mWidth-60)/3 each, 6 apart
	int aButtonWidth = (mWidth - 60) / 3;
	int aButtonY = mY + 56 + aListHeight + 10;
	mWatchButton->Resize(mX + 24, aButtonY, aButtonWidth, 32);
	mKeepButton->Resize(mX + 30 + aButtonWidth, aButtonY, aButtonWidth, 32);
	mDeleteButton->Resize(mX + 36 + 2 * aButtonWidth, aButtonY, aButtonWidth, 32);
}

void ReplayBrowserDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);
	if (mRows.empty())
	{
		SexyString aText = mApp->GetString("REPLAYSDLG_NONE", _S("No replays yet."));
		g->SetFont(mApp->mMainFont);
		g->SetColor(Color(255, 255, 0));
		g->DrawString(aText, (mWidth - mApp->mMainFont->StringWidth(aText)) / 2, 56 + ListHeight() / 2);
	}
}

void ReplayBrowserDialog::ButtonDepress(int theId)
{
	Dialog::ButtonDepress(theId);
	if (SelectedPath().empty())
		return;
	if (theId == WATCH_BUTTON_ID)
		mApp->WatchReplay(SelectedPath(), true);
	else if (theId == KEEP_BUTTON_ID)
		ToggleKeep();
	else if (theId == DELETE_BUTTON_ID)
		mApp->ConfirmDeleteReplay();
}

void ReplayBrowserDialog::ListClicked(int theId, int theIdx, int theClickCount)
{
	if ((theIdx < 0) || (theIdx >= (int) mRows.size()))
		return;
	mList->SetSelect(theIdx);
	UpdateButtons();
	if (theClickCount == 2)
		mApp->WatchReplay(SelectedPath(), true);
}

// "<base>.bjr" and "<base>-keep.bjr" trade places; a name that is taken leaves both as they are
void ReplayBrowserDialog::ToggleKeep()
{
	const Row& aRow = mRows[mList->mSelectIdx];
	std::string aBase = aRow.mName.substr(0, aRow.mName.size() - (aRow.mIsPinned ? 9 : 4));
	std::string aNewName = aBase + (aRow.mIsPinned ? ".bjr" : "-keep.bjr");
	std::string anOld = gGameConfig.mReplayDir + "/" + aRow.mName;
	std::string aNew = gGameConfig.mReplayDir + "/" + aNewName;
	FILE* anExisting = fopen(aNew.c_str(), "rb");
	if (anExisting != NULL)
	{
		fclose(anExisting);
		return;
	}
	if (rename(anOld.c_str(), aNew.c_str()) != 0)
		return;
	ReplayRecorder::Renamed(anOld, aNew);
	Platform::UserDataChanged();
	Fill(aNewName);
}

void ReplayBrowserDialog::DeleteSelected()
{
	std::string aPath = SelectedPath();
	if (aPath.empty())
		return;
	int aSelect = mList->mSelectIdx;
	remove(aPath.c_str());
	Platform::UserDataChanged();
	// The row that takes its place stays selected
	std::string aNext = (aSelect + 1 < (int) mRows.size()) ? mRows[aSelect + 1].mName : (aSelect > 0) ? mRows[aSelect - 1].mName : "";
	Fill(aNext);
}
