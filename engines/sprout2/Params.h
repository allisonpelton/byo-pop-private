#ifndef __SPROUT2_PARAMS_H__
#define __SPROUT2_PARAMS_H__

// A set of named values (engine::Params, vtable 0x54f494 (feedingfrenzy2)): an XML element's attributes as the parser
// hands them to a document handler, read as strings, numbers, points, rectangles or colours. Each getter is added here
// as the code that calls it is translated.

#include "Geometry.h"
#include "Object.h"

#include <map>
#include <string>

namespace engine
{

// 0x4762b0 (feedingfrenzy2): a byte value, 0 to 255, as a colour channel
float channelOf(int theValue);

class Params : public Object
{
public:
	// 0x469320 (feedingfrenzy2)
	bool has(const std::string& theName) const { return mValues.find(theName) != mValues.end(); }
	// 0x44e9e0 (feedingfrenzy2)
	std::string getString(const std::string& theName, const std::string& theDefault) const;
	// 0x4547c0 (feedingfrenzy2): atol of the value
	int getInt(const std::string& theName, int theDefault) const;
	// 0x469540 (feedingfrenzy2): "true" or "false"; anything else, and no value, the default
	bool getBool(const std::string& theName, bool theDefault) const;
	// 0x453a50 (feedingfrenzy2): atof of the value, rounded to a float
	float getFloat(const std::string& theName, float theDefault) const;
	// 0x46d910 (feedingfrenzy2): two numbers, " %d , %d "; one the value does not hold keeps the default's
	Vector2<int> getIntPair(const std::string& theName, const Vector2<int>& theDefault) const;
	// 0x4572a0 (feedingfrenzy2): two numbers, " %f , %f "; one the value does not hold keeps the default's
	Vector2f getFloatPair(const std::string& theName, const Vector2f& theDefault) const;
	// 0x455d50 (feedingfrenzy2): a range, "( %f , %f )" when the value starts with a bracket, else one number for
	// both ends; a number the value does not hold keeps the default's (with one number, both ends take the low one)
	FloatRange getRange(const std::string& theName, const FloatRange& theDefault) const;
	// 0x472210 (feedingfrenzy2): " %d , %d , %d , %d " (left, top, right, bottom); one the value does not hold keeps
	// the default's
	RectI getRect(const std::string& theName, const RectI& theDefault) const;
	// 0x473180 (feedingfrenzy2): upperleft, topcenter, bottomcenter or center, any case (Pivot); anything else, and
	// no value, the centre
	int getPivot(const std::string& theName) const;
	// 0x472870 (feedingfrenzy2): " %d , %d , %d , %d ", each 0 to 255: with three, the default's alpha kept; with
	// fewer, the default
	Color getColor(const std::string& theName, const Color& theDefault) const;

	// The parser's: a value set, replacing one of the same name
	void set(const std::string& theName, const std::string& theValue) { mValues[theName] = theValue; }
	void clear() { mValues.clear(); }

private:
	std::map<std::string, std::string> mValues;  // +0x0c
};

} // namespace engine

#endif
