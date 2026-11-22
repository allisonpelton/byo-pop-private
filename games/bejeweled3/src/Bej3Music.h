// Sexy::Bej3Music (the original's CustomBassMusicInterface of Bejeweled 3, vtable 0x8656c4 (bejeweled3), 30 slots,
// 0x150 bytes, made by 0x738970): the app's music interface. The game's songs are the orders of one module file, each
// with its events; properties\music.xml lists them and the game asks for a song by name and an event ("Play"). The
// port's base is SDLMusicInterface, which plays modules through libopenmpt.
#pragma once

#include "SDLMusicInterface.h"

#include <deque>
#include <map>
#include <string>

namespace Sexy {

class Bej3Music : public SDLMusicInterface {
public:
	// One <Event> of a song (0x7377b0, read by 0x7399e0)
	struct Event {
		std::map<std::string, std::string> mAttributes;  // as the file gives them: Volume's curve, RowSync, Offset, Tracks
	};
	// One <Song> (0x7399e0)
	struct Song {
		std::string mName;
		std::string mFile;                  // the module the song is in
		int mOffset = 0;                    // the order it starts at ("Offset")
		bool mNoLoop = false;               // "NoLoop"
		bool mIsSoundEffect = false;        // "Sfx"
		double mVolume = 1.0;               // "Volume"
		int mSongId = 0;                    // the module's id in the interface
		std::map<std::string, Event> mEvents;
	};
	// A song's event asked for and not yet carried out (0x738a70)
	struct QueuedEvent {
		std::string mSongName;
		std::string mEventName;
		bool mFlag = false;
	};

	std::map<std::string, Song> mSongs;         // by name
	std::map<std::string, int> mFileSongIds;    // the id each module file was loaded as
	std::deque<QueuedEvent> mQueue;
	bool mHasError = false;                     // +0x2c: the file could not be read
	std::string mError;

	// 0x73a890: reads properties\music.xml: the default events, then the songs; a failure sets the error and stops
	bool LoadMusicFile(const std::string& theFileName);
	// 0x738a70 (slot 29): asks for theEventName of theSongName; a "Play" event for a song replaces one queued
	void QueueEvent(const std::string& theEventName, const std::string& theSongName, bool theFlag);
	// 0x739980 (slot 19): the base's update, then the queue (0x738f80)
	void Update() override;

private:
	// 0x7399e0: one <Song> and its <Event>s; the parser has just read the start tag
	bool ReadSong(class XMLParser* theParser, const std::map<std::string, std::string>& theAttributes);
};

}  // namespace Sexy
