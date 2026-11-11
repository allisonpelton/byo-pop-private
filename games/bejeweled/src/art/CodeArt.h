#ifndef __ART_CODEART_H__
#define __ART_CODEART_H__

// port(variant): code-drawn art (features.codeArt, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.18). With the feature on,
// code draws every image the game loads and the five bitmap fonts at load time, under the original names and sizes, so
// the game runs without its images/ folder and the fonts' data/*.txt. The look is the port's own: panels, bevels,
// procedural textures, a stroke font (VectorFont) and ray-cast gems (GemArt).
//
// Enable registers an image generator with the platform loader (Platform::SetImageGenerator): a mod's image still wins
// over a drawn one, and image patches draw over it. Each font's descriptor is served as a generated file
// (Platform::VFS::SetGeneratedFile), unless a mod has the font's image or descriptor: then the pair stays the mod's and
// the game's.

#include "platform/ImageLoader.h"

#include <functional>
#include <string>
#include <vector>

namespace Sexy
{
struct GameFeatures;
}

namespace CodeArt
{

struct Settings
{
	std::string mGems = "cel";       // gem style: cel, glossy, su (GemArt)
	std::string mPalette = "blue";   // blue, violet, emerald, ember, night
	int mGemColors = 7;              // colours of the gem sheet, 7..12
	int mScale = 4;                  // pixel density of the drawn images (soft ones use at most 2): WinDMApp draws
	                                 // them at the window's render scale
	// The texts drawn into images (button labels, the title), by string id and default (WinDMApp::GetString)
	std::function<std::string(const std::string& theId, const std::string& theDefault)> mText;
};

// The settings features.codeArt asks for, at theGemColors colours and the platform's highest density (4; 2 on the web)
Settings SettingsFor(const Sexy::GameFeatures& theFeatures, int theGemColors);

void Enable(const Settings& theSettings);
bool IsEnabled();

// Draws one image ("images/name", lower case, no extension) at the settings' density; false when code art has no
// generator for it, or leaves it to a mod (a font whose image or descriptor a mod has)
bool Generate(const std::string& theGamePath, Platform::LoadedImage& theOut);
std::vector<std::string> ImageNames();   // every image it draws, "images/name"

// -dumpcodeart=<dir>: every image as <name>.png and <name>@<scale>x.png, and the fonts' descriptors; one line per image
// (its size and the milliseconds it took) to theLog
bool Dump(const std::string& theDir, std::string* theLog);

}

#endif
