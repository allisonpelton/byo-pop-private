#ifndef __TALISMANIA_ATLANTISTHEME_H__
#define __TALISMANIA_ATLANTISTHEME_H__

// The game's theme (AtlantisTheme over engine::Theme, its entries read by Theme::ThemeDocHandler, 0x604010): the images,
// fonts, colours and vectors the screens ask for by name, and the sounds, music, composites and animations other code
// asks for. Every <theme> file of a data set (the ones marked theme="true") is added to the one theme. Images are kept
// as entries and read when first asked for, unless the entry is marked "demandload=false"; the original does the same.
// Translated so far: the images (without the alpha type "mask", the trim and a scale other than 1), fonts, colours and
// vectors. The other entries are kept as the attributes they were given, until the code that uses each is translated.

#include "Surface.h"
#include "Theme.h"

#include <map>
#include <string>

class AtlantisTheme : public engine::Theme
{
public:
	// 0x601600 and the handler: one file read and its entries added to the theme's (the original's slot 0x04 empties the
	// theme first; the data set's files add to it)
	void addFile(const std::string& thePath);

	// 0x601e80 (slot 0x20): an image entry, loaded now unless it is to be loaded on demand
	void addImageEntry(const std::string& theName, const std::string& thePath, int thePivot, float theScaleX,
					   float theScaleY, int theAlphaType, bool theTrim, bool theDemandLoad);

	engine::Surface* getImage(const std::string& theName) override;

	// The entries of the kinds not yet translated (sound, music, randmusic, composite, animdata, animation,
	// particleEffect): kind, then name, then the attributes as written
	std::map<std::string, std::map<std::string, std::map<std::string, std::string>>> mRawEntries;

private:
	struct ImageEntry
	{
		std::string mPath;
		int mPivot = 1;
		float mScaleX = 1;
		float mScaleY = 1;
		int mAlphaType = 2;
		bool mTrim = false;
		engine::Ref<engine::Surface> mSurface;
	};
	void loadImage(ImageEntry& theEntry);
	engine::Surface* getComposite(const std::string& theName);

	std::map<std::string, engine::Ref<engine::Surface>> mComposites;  // the composites built so far, by name

	std::map<std::string, ImageEntry> mImageEntries;
};

#endif
