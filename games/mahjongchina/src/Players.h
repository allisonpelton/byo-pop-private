#ifndef __MAHJONGCHINA_PLAYERS_H__
#define __MAHJONGCHINA_PLAYERS_H__

// The players (0x416490..0x416af0, the global at 0x5335d0), kept as <user folder>list.usr, the box that picks or
// deletes one (0x416af0) and the box that names a new one (0x417330). Each player's settings and records are a
// User (User.h) of the player's id.

#include "Dialog.h"
#include "Stream.h"

#include <string>
#include <vector>

namespace jb
{
class ListBox;
class TextEdit;
}

namespace mahjongchina
{

// 0x416490 (vtable 0x51d908)
class PlayerList : public jb::Streamable
{
public:
	// 0xc bytes
	struct Entry
	{
		int mId = 0;        // +0x00
		std::string mName;  // +0x04
	};

	std::vector<Entry*> mEntries;  // +0x04
	int mMagic = 0;                // +0x14 'IL'
	int mVersion = 0;              // +0x18
	int mCurrent = -1;             // +0x1c the player's id (0x5335ec)
	bool mChanged = false;         // +0x20 (0x5335f0) a player was added

	// 0x416490
	PlayerList() {}
	// 0x4164c0, 0x4164e0
	~PlayerList() override;

	// 0x416590
	void Serialize(jb::Stream& theStream) override;
	// 0x4166e0: no players when the file is missing or bad
	void Load();
	// 0x416800
	void Save();
	// 0x4168b0: the new player's id, the lowest free
	int Add(const char* theName);
	// 0x416980
	void Remove(int theId);
	// 0x416a20
	const char* GetName(int theId);
	// 0x416a70: the current player's place, 0 when none
	int GetCurrentIndex();
	// 0x416ab0: the id of the player at theIndex made current, or -1
	int SetCurrentIndex(int theIndex);

	int GetCount() const { return (int)mEntries.size(); }
};

extern PlayerList gPlayers;  // 0x5335d0

// 0x416af0 (vtable 0x51d918, 100 bytes)
class PlayerBox : public Dialog, public DialogListener
{
public:
	jb::ListBox* mList = nullptr;  // +0x58
	PlayerList* mPlayers;          // +0x5c
	int mLastSelected = 0;         // +0x60

	// 0x416af0
	PlayerBox() : Dialog(199, 0xa5), mPlayers(&gPlayers) {}
	// 0x416b30, 0x416b50
	~PlayerBox() override {}

	void Create() override;               // 0x416b70
	bool OnCommand(int theCmd) override;  // 0x416db0
	void AfterActiveChange() override;    // 0x417070
	bool OnYes(int theId) override;       // 0x417140 the player deleted
	bool OnNo(int theId) override { return true; }  // 0x4026f0

	// 0x4170a0: the list filled, the current player selected, or the last one after one was added
	void Fill(bool theLast);
};

// 0x417330 (vtable 0x51d944, 100 bytes): theFirst when no player exists yet (no cancel button)
class NewPlayerBox : public Dialog
{
public:
	jb::TextEdit* mEdit = nullptr;  // +0x54
	DialogButton* mOk = nullptr;    // +0x58
	DialogButton* mCancel = nullptr;  // +0x5c
	bool mFirst;                    // +0x60

	// 0x417330
	explicit NewPlayerBox(bool theFirst) : Dialog(0xdf, theFirst ? 0xda : 0xb6), mFirst(theFirst) {}
	// 0x417370, 0x417390
	~NewPlayerBox() override {}

	void Create() override;               // 0x4173a0
	bool OnCommand(int theCmd) override;  // 0x417570
	void OnTextEnter() override { Accept(); }  // 0x4175a0

	// 0x4175b0
	void Accept();
};

} // namespace mahjongchina

#endif
