#ifndef __PIZZAFRENZY_LEVELMANIFEST_H__
#define __PIZZAFRENZY_LEVELMANIFEST_H__

// The level manifest (res\manifests\levelManifestSpeed.xml), which the loading state reads once the tile manifest and
// the pizzas are there: each "city" element makes a City (Level.h) and gives it to the application (addCity), and the
// elements inside it make the city's levels and, inside a level, what the level has.

#include <string>

namespace pizzafrenzy
{

// 0x405470: reads the file; throws an Exception when it cannot be read
void loadLevelManifest(const std::string& theFile);

} // namespace pizzafrenzy

#endif
