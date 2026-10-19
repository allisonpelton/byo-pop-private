#ifndef __DDIMAGE_H__
#define __DDIMAGE_H__

// port(variant): headless shim of framework/DDImage.h for the verifier (games/bejeweled/docs/CODE_MODS_DESIGN.md
// 6.9). Not part of the original game.

#include "MemoryImage.h"

namespace Sexy
{

class DDInterface;

class DDImage : public MemoryImage
{
public:
	DDImage() {}
	DDImage(DDInterface* theDDInterface) {}
};

}

#endif
