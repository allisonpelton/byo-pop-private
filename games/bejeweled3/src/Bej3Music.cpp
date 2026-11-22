// Sexy::Bej3Music: see the header.
#include "Bej3Music.h"

#include "XMLParser.h"

#include <cstdlib>

using namespace Sexy;

namespace {

// The value of an attribute, or the default when the element has none
std::string GetAttribute(const XMLElement& theElement, const std::string& theName, const std::string& theDefault = "")
{
	auto anAttribute = theElement.mAttributes.find(theName);
	return anAttribute == theElement.mAttributes.end() ? theDefault : anAttribute->second;
}

}  // namespace

bool Bej3Music::LoadMusicFile(const std::string& theFileName)
{
	XMLParser aParser;
	if (!aParser.OpenFile(theFileName)) {
		mHasError = true;
		mError = "Unable to open file " + theFileName;
		return false;
	}
	XMLElement anElement;
	if (!aParser.NextElement(&anElement) || anElement.mType != XMLElement::TYPE_START || anElement.mValue != "Music") {
		mHasError = true;
		mError = "Expected Music element";
		return false;
	}
	while (aParser.NextElement(&anElement)) {
		if (anElement.mType == XMLElement::TYPE_END)
			break;
		if (anElement.mType != XMLElement::TYPE_START)
			continue;
		if (anElement.mValue == "DefaultEvents") {
			// 0x738630: the events every song may use (FadeOut, FadeIn, Wait); they are read for their names
			// port: their volume curves come with the events' running (0x738f80)
			while (aParser.NextElement(&anElement) && anElement.mType != XMLElement::TYPE_END) {
			}
		} else if (anElement.mValue == "Song") {
			if (!ReadSong(&aParser, anElement.mAttributes))
				return false;
		} else {
			mHasError = true;
			mError = "Unknown element " + anElement.mValue;
			return false;
		}
	}
	if (aParser.HasFailed()) {
		mHasError = true;
		mError = aParser.GetErrorText();
		return false;
	}
	return true;
}

bool Bej3Music::ReadSong(XMLParser* theParser, const std::map<std::string, std::string>& theAttributes)
{
	XMLElement aSongElement;
	aSongElement.mAttributes = theAttributes;
	Song aSong;
	aSong.mName = GetAttribute(aSongElement, "Name");
	if (aSong.mName.empty() && aSongElement.mAttributes.find("Name") == aSongElement.mAttributes.end()) {
		mHasError = true;
		mError = "'Name' not specified on Song";
		return false;
	}
	aSong.mNoLoop = aSongElement.mAttributes.find("NoLoop") != aSongElement.mAttributes.end();
	aSong.mIsSoundEffect = aSongElement.mAttributes.find("Sfx") != aSongElement.mAttributes.end();
	if (aSongElement.mAttributes.find("Offset") != aSongElement.mAttributes.end())
		aSong.mOffset = atoi(GetAttribute(aSongElement, "Offset").c_str());
	if (aSongElement.mAttributes.find("File") == aSongElement.mAttributes.end()) {
		mHasError = true;
		mError = "'File' not specified on Song";
		return false;
	}
	aSong.mFile = GetAttribute(aSongElement, "File");
	if (aSongElement.mAttributes.find("Volume") != aSongElement.mAttributes.end())
		aSong.mVolume = atof(GetAttribute(aSongElement, "Volume").c_str());

	// 0x7399e0: a module file is loaded once, the first time a song names it
	auto aLoadedFile = mFileSongIds.find(aSong.mFile);
	if (aLoadedFile == mFileSongIds.end()) {
		int aSongId = (int)mFileSongIds.size();
		if (!LoadMusic(aSongId, aSong.mFile)) {
			mHasError = true;
			mError = "Failed to load " + aSong.mFile;
			return false;
		}
		aLoadedFile = mFileSongIds.insert(std::make_pair(aSong.mFile, aSongId)).first;
	}
	aSong.mSongId = aLoadedFile->second;

	XMLElement anElement;
	while (theParser->NextElement(&anElement)) {
		if (anElement.mType == XMLElement::TYPE_END && anElement.mValue == "Song")
			break;
		if (anElement.mType == XMLElement::TYPE_START && anElement.mValue == "Event") {
			Event anEvent;
			anEvent.mAttributes = anElement.mAttributes;
			aSong.mEvents[GetAttribute(anElement, "Name")] = anEvent;
		}
	}
	mSongs[aSong.mName] = aSong;
	return true;
}

void Bej3Music::QueueEvent(const std::string& theEventName, const std::string& theSongName, bool theFlag)
{
	// 0x738a70: a "Play" for a song that is queued already takes its place (the original removes the queued one)
	if (theEventName != "FadeOut") {
		for (auto anEvent = mQueue.begin(); anEvent != mQueue.end(); ++anEvent) {
			if (anEvent->mEventName == "Play" && anEvent->mSongName == theSongName) {
				mQueue.erase(anEvent);
				break;
			}
		}
	}
	QueuedEvent aQueuedEvent;
	aQueuedEvent.mSongName = theSongName;
	aQueuedEvent.mEventName = theEventName;
	aQueuedEvent.mFlag = theFlag;
	mQueue.push_back(aQueuedEvent);
}

void Bej3Music::Update()
{
	SDLMusicInterface::Update();
	// port: the original's queue (0x738f80) runs the events' volume curves, row syncs and tracks, which come with the
	// game's music; here a "Play" starts its song at its order and the rest are dropped
	while (!mQueue.empty()) {
		QueuedEvent anEvent = mQueue.front();
		mQueue.pop_front();
		if (anEvent.mEventName != "Play")
			continue;
		auto aSong = mSongs.find(anEvent.mSongName);
		if (aSong == mSongs.end())
			continue;
		StopAllMusic();
		PlayMusic(aSong->second.mSongId, aSong->second.mOffset, aSong->second.mNoLoop);
	}
}
