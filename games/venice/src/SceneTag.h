// The engine's tag reader: the scene files (levels, screens and objects) are XML-like text read tag by tag; tags such as
// <stage> and <filter> are never closed, so there is no tree, only the sequence of tags with their attributes.

#ifndef VENICE_SCENETAG_H
#define VENICE_SCENETAG_H

#include <string>
#include <vector>

struct TagAttribute
{
	std::string mName;
	std::string mValue;
};

// 0x4359d0 writes one of these (0x198 bytes in the original) for each tag; 0x405790, 0x409b50, 0x409bc0 and 0x409c40 read it
class SceneTag
{
public:
	std::vector<TagAttribute> mAttributes;
	std::string mName;
	// The text between this tag's '>' and the next '<'; only set for a tag that is neither closing nor self-closing
	const char* mContent = nullptr;
	int mContentLength = 0;
	bool mIsClosing = false;
	bool mIsSelfClosing = false;

	void Reset(); // 0x411ce0

	// 0x409c40
	bool Has(const char* theName) const;
	// 0x405790: the attribute's text, null when the tag has none
	const char* GetString(const char* theName) const;
	// 0x409b50 (atol, 0 when missing)
	int GetInt(const char* theName) const;
	// 0x409bc0 (atof, 0 when missing)
	double GetFloat(const char* theName) const;
};

// 0x4359d0: reads the tag that starts at or after theText into theTag and returns the text after it. The original runs off
// the end of its buffer when no tag is left; the port stops at the terminating zero and returns a tag with an empty name.
const char* ReadTag(const char* theText, SceneTag* theTag);

#endif
