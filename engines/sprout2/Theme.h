#ifndef __SPROUT2_THEME_H__
#define __SPROUT2_THEME_H__

// A theme (Theme, 0xa8 bytes, vtable 0x55b828 (feedingfrenzy2); its constructor 0x40afd0 (feedingfrenzy2)): the
// images, fonts, colours, vectors, particle effects, sounds, music and animations an XML file names, each under a name
// the screens and the game ask for. A theme's file is read by its own handler (Theme::ThemeDocHandler, 0xa8 bytes, its
// constructor 0x4070a0 (feedingfrenzy2)), which reads the <theme> element's basePath and passes each entry to the
// theme's slot for it. The sounds, music and animations are not yet translated: their entries are logged.

#include "Geometry.h"
#include "MasterDocHandler.h"
#include "Object.h"

#include <map>
#include <string>

namespace engine
{

class Font;
class ParticleSystem;
class ParticleSystemDef;
class ParticleSystemPool;
class Surface;

class Theme : public Object
{
public:
	Theme();
	~Theme() override;

	// 0x04 (0x40dd30 (feedingfrenzy2)): the theme emptied, the engine's images and fonts nothing else holds let go,
	// and the file read (the archives' XML); entries the file does not name stay as they were
	virtual void load(const std::string& thePath);
	// 0x08 (0x41f180 (feedingfrenzy2)): the colour of the name, white for none
	virtual Color getColor(const std::string& theName);
	// 0x0c (0x477810 (feedingfrenzy2)): the vector of the name, (0, 0) for none
	virtual Vector2f getVector(const std::string& theName);
	// 0x10 (0x407280 (feedingfrenzy2)): the images, particle effects and music emptied and the fallback dropped;
	// the fonts, colours and vectors stay
	virtual void clear();
	// 0x14 (0x431420 (feedingfrenzy2)): the image of the archives at the path, copied and given the pivot, scale and
	// blend mode, under the name. A copy made before with the same path, pivot, scale and mode is shared
	virtual void addImage(const std::string& theName, const std::string& thePath, int thePivot, float theScale,
		int theBlendMode);
	// 0x1c (0x47aa40 (feedingfrenzy2)): the effect's file read and its images loaded, kept under the name with a
	// pool of the count made at once; a name that has one keeps it (a pool is made and dropped)
	virtual void addParticleEffect(const std::string& theName, const std::string& thePath, int theCount);
	// 0x28 (0x4233e0 (feedingfrenzy2)): the engine's font at the path under the name, unless the name has one;
	// false when the font cannot be loaded
	virtual bool addFont(const std::string& theName, const std::string& thePath);

	// The handler's <color> and <vector> entries (inlined in 0x4615c0 (feedingfrenzy2)): kept unless the name has one
	void addColor(const std::string& theName, const Color& theColor) { mColors.insert(std::make_pair(theName, theColor)); }
	void addVector(const std::string& theName, const Vector2f& theVector)
	{
		mVectors.insert(std::make_pair(theName, theVector));
	}

	// 0x46c340 (feedingfrenzy2), with 0x45faa0 (feedingfrenzy2): the image of the name, here or in the fallbacks,
	// null for none
	virtual Surface* getImage(const std::string& theName);
	// 0x475950 (feedingfrenzy2): a copy of the effect of the name from its pool, here or in the fallbacks, null for
	// none
	ParticleSystem* getParticleSystem(const std::string& theName);
	// 0x48d9d0 (feedingfrenzy2): the definition of the effect of the name, null for none (clear forgets the
	// definitions and keeps the pools)
	ParticleSystemDef* getParticleDef(const std::string& theName);

	// 0x472f60 (feedingfrenzy2): the font of the name, here or in the fallbacks, null for none
	Font* getFont(const std::string& theName);

	// Set where the original writes the field (the splash's loader, 0x4946a0 (feedingfrenzy2))
	void setFallback(Theme* theTheme) { mFallback = theTheme; }

	const std::string& getBasePath() const { return mBasePath; }
	void setBasePath(const std::string& theBasePath) { mBasePath = theBasePath; }

private:
	// 0x45eaa0 (feedingfrenzy2): what an image's copy is kept under, "path_pivot_mode_scale" (the scale "%f")
	static std::string imageKey(const std::string& thePath, int thePivot, float theScale, int theBlendMode);

	std::map<std::string, Ref<Surface>> mImagesByKey;    // +0x0c: the copies, by imageKey
	std::map<std::string, std::string> mImageKeys;       // +0x18: each image name's key
	std::map<std::string, Ref<ParticleSystemDef>> mParticleDefs;     // +0x24
	std::map<std::string, Ref<ParticleSystemPool>> mParticlePools;   // +0x30
	std::map<std::string, Ref<Font>> mFonts;             // +0x3c
	std::string mBasePath;                               // +0x48: the <theme> element's basePath
	// +0x64: the sounds' manager (not yet translated)
	Ref<Theme> mFallback;                                // +0x68: looked in for an image, font or effect this
	                                                     // theme has not (the shell theme's is the minimal theme)
	// +0x6c: the music, by name; +0x7c: the animations, by name (not yet translated). +0x78: a number load and the
	// constructor set to 0, its reader not yet found
	std::map<std::string, Color> mColors;                // +0x88
	std::map<std::string, Vector2f> mVectors;            // +0x94
};

// Theme::ThemeDocHandler: the <theme> element and its entries
class ThemeDocHandler : public SubDocHandler
{
public:
	// 0x4070a0 (feedingfrenzy2)
	explicit ThemeDocHandler(Theme* theTheme) : mTheme(theTheme) {}

	// 0x04 of its own table (0x40e5a0 (feedingfrenzy2)): the basePath, kept here and in the theme
	void begin(Params& theAttributes) override;
	// 0x00 (0x4615c0 (feedingfrenzy2))
	void startElement(const std::string& theName, Params& theAttributes) override;
	// 0x04 (0x46deb0 (feedingfrenzy2)): only the end of a composite, not yet translated
	void endElement(const std::string& theName) override {}

private:
	// 0x45e7b0 (feedingfrenzy2): an entry's file, "basePath/file", or its fullPath when it has no file
	std::string entryPath(Params& theAttributes);
	// 0x45d5d0 (feedingfrenzy2): <image name file|fullPath pivot alphamode scale>
	void readImage(Params& theAttributes);
	// 0x4258f0 (feedingfrenzy2): <font name file|fullPath>
	void readFont(Params& theAttributes);
	// 0x426070 (feedingfrenzy2): <particleEffect name file|fullPath count>
	void readParticleEffect(Params& theAttributes);

	Theme* mTheme;          // +0x80
	std::string mBasePath;  // +0x84
};

} // namespace engine

#endif
