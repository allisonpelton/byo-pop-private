// Checks how Platform::Mixer keeps and plays sounds: a sound is stored as 16-bit samples at its file's own rate and
// channel count, without loss for 16-bit and 8-bit files, and a voice resamples it as it mixes. The voices must sound
// as they did when every sound was resampled to the device rate as stereo floats at load: the old pipeline is rebuilt
// here from the same samples (its load-time linear resampler, then its voice loop), and the two must match within
// float rounding at pitch 1, within the second resampling's error at other pitches, and in length. Given folders, it
// instead loads every sound in them and prints the memory they take now and took as stereo floats at 48 kHz. With
// -stream, each Ogg file given is loaded whole and streamed (Mixer::LoadStreamedSoundData), and the two are played on
// looping voices past the loop's end, at several pitches and with the pitch and gain changed on the way: the mixed
// output must be the same to the bit. With -segments, a track joined from Ogg segments as Rocket Mania joins its music
// (its PLAYLIST.txt's order, the segments decoded and written one after another into a WAV) is played against the same
// segments streamed in turn (Mixer::LoadSongSegments), past the loop's end twice, stopped and resumed on the way: the mixed
// output must be the same to the bit.
//   soundtest
//   soundtest <folder>...
//   soundtest -stream <file.ogg>...
//   soundtest -segments <game folder> <the pieces' common path> <last letter>...   (a path and a letter for each piece)
#include "platform/Audio.h"
#include "platform/VFS.h"

#include <SDL.h>  // main becomes SDL_main, which SDL2main's entry point calls
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

