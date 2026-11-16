#ifndef __PIZZAFRENZY_DECORATION_H__
#define __PIZZAFRENZY_DECORATION_H__

// A thing a city file puts on the map for the looks (0x4c bytes, constructor 0x4114b0, vtable 0x4fa63c): a picture of
// the tile manifest's "decoration" tiles, or a particle effect, in one of the layers of the game screen.

#include "DocHandler.h"
#include "ImageNode.h"
#include "Node.h"
#include "SignalParticles.h"

namespace pizzafrenzy
{

class CityMap;

class Decoration : public engine::Object
{
public:
	Decoration(const std::string& theName, int theLayer);  // 0x4114b0

	// slot 0x8: 0x411570; the manifest's decoration of the name (its picture and layer), or its particle effect's
	// layer; false when it has neither
	bool load();
	// slot 0xc: 0x411260; the decoration goes on the map
	void addTo(CityMap* theMap);
	// slot 0x10: 0x4112b0; translated with the game (it takes the decoration off the map)
	// slot 0x14: 0x411750; the node it has in the layer, or none; the picture's node is named "decoration"
	engine::Node* getNode(int theLayer);
	// slot 0x1c: 0x411210; the node's place, which the decoration remembers
	void setPosition(const engine::IntPair& thePosition);

	const std::string& getName() const { return mName; }  // +0x28

private:
	CityMap* mMap;                         // +0x0c
	int mLayer;                            // +0x10
	engine::Ref<engine::ImageNode> mNode;  // +0x14: the picture
	engine::Ref<engine::Image> mImage;     // +0x18
	// +0x1c: the particle system of an effect
	engine::Ref<engine::SignalParticleSystem> mParticleNode;
	engine::IntPair mPosition;             // +0x20
	std::string mName;                     // +0x28
};

} // namespace pizzafrenzy

#endif
