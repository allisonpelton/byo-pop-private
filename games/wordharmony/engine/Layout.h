#ifndef __WORDHARMONY_ENGINE_LAYOUT_H__
#define __WORDHARMONY_ENGINE_LAYOUT_H__

// The layout file reader (0x30 bytes, constructor 0x42c7f0, handler vtable 0x483ed8): each element of a screen's
// layout file becomes a node of the screen. Groups nest: an element goes into the innermost open group, and the screen
// is the outermost one. The screen keeps every node by its name.

#include "Xml.h"
#include <string>
#include <vector>

namespace engine
{

class Container;
class ScreenLayout;

class LayoutHandler : public XmlHandler
{
public:
	LayoutHandler();   // 0x42c7f0
	~LayoutHandler();  // 0x42c230

	void setScreen(ScreenLayout* theScreen) { mScreen = theScreen; }  // 0x42c1f0

	void startElement(const std::string& theName, const XmlAttributes& theAttributes) override;  // 0x430910
	void endElement(const std::string& theName) override;  // 0x42c4f0
	void startDocument() override;  // 0x42c8c0
	void endDocument() override;    // 0x42c380

private:
	bool addImage(const XmlAttributes& theAttributes);        // 0x430280
	bool addText(const XmlAttributes& theAttributes);         // 0x42ef20
	bool addImageButton(const XmlAttributes& theAttributes);  // 0x42c920
	bool addCheckBox(const XmlAttributes& theAttributes);     // 0x42d6f0
	bool addTextButton(const XmlAttributes& theAttributes);   // 0x42e2b0
	bool addGroup(const XmlAttributes& theAttributes);        // 0x42fa90
	bool addTextSub(const XmlAttributes& theAttributes);      // 0x4300c0
	bool addInclude(const XmlAttributes& theAttributes);      // 0x42fed0

	ScreenLayout* mScreen;            // +0x14
	std::vector<Container*> mGroups;  // +0x18: the open groups, the screen first
};

} // namespace engine

#endif
