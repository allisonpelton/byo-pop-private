#include "Json.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace Platform
{

static const JsonValue gNullValue;

const JsonValue& JsonValue::operator[](size_t i) const
{
	return mType == Array && i < mArray.size() ? mArray[i] : gNullValue;
}

bool JsonValue::Has(const std::string& theKey) const
{
	for (const auto& kv : mObject)
		if (kv.first == theKey)
			return true;
	return false;
}

const JsonValue& JsonValue::operator[](const std::string& theKey) const
{
	if (mType == Object)
		for (const auto& kv : mObject)
			if (kv.first == theKey)
				return kv.second;
	return gNullValue;
}

JsonValue& JsonValue::Set(const std::string& theKey, const JsonValue& v)
{
	mType = Object;
	for (auto& kv : mObject)
	{
		if (kv.first == theKey)
		{
			kv.second = v;
			return kv.second;
		}
	}
	mObject.emplace_back(theKey, v);
	return mObject.back().second;
}

static void EscapeString(const std::string& s, std::string& out)
{
	out += '"';
	for (unsigned char c : s)
	{
		switch (c)
		{
		case '"': out += "\\\""; break;
		case '\\': out += "\\\\"; break;
		case '\n': out += "\\n"; break;
		case '\r': out += "\\r"; break;
		case '\t': out += "\\t"; break;
		default:
			if (c < 0x20)
			{
				char buf[8];
				snprintf(buf, sizeof(buf), "\\u%04x", c);
				out += buf;
			}
			else
				out += (char)c;
		}
	}
	out += '"';
}

void JsonValue::DumpTo(std::string& out, int theIndent, int theDepth) const
{
	std::string pad = theIndent > 0 ? std::string((theDepth + 1) * theIndent, ' ') : "";
	std::string padEnd = theIndent > 0 ? std::string(theDepth * theIndent, ' ') : "";
	const char* nl = theIndent > 0 ? "\n" : "";
	switch (mType)
	{
	case Null: out += "null"; break;
	case Bool: out += mBool ? "true" : "false"; break;
	case Number:
	{
		char buf[32];
		if (std::floor(mNumber) == mNumber && std::fabs(mNumber) < 1e15)
			snprintf(buf, sizeof(buf), "%.0f", mNumber);
		else
		{
			// The shortest text that reads back as the same double (0.005, not 0.0050000000000000001)
			for (int precision = 15; precision <= 17; precision++)
			{
				snprintf(buf, sizeof(buf), "%.*g", precision, mNumber);
				if (strtod(buf, nullptr) == mNumber)
					break;
			}
		}
		out += buf;
		break;
	}
	case String: EscapeString(mString, out); break;
	case Array:
		if (mArray.empty())
		{
			out += "[]";
			break;
		}
		out += "[";
		out += nl;
		for (size_t i = 0; i < mArray.size(); i++)
		{
			out += pad;
			mArray[i].DumpTo(out, theIndent, theDepth + 1);
			if (i + 1 < mArray.size())
				out += ",";
			out += nl;
		}
		out += padEnd + "]";
		break;
	case Object:
		if (mObject.empty())
		{
			out += "{}";
			break;
		}
		out += "{";
		out += nl;
		for (size_t i = 0; i < mObject.size(); i++)
		{
			out += pad;
			EscapeString(mObject[i].first, out);
			out += theIndent > 0 ? ": " : ":";
			mObject[i].second.DumpTo(out, theIndent, theDepth + 1);
			if (i + 1 < mObject.size())
				out += ",";
			out += nl;
		}
		out += padEnd + "}";
		break;
	}
}

std::string JsonValue::Dump(int theIndent) const
{
	std::string out;
	DumpTo(out, theIndent, 0);
	return out;
}

namespace
{
struct Parser
{
	const char* p;
	const char* end;
	std::string err;

	void SkipWs()
	{
		while (p < end)
		{
			if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
				p++;
			else if (p + 1 < end && p[0] == '/' && p[1] == '/')
			{
				while (p < end && *p != '\n')
					p++;
			}
			else if (p + 1 < end && p[0] == '/' && p[1] == '*')
			{
				p += 2;
				while (p + 1 < end && !(p[0] == '*' && p[1] == '/'))
					p++;
				p = p + 2 <= end ? p + 2 : end;
			}
			else
				break;
		}
	}

	bool Fail(const char* msg)
	{
		if (err.empty())
			err = msg;
		return false;
	}

	static void AppendUtf8(std::string& s, unsigned cp)
	{
		if (cp < 0x80)
			s += (char)cp;
		else if (cp < 0x800)
		{
			s += (char)(0xC0 | (cp >> 6));
			s += (char)(0x80 | (cp & 0x3F));
		}
		else if (cp < 0x10000)
		{
			s += (char)(0xE0 | (cp >> 12));
			s += (char)(0x80 | ((cp >> 6) & 0x3F));
			s += (char)(0x80 | (cp & 0x3F));
		}
		else
		{
			s += (char)(0xF0 | (cp >> 18));
			s += (char)(0x80 | ((cp >> 12) & 0x3F));
			s += (char)(0x80 | ((cp >> 6) & 0x3F));
			s += (char)(0x80 | (cp & 0x3F));
		}
	}

	bool ParseString(std::string& out)
	{
		if (p >= end || *p != '"')
			return Fail("expected string");
		p++;
		while (p < end && *p != '"')
		{
			if (*p == '\\')
			{
				if (++p >= end)
					return Fail("bad escape");
				switch (*p)
				{
				case 'n': out += '\n'; break;
				case 't': out += '\t'; break;
				case 'r': out += '\r'; break;
				case 'b': out += '\b'; break;
				case 'f': out += '\f'; break;
				case 'u':
				{
					if (end - p < 5)
						return Fail("bad \\u escape");
					unsigned cp = (unsigned)strtoul(std::string(p + 1, p + 5).c_str(), nullptr, 16);
					p += 4;
					if (cp >= 0xD800 && cp < 0xDC00 && end - p >= 7 && p[1] == '\\' && p[2] == 'u')
					{
						unsigned lo = (unsigned)strtoul(std::string(p + 3, p + 7).c_str(), nullptr, 16);
						cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
						p += 6;
					}
					AppendUtf8(out, cp);
					break;
				}
				default: out += *p; break;
				}
				p++;
			}
			else
				out += *p++;
		}
		if (p >= end)
			return Fail("unterminated string");
		p++;
		return true;
	}

	bool ParseValue(JsonValue& out, int depth)
	{
		if (depth > 64)
			return Fail("nesting too deep");
		SkipWs();
		if (p >= end)
			return Fail("unexpected end");
		char c = *p;
		if (c == '{')
		{
			p++;
			out = JsonValue::MakeObject();
			SkipWs();
			while (p < end && *p != '}')
			{
				std::string key;
				if (!ParseString(key))
					return false;
				SkipWs();
				if (p >= end || *p != ':')
					return Fail("expected ':'");
				p++;
				JsonValue v;
				if (!ParseValue(v, depth + 1))
					return false;
				out.Set(key, v);
				SkipWs();
				if (p < end && *p == ',')
				{
					p++;
					SkipWs();
				}
				else if (p < end && *p != '}')
					return Fail("expected ',' or '}'");
			}
			if (p >= end)
				return Fail("unterminated object");
			p++;
			return true;
		}
		if (c == '[')
		{
			p++;
			out = JsonValue::MakeArray();
			SkipWs();
			while (p < end && *p != ']')
			{
				JsonValue v;
				if (!ParseValue(v, depth + 1))
					return false;
				out.Push(v);
				SkipWs();
				if (p < end && *p == ',')
				{
					p++;
					SkipWs();
				}
				else if (p < end && *p != ']')
					return Fail("expected ',' or ']'");
			}
			if (p >= end)
				return Fail("unterminated array");
			p++;
			return true;
		}
		if (c == '"')
		{
			std::string s;
			if (!ParseString(s))
				return false;
			out = JsonValue(s);
			return true;
		}
		if (end - p >= 4 && !strncmp(p, "true", 4))
		{
			p += 4;
			out = JsonValue(true);
			return true;
		}
		if (end - p >= 5 && !strncmp(p, "false", 5))
		{
			p += 5;
			out = JsonValue(false);
			return true;
		}
		if (end - p >= 4 && !strncmp(p, "null", 4))
		{
			p += 4;
			out = JsonValue();
			return true;
		}
		if (c == '-' || (c >= '0' && c <= '9'))
		{
			char* stop = nullptr;
			std::string num(p, std::min<size_t>(end - p, 64));
			double d = strtod(num.c_str(), &stop);
			if (stop == num.c_str())
				return Fail("bad number");
			// strtod also reads nan and inf, which JSON does not have, and a number too large for a double
			// becomes inf; each libc spells and signs these its own way
			if (!std::isfinite(d))
				return Fail("bad number");
			p += stop - num.c_str();
			out = JsonValue(d);
			return true;
		}
		return Fail("unexpected character");
	}
};
}

bool ParseJson(const std::string& theText, JsonValue& theOut, std::string* theError)
{
	Parser ps{theText.data(), theText.data() + theText.size(), ""};
	bool ok = ps.ParseValue(theOut, 0);
	if (ok)
	{
		ps.SkipWs();
		if (ps.p != ps.end)
			ok = ps.Fail("trailing characters");
	}
	if (!ok && theError)
	{
		int line = 1;
		for (const char* q = theText.data(); q < ps.p && q < ps.end; q++)
			if (*q == '\n')
				line++;
		*theError = ps.err + " (line " + std::to_string(line) + ")";
	}
	return ok;
}

}
