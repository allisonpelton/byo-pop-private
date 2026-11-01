#include "Players.h"

#include "App.h"
#include "JBBase.h"
#include "Manager.h"
#include "User.h"
#include "Widgets.h"

#include <filesystem>

namespace mahjongchina
{

PlayerList gPlayers;

// 0x4164e0 (the original frees the names but not the entries)
PlayerList::~PlayerList()
{
	for (Entry* anEntry : mEntries)
		delete anEntry;
	mEntries.clear();
}

// 0x416590
void PlayerList::Serialize(jb::Stream& theStream)
{
	mMagic = 0x4c49;
	mVersion = 1;
	theStream.Short(mMagic);
	theStream.Char(mVersion);
	theStream.Short(mCurrent);
	int aCount = (int)mEntries.size();
	theStream.Short(aCount);
	int i = 0;
	if (!theStream.IsReading())
	{
		if (0 < aCount)
		{
			do
			{
				theStream.Short(mEntries[i]->mId);
				theStream.String(mEntries[i]->mName);
				i++;
			} while (i < aCount);
		}
	}
	else if (0 < aCount)
	{
		do
		{
			mEntries.push_back(new Entry());
			theStream.Short(mEntries[i]->mId);
			theStream.String(mEntries[i]->mName);
			i++;
		} while (i < aCount);
	}
	theStream.Checksum();
}

// 0x4166e0 (the original drops the entries of a bad file without freeing them)
void PlayerList::Load()
{
	jb::Stream aStream;
	std::string aPath = jb::GetUserFolder() + "list.usr";
	if (aStream.OpenRead(aPath.c_str()))
	{
		aStream.Serialize(this);
		if (mMagic == 0x4c49 && mVersion == 1 && aStream.ChecksumOk())
			return;
	}
	for (Entry* anEntry : mEntries)
		delete anEntry;
	mEntries.clear();
}

// 0x416800
void PlayerList::Save()
{
	jb::Stream aStream;
	std::string aPath = jb::GetUserFolder() + "list.usr";
	if (aStream.OpenWrite(aPath.c_str()))
		aStream.Serialize(this);
}

// 0x4168b0
int PlayerList::Add(const char* theName)
{
	int aCount = (int)mEntries.size();
	int anId = 0;
	int i = 0;
	if (0 < aCount)
	{
		do
		{
			if (mEntries[i]->mId == anId)
			{
				anId = anId + 1;
				i = 0;
			}
			else
				i = i + 1;
		} while (i < aCount);
	}
	mEntries.push_back(new Entry());
	mEntries[aCount]->mId = anId;
	mEntries[aCount]->mName = theName;
	mChanged = true;
	return anId;
}

// 0x416980 (the original frees the name but not the entry)
void PlayerList::Remove(int theId)
{
	int aCount = (int)mEntries.size();
	int i = 0;
	if (0 < aCount)
	{
		while (mEntries[i]->mId != theId)
		{
			i = i + 1;
			if (aCount <= i)
				return;
		}
		delete mEntries[i];
		mEntries.erase(mEntries.begin() + i);
	}
}

// 0x416a20
const char* PlayerList::GetName(int theId)
{
	int aCount = (int)mEntries.size();
	int i = 0;
	if (0 < aCount)
	{
		do
		{
			if (mEntries[i]->mId == theId)
				return mEntries[i]->mName.c_str();
			i = i + 1;
		} while (i < aCount);
	}
	return "Player 1";
}

// 0x416a70
int PlayerList::GetCurrentIndex()
{
	int aCount = (int)mEntries.size();
	int i = 0;
	if (0 < aCount)
	{
		do
		{
			if (mEntries[i]->mId == mCurrent)
				return i;
			i = i + 1;
		} while (i < aCount);
	}
	return 0;
}

// 0x416ab0
int PlayerList::SetCurrentIndex(int theIndex)
{
	if ((unsigned)theIndex < mEntries.size())
	{
		mCurrent = mEntries[theIndex]->mId;
		return mCurrent;
	}
	mCurrent = -1;
	return mCurrent;
}

// 0x416b70
void PlayerBox::Create()
{
	int aLeft[] = {3, 3, 0};
	int aRight[] = {3, 3, 0};
	int aTop[] = {1, 3, 2, 1, 0};
	int aBottom[] = {3, 1, 1, 2, 0};
	SetCorners(0, 1, 1, 0);
	SetEdges(aLeft, aRight, aTop, aBottom);
	SetTitle(0xab);
	mList = new jb::ListBox(4, 0x118, 0x17, 6, 1);
	Add(mList, 0x32, 0x46);
	mList->SetSelColour(0x81060);
	mList->SetFrame(0x304838, 0x90b8a8, 0x102860);
	mList->SetFont(0x122, 0x121, 5, 0x11, 0x40);
	mList->SetScroll(0x125, 0x126);
	Fill(false);
	DialogButton* aButton = new DialogButton(3, 0x66);
	Add(aButton, 0x2d, 0xf0);
	aButton->SetText(0xa7);
	aButton = new DialogButton(1, 0x66);
	Add(aButton, 0x9b, 0xf0);
	aButton->SetText(0xa8);
	aButton = new DialogButton(2, 0x66);
	Add(aButton, 0x109, 0xf0);
	aButton->SetText(0xa6);
}

// 0x416db0
bool PlayerBox::OnCommand(int theCmd)
{
	int aSelected;
	switch (theCmd - 1)
	{
	case 0:
		if (mPlayers->GetCount() != 0)
		{
			aSelected = mList->GetSelected();
			int anId = mList->GetData(aSelected);
			mPlayers->mCurrent = anId;
			gUser.Load(anId, mList->GetText(aSelected).c_str());
			mPlayers->Save();
			jb::gManager->Pop(true, 1);
			return true;
		}
		break;
	case 1:
		if (mPlayers->GetCount() != 0)
		{
			jb::gManager->Pop(true, 1);
			return true;
		}
		break;
	case 2:
		aSelected = mList->GetSelected();
		if (0 < aSelected)
		{
			YesNoBox* aBox = new YesNoBox(this, 1, 1, 0x1b, 0x16, true, 0x66, 0x66);
			aBox->SetPos(0xd4, 0xb4);
			jb::gManager->Push(aBox, true, false, true);
			aBox->SetTitle(0xa4);
			std::string aText = jb::FormatDll(0xa3, mList->GetText(aSelected).c_str());
			aBox->AddText(aText.c_str(), 0, 0, 0x4d, false, nullptr, 0);
			aText = jb::LoadDllString(0xa5);
			aBox->AddText(aText.c_str(), 0, 0, 0x6b, false, nullptr, 0);
		}
		return true;
	case 3:
		aSelected = mList->GetSelected();
		if (aSelected != 0)
		{
			mLastSelected = aSelected;
			return true;
		}
		mList->SetSelected(mLastSelected);
		jb::gManager->Push(new NewPlayerBox(false), true, false, true);
		return true;
	default:
		return false;
	}
	ShowMessage(0x30, 0xe4, 0xaa, 2);
	return true;
}

// 0x417070
void PlayerBox::AfterActiveChange()
{
	if (IsActive() && mPlayers->mChanged)
	{
		mPlayers->mChanged = false;
		Fill(true);
	}
}

// 0x4170a0
void PlayerBox::Fill(bool theLast)
{
	mList->ClearItems();
	mList->Add(0xa9, 0xffff);
	int aCount = mPlayers->GetCount();
	int i = 0;
	if (0 < aCount)
	{
		do
		{
			PlayerList::Entry* anEntry = mPlayers->mEntries[i];
			mList->Add(anEntry->mName.c_str(), anEntry->mId);
			i++;
		} while (i < aCount);
	}
	int aSelected;
	if (!theLast)
	{
		int anIndex = mPlayers->GetCurrentIndex();
		if (anIndex == -1)
			aSelected = 0;
		else
			aSelected = anIndex + 1;
	}
	else
		aSelected = mList->GetCount() - 1;
	mList->SetSelected(aSelected);
	mLastSelected = mList->GetSelected();
}

// 0x417250: every file of the pattern in theFolder deleted (FindFirstFileA, FindNextFileA)
static void DeleteFiles(const char* thePattern, const char* theFolder)
{
	std::string aPattern = thePattern;
	size_t aSlash = aPattern.find_last_of("\\/");
	std::string aName = aSlash == std::string::npos ? aPattern : aPattern.substr(aSlash + 1);
	size_t aStar = aName.find('*');
	std::string aPrefix = aName.substr(0, aStar);
	std::string aSuffix = aStar == std::string::npos ? std::string() : aName.substr(aStar + 1);
	std::error_code anError;
	std::filesystem::directory_iterator anIt(theFolder, anError);
	if (anError)
		return;
	std::vector<std::string> aFiles;
	for (const std::filesystem::directory_entry& anEntry : anIt)
	{
		std::string aFile = anEntry.path().filename().string();
		if (aStar == std::string::npos ? aFile == aName :
			aFile.size() >= aPrefix.size() + aSuffix.size() && aFile.compare(0, aPrefix.size(), aPrefix) == 0 &&
			aFile.compare(aFile.size() - aSuffix.size(), aSuffix.size(), aSuffix) == 0)
			aFiles.push_back(aFile);
	}
	for (const std::string& aFile : aFiles)
	{
		std::string aPath = std::string(theFolder) + aFile;
		jb::RemoveFile(aPath.c_str());
	}
}

// 0x417140
bool PlayerBox::OnYes(int theId)
{
	int anId = mList->GetData(mList->GetSelected());
	std::string aPath;
	User::GetUserPath(anId, aPath);
	jb::RemoveFile(aPath.c_str());
	std::string aFolder;
	User::GetSavePattern(anId, aPath, aFolder);
	DeleteFiles(aPath.c_str(), aFolder.c_str());
	mPlayers->Remove(anId);
	mList->Remove(mList->GetSelected());
	if (anId == mPlayers->mCurrent)
		mPlayers->SetCurrentIndex(0);
	mPlayers->Save();
	mLastSelected = mList->GetSelected();
	return true;
}

// 0x4173a0
void NewPlayerBox::Create()
{
	int aLeft[] = {1, 0};
	int aRight[] = {1, 0};
	int aTop[] = {1, 3, 2, 0};
	int aBottom[] = {3, 1, 2, 0};
	SetCorners(0, 1, 1, 0);
	SetEdges(aLeft, aRight, aTop, aBottom);
	SetTitle(0xad);
	mEdit = new jb::TextEdit(0x10e, 0x16, 100, false);
	AddFocusable(mEdit, 0x30, 0x46);
	mEdit->SetFont(0x122, 0x121, 0x10, 0x40);
	mEdit->SetSelColour(0x81060);
	mEdit->SetFrame(0x304838, 0x90b8a8, 0x102860);
	int anOffset = mFirst ? 0x37 : 0;
	mOk = new DialogButton(1, 0x66);
	Add(mOk, anOffset + 0x4c, 0x75);
	mOk->SetText(0xa8);
	if (!mFirst)
	{
		mCancel = new DialogButton(2, 0x66);
		Add(mCancel, anOffset + 0xba, 0x75);
		mCancel->SetText(0xa6);
	}
}

// 0x417570
bool NewPlayerBox::OnCommand(int theCmd)
{
	if (theCmd == 1)
	{
		Accept();
		return true;
	}
	if (theCmd - 2 != 0)
		return false;
	jb::gManager->Pop(true, 1);
	return true;
}

// 0x4175b0
void NewPlayerBox::Accept()
{
	std::string aName = mEdit->GetText();
	if (!aName.empty())
	{
		int aCount = gPlayers.GetCount();
		int anId = gPlayers.Add(aName.c_str());
		if (aCount == 0)
			gPlayers.mCurrent = anId;
		gPlayers.Save();
		jb::gManager->Pop(true, 1);
	}
	else
		ShowMessage(0x2b, 0x6e, 0xac, 2);
}

} // namespace mahjongchina
