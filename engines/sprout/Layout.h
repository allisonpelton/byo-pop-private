#ifndef __SPROUT_LAYOUT_H__
#define __SPROUT_LAYOUT_H__

// The layout file reader (0x30 bytes, constructor 0x42c7f0 (wordharmony), handler vtable 0x483ed8 (wordharmony)): each
// element of a screen's layout file becomes a node of the screen. Groups nest: an element goes into the innermost open
// group, and the screen is the outermost one. The screen keeps every node by its name.

#include "Geometry.h"
#include "Xml.h"
#include <string>
#include <vector>

namespace engine
{

class Container;
class ScreenLayout;

// The attribute readers of the layout elements, and of the particle file's: each takes a copy of the attributes and
// gives the default for a missing attribute
// 0x42abd0 (wordharmony)
std::string getStringAttribute(XmlAttributes theAttributes, const std::string& theName, const std::string& theDefault);
// 0x42ad20 (wordharmony)
long getIntAttribute(XmlAttributes theAttributes, const std::string& theName, long theDefault);
// 0x42aeb0 (wordharmony)
float getFloatAttribute(XmlAttributes theAttributes, const std::string& theName, float theDefault);
// 0x42b040 (wordharmony): "(x,y)", or one number for both
Point getScaleAttribute(XmlAttributes theAttributes, const std::string& theName, Point theDefault);
// 0x42b1c0 (wordharmony): "x,y"
Point getPointAttribute(XmlAttributes theAttributes, const std::string& theName, Point theDefault);
// 0x42b350 (wordharmony): "true" or "false"
bool getBoolAttribute(XmlAttributes theAttributes, const std::string& theName, bool theDefault);

class LayoutHandler : public XmlHandler
{
public:
	LayoutHandler();   // 0x42c7f0 (wordharmony)
	~LayoutHandler();  // 0x42c230 (wordharmony)

	void setScreen(ScreenLayout* theScreen) { mScreen = theScreen; }  // 0x42c1f0 (wordharmony)

	void startElement(const std::string& theName, const XmlAttributes& theAttributes) override;  // 0x430910 (wordharmony)
	void endElement(const std::string& theName) override;  // 0x42c4f0 (wordharmony)
	void startDocument() override;  // 0x42c8c0 (wordharmony)
	void endDocument() override;    // 0x42c380 (wordharmony)

private:
	bool addImage(const XmlAttributes& theAttributes);        // 0x430280 (wordharmony)
	bool addText(const XmlAttributes& theAttributes);         // 0x42ef20 (wordharmony)
	bool addImageButton(const XmlAttributes& theAttributes);  // 0x42c920 (wordharmony)
	bool addCheckBox(const XmlAttributes& theAttributes);     // 0x42d6f0 (wordharmony)
	bool addGroup(const XmlAttributes& theAttributes);        // 0x42fa90 (wordharmony)
	bool addParticleSystem(const XmlAttributes& theAttributes);  // 0x42f760 (wordharmony)

	ScreenLayout* mScreen;            // +0x14
	std::vector<Container*> mGroups;  // +0x18: the open groups, the screen first
};

} // namespace engine

#endif
