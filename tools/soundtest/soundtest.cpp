// Checks how Platform::Mixer keeps and plays sounds: a sound is stored as 16-bit samples at its file's own rate and
// channel count, without loss for 16-bit and 8-bit files, and a voice resamples it as it mixes. The voices must sound
// as they did when every sound was resampled to the device rate as stereo floats at load: the old pipeline is rebuilt
// here from the same samples (its load-time linear resampler, then its voice loop), and the two must match within
// float rounding at pitch 1, within the second resampling's error at other pitches, and in length. Given folders, it
// instead loads every sound in them and prints the memory they take now and took as stereo floats at 48 kHz. With
// -stream, each Ogg file given is loaded whole and streamed (Mixer::LoadStreamedSoundData), and the two are played on
// looping voices past the loop's end, at several pitches and with the pitch and gain changed on the way: the mixed
// output must be the same to the bit.
//   soundtest
//   soundtest <folder>...
//   soundtest -stream <file.ogg>...
#include "platform/Audio.h"

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

int main(int argc, char** argv)
{
	Platform::Mixer::Get().InitOffline(kRate);
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

	printf("sound: %d checks, %d failed\n", gChecks, gFailed);
	return gFailed ? 1 : 0;
}
