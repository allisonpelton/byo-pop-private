// Decodes sound files with the port's own loader (Platform::Mixer::LoadSoundData, the decoder the game's sounds go
// through) and writes each as raw 16-bit PCM at its own rate and channels, with a line "<file> <rate> <channels>
// <frames>" on stdout, to compare with another decoder's output (NOTES.md, "Sound"). With --tremor first, an Ogg file
// goes through Tremor (third_party/tremor) instead, the loader's fallback decoder.
//   bjt_sounddecode [--tremor] <out dir> <sound file>...
// With --compare, each Ogg file is decoded both ways and nothing is written: a file whose two decodes differ by more
// than -40 dB of its level, and by more than 1% of full scale at some sample, is printed, and the exit code is 1 if
// there was one. Tremor is an integer decoder, about -50 to -60 dB from a float one on these files.
//   bjt_sounddecode --compare <sound file>...
#include "platform/Audio.h"

#include <ivorbisfile.h>

#include <SDL.h>  // main becomes SDL_main, which SDL2main's entry point calls
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

struct MemFile
{
	const std::vector<uint8_t>* mData;
	size_t mPos;
};

static size_t MemRead(void* thePtr, size_t theSize, size_t theCount, void* theSource)
{
	MemFile* f = (MemFile*)theSource;
	size_t n = std::min(theSize * theCount, f->mData->size() - f->mPos);
	memcpy(thePtr, f->mData->data() + f->mPos, n);
	f->mPos += n;
	return theSize ? n / theSize : 0;
}

static int MemSeek(void* theSource, ogg_int64_t theOffset, int theWhence)
{
	MemFile* f = (MemFile*)theSource;
	ogg_int64_t base = theWhence == SEEK_CUR ? (ogg_int64_t)f->mPos : theWhence == SEEK_END ? (ogg_int64_t)f->mData->size() : 0;
	if (base + theOffset < 0 || base + theOffset > (ogg_int64_t)f->mData->size())
		return -1;
	f->mPos = (size_t)(base + theOffset);
	return 0;
}

static int MemClose(void*)
{
	return 0;
}

static long MemTell(void* theSource)
{
	return (long)((MemFile*)theSource)->mPos;
}

static std::shared_ptr<Platform::SoundData> DecodeTremor(const std::vector<uint8_t>& theFile)
{
	MemFile src = {&theFile, 0};
	ov_callbacks cb = {MemRead, MemSeek, MemClose, MemTell};
	OggVorbis_File vf;
	if (ov_open_callbacks(&src, &vf, NULL, 0, cb) < 0)
		return nullptr;
	vorbis_info* info = ov_info(&vf, -1);
	auto data = std::make_shared<Platform::SoundData>();
	data->mChannels = info->channels;
	data->mRate = (int)info->rate;
	char buf[4096];
	int section = 0;
	long got;
	while ((got = ov_read(&vf, buf, sizeof(buf), &section)) > 0)
	{
		size_t at = data->mSamples.size();
		data->mSamples.resize(at + got / 2);
		memcpy(data->mSamples.data() + at, buf, (size_t)(got / 2) * 2);
	}
	ov_clear(&vf);
	data->mFrames = data->mSamples.size() / data->mChannels;
	return data;
}

static std::vector<uint8_t> ReadFile(const std::string& thePath)
{
	std::vector<uint8_t> file;
	FILE* f = fopen(thePath.c_str(), "rb");
	if (f == nullptr)
		return file;
	uint8_t buf[65536];
	size_t got;
	while ((got = fread(buf, 1, sizeof(buf), f)) > 0)
		file.insert(file.end(), buf, buf + got);
	fclose(f);
	return file;
}

static int Compare(int argc, char* argv[])
{
	int bad = 0, files = 0;
	double worst = -1000;
	std::string worstName;
	for (int i = 1; i < argc; i++)
	{
		std::vector<uint8_t> file = ReadFile(argv[i]);
		std::shared_ptr<Platform::SoundData> port = Platform::Mixer::Get().LoadSoundData(file, argv[i]);
		std::shared_ptr<Platform::SoundData> ref = DecodeTremor(file);
		if (!port || !ref || port->mChannels != std::min(ref->mChannels, 2))
		{
			printf("%s: %s\n", argv[i], !port ? "the port cannot decode it" : !ref ? "Tremor cannot decode it" : "channels differ");
			bad++;
			continue;
		}
		// the decoders may differ by a block at the end; compare the frames both have
		size_t frames = std::min(port->mFrames, ref->mFrames);
		double sig = 0, diff = 0, peak = 0;
		for (size_t f = 0; f < frames; f++)
			for (int c = 0; c < port->mChannels; c++)
			{
				double a = port->mSamples[f * port->mChannels + c] / 32768.0;
				double b = ref->mSamples[f * ref->mChannels + c] / 32768.0;
				sig += b * b;
				diff += (a - b) * (a - b);
				peak = std::max(peak, std::fabs(a - b));
			}
		double level = 10 * std::log10((diff + 1e-20) / (sig + 1e-20));
		files++;
		if (level > worst)
		{
			worst = level;
			worstName = argv[i];
		}
		if (level > -40 && peak > 0.01)
		{
			printf("%s: the port's decode differs from Tremor's by %.1f dB, peak %.3f\n", argv[i], level, peak);
			bad++;
		}
	}
	printf("%d files, %d differ; closest to failing %s at %.1f dB\n", files, bad, worstName.c_str(), worst);
	return bad ? 1 : 0;
}

int main(int argc, char* argv[])
{
	if (argc > 1 && std::string(argv[1]) == "--compare")
		return Compare(argc - 1, argv + 1);
	bool tremor = argc > 1 && std::string(argv[1]) == "--tremor";
	if (tremor)
	{
		argv++;
		argc--;
	}
	if (argc < 3)
	{
		fprintf(stderr, "usage: bjt_sounddecode <out dir> <sound file>...\n");
		return 2;
	}
	int failed = 0;
	for (int i = 2; i < argc; i++)
	{
		std::string path = argv[i];
		FILE* f = fopen(path.c_str(), "rb");
		if (f == nullptr)
		{
			printf("%s cannot read\n", path.c_str());
			failed++;
			continue;
		}
		std::vector<uint8_t> file;
		uint8_t buf[65536];
		size_t got;
		while ((got = fread(buf, 1, sizeof(buf), f)) > 0)
			file.insert(file.end(), buf, buf + got);
		fclose(f);
		std::shared_ptr<Platform::SoundData> data = tremor ? DecodeTremor(file) : Platform::Mixer::Get().LoadSoundData(file, path);
		if (!data)
		{
			printf("%s cannot decode\n", path.c_str());
			failed++;
			continue;
		}
		size_t slash = path.find_last_of("/\\");
		std::string out = std::string(argv[1]) + "/" + path.substr(slash == std::string::npos ? 0 : slash + 1) + ".pcm";
		FILE* o = fopen(out.c_str(), "wb");
		if (o != nullptr)
		{
			fwrite(data->mSamples.data(), sizeof(int16_t), data->mSamples.size(), o);
			fclose(o);
		}
		printf("%s %d %d %zu\n", path.c_str(), data->mRate, data->mChannels, data->mFrames);
	}
	return failed ? 1 : 0;
}
