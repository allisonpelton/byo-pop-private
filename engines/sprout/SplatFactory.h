#ifndef __SPROUT_SPLATFACTORY_H__
#define __SPROUT_SPLATFACTORY_H__

// The signal build's splat factory (Pizza Frenzy's; 0x14c bytes, constructor 0x474f40 (pizzafrenzy), vtable
// 0x5028e8 (pizzafrenzy); its name is "SplatFactory"): a container the scene holds, which the game fills with the short
// lived nodes (splats) it makes from the definitions of res\manifests\splats.xml. Only what the loading state reaches
// is translated: the construction, the file's name and the layout.

#include "Node.h"
#include <string>

namespace engine
{

class SplatFactory : public Container
{
public:
	SplatFactory();  // 0x474f40 (pizzafrenzy)

	// 0x4739f0 (pizzafrenzy) reads the file's definitions through the application's slot 0x40 (the splats' handler
	// is not translated: nothing but play makes a splat)
	void load(const std::string& theFile) { mFile = theFile; }
	// 0x98: 0x473aa0 (pizzafrenzy): the bounds are an empty rectangle, whatever the children are
	void layout() override;
	// 0x60: 0x4749f0 (pizzafrenzy)
	std::string getName() override { return "SplatFactory"; }
	// 0x94: 0x473b90 (pizzafrenzy), which moves the splats made since the last frame into the children and removes the
	// ones whose time is up, is play and is translated with the game

private:
	std::string mFile;  // the definitions' file
};

} // namespace engine

#endif
