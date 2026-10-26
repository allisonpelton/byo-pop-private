// Sexy::PopAnimManager (0x20 bytes, constructor 0x442f90, vtable 0x7adc30; the app's, +0x7c8): the scripts'
// animations, by the ids the scripts give them (sexy.LoadPAMFile and the other sexy.*PopAnim functions,
// LuaInterface.cpp). It is the animations' listener: their stops and commands go to the Lua function each was
// loaded with, a path such as "PAMAnimator.Callback", called with the id and two strings.
#pragma once

#include <map>
#include <string>

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
	// (+0x10: a list of ids that the original's add filled when asked to; nothing read it)

	~PopAnimManager() override;  // 0x485740: deletes the animations

	// 0x485830
	void Add(int theId, PopAnim* theAnim, const std::string& theCallback);
	// 0x485de0
	bool Has(int theId) const { return mAnims.count(theId) != 0; }
	// 0x486310: NULL when there is none
	PopAnim* Get(int theId) const;

	// The scripts' operations. A missing id is a script error (the original also showed it in a dialog).
	void Update(lua_State* L, int theId);                         // 0x485ac0
	void Draw(lua_State* L, Graphics* g, int theId, int theX, int theY);  // 0x485c40
	void ChangeCallback(lua_State* L, int theId, const std::string& theCallback);  // 0x485930
	// 0x486100: tells PAMAnimator.AnimDeleted, then deletes the animation
	void Delete(int theId);

	// PopAnimListener (vf00 0x485e30, vf01 0x485fa0)
	void PopAnimStopped(int theId) override;
	void PopAnimCommand(int theId, const std::string& theCommand, const std::string& theParam) override;
};

}  // namespace Sexy
