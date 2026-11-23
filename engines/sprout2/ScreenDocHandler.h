#ifndef __SPROUT2_SCREENDOCHANDLER_H__
#define __SPROUT2_SCREENDOCHANDLER_H__

// What reads a screen's XML into its layout (ScreenDocHandler, vtable 0x55ec00 (feedingfrenzy2), 0xc8 bytes; its
// constructor 0x47d880 (feedingfrenzy2)): the <screen> element's attributes, then each element inside it, made into
// a component added to the group it is in, or kept by name (a modifier, an effect). An element with a detail
// attribute is read, with all inside it, only when the attribute holds the engine's detail level.

#include "MasterDocHandler.h"

#include <deque>
#include <vector>

namespace engine
{

class Component;
class Container;
class Font;
class ScreenLayout;
class Surface;
class Theme;

class ScreenDocHandler : public SubDocHandler
{
public:
	// Talismania's reader of an element's common attributes (0x620da0 and its callers (talismania)) also reads "visible"; Feeding
	// Frenzy 2's does not, and its layouts are read without it. A game switches it on before its screens are read
	static void setHonorVisible(bool theHonor) { sHonorVisible = theHonor; }
	// On, an image item's scaleX and scaleY scale the image as drawn (Talismania); off, a scaled copy of the surface takes
	// their size and they only flip it (Feeding Frenzy 2)
	static void setScaleImages(bool theScale) { sScaleImages = theScale; }

	// 0x47d880 (feedingfrenzy2)
	ScreenDocHandler(ScreenLayout* theLayout, int theDetailLevel, Theme* theTheme);

	// 0x04 (0x4315b0 (feedingfrenzy2)): the <screen> element
	void begin(Params& theAttributes) override;
	// 0x00 (0x456de0 (feedingfrenzy2))
	void startElement(const std::string& theName, Params& theAttributes) override;
	// 0x04 (0x45c0f0 (feedingfrenzy2)): a group's end returns to the one it is in
	void endElement(const std::string& theName) override;
	// 0x08 (0x414b30) and 0x0c (0x414bc0) (feedingfrenzy2): the layout is the outermost group
	void startDocument() override;
	void endDocument() override;

private:
	// 0x472160 (feedingfrenzy2): the theme's image of the name, else the engine's
	Surface* findImage(const std::string& thePath);
	// 0x4737f0 (feedingfrenzy2): the theme's font of the name, else the engine's (none for no name)
	Font* findFont(const std::string& thePath);
	// 0x472f20 (feedingfrenzy2): the group the elements go into
	Container* getCurrentContainer() { return mContainers.back(); }
	// 0x461010 (feedingfrenzy2): a colour given as "r,g,b[,a]" (0 to 255), or the theme's colour of the name
	Color readColor(Params& theAttributes, const std::string& theName, const Color& theDefault);
	// 0x454510 (feedingfrenzy2): what every component may have, its smoothing, a modifier and an effect
	void readCommon(Params& theAttributes, Component* theComponent);

	// The elements, by their handlers
	void readImage(Params& theAttributes);                                 // 0x453c50 (feedingfrenzy2)
	void readGroup(Params& theAttributes, bool theComposite, bool theAttachPoint);  // 0x45acf0 (feedingfrenzy2)
	void readModifier(Params& theAttributes);                              // 0x477d60 (feedingfrenzy2)
	void readBltFx(Params& theAttributes);                                 // 0x4195f0 (feedingfrenzy2)
	void readParticleSystem(Params& theAttributes);                        // 0x46ed00 (feedingfrenzy2)
	void readImageButton(Params& theAttributes);                           // 0x432b10 (feedingfrenzy2)
	void readComboButton(Params& theAttributes);                           // 0x45c5a0 (feedingfrenzy2)
	void readListBox(Params& theAttributes);                               // 0x4804a0 (feedingfrenzy2)
	void readEditBox(Params& theAttributes);                               // 0x412680 (feedingfrenzy2)
	void readText(Params& theAttributes);                                  // 0x4548c0 (feedingfrenzy2)
	void readKeyAction(Params& theAttributes);                             // 0x426cb0 (feedingfrenzy2)

	ScreenLayout* mLayout;                  // +0x50
	std::vector<Container*> mContainers;    // +0x54: the groups the current element is inside, the innermost last
	// +0x64 (a string), +0x80 and +0x84 (references), set by elements not yet translated
	Ref<Theme> mTheme;                      // +0x88
	std::deque<bool> mReading;              // +0x8c: for each open element, whether it and its contents are read
	int mDetailLevel;                       // +0xa0
	std::string mDetailString;              // +0xa4: the detail level as a number
private:
	static bool sHonorVisible;
	static bool sScaleImages;
};

} // namespace engine

#endif
