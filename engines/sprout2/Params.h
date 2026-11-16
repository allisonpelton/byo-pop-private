#ifndef __SPROUT2_PARAMS_H__
#define __SPROUT2_PARAMS_H__

// A set of named values (engine::Params, vtable 0x54f494 (feedingfrenzy2)): an XML element's attributes as the parser
// hands them to a document handler, read as strings, numbers, points, rectangles or colours. Each getter is added here
// as the code that calls it is translated.

#include "Object.h"

#include <map>
#include <string>

namespace engine
{

class Params : public Object
{
public:
	// 0x469320 (feedingfrenzy2)
	bool has(const std::string& theName) const { return mValues.find(theName) != mValues.end(); }
	// 0x44e9e0 (feedingfrenzy2)
	std::string getString(const std::string& theName, const std::string& theDefault) const;
	// 0x4547c0 (feedingfrenzy2): atol of the value
	int getInt(const std::string& theName, int theDefault) const;

	// The parser's: a value set, replacing one of the same name
	void set(const std::string& theName, const std::string& theValue) { mValues[theName] = theValue; }
	void clear() { mValues.clear(); }

private:
	std::map<std::string, std::string> mValues;  // +0x0c
};

} // namespace engine

#endif
