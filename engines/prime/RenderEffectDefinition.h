// Sexy::RenderEffectDefinition (0x50 bytes; 0x465610 (bejeweled3) makes one, 0x5c6920 (bejeweled3) loads it): a 3D-mode
// shader effect of the manifest's <RenderEffect> entries, a compiled Direct3D effect file (.d3dfx) and its source's
// name (srcpath). The port keeps the file's bytes for the 3D renderer, which turns the effect into its own shaders; the
// 2D mode never draws with one.
#pragma once

#include <string>
#include <vector>

namespace Sexy {

class RenderEffectDefinition {
public:
	std::vector<unsigned char> mData;   // the .d3dfx file
	std::string mSrcFileName;           // the effect's source (.fx), which the PC game does not ship

	// reads thePath with its extension made .d3dfx; false when there is no such file
	bool LoadFromFile(const std::string& thePath, const std::string& theSrcFileName);
};

}  // namespace Sexy
