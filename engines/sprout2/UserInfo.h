#ifndef __SPROUT2_USERINFO_H__
#define __SPROUT2_USERINFO_H__

// A player (UserInfo, vtable 0x55a97c (feedingfrenzy2), 0x50 bytes; its constructor 0x4901e0 (feedingfrenzy2)): a name
// and the parameters the game keeps for the player, each a name and a text, which are the string values of the
// player's registry key (UserManager).

#include "Object.h"
#include "Params.h"

#include <string>

namespace engine
{

class UserInfo : public Object
{
public:
	const std::string& getName() const { return mName; }
	// 0x4547b0 (feedingfrenzy2)
	void setName(const std::string& theName) { mName = theName; }
	Params& getParams() { return mParams; }

private:
	std::string mName;  // +0x0c
	Params mParams;     // +0x28
};

} // namespace engine

#endif
