#include "Audio.h"
#include "VFS.h"

#include <SDL.h>
#include <xmp.h>
#include <ivorbisfile.h>
#include <algorithm>
#include <cmath>
#include <cstring>

#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>
#include <minimp3.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace Platform
{

static const int kMaxVoices = 48;
static const int kVoiceSerialShift = 8;  // handle = serial << 8 | slot

struct Mixer::Voice
{
	std::shared_ptr<SoundData> mData;
	double mPos = 0.0;
	double mStep = 1.0;
	float mGainL = 1.0f, mGainR = 1.0f;
	bool mLoop = false;
	bool mActive = false;
	int mHandle = 0;
};

struct ModuleFormat
{
	ModuleDetector mDetect;
	ModuleFactory mCreate;
};

static std::vector<ModuleFormat>& ModuleFormats()
{
	static std::vector<ModuleFormat> sFormats;
	return sFormats;
}

void Mixer::RegisterModuleFormat(ModuleDetector theDetector, ModuleFactory theFactory)
{
	ModuleFormats().push_back({theDetector, theFactory});
}

// A song is either a tracker module (libxmp, or a registered ModulePlayer) or an audio track supplied by a mod. Ogg tracks are
// decoded while they play (a rendered 30-minute suite would need ~700 MB as float PCM); WAV tracks
// are decoded up front. Both are resampled to the device rate with linear interpolation.
struct Mixer::Song
{
	xmp_context mCtx = nullptr;
	std::unique_ptr<ModulePlayer> mModule;   // a registered module format (e.g. MO3 via libopenmpt)

	std::vector<uint8_t> mFile;              // Ogg bytes, kept alive for the decoder
	stb_vorbis* mVorbis = nullptr;
	std::shared_ptr<SoundData> mPcm;         // decoded WAV track (at its own rate, resampled as below)
	int mSourceRate = 0;
	int mSourceChannels = 0;
	int64_t mSourceFrames = 0;               // total length in source frames (0 = unknown)
	int64_t mPcmPos = 0;
	std::vector<float> mDecoded;             // Ogg decode buffer (interleaved, source channels)
	size_t mDecodedPos = 0;                  // next unread frame in mDecoded
	size_t mDecodedFrames = 0;
	float mFrameA[2] = {0, 0};               // resampler: previous and next source frame
	float mFrameB[2] = {0, 0};
	double mFrac = 0;
	bool mEnded = false;

	int64_t mStartFrame = 0;                 // source frame playback starts at (mod remap "start")
	int64_t mLoopStartFrame = 0;             // source frame playback loops back to ("loopStart")
	int mDefaultOrder = -1;                  // module order a mod remap starts this track at
	float mTrackGain = 1.0f;                 // per-track gain from a mod remap

	bool mLoaded = false;
	bool mPlaying = false;
	bool mPaused = false;
	bool mLoop = true;
	float mGain = 1.0f;
	float mCurGain = 1.0f;  // smoothed per buffer to avoid zipper noise on fades

	// The song's last frames before any gain, the two channels averaged, for GetSongFFT.
	static const int kHistoryFrames = 4096;
	float mHistory[kHistoryFrames] = {};
	int mHistoryPos = 0;

	void Remember(float theLeft, float theRight)
	{
		mHistory[mHistoryPos] = (theLeft + theRight) * 0.5f;
		mHistoryPos = (mHistoryPos + 1) & (kHistoryFrames - 1);
	}

	bool IsStream() const { return mVorbis || mPcm; }

	void SeekSource(int64_t theFrame)
	{
		theFrame = std::max<int64_t>(0, mSourceFrames > 0 ? std::min(theFrame, mSourceFrames - 1) : theFrame);
		if (mVorbis)
			stb_vorbis_seek(mVorbis, (unsigned)theFrame);
		mPcmPos = theFrame;
		mDecodedPos = mDecodedFrames = 0;
		mEnded = false;
	}

	// Reads the next source frame as stereo; at the end of the data it loops or reports the end.
	bool ReadSourceFrame(float* theOut)
	{
		for (int attempt = 0; attempt < 2; attempt++)
		{
			if (mPcm)
			{
				if (mPcmPos < (int64_t)mPcm->mFrames)
				{
					const int16_t* f = &mPcm->mSamples[mPcmPos * mPcm->mChannels];
					theOut[0] = f[0] / 32768.0f;
					theOut[1] = (mPcm->mChannels > 1 ? f[1] : f[0]) / 32768.0f;
					mPcmPos++;
					return true;
				}
			}
			else
			{
				if (mDecodedPos >= mDecodedFrames)
				{
					mDecoded.resize(1024 * mSourceChannels);
					int got = stb_vorbis_get_samples_float_interleaved(mVorbis, mSourceChannels, mDecoded.data(), (int)mDecoded.size());
					mDecodedFrames = got > 0 ? (size_t)got : 0;
					mDecodedPos = 0;
				}
				if (mDecodedPos < mDecodedFrames)
				{
					const float* f = &mDecoded[mDecodedPos * mSourceChannels];
					theOut[0] = f[0];
					theOut[1] = mSourceChannels > 1 ? f[1] : f[0];
					mDecodedPos++;
					return true;
				}
			}
			if (!mLoop)
				break;
			SeekSource(mLoopStartFrame);
		}
		mEnded = true;
		return false;
	}

	void Restart(int64_t theFrame)
	{
		SeekSource(theFrame);
		mFrac = 0;
		if (!ReadSourceFrame(mFrameA))
			mFrameA[0] = mFrameA[1] = 0;
		if (!ReadSourceFrame(mFrameB))
			mFrameB[0] = mFrameB[1] = 0;
	}
};

Mixer& Mixer::Get()
{
	static Mixer sMixer;
	return sMixer;
}

Mixer::~Mixer()
{
	Shutdown();
}

bool Mixer::Init()
{
	if (mDevice)
		return true;
	if (!SDL_WasInit(SDL_INIT_AUDIO) && SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
	{
		SDL_Log("audio init failed: %s", SDL_GetError());
		return false;
	}
#ifdef __EMSCRIPTEN__
	// SDL adopts the AudioContext's native rate (often 96/192 kHz) and sizes its ScriptProcessor in
	// frames at that rate, which underruns and crackles. SDL reuses an existing context, so create one
	// at 48 kHz first.
	EM_ASM({
		Module['SDL2'] = Module['SDL2'] || {};
		if (!Module['SDL2'].audioContext) {
			var Ctx = window.AudioContext || window.webkitAudioContext;
			if (Ctx) {
				try { Module['SDL2'].audioContext = new Ctx({ sampleRate: 48000 }); }
				catch (e) { Module['SDL2'].audioContext = new Ctx(); }
				var ctx = Module['SDL2'].audioContext;
				var resume = function() { if (ctx.state !== 'running') ctx.resume(); };
				// The array is parenthesized: a comma outside parentheses would end EM_ASM's code argument.
				(['pointerdown', 'keydown', 'touchstart']).forEach(function(t) { document.addEventListener(t, resume, true); });
			}
		}
	});
	// Test runs in a browser pane can be hidden, so they must not play sound. The page asks with
	// ?silent=1; SDL's dummy driver needs threads, so the device stays real and the callback outputs
	// silence after mixing, which keeps playback timing as it is (like rundesk's dummy driver on native).
	mSilent = EM_ASM_INT({ return Module['silentAudio'] ? 1 : 0; }) != 0;
	const int bufferFrames = 2048;
#else
	const int bufferFrames = 1024;
#endif
	SDL_AudioSpec want = {}, have = {};
	want.freq = 48000;
	want.format = AUDIO_F32SYS;
	want.channels = 2;
	want.samples = bufferFrames;
	want.callback = &Mixer::Callback;
	want.userdata = this;
	mDevice = SDL_OpenAudioDevice(nullptr, 0, &want, &have, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
	if (!mDevice)
	{
		SDL_Log("SDL_OpenAudioDevice failed: %s", SDL_GetError());
		return false;
	}
	mRate = have.freq;
	mVoices.clear();
	for (int i = 0; i < kMaxVoices; i++)
		mVoices.emplace_back(new Voice());
	SDL_PauseAudioDevice(mDevice, 0);
	SDL_Log("audio: %d Hz, %d frames/buffer", have.freq, have.samples);
	return true;
}

void Mixer::InitOffline(int theRate)
{
	mOffline = true;
	mRate = theRate;
	mVoices.clear();
	for (int i = 0; i < kMaxVoices; i++)
		mVoices.emplace_back(new Voice());
}

void Mixer::Shutdown()
{
	if (!mDevice)
		return;
	SDL_CloseAudioDevice(mDevice);
	mDevice = 0;
	for (auto& s : mSongs)
	{
		if (s && s->mCtx)
		{
			xmp_end_player(s->mCtx);
			xmp_release_module(s->mCtx);
			xmp_free_context(s->mCtx);
		}
	}
	mSongs.clear();
	mVoices.clear();
}

void Mixer::Lock()
{
	if (mDevice)
		SDL_LockAudioDevice(mDevice);
}

void Mixer::Unlock()
{
	if (mDevice)
		SDL_UnlockAudioDevice(mDevice);
}

// ---------------------------------------------------------------------------------------------
// Sound loading

// Keeps a decoded sound as 16-bit samples at its own rate, with its first one or two channels. The decoders give
// 16-bit or 8-bit values scaled by 1/32768, which this keeps exactly; only a WAV of 24 or 32 bits or floats is reduced
// to 16 bits (and clipped at full scale). A sound with no frames or no valid rate keeps none, and plays nothing.
static void StoreSound(const float* theIn, int theInFrames, int theInChannels, int theInRate, SoundData& theOut)
{
	if (theInFrames <= 0 || theInChannels <= 0 || theInRate <= 0)
		return;
	int channels = std::min(theInChannels, 2);
	theOut.mChannels = channels;
	theOut.mRate = theInRate;
	theOut.mFrames = (size_t)theInFrames;
	theOut.mSamples.resize((size_t)theInFrames * channels);
	for (size_t i = 0; i < (size_t)theInFrames; i++)
	{
		for (int c = 0; c < channels; c++)
		{
			float v = std::round(theIn[i * theInChannels + c] * 32768.0f);
			theOut.mSamples[i * channels + c] = (int16_t)std::max(-32768.0f, std::min(32767.0f, v));
		}
	}
}

// Ogg Vorbis through Tremor (third_party/tremor), reading the file from memory. 16-bit output as
// SexyAppFramework 1.2's DSoundManager::LoadOGGSound got it from ov_read.
struct TremorMemFile
{
	const std::vector<uint8_t>* mData;
	size_t mPos;
};

static size_t TremorRead(void* thePtr, size_t theSize, size_t theCount, void* theSource)
{
	TremorMemFile* f = (TremorMemFile*)theSource;
	size_t want = theSize * theCount;
	size_t left = f->mData->size() - f->mPos;
	size_t n = std::min(want, left);
	memcpy(thePtr, f->mData->data() + f->mPos, n);
	f->mPos += n;
	return theSize ? n / theSize : 0;
}

static int TremorSeek(void* theSource, ogg_int64_t theOffset, int theWhence)
{
	TremorMemFile* f = (TremorMemFile*)theSource;
	ogg_int64_t base = theWhence == SEEK_CUR ? (ogg_int64_t)f->mPos : theWhence == SEEK_END ? (ogg_int64_t)f->mData->size() : 0;
	ogg_int64_t pos = base + theOffset;
	if (pos < 0 || pos > (ogg_int64_t)f->mData->size())
		return -1;
	f->mPos = (size_t)pos;
	return 0;
}

static int TremorClose(void*)
{
	return 0;
}

static long TremorTell(void* theSource)
{
	return (long)((TremorMemFile*)theSource)->mPos;
}

static bool DecodeOggTremor(const std::vector<uint8_t>& theFile, std::vector<float>& theOut, int* theChannels, int* theRate)
{
	TremorMemFile src = {&theFile, 0};
	ov_callbacks cb = {TremorRead, TremorSeek, TremorClose, TremorTell};
	OggVorbis_File vf;
	if (ov_open_callbacks(&src, &vf, NULL, 0, cb) < 0)
		return false;
	vorbis_info* info = ov_info(&vf, -1);
	if (info == NULL || info->channels <= 0)
	{
		ov_clear(&vf);
		return false;
	}
	int channels = info->channels;
	std::vector<short> pcm;
	char buf[4096];
	int section = 0;
	for (;;)
	{
		long got = ov_read(&vf, buf, sizeof(buf), &section);
		if (got <= 0)
			break;  // 0 = end of stream; a decode error ends the sound there, as in the framework
		size_t at = pcm.size();
		pcm.resize(at + got / 2);
		memcpy(pcm.data() + at, buf, (size_t)(got / 2) * 2);
	}
	*theChannels = channels;
	*theRate = (int)info->rate;
	ov_clear(&vf);
	size_t frames = pcm.size() / channels;
	if (frames == 0)
		return false;
	theOut.resize(frames * channels);
	for (size_t i = 0; i < theOut.size(); i++)
		theOut[i] = pcm[i] / 32768.0f;
	return true;
}

// Sun .au: big-endian header ".snd", header size, data size, encoding, rate, channels. Decodes what
// SexyAppFramework's DSoundManager::LoadAUSound accepts, the same way: encoding 1 (8-bit mu-law) with
// the framework's own segment table to 16 bits (sign * level * 4; it is not exact G.711), and
// encoding 2 (8-bit linear) copied as-is into an 8-bit (unsigned) buffer. Like the framework it reads
// "data size" bytes after the header and fails when the file is shorter.
static bool DecodeAU(const std::vector<uint8_t>& theFile, std::vector<float>& theOut, int* theChannels, int* theRate)
{
	auto be32 = [&](size_t o) { return ((uint32_t)theFile[o] << 24) | ((uint32_t)theFile[o + 1] << 16) | ((uint32_t)theFile[o + 2] << 8) | theFile[o + 3]; };
	uint32_t aHeaderSize = be32(4);
	uint32_t aDataSize = be32(8);
	uint32_t anEncoding = be32(12);
	uint32_t aRate = be32(16);
	uint32_t aChannels = be32(20);
	if ((anEncoding != 1 && anEncoding != 2) || aChannels == 0 || aChannels > 8 || aRate == 0 ||
		aHeaderSize > theFile.size() || aDataSize > theFile.size() - aHeaderSize)
		return false;
	const uint8_t* aSrc = theFile.data() + aHeaderSize;
	size_t aCount = aDataSize - aDataSize % aChannels;
	theOut.resize(aCount);
	for (size_t i = 0; i < aCount; i++)
	{
		int ch = aSrc[i];
		if (anEncoding == 2)
		{
			theOut[i] = (ch - 128) / 128.0f;
			continue;
		}
		int sign = (ch < 128) ? -1 : 1;
		ch = ch | 0x80;
		if (ch > 239)
			ch = ((0xF0 | 15) - ch) * 2;
		else if (ch > 223)
			ch = (((0xE0 | 15) - ch) * 4) + 32;
		else if (ch > 207)
			ch = (((0xD0 | 15) - ch) * 8) + 96;
		else if (ch > 191)
			ch = (((0xC0 | 15) - ch) * 16) + 224;
		else if (ch > 175)
			ch = (((0xB0 | 15) - ch) * 32) + 480;
		else if (ch > 159)
			ch = (((0xA0 | 15) - ch) * 64) + 992;
		else if (ch > 143)
			ch = (((0x90 | 15) - ch) * 128) + 2016;
		else if (ch > 128)
			ch = (((0x80 | 15) - ch) * 256) + 4064;
		else
			ch = 0xff;
		theOut[i] = (short)(sign * ch * 4) / 32768.0f;
	}
	*theChannels = (int)aChannels;
	*theRate = (int)aRate;
	return true;
}

// MPEG audio (Mummy Maze's one MP3 sound): an ID3v2 tag or an MPEG frame sync at the start
static bool IsMP3(const std::vector<uint8_t>& theFile)
{
	if (theFile.size() >= 10 && !memcmp(theFile.data(), "ID3", 3))
		return true;
	return theFile.size() >= 4 && theFile[0] == 0xFF && (theFile[1] & 0xE0) == 0xE0;
}

// Decodes a whole MP3 with minimp3 (the decoder libopenmpt uses for MP3 samples). An ID3v2 tag at the start is
// skipped by its size, and an encoder's Xing/Info frame (a silent frame that only describes the stream) gives no samples.
static bool DecodeMP3(const std::vector<uint8_t>& theFile, std::vector<float>& theOut, int* theChannels, int* theRate)
{
	size_t aPos = 0;
	if (theFile.size() >= 10 && !memcmp(theFile.data(), "ID3", 3))
	{
		// the tag's size is four 7-bit bytes, without the 10-byte header (and a 10-byte footer when flag 0x10 is set)
		size_t aTagSize = ((theFile[6] & 0x7F) << 21) | ((theFile[7] & 0x7F) << 14) | ((theFile[8] & 0x7F) << 7) | (theFile[9] & 0x7F);
		aPos = 10 + aTagSize + ((theFile[5] & 0x10) ? 10 : 0);
	}

	mp3dec_t aDecoder;
	mp3dec_init(&aDecoder);
	std::vector<mp3d_sample_t> aPcm(MINIMP3_MAX_SAMPLES_PER_FRAME);
	bool aFirstFrame = true;
	*theChannels = 0;
	*theRate = 0;
	while (aPos < theFile.size())
	{
		mp3dec_frame_info_t anInfo;
		int aSamples = mp3dec_decode_frame(&aDecoder, theFile.data() + aPos, (int)(theFile.size() - aPos), aPcm.data(), &anInfo);
		if (anInfo.frame_bytes <= 0)
			break; // no further frame
		const uint8_t* aFrame = theFile.data() + aPos + anInfo.frame_offset;
		int aFrameSize = anInfo.frame_bytes - anInfo.frame_offset;
		aPos += anInfo.frame_bytes;
		if (aSamples <= 0)
			continue; // skipped data
		if (aFirstFrame)
		{
			aFirstFrame = false;
			bool anInfoFrame = false;
			for (int i = 0; i + 4 <= aFrameSize && i < 64 && !anInfoFrame; i++)
				anInfoFrame = !memcmp(aFrame + i, "Xing", 4) || !memcmp(aFrame + i, "Info", 4);
			if (anInfoFrame)
				continue;
		}
		if (*theChannels == 0)
		{
			*theChannels = anInfo.channels;
			*theRate = anInfo.hz;
		}
		if (anInfo.channels != *theChannels)
			continue; // a stream that changes its channels mid-way keeps its first layout
		for (int i = 0; i < aSamples * anInfo.channels; i++)
			theOut.push_back(aPcm[i] / 32768.0f);
	}
	return *theChannels > 0 && *theRate > 0 && !theOut.empty();
}

// A WAV whose format is MPEG Layer 3 (tag 0x55; Word Harmony's music): its data chunk, which is a plain MP3 stream
static bool WavMP3Data(const std::vector<uint8_t>& theFile, std::vector<uint8_t>& theData)
{
	if (theFile.size() < 12 || memcmp(theFile.data(), "RIFF", 4) || memcmp(theFile.data() + 8, "WAVE", 4))
		return false;
	bool anMP3 = false;
	size_t aPos = 12;
	while (aPos + 8 <= theFile.size())
	{
		const uint8_t* aChunk = theFile.data() + aPos;
		size_t aSize = aChunk[4] | (aChunk[5] << 8) | (aChunk[6] << 16) | ((size_t)aChunk[7] << 24);
		size_t aBody = aPos + 8;
		// a chunk said to run past the end ends there (size_t is 32 bits in the web build: no sum may wrap)
		aSize = std::min(aSize, theFile.size() - aBody);
		if (!memcmp(aChunk, "fmt ", 4) && aSize >= 2 && aBody + 2 <= theFile.size())
			anMP3 = (theFile[aBody] | (theFile[aBody + 1] << 8)) == 0x55;
		else if (!memcmp(aChunk, "data", 4))
		{
			if (!anMP3)
				return false;
			theData.assign(theFile.begin() + aBody, theFile.begin() + aBody + aSize);
			return true;
		}
		aPos = aBody + aSize + (aSize & 1);
	}
	return false;
}

std::shared_ptr<SoundData> Mixer::LoadSound(const std::string& theGamePath)
{
	std::vector<uint8_t> file;
	if (!VFS::Get().ReadAll(theGamePath, file) || file.empty())
		return nullptr;
	return LoadSoundData(file, theGamePath);
}

std::shared_ptr<SoundData> Mixer::LoadSoundData(const std::vector<uint8_t>& file, const std::string& theName)
{
	auto data = std::make_shared<SoundData>();
	// Detect Ogg by content: mods may remap a game path to a file with another name.
	if (file.size() > 4 && !memcmp(file.data(), "OggS", 4))
	{
		int channels = 0, rate = 0;
		short* pcm = nullptr;
		int frames = stb_vorbis_decode_memory(file.data(), (int)file.size(), &channels, &rate, &pcm);
		if (frames <= 0 || !pcm)
		{
			// stb_vorbis has no Vorbis floor 0 (pre-1.0 encoders, e.g. Big Money's 2002 sounds): decode
			// with Tremor, the decoder SexyAppFramework 1.2 itself used
			free(pcm);
			std::vector<float> f;
			if (!DecodeOggTremor(file, f, &channels, &rate))
				return nullptr;
			StoreSound(f.data(), (int)(f.size() / channels), channels, rate, *data);
			return data;
		}
		// already 16-bit: keep the first one or two channels as they are
		if (rate <= 0)
		{
			free(pcm);
			return data;
		}
		data->mChannels = std::min(channels, 2);
		data->mRate = rate;
		data->mFrames = (size_t)frames;
		data->mSamples.resize((size_t)frames * data->mChannels);
		for (size_t i = 0; i < (size_t)frames; i++)
			for (int c = 0; c < data->mChannels; c++)
				data->mSamples[i * data->mChannels + c] = pcm[i * channels + c];
		free(pcm);
		return data;
	}
	// Sun/NeXT .au (Big Money, Insaniquarium, Typer Shark, BookWorm)
	if (file.size() >= 24 && !memcmp(file.data(), ".snd", 4))
	{
		std::vector<float> f;
		int channels = 0, rate = 0;
		if (!DecodeAU(file, f, &channels, &rate))
		{
			SDL_Log("cannot decode sound %s (.au: only 8-bit mu-law and 8-bit linear are supported)", theName.c_str());
			return nullptr;
		}
		StoreSound(f.data(), (int)(f.size() / channels), channels, rate, *data);
		return data;
	}
	// MP3 (Mummy Maze's fight sound)
	if (IsMP3(file))
	{
		std::vector<float> f;
		int channels = 0, rate = 0;
		if (!DecodeMP3(file, f, &channels, &rate))
		{
			SDL_Log("cannot decode sound %s (MP3: no frames decoded)", theName.c_str());
			return nullptr;
		}
		StoreSound(f.data(), (int)(f.size() / channels), channels, rate, *data);
		return data;
	}
	// MP3 inside a WAV (Word Harmony's music)
	std::vector<uint8_t> aWavMP3;
	if (WavMP3Data(file, aWavMP3))
	{
		std::vector<float> f;
		int channels = 0, rate = 0;
		if (!DecodeMP3(aWavMP3, f, &channels, &rate))
		{
			SDL_Log("cannot decode sound %s (MP3 in WAV: no frames decoded)", theName.c_str());
			return nullptr;
		}
		StoreSound(f.data(), (int)(f.size() / channels), channels, rate, *data);
		return data;
	}
	// WAV (and anything else SDL can read)
	SDL_RWops* rw = SDL_RWFromConstMem(file.data(), (int)file.size());
	SDL_AudioSpec spec;
	Uint8* buf = nullptr;
	Uint32 len = 0;
	if (!SDL_LoadWAV_RW(rw, 1, &spec, &buf, &len))
	{
		SDL_Log("cannot decode sound %s (supported: Ogg Vorbis, MP3, Sun .au, WAV)", theName.c_str());
		return nullptr;
	}
	SDL_AudioCVT cvt;
	if (SDL_BuildAudioCVT(&cvt, spec.format, spec.channels, spec.freq, AUDIO_F32SYS, spec.channels, spec.freq) < 0)
	{
		SDL_FreeWAV(buf);
		return nullptr;
	}
	std::vector<uint8_t> conv((size_t)len * cvt.len_mult);
	memcpy(conv.data(), buf, len);
	SDL_FreeWAV(buf);
	cvt.buf = conv.data();
	cvt.len = (int)len;
	SDL_ConvertAudio(&cvt);
	int frames = cvt.len_cvt / (int)(sizeof(float) * spec.channels);
	StoreSound((const float*)conv.data(), frames, spec.channels, spec.freq, *data);
	return data;
}

// ---------------------------------------------------------------------------------------------
// Voices

int Mixer::StartVoice(const std::shared_ptr<SoundData>& theData, float theGainL, float theGainR, double thePitch, bool theLoop)
{
	if ((!mDevice && !mOffline) || !theData || theData->mFrames == 0)
		return 0;
	Lock();
	int slot = -1;
	for (int i = 0; i < (int)mVoices.size(); i++)
	{
		if (!mVoices[i]->mActive)
		{
			slot = i;
			break;
		}
	}
	int handle = 0;
	if (slot >= 0)
	{
		Voice& v = *mVoices[slot];
		v.mData = theData;
		v.mPos = 0.0;
		v.mStep = thePitch * theData->mRate / mRate;  // the sound's frames per mixed frame
		v.mGainL = theGainL;
		v.mGainR = theGainR;
		v.mLoop = theLoop;
		v.mActive = true;
		v.mHandle = handle = (mNextVoiceSerial++ << kVoiceSerialShift) | slot;
		if (mNextVoiceSerial > 0x7fffff)
			mNextVoiceSerial = 1;
	}
	Unlock();
	return handle;
}

#define VOICE_FOR_HANDLE(h) \
	int slot_ = (h) & ((1 << kVoiceSerialShift) - 1); \
	Voice* v = (h) > 0 && slot_ < (int)mVoices.size() && mVoices[slot_]->mHandle == (h) ? mVoices[slot_].get() : nullptr

void Mixer::SetVoiceGain(int theVoice, float theGainL, float theGainR)
{
	Lock();
	VOICE_FOR_HANDLE(theVoice);
	if (v)
	{
		v->mGainL = theGainL;
		v->mGainR = theGainR;
	}
	Unlock();
}

void Mixer::SetVoicePitch(int theVoice, double thePitch)
{
	Lock();
	VOICE_FOR_HANDLE(theVoice);
	if (v && v->mData)
		v->mStep = thePitch * v->mData->mRate / mRate;
	Unlock();
}

void Mixer::StopVoice(int theVoice)
{
	Lock();
	VOICE_FOR_HANDLE(theVoice);
	if (v)
	{
		v->mActive = false;
		v->mData.reset();
	}
	Unlock();
}

bool Mixer::IsVoicePlaying(int theVoice)
{
	Lock();
	VOICE_FOR_HANDLE(theVoice);
	bool playing = v && v->mActive;
	Unlock();
	return playing;
}

void Mixer::StopAllVoices()
{
	Lock();
	for (auto& v : mVoices)
	{
		v->mActive = false;
		v->mData.reset();
	}
	Unlock();
}

// ---------------------------------------------------------------------------------------------
// Songs

int Mixer::LoadSong(const std::string& theGamePath)
{
	std::vector<uint8_t> file;
	if (!VFS::Get().ReadAll(theGamePath, file) || file.empty())
		return 0;
	std::unique_ptr<Song> song(new Song());
	const VFSRemap* remap = VFS::Get().GetRemap(theGamePath);
	if (remap)
	{
		song->mDefaultOrder = remap->mStartOrder;
		song->mTrackGain = (float)remap->mVolume;
	}
	bool isOgg = file.size() > 4 && !memcmp(file.data(), "OggS", 4);
	bool isWav = file.size() > 4 && !memcmp(file.data(), "RIFF", 4);
	if (isOgg)
	{
		// Mods may replace tracker music with rendered audio; it is decoded while it plays.
		song->mFile.swap(file);
		int err = 0;
		song->mVorbis = stb_vorbis_open_memory(song->mFile.data(), (int)song->mFile.size(), &err, nullptr);
		if (!song->mVorbis)
		{
			SDL_Log("cannot open Ogg music %s (error %d)", theGamePath.c_str(), err);
			return 0;
		}
		stb_vorbis_info info = stb_vorbis_get_info(song->mVorbis);
		song->mSourceRate = (int)info.sample_rate;
		song->mSourceChannels = info.channels;
		song->mSourceFrames = stb_vorbis_stream_length_in_samples(song->mVorbis);
	}
	else if (isWav)
	{
		song->mPcm = LoadSoundData(file, theGamePath);
		if (!song->mPcm)
			return 0;
		song->mSourceRate = song->mPcm->mRate;
		song->mSourceChannels = song->mPcm->mChannels;
		song->mSourceFrames = (int64_t)song->mPcm->mFrames;
	}
	if (song->IsStream())
	{
		if (remap)
		{
			song->mStartFrame = (int64_t)std::max(0.0, remap->mStartTime * song->mSourceRate);
			song->mLoopStartFrame = (int64_t)std::max(0.0, remap->mLoopStart * song->mSourceRate);
		}
		if (song->mSourceFrames > 0 && song->mLoopStartFrame >= song->mSourceFrames)
			song->mLoopStartFrame = 0;
		if (song->mSourceFrames > 0 && song->mStartFrame >= song->mSourceFrames)
			song->mStartFrame = 0;
	}
	else
	{
		for (const ModuleFormat& fmt : ModuleFormats())
		{
			if (fmt.mDetect(file))
			{
				song->mModule = fmt.mCreate(file, mRate);
				if (!song->mModule)
				{
					SDL_Log("cannot load music %s", theGamePath.c_str());
					return 0;
				}
				break;
			}
		}
	}
	if (!song->IsStream() && !song->mModule)
	{
		song->mCtx = xmp_create_context();
		if (xmp_load_module_from_memory(song->mCtx, file.data(), (long)file.size()) != 0)
		{
			xmp_free_context(song->mCtx);
			return 0;
		}
		xmp_start_player(song->mCtx, mRate, 0);
		xmp_set_player(song->mCtx, XMP_PLAYER_INTERP, XMP_INTERP_SPLINE);
	}
	song->mLoaded = true;
	Lock();
	mSongs.push_back(std::move(song));
	int handle = (int)mSongs.size();
	Unlock();
	return handle;
}

int Mixer::AddSong(std::unique_ptr<ModulePlayer> thePlayer)
{
	if (!thePlayer)
		return 0;
	std::unique_ptr<Song> song(new Song());
	song->mModule = std::move(thePlayer);
	song->mLoaded = true;
	Lock();
	mSongs.push_back(std::move(song));
	int handle = (int)mSongs.size();
	Unlock();
	return handle;
}

void Mixer::UnloadSong(int theSong)
{
	if (theSong <= 0 || theSong > (int)mSongs.size() || !mSongs[theSong - 1])
		return;
	Lock();
	std::unique_ptr<Song> s = std::move(mSongs[theSong - 1]);
	Unlock();
	if (s->mCtx)
	{
		xmp_end_player(s->mCtx);
		xmp_release_module(s->mCtx);
		xmp_free_context(s->mCtx);
	}
	if (s->mVorbis)
		stb_vorbis_close(s->mVorbis);
}

#define SONG_FOR_HANDLE(h) \
	Song* s = (h) > 0 && (h) <= (int)mSongs.size() ? mSongs[(h) - 1].get() : nullptr

void Mixer::PlaySong(int theSong, int theOrder, int theRow, bool theLoop, bool theResetPosition)
{
	Lock();
	SONG_FOR_HANDLE(theSong);
	if (s)
	{
		s->mLoop = theLoop;
		if (s->IsStream())
		{
			// streams ignore the module order; a mod remap's "start" picks the entry point
			if (theResetPosition || s->mEnded || !s->mPlaying)
				s->Restart(theResetPosition || s->mEnded ? s->mStartFrame : s->mPcmPos);
		}
		else if (theResetPosition)
		{
			// A mod remap can start a track partway into a multi-song module; the game's own offset
			// is relative to that.
			int order = theOrder + std::max(0, s->mDefaultOrder);
			if (s->mModule)
				s->mModule->SetPosition(order, theRow);
			else
			{
			xmp_restart_module(s->mCtx);
			if (order > 0)
				xmp_set_position(s->mCtx, order);
			if (theRow > 0)
				xmp_set_row(s->mCtx, theRow);
			}
		}
		s->mLoop = theLoop;
		s->mPlaying = true;
		s->mPaused = false;
		s->mCurGain = s->mGain;
		if (theResetPosition)
		{
			std::fill(s->mHistory, s->mHistory + Song::kHistoryFrames, 0.0f);
			s->mHistoryPos = 0;
		}
	}
	Unlock();
}

void Mixer::StopSong(int theSong)
{
	Lock();
	SONG_FOR_HANDLE(theSong);
	if (s)
		s->mPlaying = false;
	Unlock();
}

void Mixer::PauseSong(int theSong, bool thePaused)
{
	Lock();
	SONG_FOR_HANDLE(theSong);
	if (s)
		s->mPaused = thePaused;
	Unlock();
}

void Mixer::SetSongGain(int theSong, float theGain)
{
	Lock();
	SONG_FOR_HANDLE(theSong);
	if (s)
		s->mGain = theGain;
	Unlock();
}

bool Mixer::IsSongPlaying(int theSong)
{
	Lock();
	SONG_FOR_HANDLE(theSong);
	bool playing = s && s->mPlaying && !s->mPaused;
	Unlock();
	return playing;
}

bool Mixer::GetSongPosition(int theSong, int& theOrder, int& theRow)
{
	bool known = false;
	Lock();
	SONG_FOR_HANDLE(theSong);
	if (s && s->mPlaying)
	{
		if (s->mCtx)
		{
			xmp_frame_info anInfo;
			xmp_get_frame_info(s->mCtx, &anInfo);
			theOrder = anInfo.pos;
			theRow = anInfo.row;
			known = true;
		}
		else if (s->mModule)
			known = s->mModule->GetPosition(theOrder, theRow);
	}
	Unlock();
	return known;
}

bool Mixer::GetSongFFT(int theSong, float* theOut, int theSamples)
{
	const int n = theSamples;
	if (n < 64 || n > Song::kHistoryFrames || (n & (n - 1)) != 0)
		return false;
	std::vector<double> re(n), im(n, 0.0);
	bool playing = false;
	Lock();
	SONG_FOR_HANDLE(theSong);
	if (s && s->mPlaying)
	{
		playing = true;
		for (int i = 0; i < n; i++)
			re[i] = s->mHistory[(s->mHistoryPos - n + i) & (Song::kHistoryFrames - 1)];
	}
	Unlock();
	if (!playing)
		return false;

	// BASS's default window: a periodic Hann window (measured with BASS 2.3.0.1: a sine centred on a bin puts
	// half its value in each neighbour)
	const double pi = 3.14159265358979323846;
	for (int i = 0; i < n; i++)
		re[i] *= 0.5 - 0.5 * std::cos(2.0 * pi * i / n);

	// in-place radix-2 FFT
	for (int i = 1, j = 0; i < n; i++)
	{
		int bit = n >> 1;
		for (; j & bit; bit >>= 1)
			j ^= bit;
		j ^= bit;
		if (i < j)
		{
			std::swap(re[i], re[j]);
			std::swap(im[i], im[j]);
		}
	}
	for (int len = 2; len <= n; len <<= 1)
	{
		const double ang = -2.0 * pi / len;
		for (int i = 0; i < n; i += len)
			for (int k = 0; k < len / 2; k++)
			{
				const double wr = std::cos(ang * k), wi = std::sin(ang * k);
				const int a = i + k, b = i + k + len / 2;
				const double xr = re[b] * wr - im[b] * wi, xi = re[b] * wi + im[b] * wr;
				re[b] = re[a] - xr;
				im[b] = im[a] - xi;
				re[a] += xr;
				im[a] += xi;
			}
	}

	// BASS's scale: twice the magnitude over the window's sum (n / 2), so that a full-scale sine reads 1.0 in its
	// bin, with or without the window, and a constant 0.5 reads 1.0 in bin 0
	for (int k = 0; k < n / 2; k++)
		theOut[k] = (float)(4.0 * std::sqrt(re[k] * re[k] + im[k] * im[k]) / n);
	return true;
}

// ---------------------------------------------------------------------------------------------
// Mixing

void Mixer::Callback(void* theUser, uint8_t* theStream, int theLen)
{
	Mixer* m = (Mixer*)theUser;
	m->Mix((float*)theStream, theLen / (int)(sizeof(float) * 2));
	if (m->mSilent)
		memset(theStream, 0, theLen);
#ifdef __EMSCRIPTEN__
	if (m->mPageMuted)
		memset(theStream, 0, theLen);
#endif
}

void Mixer::Mix(float* theOut, int theFrames)
{
	memset(theOut, 0, sizeof(float) * 2 * theFrames);

	for (auto& vp : mVoices)
	{
		Voice& v = *vp;
		if (!v.mActive || !v.mData)
			continue;
		const int16_t* src = v.mData->mSamples.data();
		const int ch = v.mData->mChannels;
		const double len = (double)v.mData->mFrames;
		const float gl = v.mGainL * mSfxMaster, gr = v.mGainR * mSfxMaster;
		for (int i = 0; i < theFrames; i++)
		{
			if (v.mPos >= len)
			{
				if (v.mLoop)
					v.mPos = std::fmod(v.mPos, len);
				else
				{
					v.mActive = false;
					break;
				}
			}
			int i0 = (int)v.mPos;
			int i1 = i0 + 1 < (int)len ? i0 + 1 : (v.mLoop ? 0 : i0);
			float t = (float)(v.mPos - i0);
			const int16_t* a = src + i0 * ch;
			const int16_t* b = src + i1 * ch;
			float l = (a[0] + (b[0] - a[0]) * t) * (1.0f / 32768.0f);
			float r = ch > 1 ? (a[1] + (b[1] - a[1]) * t) * (1.0f / 32768.0f) : l;
			theOut[i * 2] += l * gl;
			theOut[i * 2 + 1] += r * gr;
			v.mPos += v.mStep;
		}
		if (!v.mActive)
			v.mData.reset();
	}

	mSongScratch.resize((size_t)theFrames * 2);
	for (auto& sp : mSongs)
	{
		if (!sp)
			continue;
		Song& s = *sp;
		if (!s.mPlaying || s.mPaused)
			continue;
		// ramp gain across the buffer so per-tick fades don't click
		float g0 = s.mCurGain * mMusicMaster * s.mTrackGain, g1 = s.mGain * mMusicMaster * s.mTrackGain;
		if (s.IsStream())
		{
			const double step = (double)s.mSourceRate / mRate;
			for (int i = 0; i < theFrames; i++)
			{
				float g = g0 + (g1 - g0) * ((float)i / theFrames);
				float t = (float)s.mFrac;
				float l = s.mFrameA[0] + (s.mFrameB[0] - s.mFrameA[0]) * t;
				float r = s.mFrameA[1] + (s.mFrameB[1] - s.mFrameA[1]) * t;
				s.Remember(l, r);
				theOut[i * 2] += l * g;
				theOut[i * 2 + 1] += r * g;
				s.mFrac += step;
				while (s.mFrac >= 1.0)
				{
					s.mFrac -= 1.0;
					s.mFrameA[0] = s.mFrameB[0];
					s.mFrameA[1] = s.mFrameB[1];
					if (!s.ReadSourceFrame(s.mFrameB))
						s.mFrameB[0] = s.mFrameB[1] = 0;
				}
				if (s.mEnded && s.mFrameA[0] == 0 && s.mFrameA[1] == 0)
				{
					s.mPlaying = false;
					break;
				}
			}
			s.mCurGain = s.mGain;
			continue;
		}
		if (s.mModule)
		{
			mModuleScratch.resize((size_t)theFrames * 2);
			if (!s.mModule->Render(mModuleScratch.data(), theFrames, s.mLoop))
				s.mPlaying = false;
			for (int i = 0; i < theFrames; i++)
			{
				float g = g0 + (g1 - g0) * ((float)i / theFrames);
				s.Remember(mModuleScratch[i * 2], mModuleScratch[i * 2 + 1]);
				theOut[i * 2] += mModuleScratch[i * 2] * g;
				theOut[i * 2 + 1] += mModuleScratch[i * 2 + 1] * g;
			}
			s.mCurGain = s.mGain;
			continue;
		}
		int ret = xmp_play_buffer(s.mCtx, mSongScratch.data(), theFrames * 4, s.mLoop ? 0 : 1);
		if (ret != 0)
		{
			s.mPlaying = false;
			continue;
		}
		for (int i = 0; i < theFrames; i++)
		{
			float g = g0 + (g1 - g0) * ((float)i / theFrames);
			s.Remember(mSongScratch[i * 2] / 32768.0f, mSongScratch[i * 2 + 1] / 32768.0f);
			theOut[i * 2] += mSongScratch[i * 2] * (g / 32768.0f);
			theOut[i * 2 + 1] += mSongScratch[i * 2 + 1] * (g / 32768.0f);
		}
		s.mCurGain = s.mGain;
	}

	for (int i = 0; i < theFrames * 2; i++)
		theOut[i] = std::max(-1.0f, std::min(1.0f, theOut[i]));
}

}