namespace
{

const int kRate = 48000;

int gChecks = 0;
int gFailed = 0;

void Expect(const char* theCase, const char* theWhat, double theGot, double theWanted, double theTolerance)
{
	gChecks++;
	if (std::fabs(theGot - theWanted) > theTolerance)
	{
		gFailed++;
		printf("sound: %s: %s is %.6f, not %.6f\n", theCase, theWhat, theGot, theWanted);
	}
}

void Put16(std::vector<uint8_t>& theOut, int theValue)
{
	theOut.push_back((uint8_t)theValue);
	theOut.push_back((uint8_t)(theValue >> 8));
}

void Put32(std::vector<uint8_t>& theOut, uint32_t theValue)
{
	for (int i = 0; i < 4; i++)
		theOut.push_back((uint8_t)(theValue >> (8 * i)));
}

// Two sines per channel, a different pair on the right: 16-bit values.
std::vector<int> Signal(int theChannels, int theRate, int theFrames)
{
	std::vector<int> s((size_t)theFrames * theChannels);
	for (int i = 0; i < theFrames; i++)
		for (int c = 0; c < theChannels; c++)
		{
			double t = (double)i / theRate;
			double v = 12000 * std::sin(2 * 3.14159265358979 * (440 + 220 * c) * t) +
				3000 * std::sin(2 * 3.14159265358979 * (1500 + 500 * c) * t);
			s[(size_t)i * theChannels + c] = (int)std::lround(v);
		}
	return s;
}

std::vector<uint8_t> Wav(int theChannels, int theRate, int theBits, const std::vector<int>& theSamples)
{
	std::vector<uint8_t> w;
	uint32_t aBytes = (uint32_t)(theSamples.size() * theBits / 8);
	w.insert(w.end(), {'R', 'I', 'F', 'F'});
	Put32(w, 36 + aBytes);
	w.insert(w.end(), {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '});
	Put32(w, 16);
	Put16(w, 1);
	Put16(w, theChannels);
	Put32(w, theRate);
	Put32(w, theRate * theChannels * theBits / 8);
	Put16(w, theChannels * theBits / 8);
	Put16(w, theBits);
	w.insert(w.end(), {'d', 'a', 't', 'a'});
	Put32(w, aBytes);
	for (int v : theSamples)
	{
		if (theBits == 16)
			Put16(w, v);
		else
			w.push_back((uint8_t)(v / 256 + 128));
	}
	return w;
}

// The old pipeline: the load-time resampler to stereo at the device rate...
std::vector<float> OldStereo(const Platform::SoundData& theData)
{
	std::vector<float> in(theData.mSamples.size());
	for (size_t i = 0; i < in.size(); i++)
		in[i] = theData.mSamples[i] / 32768.0f;
	int inFrames = (int)theData.mFrames;
	double ratio = (double)theData.mRate / kRate;
	size_t outFrames = (size_t)std::ceil(inFrames / ratio);
	std::vector<float> out(outFrames * 2);
	for (size_t i = 0; i < outFrames; i++)
	{
		double src = i * ratio;
		int i0 = (int)src;
		int i1 = std::min(i0 + 1, inFrames - 1);
		float t = (float)(src - i0);
		for (int c = 0; c < 2; c++)
		{
			int ch = std::min(c, theData.mChannels - 1);
			float a = in[i0 * theData.mChannels + ch];
			float b = in[i1 * theData.mChannels + ch];
			out[i * 2 + c] = a + (b - a) * t;
		}
	}
	return out;
}

// ...and its voice loop, which stepped through that copy by the pitch. Returns the frames it played.
int OldVoice(const std::vector<float>& theStereo, double thePitch, bool theLoop, int theFrames, std::vector<float>& theOut)
{
	theOut.assign((size_t)theFrames * 2, 0.0f);
	const float* src = theStereo.data();
	const double len = (double)(theStereo.size() / 2);
	double pos = 0;
	for (int i = 0; i < theFrames; i++)
	{
		if (pos >= len)
		{
			if (theLoop)
				pos = std::fmod(pos, len);
			else
				return i;
		}
		int i0 = (int)pos;
		int i1 = i0 + 1 < (int)len ? i0 + 1 : (theLoop ? 0 : i0);
		float t = (float)(pos - i0);
		theOut[i * 2] = src[i0 * 2] + (src[i1 * 2] - src[i0 * 2]) * t;
		theOut[i * 2 + 1] = src[i0 * 2 + 1] + (src[i1 * 2 + 1] - src[i0 * 2 + 1]) * t;
		pos += thePitch;
	}
	return theFrames;
}

// Plays the sound on a voice, a frame at a time so that its end is seen. Returns the frames it played.
int NewVoice(const std::shared_ptr<Platform::SoundData>& theData, double thePitch, bool theLoop, int theFrames,
	std::vector<float>& theOut)
{
	Platform::Mixer& mixer = Platform::Mixer::Get();
	theOut.assign((size_t)theFrames * 2, 0.0f);
	int voice = mixer.StartVoice(theData, 1.0f, 1.0f, thePitch, theLoop);
	int played = 0;
	for (int i = 0; i < theFrames && mixer.IsVoicePlaying(voice); i++)
	{
		float f[2];
		mixer.RenderOffline(f, 1);
		if (!mixer.IsVoicePlaying(voice))
			break;  // it ended on this frame and mixed nothing
		theOut[i * 2] = f[0];
		theOut[i * 2 + 1] = f[1];
		played++;
	}
	mixer.StopVoice(voice);
	return played;
}

// Loads a WAV of the signal and checks what is kept; then plays it at each pitch against the old pipeline.
void CheckSound(const char* theCase, int theChannels, int theRate, int theBits, double theSeconds)
{
	int frames = (int)(theRate * theSeconds);
	std::vector<int> signal = Signal(theChannels, theRate, frames);
	if (theBits == 8)
		for (int& v : signal)
			v = (v / 256) * 256;  // what an 8-bit file can hold
	std::vector<uint8_t> file = Wav(theChannels, theRate, theBits, signal);
	auto data = Platform::Mixer::Get().LoadSoundData(file, theCase);
	gChecks++;
	if (!data)
	{
		gFailed++;
		printf("sound: %s: not loaded\n", theCase);
		return;
	}
	Expect(theCase, "rate", data->mRate, theRate, 0);
	Expect(theCase, "channels", data->mChannels, theChannels, 0);
	Expect(theCase, "frames", (double)data->mFrames, frames, 0);
	Expect(theCase, "bytes", (double)data->Bytes(), (double)frames * theChannels * 2, 0);
	int wrong = 0;
	for (size_t i = 0; i < signal.size() && i < data->mSamples.size(); i++)
		if (data->mSamples[i] != signal[i])
			wrong++;
	Expect(theCase, "samples not as in the file", wrong, 0, 0);

	std::vector<float> old = OldStereo(*data);
	const double pitches[] = {1.0, 1.5, 0.75};
	for (double pitch : pitches)
	{
		char what[96];
		std::vector<float> a, b;
		int want = (int)(old.size() / 2 / pitch) + 64;
		int na = OldVoice(old, pitch, false, want, a);
		int nb = NewVoice(data, pitch, false, want, b);
		double maxDiff = 0;
		for (int i = 0; i < std::min(na, nb) * 2; i++)
			maxDiff = std::max(maxDiff, (double)std::fabs(a[i] - b[i]));
		// at pitch 1 both read the file's samples at the same places; at other pitches the old pipeline
		// interpolated twice, which differs by its own resampling error (here at most 0.005 of full scale)
		snprintf(what, sizeof(what), "pitch %.2f: largest difference from the old pipeline", pitch);
		Expect(theCase, what, maxDiff, 0, pitch == 1.0 ? 2e-6 : 0.01);
		snprintf(what, sizeof(what), "pitch %.2f: frames played", pitch);
		Expect(theCase, what, nb, na, 2);

		// Looping, over three passes and their wraps: the old copy's length was rounded up to a whole device frame,
		// so it is no reference after the first pass. The exact one: each frame at (i * step) mod the sound's
		// length, its last frame leading back to its first.
		double step = pitch * data->mRate / kRate;
		want = (int)(3 * frames / step) + 64;
		nb = NewVoice(data, pitch, true, want, b);
		maxDiff = 0;
		for (int i = 0; i < nb; i++)
		{
			double pos = std::fmod(i * step, (double)frames);
			int i0 = (int)pos, i1 = (i0 + 1) % frames;
			float t = (float)(pos - i0);
			for (int c = 0; c < 2; c++)
			{
				int ch = std::min(c, theChannels - 1);
				float s0 = data->mSamples[(size_t)i0 * theChannels + ch] / 32768.0f;
				float s1 = data->mSamples[(size_t)i1 * theChannels + ch] / 32768.0f;
				maxDiff = std::max(maxDiff, (double)std::fabs(s0 + (s1 - s0) * t - b[i * 2 + c]));
			}
		}
		snprintf(what, sizeof(what), "pitch %.2f looping: largest difference over three passes", pitch);
		Expect(theCase, what, maxDiff, 0, 1e-5);
		snprintf(what, sizeof(what), "pitch %.2f looping: frames played", pitch);
		Expect(theCase, what, nb, want, 0);
	}
}

// A Sun .au with 8-bit linear samples (encoding 2): kept as 16-bit at its rate.
void CheckAU()
{
	std::vector<uint8_t> f = {'.', 's', 'n', 'd', 0, 0, 0, 24, 0, 0, 0, 100, 0, 0, 0, 2, 0, 0, 0x1f, 0x40, 0, 0, 0, 1};
	for (int i = 0; i < 100; i++)
		f.push_back((uint8_t)(128 + (i % 10) * 10));
	auto data = Platform::Mixer::Get().LoadSoundData(f, "au");
	gChecks++;
	if (!data)
	{
		gFailed++;
		printf("sound: au: not loaded\n");
		return;
	}
	Expect("au 8-bit 8000 Hz", "rate", data->mRate, 8000, 0);
	Expect("au 8-bit 8000 Hz", "channels", data->mChannels, 1, 0);
	Expect("au 8-bit 8000 Hz", "frames", (double)data->mFrames, 100, 0);
	Expect("au 8-bit 8000 Hz", "sample 7", data->mSamples[7], 70 * 256, 0);
}

// The sounds under a folder: their memory now, and as stereo floats at the device rate.
void Measure(const char* theFolder)
{
	namespace fs = std::filesystem;
	int count = 0, failed = 0;
	double now = 0, before = 0;
	std::error_code ec;
	for (auto it = fs::recursive_directory_iterator(theFolder, ec); it != fs::recursive_directory_iterator(); ++it)
	{
		std::string ext = it->path().extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)std::tolower(c); });
		if (!it->is_regular_file() || (ext != ".ogg" && ext != ".wav" && ext != ".au" && ext != ".mp3"))
			continue;
		std::ifstream in(it->path(), std::ios::binary);
		std::vector<uint8_t> file((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
		auto data = Platform::Mixer::Get().LoadSoundData(file, it->path().string());
		if (!data || data->mFrames == 0)
		{
			failed++;
			continue;
		}
		count++;
		now += (double)data->Bytes();
		before += std::ceil((double)data->mFrames * kRate / data->mRate) * 2 * sizeof(float);
	}
	printf("sounds %s: %d files (%d not decoded), %.1f MB, were %.1f MB\n", theFolder, count, failed, now / 1048576,
		before / 1048576);
}

}

// the whole and the streamed sound of one Ogg file, each alone on a looping voice for its length and two seconds more, in
// blocks of 1000 frames (a pitch change at a third of the way, a gain change at two thirds), compared to the bit
void CheckStreamed(const char* thePath)
{
	std::ifstream aStream(thePath, std::ios::binary);
	std::vector<uint8_t> aFile((std::istreambuf_iterator<char>(aStream)), std::istreambuf_iterator<char>());
	Platform::Mixer& mixer = Platform::Mixer::Get();
	// the time each load takes, and each mix at the first pitch (the processor time streaming moves into the mix)
	auto aMs = [](std::chrono::steady_clock::time_point theFrom) {
		return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - theFrom).count();
	};
	auto aStart = std::chrono::steady_clock::now();
	std::shared_ptr<Platform::SoundData> aWhole = mixer.LoadSoundData(aFile, thePath);
	double aWholeMs = aMs(aStart);
	aStart = std::chrono::steady_clock::now();
	std::shared_ptr<Platform::SoundData> aStreamed = mixer.LoadStreamedSoundData(aFile, thePath);
	double aStreamedMs = aMs(aStart);
	double aMixMs[2] = {0, 0};
	gChecks++;
	if (!aWhole || !aStreamed || !aStreamed->mStreamFile || aWhole->mFrames != aStreamed->mFrames)
	{
		gFailed++;
		printf("sound: %s: not streamed, or %u frames streamed against %u whole\n", thePath, aStreamed ? (unsigned)aStreamed->mFrames : 0u,
			aWhole ? (unsigned)aWhole->mFrames : 0u);
		return;
	}
	const double aPitches[] = {1.0, 22050.0 / 48000.0, 1.37};
	for (double aPitch : aPitches)
	{
		std::vector<float> anOut[2];
		const int aFrames = (int)((double)aWhole->mFrames / aWhole->mRate * kRate / aPitch) + 2 * kRate;
		for (int k = 0; k < 2; k++)
		{
			anOut[k].assign((size_t)aFrames * 2, 0.0f);
			int aVoice = mixer.StartVoice(k == 0 ? aWhole : aStreamed, 0.8f, 0.6f, aPitch, true);
			aStart = std::chrono::steady_clock::now();
			for (int i = 0; i < aFrames; i += 1000)
			{
				if (i / 1000 == aFrames / 3000)
					mixer.SetVoicePitch(aVoice, aPitch * 1.01);
				if (i / 1000 == 2 * aFrames / 3000)
					mixer.SetVoiceGain(aVoice, 0.5f, 0.9f);
				mixer.RenderOffline(&anOut[k][(size_t)i * 2], std::min(1000, aFrames - i));
			}
			mixer.StopVoice(aVoice);
			if (aPitch == aPitches[0])
				aMixMs[k] = aMs(aStart);
		}
		gChecks++;
		if (memcmp(anOut[0].data(), anOut[1].data(), anOut[0].size() * sizeof(float)) != 0)
		{
			gFailed++;
			size_t aFirst = 0;
			while (anOut[0][aFirst] == anOut[1][aFirst])
				aFirst++;
			printf("sound: %s: at pitch %.3f the streamed voice differs from frame %u\n", thePath, aPitch, (unsigned)(aFirst / 2));
		}
	}
	printf("sound: %s: %u frames; load %.1f ms whole, %.1f ms streamed; the mix at pitch 1 (%.1f s) %.1f ms whole, %.1f ms streamed\n",
		thePath, (unsigned)aWhole->mFrames, aWholeMs, aStreamedMs, (double)aWhole->mFrames / aWhole->mRate + 2.0, aMixMs[0],
		aMixMs[1]);
}

