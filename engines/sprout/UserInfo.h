#ifndef __SPROUT_USERINFO_H__
#define __SPROUT_USERINFO_H__

// A player (0x50 bytes, constructor 0x40d280 (pizzafrenzy); Iggle Pop's Mac build names the class UserInfo): a name and
// the parameters the game saves for the player, each a name and a text.

#include "Object.h"
#include "Xml.h"

#include <string>

namespace engine
{

// The parameters (the exe's Params, 0x28 bytes of a UserInfo): the texts by name, read as the attributes of an element
// are, written by the setters
class Params : public XmlAttributes
{
public:
	void setString(const std::string& theName, const std::string& theValue);  // 0x4769b0 (pizzafrenzy)
	void setInt(const std::string& theName, int theValue);                    // 0x4769d0 (pizzafrenzy)
	void setFloat(const std::string& theName, float theValue);                // 0x476a80 (pizzafrenzy)
	void setBool(const std::string& theName, bool theValue);                  // 0x476b30 (pizzafrenzy)
};

class UserInfo : public Object
{
public:
	const std::string& getName() const { return mName; }           // 0x476bf0 (pizzafrenzy)
	void setName(const std::string& theName) { mName = theName; }  // 0x476c00 (pizzafrenzy)
	Params& getParams() { return mParams; }                        // 0x411480 (pizzafrenzy)

private:
	std::string mName;  // +0x0c
	Params mParams;     // +0x28
};

} // namespace engine

#endif
