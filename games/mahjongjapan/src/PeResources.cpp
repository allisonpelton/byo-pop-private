#include "PeResources.h"

#include <cctype>
#include <cstdio>
#include <cstring>

using namespace mahjongjapan;

namespace
{

uint16_t U16(const std::vector<uint8_t>& d, size_t o) { return o + 2 <= d.size() ? uint16_t(d[o] | d[o + 1] << 8) : 0; }
uint32_t U32(const std::vector<uint8_t>& d, size_t o)
{
	return o + 4 <= d.size() ? uint32_t(d[o] | d[o + 1] << 8 | d[o + 2] << 16 | uint32_t(d[o + 3]) << 24) : 0;
}

// UTF-16 to Windows-1252: the game's texts are English, so anything outside the code page becomes '?'
void AppendCp1252(std::string& theOut, uint16_t c)
{
	static const uint16_t kHigh[32] = {0x20ac, 0, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030,
		0x0160, 0x2039, 0x0152, 0, 0x017d, 0, 0, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x02dc,
		0x2122, 0x0161, 0x203a, 0x0153, 0, 0x017e, 0x0178};
	if (c < 0x80 || (c >= 0xa0 && c < 0x100))
	{
		theOut += char(c);
		return;
	}
	for (int i = 0; i < 32; i++)
		if (kHigh[i] == c)
		{
			theOut += char(0x80 + i);
			return;
		}
	theOut += '?';
}

} // namespace

PeResources::Key::Key(const char* theName) : mIsName(true), mId(0)
{
	for (const char* p = theName; *p; p++)
		mName += char(toupper((unsigned char)*p));
}

bool PeResources::Key::operator<(const Key& o) const
{
	if (mIsName != o.mIsName)
		return mIsName;  // names come before ids, as in the directory
	return mIsName ? mName < o.mName : mId < o.mId;
}

uint32_t PeResources::RvaToOffset(uint32_t theRva) const
{
	uint32_t pe = U32(mData, 0x3c);
	int nsec = U16(mData, pe + 6);
	uint32_t s = pe + 24 + U16(mData, pe + 20);
	for (int i = 0; i < nsec; i++, s += 40)
	{
		uint32_t vsize = U32(mData, s + 8), va = U32(mData, s + 12), rsize = U32(mData, s + 16), raw = U32(mData, s + 20);
		uint32_t span = vsize > rsize ? vsize : rsize;
		if (theRva >= va && theRva < va + span)
			return raw + theRva - va;
	}
	return 0;
}

bool PeResources::Open(const std::string& thePath)
{
	mData.clear();
	mDir.clear();
	mOrder.clear();
	FILE* f = fopen(thePath.c_str(), "rb");
	if (!f)
		return false;
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	fseek(f, 0, SEEK_SET);
	mData.resize(n > 0 ? size_t(n) : 0);
	if (n <= 0 || fread(mData.data(), 1, mData.size(), f) != mData.size())
		mData.clear();
	fclose(f);
	if (mData.size() < 0x40 || mData[0] != 'M' || mData[1] != 'Z')
	{
		mData.clear();
		return false;
	}
	uint32_t pe = U32(mData, 0x3c);
	if (U32(mData, pe) != 0x4550)
	{
		mData.clear();
		return false;
	}
	uint32_t opt = pe + 24;
	uint32_t resRva = U32(mData, opt + 96 + 8 * 2);
	uint32_t base = resRva ? RvaToOffset(resRva) : 0;
	if (!base)
		return true;

	auto keyAt = [&](uint32_t theName) -> Key {
		if (!(theName & 0x80000000))
			return Key(theName);
		uint32_t o = base + (theName & 0x7fffffff);
		int len = U16(mData, o);
		std::string s;
		for (int i = 0; i < len; i++)
			AppendCp1252(s, U16(mData, o + 2 + 2 * i));
		return Key(s.c_str());
	};
	auto forEach = [&](uint32_t theDir, auto&& theFn) {
		int count = U16(mData, base + theDir + 12) + U16(mData, base + theDir + 14);
		for (int i = 0; i < count; i++)
			theFn(U32(mData, base + theDir + 16 + 8 * i), U32(mData, base + theDir + 20 + 8 * i));
	};
	forEach(0, [&](uint32_t theType, uint32_t theTypeDir) {
		if (!(theTypeDir & 0x80000000))
			return;
		Key type = keyAt(theType);
		mOrder.push_back({type, {}});
		forEach(theTypeDir & 0x7fffffff, [&](uint32_t theName, uint32_t theNameDir) {
			if (!(theNameDir & 0x80000000))
				return;
			Key name = keyAt(theName);
			bool first = true;
			forEach(theNameDir & 0x7fffffff, [&](uint32_t, uint32_t theLeaf) {
				if (!first || (theLeaf & 0x80000000))
					return;
				first = false;
				uint32_t rva = U32(mData, base + theLeaf), size = U32(mData, base + theLeaf + 4);
				uint32_t o = RvaToOffset(rva);
				if (o && o + size <= mData.size())
				{
					mDir[type][name] = {o, size};
					mOrder.back().second.push_back(name);
				}
			});
		});
	});
	return true;
}

const uint8_t* PeResources::Find(const Key& theType, const Key& theName, uint32_t* theSize) const
{
	auto t = mDir.find(theType);
	if (t == mDir.end())
		return nullptr;
	auto e = t->second.find(theName);
	if (e == t->second.end())
		return nullptr;
	if (theSize)
		*theSize = e->second.mSize;
	return mData.data() + e->second.mOffset;
}

bool PeResources::Load(const Key& theType, const Key& theName, std::vector<uint8_t>& theOut) const
{
	uint32_t size = 0;
	const uint8_t* p = Find(theType, theName, &size);
	if (!p)
		return false;
	theOut.assign(p, p + size);
	return true;
}

bool PeResources::LoadString(uint32_t theId, std::string& theOut) const
{
	theOut.clear();
	uint32_t size = 0;
	const uint8_t* p = Find(Key(6u), Key((theId >> 4) + 1), &size);  // RT_STRING: blocks of 16
	if (!p)
		return false;
	uint32_t o = 0;
	for (uint32_t i = 0; i < (theId & 15); i++)
	{
		if (o + 2 > size)
			return false;
		o += 2 + 2 * uint32_t(p[o] | p[o + 1] << 8);
	}
	if (o + 2 > size)
		return false;
	uint32_t len = p[o] | p[o + 1] << 8;
	for (uint32_t i = 0; i < len && o + 4 + 2 * i <= size; i++)
		AppendCp1252(theOut, uint16_t(p[o + 2 + 2 * i] | p[o + 3 + 2 * i] << 8));
	return len != 0;
}

std::vector<PeResources::Key> PeResources::Names(const Key& theType) const
{
	for (auto& t : mOrder)
		if (!(t.first < theType) && !(theType < t.first))
			return t.second;
	return {};
}
