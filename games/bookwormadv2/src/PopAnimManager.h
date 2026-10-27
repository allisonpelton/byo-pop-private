// Sexy::PopAnimManager (0x20 bytes, constructor V1:0x442f90, vtable 0x6fbd9c; the app's, +0x7c8): the scripts'
// animations, by the ids the scripts give them (sexy.LoadPAMFile and the other sexy.*PopAnim functions,
// LuaInterface.cpp). It is the animations' listener: their stops and commands go to the Lua function each was
// loaded with, a path such as "PAMAnimator.Callback", called with the id and two strings.
#pragma once

#include <map>
#include <string>
#include <vector>

#include "PopAnim.h"
#include "lua.h"

namespace Sexy {

class PopAnimManager : public PopAnimListener {
public:
	struct Entry {
		PopAnim* mAnim;
		std::string mCallback;
	};
	std::map<int, Entry> mAnims;  // +0x04
	std::vector<int> mClones;  // +0x10: the ids of sexy.ClonePAM's animations, deleted when a battle ends

	~PopAnimManager() override;  // 0x475540: deletes the animations

	// V1:0x485830; theClone: one of mClones
	void Add(int theId, PopAnim* theAnim, const std::string& theCallback, bool theClone = false);
	// 0x475bc0
	bool Has(int theId) const { return mAnims.count(theId) != 0; }
	// 0x476090: NULL when there is none
	PopAnim* Get(int theId) const;

	// The scripts' operations. A missing id is a script error (the original also showed it in a dialog).
	void Update(lua_State* L, int theId);                         // V1:0x485ac0
	void Draw(lua_State* L, Graphics* g, int theId, int theX, int theY);  // V1:0x485c40
	void ChangeCallback(lua_State* L, int theId, const std::string& theCallback);  // V1:0x485930
	// V1:0x486100: tells PAMAnimator.AnimDeleted, then deletes the animation
	void Delete(int theId);
	// V1:0x486370: Delete for each of mClones, then none
	void DeleteClones();

	// PopAnimListener (vf00 0x475c10, vf01 0x475d80)
	void PopAnimStopped(int theId) override;
	void PopAnimCommand(int theId, const std::string& theCommand, const std::string& theParam) override;
};

}  // namespace Sexy
