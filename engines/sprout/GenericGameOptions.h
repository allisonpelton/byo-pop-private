#ifndef __SPROUT_GENERICGAMEOPTIONS_H__
#define __SPROUT_GENERICGAMEOPTIONS_H__

// The player's preferences (its fields end at +0x4c, where a game's subclass goes on; constructor
// 0x479960 (pizzafrenzy), vtable 0x502edc (pizzafrenzy)): full screen, music and sound, kept in the registry key
// Software\<publisher>\<game> that the constructor opens. A game derives from it for its own values (load and save are
// the slots it extends).
//
// The original also keeps the link settings ShowLink, VisitURL and ILink (slots 0x1c to 0x30, read by load and written
// by save): they only served the original's outbound links, which the port removes, so nothing here keeps them.

#include "Object.h"
#include "PersistKey.h"

#include <string>

namespace engine
{

class GenericGameOptions : public Object
{
public:
	// 0x479960 (pizzafrenzy): the key Software\<publisher>\<game> opened (the application's slot 0x98)
	GenericGameOptions(const std::string& thePublisher, const std::string& theGame);
	~GenericGameOptions() override {}  // 0x479940 (pizzafrenzy)

	void setFullScreen(bool theFullScreen) { mFullScreen = theFullScreen; }  // 0x04: 0x4797d0 (pizzafrenzy)
	bool isFullScreen() { return mFullScreen; }                              // 0x08: 0x44e1e0 (pizzafrenzy)
	void setMusicEnabled(bool theEnabled) { mMusicEnabled = theEnabled; }    // 0x0c: 0x4797e0 (pizzafrenzy)
	bool isMusicEnabled() { return mMusicEnabled; }                          // 0x10: 0x4797f0 (pizzafrenzy)
	void setSoundEnabled(bool theEnabled) { mSoundEnabled = theEnabled; }    // 0x14: 0x479800 (pizzafrenzy)
	bool isSoundEnabled() { return mSoundEnabled; }                          // 0x18: 0x479810 (pizzafrenzy)

	// 0x34: 0x479ae0 (pizzafrenzy): each value from the registry, 1 (on) for one that is not there
	virtual void load();
	// 0x38: 0x479e50 (pizzafrenzy): each value written
	virtual void save();

protected:
	Ref<PersistKey> mKey;  // +0x0c
	bool mFullScreen;      // +0x10
	bool mMusicEnabled;    // +0x11
	bool mSoundEnabled;    // +0x12
};

} // namespace engine

#endif
