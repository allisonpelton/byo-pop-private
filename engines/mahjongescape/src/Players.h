#ifndef __MAHJONG_PLAYERS_H__
#define __MAHJONG_PLAYERS_H__

// The players (0x415ee0..0x416530 (mahjongchina), 0x416490..0x416af0 (mahjongjapan), the global at
// 0x532928 (mahjongchina), 0x5335d0 (mahjongjapan)), kept as <user folder>list.usr, the box that picks or deletes one
// (0x416740 (mahjongchina), 0x416af0 (mahjongjapan)) and the box that names a new one (0x416f70 (mahjongchina),
// 0x417330 (mahjongjapan)). Each player's settings and records are a User (User.h) of the player's id.

#include "Dialog.h"
#include "Stream.h"

#include <string>
#include <vector>

namespace jb
{
class ListBox;
class TextEdit;
}

namespace mahjong
{

// 0x415ee0 (mahjongchina), 0x416490 (mahjongjapan) (vtable 0x51c7f0 (mahjongchina), 0x51d908 (mahjongjapan))
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
	int mCurrent = -1;             // +0x1c the player's id (0x532944 (mahjongchina), 0x5335ec (mahjongjapan))
	bool mChanged = false;         // +0x20 (0x532948 (mahjongchina), 0x5335f0 (mahjongjapan)) a player was added

	// 0x415ee0 (mahjongchina), 0x416490 (mahjongjapan)
	PlayerList() {}
	// 0x415f10 (mahjongchina), 0x4164c0 (mahjongjapan), 0x415f30 (mahjongchina), 0x4164e0 (mahjongjapan)
	~PlayerList() override;

	// 0x415fe0 (mahjongchina), 0x416590 (mahjongjapan)
	void Serialize(jb::Stream& theStream) override;
	// 0x416130 (mahjongchina), 0x4166e0 (mahjongjapan): no players when the file is missing or bad
	void Load();
	// 0x416240 (mahjongchina), 0x416800 (mahjongjapan)
	void Save();
	// 0x4162f0 (mahjongchina), 0x4168b0 (mahjongjapan): the new player's id, the lowest free
	int Add(const char* theName);
	// 0x4163c0 (mahjongchina), 0x416980 (mahjongjapan)
	void Remove(int theId);
	// 0x416460 (mahjongchina), 0x416a20 (mahjongjapan)
	const char* GetName(int theId);
	// 0x4164b0 (mahjongchina), 0x416a70 (mahjongjapan): the current player's place, 0 when none
	int GetCurrentIndex();
	// 0x4164f0 (mahjongchina), 0x416ab0 (mahjongjapan): the id of the player at theIndex made current, or -1
	int SetCurrentIndex(int theIndex);

	int GetCount() const { return (int)mEntries.size(); }
};

extern PlayerList gPlayers;  // 0x532928 (mahjongchina), 0x5335d0 (mahjongjapan)

// 0x416740 (mahjongchina), 0x416af0 (mahjongjapan) (vtable 0x51c800 (mahjongchina), 0x51d918 (mahjongjapan), 100 bytes)
class PlayerBox : public Dialog, public DialogListener
{
public:
	jb::ListBox* mList = nullptr;  // +0x58
	PlayerList* mPlayers;          // +0x5c
	int mLastSelected = 0;         // +0x60

	// 0x416740 (mahjongchina), 0x416af0 (mahjongjapan)
	PlayerBox() : Dialog(199, 0xa5), mPlayers(&gPlayers) {}
	// 0x416780 (mahjongchina), 0x416b30 (mahjongjapan), 0x4167a0 (mahjongchina), 0x416b50 (mahjongjapan)
	~PlayerBox() override {}

	void Create() override;               // 0x4167c0 (mahjongchina), 0x416b70 (mahjongjapan)
	bool OnCommand(int theCmd) override;  // 0x416a00 (mahjongchina), 0x416db0 (mahjongjapan)
	void AfterActiveChange() override;    // 0x416cb0 (mahjongchina), 0x417070 (mahjongjapan)
	bool OnYes(int theId) override;       // 0x416d90 (mahjongchina), 0x417140 (mahjongjapan) the player deleted
	bool OnNo(int theId) override { return true; }  // 0x402750 (mahjongchina), 0x4026f0 (mahjongjapan)

	// 0x416cd0 (mahjongchina), 0x4170a0 (mahjongjapan): the list filled, the current player selected, or the last one
	// after one was added
	void Fill(bool theLast);
};

// 0x416f70 (mahjongchina), 0x417330 (mahjongjapan) (vtable 0x51c82c (mahjongchina), 0x51d944 (mahjongjapan), 100
// bytes): theFirst when no player exists yet (no cancel button)
class NewPlayerBox : public Dialog
{
public:
	jb::TextEdit* mEdit = nullptr;  // +0x54
	DialogButton* mOk = nullptr;    // +0x58
	DialogButton* mCancel = nullptr;  // +0x5c
	bool mFirst;                    // +0x60

	// 0x416f70 (mahjongchina), 0x417330 (mahjongjapan)
	explicit NewPlayerBox(bool theFirst) : Dialog(0xdf, theFirst ? 0xda : 0xb6), mFirst(theFirst) {}
	// 0x416fb0 (mahjongchina), 0x417370 (mahjongjapan), 0x416fd0 (mahjongchina), 0x417390 (mahjongjapan)
	~NewPlayerBox() override {}

	void Create() override;               // 0x416fe0 (mahjongchina), 0x4173a0 (mahjongjapan)
	bool OnCommand(int theCmd) override;  // 0x4171b0 (mahjongchina), 0x417570 (mahjongjapan)
	void OnTextEnter() override { Accept(); }  // 0x4171e0 (mahjongchina), 0x4175a0 (mahjongjapan)

	// 0x4171f0 (mahjongchina), 0x4175b0 (mahjongjapan)
	void Accept();
};

} // namespace mahjong

#endif
