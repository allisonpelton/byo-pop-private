// Checks how Platform::Mixer keeps and plays sounds: a sound is stored as 16-bit samples at its file's own rate and
// channel count, without loss for 16-bit and 8-bit files, and a voice resamples it as it mixes. The voices must sound
// as they did when every sound was resampled to the device rate as stereo floats at load: the old pipeline is rebuilt
// here from the same samples (its load-time linear resampler, then its voice loop), and the two must match within
// float rounding at pitch 1, within the second resampling's error at other pitches, and in length. Given folders, it
// instead loads every sound in them and prints the memory they take now and took as stereo floats at 48 kHz.
//   soundtest
//   soundtest <folder>...
#include "platform/Audio.h"

#include <SDL.h>  // main becomes SDL_main, which SDL2main's entry point calls
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
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
		for (int loop = 0; loop < 2; loop++)
		{
			char what[96];
			// a looping voice is compared over its first pass only: the old copy's length was rounded up to a whole
			// device frame, so its later passes drift from the exact length by up to a frame each
			int want = (int)(old.size() / 2 / pitch) + 64;
			std::vector<float> a, b;
			int na = OldVoice(old, pitch, loop != 0, want, a);
			int nb = NewVoice(data, pitch, loop != 0, want, b);
			// (a looping voice's last fraction of a frame leads back to the first frame, which the old copy did not)
			int n = loop ? (int)((double)(frames - 1) * kRate / theRate / pitch) - 1 : std::min(na, nb);
			double maxDiff = 0;
			for (int i = 0; i < n * 2; i++)
				maxDiff = std::max(maxDiff, (double)std::fabs(a[i] - b[i]));
			// at pitch 1 both read the file's samples at the same places; at other pitches the old pipeline
			// interpolated twice, which differs by its own resampling error (here at most 0.005 of full scale)
			snprintf(what, sizeof(what), "pitch %.2f%s: largest difference", pitch, loop ? " looping" : "");
			Expect(theCase, what, maxDiff, 0, pitch == 1.0 ? 2e-6 : 0.01);
			snprintf(what, sizeof(what), "pitch %.2f%s: frames played", pitch, loop ? " looping" : "");
			Expect(theCase, what, nb, loop ? want : na, loop ? 0 : 2);
		}
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

int main(int argc, char** argv)
{
	Platform::Mixer::Get().InitOffline(kRate);
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
