#ifndef __BONNIES_RESOURCES_H__
#define __BONNIES_RESOURCES_H__

// The resource cache (0x421960-0x421e50, the object 0x480b1c): fonts, images and sounds by name, counted, and freed a
// while after their last release (images 1.5 s, fonts and sounds 50 s).

#include "Str.h"

namespace bonnies
{

struct Font;
struct FontDesc;
struct Image;
struct Sample;

class Resources
{
public:
	enum
	{
		TYPE_FONT = 0,
		TYPE_IMAGE = 1,
		TYPE_SOUND = 2
	};

	// 0x421960
	Resources();

	// 0x421980: an entry's key, hashName(theName) + theType * 1000
	static int key(const Str& theName, int theType);
	// 0x4219f0: the resource, loaded on the first request and counted
	void* get(const Str& theName, int theType, FontDesc* theDesc);
	// 0x421bc0: one request fewer; at none the resource is due to be freed (now while releasing everything)
	void release(void* theResource);
	// 0x421c40, 0x421cc0, 0x421d30
	Image* getImage(const Str& theName);
	Sample* getSound(const Str& theName);
	Font* getFont(FontDesc* theDesc);
	// 0x421d70: frees everything
	void releaseAll();
	// 0x421db0, 0x421e30: frees the resources whose time is up, for at most about 20 ms, every 2.5 s
	void sweep();
	void update();

	struct Entry
	{
		void* mResource;  // +0x00
		int mDue;         // +0x04
		int mType;        // +0x08
		int mCount;       // +0x0c
		int mKey;         // +0x10
		Entry* mPrev;     // +0x14
		Entry* mNext;     // +0x18
	};

	// 0x421b30
	static void free(Entry* theEntry);

	int mFreeNow;    // +0x00
	int mNextSweep;  // +0x04
	int mNoSharing;  // +0x08 (never set)
};

extern Resources* gResources;  // 0x480b1c
extern int gLoadingFont;       // 0x480b20: inside getFont

} // namespace bonnies

#endif
