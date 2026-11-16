#ifndef __SPROUT_LAYOUT_H__
#define __SPROUT_LAYOUT_H__

// The layout file reader (0x30 bytes, constructor 0x42c7f0 (wordharmony), handler vtable 0x483ed8 (wordharmony); 0x5c
// bytes, constructor 0x48bfa0 (pizzafrenzy), handler vtable 0x503e30 (pizzafrenzy); 0x30 bytes, constructor
// 0x457670 (feedingfrenzy), handler vtable 0x4a914c (feedingfrenzy)): each element of a screen's layout file becomes a
// node of the screen. Groups nest: an element goes into the innermost open group, and the screen is the outermost
// one. The screen keeps every node by its name.
//
// The signal build's reader (Build.h) also reads the screen element (the screen's default and cancel actions and its
// name), the key actions, the edit box, the list box and the table with its columns, and sets more of the nodes. It
// knows the text button, the selector, the particle system, the text substitution, the include and the radio group,
// which no layout of Pizza Frenzy uses: they are not translated.
//
// The early build's reader (Feeding Frenzy's, gEarlyBuild) is the listener build's elements with the text button, the
// text substitution and the include added: it has no screen, key action, edit box, list box, table or selector
// element, and the elements it has read the listener build's attributes (the image and the text are the listener
// build's functions, the check box has no pivot and no start state, the image button sets no base scale and the group
// does not lay itself out). Its point reader is the listener build's too.

#include "Build.h"
#include "Color4.h"
#include "Geometry.h"
#include "Object.h"
#include "Xml.h"
#include <string>
#include <vector>

namespace engine
{

class Container;
class ScreenLayout;
class Table;

// The attribute readers of the layout elements, and of the particle file's: each takes a copy of the attributes and
// gives the default for a missing attribute
// 0x42abd0 (wordharmony), 0x475890 (pizzafrenzy)
std::string getStringAttribute(XmlAttributes theAttributes, const std::string& theName, const std::string& theDefault);
// 0x42ad20 (wordharmony), 0x475960 (pizzafrenzy)
long getIntAttribute(XmlAttributes theAttributes, const std::string& theName, long theDefault);
// 0x42aeb0 (wordharmony), 0x475a40 (pizzafrenzy)
float getFloatAttribute(XmlAttributes theAttributes, const std::string& theName, float theDefault);
// 0x42b040 (wordharmony): "(x,y)", or one number for both
Point getScaleAttribute(XmlAttributes theAttributes, const std::string& theName, Point theDefault);
// 0x42b1c0 (wordharmony), 0x475fa0 (pizzafrenzy), 0x448540 (feedingfrenzy): "x,y" (the signal build's reads "x , y",
// spaces allowed; Feeding Frenzy's is the listener build's)
Point getPointAttribute(XmlAttributes theAttributes, const std::string& theName, Point theDefault);
// 0x42b350 (wordharmony), 0x475ea0 (pizzafrenzy): "true" or "false"
bool getBoolAttribute(XmlAttributes theAttributes, const std::string& theName, bool theDefault);
// 0x4761a0 (pizzafrenzy): "r, g, b" or "r, g, b, a", bytes; a value of fewer than three numbers is the default, and
// three leave the default's alpha
Color4 getColorAttribute(XmlAttributes theAttributes, const std::string& theName, Color4 theDefault);
// 0x448680 (feedingfrenzy): "r,g,b", three floats (a colour offset, not bytes); a number that does not parse is the
// default's
Color getColorOffsetAttribute(XmlAttributes theAttributes, const std::string& theName, Color theDefault);

class LayoutHandler : public XmlHandler
{
public:
	LayoutHandler();   // 0x42c7f0 (wordharmony), 0x48bfa0 (pizzafrenzy), 0x457670 (feedingfrenzy)
	~LayoutHandler();  // 0x42c230 (wordharmony), 0x452da0 (feedingfrenzy)

	void setScreen(ScreenLayout* theScreen) { mScreen = theScreen; }  // 0x42c1f0 (wordharmony)

	// 0x430910 (wordharmony), 0x48d250 (pizzafrenzy), 0x457ee0 (feedingfrenzy)
	void startElement(const std::string& theName, const XmlAttributes& theAttributes) override;
	// 0x42c4f0 (wordharmony), 0x48b7f0 (pizzafrenzy), 0x457370 (feedingfrenzy)
	void endElement(const std::string& theName) override;
	// 0x42c8c0 (wordharmony), 0x48cc30 (pizzafrenzy), 0x457e80 (feedingfrenzy)
	void startDocument() override;
	// 0x42c380 (wordharmony), 0x484c40 (pizzafrenzy), 0x452f50 (feedingfrenzy)
	void endDocument() override;

private:
	// The signal build's startElement
	void startSignalElement(const std::string& theName, const XmlAttributes& theAttributes);
	// The early build's startElement (0x457ee0 (feedingfrenzy))
	void startEarlyElement(const std::string& theName, const XmlAttributes& theAttributes);
	// 0x430280 (wordharmony), 0x484ea0 (pizzafrenzy), 0x453120 (feedingfrenzy); the signal build's is addSignalImage
	bool addImage(const XmlAttributes& theAttributes);
	bool addSignalImage(const XmlAttributes& theAttributes);
	// 0x42ef20 (wordharmony), 0x488550 (pizzafrenzy), 0x4562f0 (feedingfrenzy); the signal build's is addSignalText
	bool addText(const XmlAttributes& theAttributes);
	bool addSignalText(const XmlAttributes& theAttributes);
	// 0x42c920 (wordharmony), 0x4858a0 (pizzafrenzy), 0x453930 (feedingfrenzy)
	bool addImageButton(const XmlAttributes& theAttributes);
	// 0x42d6f0 (wordharmony), 0x4867a0 (pizzafrenzy), 0x4547b0 (feedingfrenzy)
	bool addCheckBox(const XmlAttributes& theAttributes);
	// 0x42fa90 (wordharmony), 0x48c2a0 (pizzafrenzy), 0x457990 (feedingfrenzy)
	bool addGroup(const XmlAttributes& theAttributes);
	// 0x42f760 (wordharmony), 0x456d20 (feedingfrenzy)
	bool addParticleSystem(const XmlAttributes& theAttributes);
	// The early build's elements
	bool addTextButton(const XmlAttributes& theAttributes);  // 0x455440 (feedingfrenzy)
	bool addTextSub(const XmlAttributes& theAttributes);     // 0x457120 (feedingfrenzy)
	bool addInclude(const XmlAttributes& theAttributes);     // 0x457740 (feedingfrenzy)
	// The signal build's elements
	bool addEditBox(const XmlAttributes& theAttributes);   // 0x489910 (pizzafrenzy)
	bool addListBox(const XmlAttributes& theAttributes);   // 0x48a160 (pizzafrenzy)
	bool addTable(const XmlAttributes& theAttributes);     // 0x48ade0 (pizzafrenzy)
	bool addColumn(const XmlAttributes& theAttributes);    // 0x48b4f0 (pizzafrenzy)
	bool addKeyAction(const XmlAttributes& theAttributes); // 0x48b990 (pizzafrenzy)

	ScreenLayout* mScreen;            // +0x14
	std::vector<Container*> mGroups;  // +0x18: the open groups, the screen first; +0x1c (pizzafrenzy), +0x1c (feedingfrenzy)
	Ref<Table> mTable;                // +0x48 (pizzafrenzy): the open table, which its columns go into
};

} // namespace engine

#endif