// a PLAYLIST.txt's segment letters, as Rocket Mania reads them (games/rocketmania/src/AppMusic.cpp, ReadPlaylistLetters:
// scanf's "%[^\n]\n" three times, then "%c%*[^\n]\n" for at most 50 letters; each "\n" skips all white space)
std::string PlaylistLetters(const std::vector<uint8_t>& theFile)
{
	size_t aPos = 0;
	const size_t aSize = theFile.size();
	auto aRead = [&](char* theLetter) -> bool
	{
		if (aPos >= aSize)
			return false;
		if (theLetter != nullptr)
			*theLetter = (char)theFile[aPos++];
		while ((aPos < aSize) && (theFile[aPos] != '\n'))
			aPos++;
		while ((aPos < aSize) && isspace(theFile[aPos]))
			aPos++;
		return true;
	};
	for (int i = 0; i < 3; i++)
		aRead(nullptr);
	std::string aLetters;
	char aLetter = 0;
	while ((aLetters.size() < 50) && aRead(&aLetter))
		aLetters.push_back(aLetter);
	return aLetters;
}

// the bytes the loaded songs hold for their tracks (Mixer::GetSongMemory)
size_t SongBytes()
{
	int aCount = 0;
	size_t aTrack = 0, aModules = 0;
	Platform::Mixer::Get().GetSongMemory(&aCount, &aTrack, &aModules);
	return aTrack;
}

