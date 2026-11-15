#ifndef __PIZZAFRENZY_TILEMANIFEST_H__
#define __PIZZAFRENZY_TILEMANIFEST_H__

// The game's data (0x98 bytes, constructor 0x423090, vtable 0x4fb468; the application keeps it at +0x124): the tiles of
// the map, the vehicles, the customers, the toppings, the particle effects, the special events, the music and the score
// titles, read by the loading state from res\manifests\tileManifest.xml and found by name.
//
// Left out of what the manifest reads: the particle systems of the effects (the signal build's particle document
// handler, 0x47bc40, is not translated), and the previews of the toppings (Topping::setImage). The finders no state of
// the loading reaches are not translated: getVehicle 0x422ca0 and getCharacter 0x422d60, nor the destructor 0x4263c0.

#include "GameMusicPlayer.h"
#include "Object.h"
#include "TileEntries.h"

#include <map>
#include <string>
#include <vector>

namespace pizzafrenzy
{

class TileManifest : public engine::Object
{
public:
	TileManifest() {}  // 0x423090

	// 0x41f8c0: reads the file; throws an Exception when it cannot be read
	void load(const std::string& theFile);

	// 0x422d80: the topping of that name, null when there is none
	Topping* getTopping(const std::string& theName);
	// 0x422c00: the tile of that name of the class (not "roads", whose tiles getRoadTile gives), null when there is none
	TileType* getTile(const std::string& theName, const std::string& theClass);
	// 0x422d30: the picture of a road's sheet, 0 to 15, null when the road has none; the original makes an empty entry
	// for a name it does not have, which the port does not
	TileType* getRoadTile(const std::string& theName, int theIndex);
	// 0x422dd0: the particle effect of that name, null when there is none
	ParticleEffect* getParticleEffect(const std::string& theName);
	// The toppings by name (+0x30)
	std::map<std::string, engine::Ref<Topping>>& getToppings() { return mToppings; }
	// The toppings in the order the manifest gives them (+0x3c)
	std::vector<engine::Ref<Topping>>& getToppingList() { return mToppingList; }
	// The score titles in the order the manifest gives them (+0x70)
	std::vector<engine::Ref<ScoreTitle>>& getTitles() { return mTitles; }
	// 0x422df0: the special event of that name, null when there is none
	SpecialEvent* getEvent(const std::string& theName);
	// 0x422f90: the customer "<name>_<theme>", the theme the one of the city that is played; null when there is none
	Character* getCharacterByNameTheme(const std::string& theName);
	// 0x422e10: the music clip of that name, null when there is none
	GameMusicClip* getMusic(const std::string& theName);

	// The registrations of the manifest's elements; each throws an Exception for a name that is there already
	void registerTile(const std::string& theName, const std::string& theClass, TileType* theTile);  // 0x422e60
	void addRoadTiles(const std::string& theName, const std::string& thePath);                       // 0x4229f0
	void addDecoration(const std::string& theName, const std::string& theClass, const std::string& thePath,
	                   int theLayer, const std::string& theHsvShift);                                 // 0x423980
	void registerCharacter(const std::string& theKey, Character* theCharacter);                       // 0x422590
	void registerTopping(const std::string& theName, Topping* theTopping);                            // 0x422660
	void registerParticleEffect(const std::string& theName, ParticleEffect* theEffect);               // 0x422780
	void registerEvent(const std::string& theName, SpecialEvent* theEvent);                           // 0x422850
	void registerMusic(const std::string& theName, GameMusicClip* theClip);                           // 0x422920
	void addFunFact(const std::string& theText);                                                      // 0x4223e0
	void addTitle(ScoreTitle* theTitle);                                                              // 0x421970

private:
	// by class, then by name
	std::map<std::string, std::map<std::string, engine::Ref<TileType>>> mTiles;           // +0x0c
	std::map<std::string, std::vector<engine::Ref<TileType>>> mRoadTiles;                 // +0x18: 16 of each
	std::map<std::string, engine::Ref<Character>> mCharacters;                            // +0x24
	std::map<std::string, engine::Ref<Topping>> mToppings;                                // +0x30
	std::vector<engine::Ref<Topping>> mToppingList;                                       // +0x3c
	std::map<std::string, engine::Ref<ParticleEffect>> mParticleEffects;                  // +0x4c
	std::map<std::string, engine::Ref<SpecialEvent>> mEvents;                             // +0x58
	std::map<std::string, engine::Ref<GameMusicClip>> mMusic;                             // +0x64
	std::vector<engine::Ref<ScoreTitle>> mTitles;                                         // +0x70
	std::vector<std::string> mFunFacts;                                                   // +0x80
};

} // namespace pizzafrenzy

#endif
