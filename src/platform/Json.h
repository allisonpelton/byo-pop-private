#ifndef __PLATFORM_JSON_H__
#define __PLATFORM_JSON_H__

// Small JSON reader/writer for mod manifests, tunables and settings (no exceptions, no dependencies).

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Platform
{

class JsonValue
{
public:
	enum Type { Null, Bool, Number, String, Array, Object };

	JsonValue() = default;
	explicit JsonValue(bool b) : mType(Bool), mBool(b) {}
	explicit JsonValue(double n) : mType(Number), mNumber(n) {}
	explicit JsonValue(const std::string& s) : mType(String), mString(s) {}
	static JsonValue MakeArray() { JsonValue v; v.mType = Array; return v; }
	static JsonValue MakeObject() { JsonValue v; v.mType = Object; return v; }

	Type GetType() const { return mType; }
	bool IsNull() const { return mType == Null; }
	bool IsBool() const { return mType == Bool; }
	bool IsNumber() const { return mType == Number; }
	bool IsString() const { return mType == String; }
	bool IsArray() const { return mType == Array; }
	bool IsObject() const { return mType == Object; }

	bool AsBool(bool theDefault = false) const { return mType == Bool ? mBool : (mType == Number ? mNumber != 0 : theDefault); }
	double AsNumber(double theDefault = 0) const { return mType == Number ? mNumber : (mType == Bool ? (mBool ? 1 : 0) : theDefault); }
	int AsInt(int theDefault = 0) const { return mType == Number ? (int)mNumber : theDefault; }
	const std::string& AsString() const { return mString; }
	std::string AsString(const std::string& theDefault) const { return mType == String ? mString : theDefault; }

	// Arrays
	size_t Size() const { return mType == Array ? mArray.size() : (mType == Object ? mObject.size() : 0); }
	const JsonValue& operator[](size_t i) const;
	void Push(const JsonValue& v) { mType = Array; mArray.push_back(v); }
	const std::vector<JsonValue>& Items() const { return mArray; }

	// Objects (insertion order is preserved)
	bool Has(const std::string& theKey) const;
	const JsonValue& operator[](const std::string& theKey) const;
	JsonValue& Set(const std::string& theKey, const JsonValue& v);
	const std::vector<std::pair<std::string, JsonValue>>& Members() const { return mObject; }

	std::string Dump(int theIndent = 2) const;

private:
	void DumpTo(std::string& theOut, int theIndent, int theDepth) const;

	Type mType = Null;
	bool mBool = false;
	double mNumber = 0;
	std::string mString;
	std::vector<JsonValue> mArray;
	std::vector<std::pair<std::string, JsonValue>> mObject;
};

// Parses JSON text (comments // and /* */ and trailing commas are tolerated for hand-edited files).
bool ParseJson(const std::string& theText, JsonValue& theOut, std::string* theError = nullptr);

}

#endif