// One piece of segments (thePrefix "music/<name>", its segments " A.ogg" to theLast, its order in " PLAYLIST.txt"): the WAV
// the game would join from them, loaded as a song, against LoadSongSegments' song of the same files. Each plays alone for
// twice its length and two seconds more in blocks of 1000 frames, its gain changed at a third and the song stopped and resumed
// (PlaySong without a reset: from the frame it had reached) at two points, and the two outputs are compared to the bit.
void CheckSegments(const std::string& thePrefix, char theLast)
{
	Platform::Mixer& mixer = Platform::Mixer::Get();
	Platform::VFS& aVFS = Platform::VFS::Get();
	auto aMs = [](std::chrono::steady_clock::time_point theFrom) {
		return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - theFrom).count();
	};
	std::vector<uint8_t> aPlaylist;
	gChecks++;
	if (!aVFS.ReadAll(thePrefix + " PLAYLIST.txt", aPlaylist))
	{
		gFailed++;
		printf("segments: %s: no playlist\n", thePrefix.c_str());
		return;
	}
	std::vector<std::string> aPaths;
	for (char aLetter : PlaylistLetters(aPlaylist))
		if ((aLetter >= 'A') && (aLetter <= theLast) && aVFS.Exists(thePrefix + " " + aLetter + ".ogg"))
			aPaths.push_back(thePrefix + " " + aLetter + ".ogg");

	// the joined WAV, as the game's AssembleMusic writes it: each segment decoded (LoadSoundData) and its samples written in
	// the order, after a header in the first one's format
	auto aStart = std::chrono::steady_clock::now();
	std::vector<std::shared_ptr<Platform::SoundData> > aDecoded;
	std::vector<std::string> aDecodedPaths;
	std::vector<uint8_t> aSamples;
	size_t aWavFrames = 0;
	for (const std::string& aPath : aPaths)
	{
		size_t k = std::find(aDecodedPaths.begin(), aDecodedPaths.end(), aPath) - aDecodedPaths.begin();
		if (k == aDecodedPaths.size())
		{
			std::vector<uint8_t> aFile;
			aVFS.ReadAll(aPath, aFile);
			aDecoded.push_back(mixer.LoadSoundData(aFile, aPath));
			aDecodedPaths.push_back(aPath);
		}
		for (int16_t aSample : aDecoded[k]->mSamples)
			Put16(aSamples, aSample);
		aWavFrames += aDecoded[k]->mFrames;
	}
	const int aChannels = aDecoded.empty() ? 1 : aDecoded[0]->mChannels;
	const int aRate = aDecoded.empty() ? 44100 : aDecoded[0]->mRate;
	std::vector<uint8_t> aWav;
	aWav.insert(aWav.end(), {'R', 'I', 'F', 'F'});
	Put32(aWav, (uint32_t)aSamples.size() + 36);
	aWav.insert(aWav.end(), {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '});
	Put32(aWav, 16);
	Put16(aWav, 1);
	Put16(aWav, aChannels);
	Put32(aWav, (uint32_t)aRate);
	Put32(aWav, (uint32_t)(aRate * aChannels * 2));
	Put16(aWav, aChannels * 2);
	Put16(aWav, 16);
	aWav.insert(aWav.end(), {'d', 'a', 't', 'a'});
	Put32(aWav, (uint32_t)aSamples.size());
	aWav.insert(aWav.end(), aSamples.begin(), aSamples.end());
	aDecoded.clear();
	aSamples = std::vector<uint8_t>();
	const std::string aWavName = thePrefix + " joined.wav";
	aVFS.SetGeneratedFile(aWavName, std::string(aWav.begin(), aWav.end()));
	aWav = std::vector<uint8_t>();
	size_t aBytes = SongBytes();
	const int aJoined = mixer.LoadSong(aWavName);
	aVFS.SetGeneratedFile(aWavName, std::string());
	const double aJoinedMs = aMs(aStart);
	const size_t aJoinedBytes = SongBytes() - aBytes;
	aBytes = SongBytes();
	aStart = std::chrono::steady_clock::now();
	const int aStreamed = mixer.LoadSongSegments(aPaths);
	const double aStreamedMs = aMs(aStart);
	const size_t aStreamedBytes = SongBytes() - aBytes;
	if (!aJoined || !aStreamed)
	{
		gFailed++;
		printf("segments: %s: not loaded (joined %d, streamed %d)\n", thePrefix.c_str(), aJoined, aStreamed);
		return;
	}
	const int aFrames = (int)((double)aWavFrames / aRate * kRate * 2) + 2 * kRate;
	std::vector<float> anOut[2];
	double aMixMs[2] = {0, 0};
	const int aStops[2] = {aFrames / 5000, 3 * aFrames / 5000};  // blocks after which the song is stopped and resumed
	for (int k = 0; k < 2; k++)
	{
		const int aSong = (k == 0) ? aJoined : aStreamed;
		anOut[k].assign((size_t)aFrames * 2, 0.0f);
		mixer.SetSongGain(aSong, 0.8f);
		mixer.PlaySong(aSong, 0, 0, true, true);
		aStart = std::chrono::steady_clock::now();
		for (int i = 0; i < aFrames; i += 1000)
		{
			if (i / 1000 == aFrames / 3000)
				mixer.SetSongGain(aSong, 0.5f);
			if ((i / 1000 == aStops[0]) || (i / 1000 == aStops[1]))
			{
				mixer.StopSong(aSong);
				mixer.PlaySong(aSong, 0, 0, true, false);
			}
			mixer.RenderOffline(&anOut[k][(size_t)i * 2], std::min(1000, aFrames - i));
		}
		aMixMs[k] = aMs(aStart);
		mixer.StopSong(aSong);
	}
	gChecks++;
	if (memcmp(anOut[0].data(), anOut[1].data(), anOut[0].size() * sizeof(float)) != 0)
	{
		gFailed++;
		size_t aFirst = 0;
		while (anOut[0][aFirst] == anOut[1][aFirst])
			aFirst++;
		printf("segments: %s: the streamed song differs from the joined WAV's from frame %u\n", thePrefix.c_str(), (unsigned)(aFirst / 2));
	}
	gChecks++;
	bool aHeard = false;
	for (float aSample : anOut[1])
		aHeard = aHeard || (aSample != 0.0f);
	if (!aHeard)
	{
		gFailed++;
		printf("segments: %s: silent\n", thePrefix.c_str());
	}
	mixer.UnloadSong(aJoined);
	mixer.UnloadSong(aStreamed);
	printf("segments: %s: %u segments of %u files, %u frames; load %.1f ms joined, %.1f ms streamed; held %.2f MB joined, %.2f MB "
		"streamed; the mix (%.1f s) %.1f ms joined, %.1f ms streamed\n", thePrefix.c_str(), (unsigned)aPaths.size(),
		(unsigned)aDecodedPaths.size(), (unsigned)aWavFrames, aJoinedMs, aStreamedMs, aJoinedBytes / 1048576.0,
		aStreamedBytes / 1048576.0, (double)aFrames / kRate, aMixMs[0], aMixMs[1]);
}

// The voice's position (StartVoice's start frame, GetVoicePosition, SetVoicePosition, SetVoiceLoop: a DirectSound buffer stopped
// and played on, Chuzzle's): on the game's clock (-fast) and mixed. A one-second mono sound at the mixer's rate, so that a voice
// moves one frame of it for each frame mixed.
void CheckVoicePosition()
{
	Platform::Mixer& mixer = Platform::Mixer::Get();
	const char* aCase = "voice position";
	auto aData = mixer.LoadSoundData(Wav(1, kRate, 16, Signal(1, kRate, kRate)), aCase);
	const double aLength = (double)aData->mFrames;
	mixer.StopAllVoices();
	// on the game's clock: a voice started at frame 12000 is there, and 10 ms later 480 frames on
	mixer.SetGameClocked(true);
	int aVoice = mixer.StartVoice(aData, 1.0f, 1.0f, 1.0, false, 12000.0);
	Expect(aCase, "the start frame", mixer.GetVoicePosition(aVoice), 12000.0, 0);
	mixer.AdvanceVoices(10.0);
	Expect(aCase, "10 ms on", mixer.GetVoicePosition(aVoice), 12480.0, 1e-6);
	mixer.SetVoicePosition(aVoice, 40000.0);
	Expect(aCase, "moved", mixer.GetVoicePosition(aVoice), 40000.0, 0);
	mixer.SetVoicePosition(aVoice, 1e9);
	Expect(aCase, "moved past the end", mixer.GetVoicePosition(aVoice), aLength - 1.0, 0);
	mixer.SetVoicePosition(aVoice, -5.0);
	Expect(aCase, "moved before the start", mixer.GetVoicePosition(aVoice), 0.0, 0);
	// a one-shot voice that reaches its end: no longer playing, its position -1, and its handle reaches nothing after its
	// slot starts another voice
	mixer.AdvanceVoices(1000.0 * aLength / kRate + 1.0);
	Expect(aCase, "ended", mixer.IsVoicePlaying(aVoice) ? 1 : 0, 0, 0);
	Expect(aCase, "ended, its position", mixer.GetVoicePosition(aVoice), -1.0, 0);
	int anOther = mixer.StartVoice(aData, 1.0f, 1.0f, 1.0, true, 0.0);
	mixer.SetVoicePosition(aVoice, 30000.0);
	mixer.SetVoiceLoop(aVoice, false);
	Expect(aCase, "a stale handle", mixer.GetVoicePosition(aVoice), -1.0, 0);
	Expect(aCase, "the slot's new voice, untouched", mixer.GetVoicePosition(anOther), 0.0, 0);
	// a looping voice goes round; with SetVoiceLoop(false) it stops at the sound's end instead
	mixer.AdvanceVoices(1000.0 * (aLength + 100.0) / kRate);
	Expect(aCase, "looped", mixer.GetVoicePosition(anOther), 100.0, 1e-6);
	mixer.SetVoiceLoop(anOther, false);
	mixer.AdvanceVoices(1000.0 * (aLength - 200.0) / kRate);
	Expect(aCase, "loop off, before the end", mixer.IsVoicePlaying(anOther) ? 1 : 0, 1, 0);
	mixer.AdvanceVoices(1000.0 * 200.0 / kRate);
	Expect(aCase, "loop off, at the end", mixer.IsVoicePlaying(anOther) ? 1 : 0, 0, 0);
	// and back on: a voice set to loop again goes round
	int aThird = mixer.StartVoice(aData, 1.0f, 1.0f, 1.0, false, aLength - 100.0);
	mixer.SetVoiceLoop(aThird, true);
	mixer.AdvanceVoices(1000.0 * 300.0 / kRate);
	Expect(aCase, "loop on", mixer.GetVoicePosition(aThird), 200.0, 1e-6);
	mixer.StopVoice(aThird);
	Expect(aCase, "stopped", mixer.GetVoicePosition(aThird), -1.0, 0);
	mixer.SetGameClocked(false);

	// mixed: a voice started at frame kAt mixes what a voice started at 0 mixes from its frame kAt on, to the bit, and one
	// moved there by SetVoicePosition does too (kAt not a whole number of the test signal's periods, which 12000 frames are)
	const int kAt = 12345;
	const int kFrames = 20000;
	std::vector<float> aFromStart((size_t)(kAt + kFrames) * 2), aStarted((size_t)kFrames * 2), aMoved((size_t)kFrames * 2);
	aVoice = mixer.StartVoice(aData, 0.8f, 0.6f, 1.0, false);
	mixer.RenderOffline(aFromStart.data(), kAt + kFrames);
	mixer.StopVoice(aVoice);
	aVoice = mixer.StartVoice(aData, 0.8f, 0.6f, 1.0, false, (double)kAt);
	mixer.RenderOffline(aStarted.data(), kFrames);
	mixer.StopVoice(aVoice);
	aVoice = mixer.StartVoice(aData, 0.8f, 0.6f, 1.0, false);
	mixer.SetVoicePosition(aVoice, (double)kAt);
	mixer.RenderOffline(aMoved.data(), kFrames);
	Expect(aCase, "mixed on", mixer.GetVoicePosition(aVoice), (double)(kAt + kFrames), 1e-6);
	mixer.StopVoice(aVoice);
	gChecks += 2;
	if (memcmp(aStarted.data(), &aFromStart[kAt * 2], aStarted.size() * sizeof(float)) != 0)
	{
		gFailed++;
		printf("sound: %s: a voice started at frame %d differs from one started at 0, %d frames on\n", aCase, kAt, kAt);
	}
	if (memcmp(aMoved.data(), &aFromStart[kAt * 2], aMoved.size() * sizeof(float)) != 0)
	{
		gFailed++;
		printf("sound: %s: a voice moved to frame %d differs from one started at 0, %d frames on\n", aCase, kAt, kAt);
	}
}

int main(int argc, char** argv)
{
	Platform::Mixer::Get().InitOffline(kRate);
	if ((argc > 2) && (strcmp(argv[1], "-segments") == 0))
	{
		Platform::VFS::Get().SetBaseDir(argv[2]);
		for (int i = 3; i + 1 < argc; i += 2)
			CheckSegments(argv[i], argv[i + 1][0]);
		printf("sound segments: %d checks, %d failed\n", gChecks, gFailed);
		return gFailed ? 1 : 0;
	}
	if ((argc > 1) && (strcmp(argv[1], "-stream") == 0))
	{
		for (int i = 2; i < argc; i++)
			CheckStreamed(argv[i]);
		printf("sound streamed: %d checks, %d failed\n", gChecks, gFailed);
		return gFailed ? 1 : 0;
	}
	if (argc > 1)
	{
		for (int i = 1; i < argc; i++)
			Measure(argv[i]);
		return 0;
	}
	CheckSound("mono 16-bit 22050 Hz", 1, 22050, 16, 0.2);
	CheckSound("stereo 16-bit 44100 Hz", 2, 44100, 16, 0.2);
	CheckSound("stereo 16-bit 48000 Hz", 2, 48000, 16, 0.2);
	CheckSound("mono 8-bit 11025 Hz", 1, 11025, 8, 0.2);
	CheckAU();
	CheckVoicePosition();

	printf("sound: %d checks, %d failed\n", gChecks, gFailed);
	return gFailed ? 1 : 0;
}
