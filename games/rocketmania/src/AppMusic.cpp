// The music pieces of Rocket Mania Deluxe are assembled from segments (RocketManiaGame 0x429460, its MySound class
// 0x429880 and the WAV writer 0x4299e0): the in-game and the menu music are made of Ogg files "<name> A.ogg",
// "<name> B.ogg" and so on, and "<name> PLAYLIST.txt" lists the order in which they are joined. The original
// writes the result as "cached_<name>.wav" next to them the first time and plays that (hence its "Please wait,
// unpacking sounds..." on the first run). The port does the same in memory: Platform::VFS serves the joined WAV
// as that file. See docs/GAME_NOTES.md.

#include "RocketApp.h"
#include "platform/Audio.h"
#include "platform/VFS.h"

#include <cctype>
#include <cstdint>
#include <cstring>
#include <memory>

using namespace Sexy;

static void PutLE32(std::string& theOut, uint32_t theValue)
{
	for (int i = 0; i < 4; i++)
		theOut.push_back((char) ((theValue >> (8 * i)) & 0xff));
}

static void PutLE16(std::string& theOut, uint32_t theValue)
{
	theOut.push_back((char) (theValue & 0xff));
	theOut.push_back((char) ((theValue >> 8) & 0xff));
}

// the pieces of a PLAYLIST.txt: three header lines, then one segment letter at the start of each line, at most 50
// (the original reads them with scanf: "%[^\n]\n" three times, then "%c%*[^\n]\n" until the end of the file; the
// "\n" in a format skips all white space, so empty lines vanish)
static std::string ReadPlaylistLetters(const std::string& thePlaylist)
{
	size_t aPos = 0;
	size_t aSize = thePlaylist.size();
	// one scanf call: aWantLetter reads a character first, then everything up to the end of the line, then the white space
	// that follows; false means the end of the file was reached before the first character
	auto aRead = [&](bool aWantLetter, char* theLetter) -> bool
	{
		if (aPos >= aSize)
			return false;
		if (aWantLetter)
			*theLetter = thePlaylist[aPos++];
		while (aPos < aSize && thePlaylist[aPos] != '\n')
			aPos++;
		while (aPos < aSize && isspace((unsigned char) thePlaylist[aPos]))
			aPos++;
		return true;
	};

	std::string aLetters;
	for (int i = 0; i < 3; i++)
		aRead(false, NULL);
	char aLetter = 0;
	bool aGot = aRead(true, &aLetter);
	for (int i = 0; aGot && i < 50; i++)
	{
		aLetters.push_back(aLetter);
		aGot = aRead(true, &aLetter);
	}
	return aLetters;
}

// 0x429460: thePrefix is the pieces' common path ("music\Tubetricks Menu One v02"), theLastSegment the last letter
void RocketApp::AssembleMusic(const std::string& thePrefix, char theLastSegment)
{
	// the cache's name: "cached_" in front of the file name part, ".wav" after it
	size_t aSlash = thePrefix.find_last_of("\\/");
	std::string aCacheName = (aSlash == std::string::npos) ? std::string() : thePrefix.substr(0, aSlash + 1);
	aCacheName += "cached_" + thePrefix.substr(aSlash == std::string::npos ? 0 : aSlash + 1) + ".wav";

	Platform::VFS& aVFS = Platform::VFS::Get();
	if (aVFS.Exists(aCacheName))
		return;

	// the segments, A to the last letter
	std::shared_ptr<Platform::SoundData> aSegments[26];
	int aNumSegments = 0;
	for (char aLetter = 'A'; aLetter <= theLastSegment; aLetter++)
	{
		std::string aName = thePrefix + " " + aLetter + ".ogg";
		std::vector<uint8_t> aFile;
		if (aVFS.ReadAll(aName, aFile) && !aFile.empty())
			aSegments[aLetter - 'A'] = Platform::Mixer::Get().LoadSoundData(aFile, aName);
		aNumSegments++;
	}

	std::vector<uint8_t> aPlaylistFile;
	if (!aVFS.ReadAll(thePrefix + " PLAYLIST.txt", aPlaylistFile))
		return;
	std::string aLetters = ReadPlaylistLetters(std::string(aPlaylistFile.begin(), aPlaylistFile.end()));

	// the playlist's segments in order, and the length of the whole
	std::vector<Platform::SoundData*> anOrder;
	size_t aDataBytes = 0;
	for (size_t i = 0; i < aLetters.size(); i++)
	{
		int anIndex = aLetters[i] - 'A';
		if (anIndex < 0 || anIndex >= aNumSegments || !aSegments[anIndex])
			continue;
		anOrder.push_back(aSegments[anIndex].get());
		aDataBytes += aSegments[anIndex]->mSamples.size() * 2;
	}
	if (anOrder.empty())
		return;

	// 0x4299e0: the RIFF header in the first piece's format, then every piece's samples
	Platform::SoundData* aFirst = anOrder[0];
	uint32_t aBlockAlign = (uint32_t) aFirst->mChannels * 2;
	std::string aWav;
	aWav.reserve(aDataBytes + 44);
	aWav += "RIFF";
	PutLE32(aWav, (uint32_t) aDataBytes + 0x24);
	aWav += "WAVEfmt ";
	PutLE32(aWav, 0x10);
	PutLE16(aWav, 1);
	PutLE16(aWav, (uint32_t) aFirst->mChannels);
	PutLE32(aWav, (uint32_t) aFirst->mRate);
	PutLE32(aWav, aBlockAlign * (uint32_t) aFirst->mRate);
	PutLE16(aWav, aBlockAlign);
	PutLE16(aWav, 16);
	aWav += "data";
	PutLE32(aWav, (uint32_t) aDataBytes);
	for (size_t i = 0; i < anOrder.size(); i++)
	{
		// 16-bit samples, little-endian on every target
		for (size_t s = 0; s < anOrder[i]->mSamples.size(); s++)
			PutLE16(aWav, (uint16_t) anOrder[i]->mSamples[s]);
	}
	aVFS.SetGeneratedFile(aCacheName, aWav);
}
