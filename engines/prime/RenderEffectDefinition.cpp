// Sexy::RenderEffectDefinition: see the header.
#include "RenderEffectDefinition.h"

#include "Common.h"
#include "PakInterface.h"

using namespace Sexy;

// port: the file DoLoadRenderEffect (0x463400 (bejeweled3)) reads, kept as its bytes until the 3D layer compiles it
bool RenderEffectDefinition::LoadFromFile(const std::string& thePath, const std::string& theSrcFileName)
{
	mSrcFileName = theSrcFileName;
	std::string aFileName = GetFileDir(thePath, true) + GetFileName(thePath, true) + ".d3dfx";
	PFILE* aFile = p_fopen(aFileName.c_str(), "rb");
	if (aFile == nullptr)
		return false;
	p_fseek(aFile, 0, SEEK_END);
	int aSize = p_ftell(aFile);
	p_fseek(aFile, 0, SEEK_SET);
	mData.resize(aSize > 0 ? aSize : 0);
	bool aRead = aSize <= 0 || p_fread(mData.data(), 1, aSize, aFile) == (size_t)aSize;
	p_fclose(aFile);
	return aRead;
}
