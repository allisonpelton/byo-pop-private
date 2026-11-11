#include "Audio.h"
#include "VFS.h"
#include "MusicLevels.h"

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
#include "ModuleText.h"
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

// The web build's sound output (docs/WEB_PLAYER.md, "Sound"). SDL's own ScriptProcessor (2048 frames, 43 ms) held each
// mix for a whole buffer after waiting up to another for its turn, so a sound reached the speaker 60 to 110 ms after the
// game played it, and went on that long after the game muted. Here an AudioWorklet keeps a short queue of mixed sound and
// asks the page for the next small chunk whenever the queue runs low; the page mixes it at once (the mixer runs on the
// page, as the game does) and sends it over. The queue is as long as the page's slowest answer of the last few seconds
// needs: it grows at once when the page answers late (a busy frame, loading) and shrinks again once it answers quickly.
// Where there is no AudioWorklet (a page not served from https or localhost) a ScriptProcessor of 1024 frames does the
// same job. The statements end in semicolons: EM_JS joins the lines into one.
EM_JS_DEPS(byopop_audio, "$dynCall");

EM_JS(void, byopop_audio_open, (void* theMixer, void* theRender, int theChunk, int theTarget, int theMax, int theMargin), {
	var ctx = Module['SDL2'].audioContext;
	var rate = ctx.sampleRate;
	var out = { kind: 'none', node: null, timer: 0, pulled: false, closed: false, target: theTarget, gaps: 0, late: 0, mixedEnd: 0 };
	Module['byopopAudio'] = out;
	var render = function (frames) {
		var p = dynCall('ppi', theRender, [theMixer, frames]);
		return HEAPF32.subarray(p >> 2, (p >> 2) + frames * 2);
	};
	var earOf = function (contextSeconds) {
		var ts = ctx.getOutputTimestamp ? ctx.getOutputTimestamp() : null;
		return ts && ts.performanceTime ? ts.performanceTime + (contextSeconds - ts.contextTime) * 1000 : 0;
	};
	var pulled = function () {
		if (out.pulled) return;
		out.pulled = true;
		clearInterval(out.timer);
		out.timer = 0;
	};
	out.timer = setInterval(function () {
		if (ctx.state === 'running') return;
		if (navigator.userActivation && navigator.userActivation.hasBeenActive) ctx.resume();
		render(1024);
	}, 1024 / rate * 1000);
	out.lead = function () {
		if (out.kind !== 'worklet' || !out.mixedEnd || ctx.state !== 'running') return 0;
		var n = Math.min(out.mixedEnd - Math.ceil(ctx.currentTime * rate) - theMargin, theMax + theChunk);
		if (n <= 0) return 0;
		Module['audioChunkEar'] = earOf((out.mixedEnd - n) / rate);
		return n;
	};
	out.sendLead = function (p, frames) {
		var buf = new Float32Array(frames * 2);
		buf.set(HEAPF32.subarray(p >> 2, (p >> 2) + frames * 2));
		out.node.port.postMessage({ lead: buf }, [buf.buffer]);
	};
	out.drop = function () {
		if (out.kind === 'worklet' && out.node) out.node.port.postMessage({ drop: true });
	};
	out.close = function () {
		out.closed = true;
		if (out.timer) clearInterval(out.timer);
		if (out.node) {
			try { out.node.disconnect(); } catch (e) {}
			if (out.node.port) out.node.port.onmessage = null; else out.node.onaudioprocess = null;
		}
	};
	var script = function () {
		if (out.closed) return;
		out.kind = 'script';
		var sp = ctx.createScriptProcessor(1024, 0, 2);
		sp.onaudioprocess = function (e) {
			pulled();
			var b = e.outputBuffer;
			var n = b.length;
			Module['audioChunkEar'] = earOf(e.playbackTime);
			var src = render(n);
			var l = b.getChannelData(0);
			var r = b.numberOfChannels > 1 ? b.getChannelData(1) : null;
			for (var i = 0; i < n; i++) {
				l[i] = src[i * 2];
				if (r) r[i] = src[i * 2 + 1];
			}
		};
		sp.connect(ctx.destination);
		out.node = sp;
	};
	var processor = function () {
		class ByopopOutput extends AudioWorkletProcessor {
			constructor(options) {
				super();
				var o = options.processorOptions;
				this.chunk = o.chunk;
				this.least = o.target;
				this.target = o.target;
				this.max = o.max;
				this.queue = [];
				this.offset = 0;
				this.queued = 0;
				this.asked = [];
				this.fed = false;
				this.dryAt = -1;
				this.slowest = 0;
				this.stretchEnd = 0;
				this.port.onmessage = (e) => {
					var d = e.data;
					if (d instanceof Float32Array) this.take(d);
					else if (d.lead) this.lead(d.lead);
					else if (d.drop) this.drop();
				};
			}
			need(wait) {
				return Math.min(this.max, Math.max(this.least, Math.ceil((wait + this.chunk) * 1.25 / 128) * 128));
			}
			take(chunk) {
				var frames = chunk.length >> 1;
				var wait = currentFrame - (this.asked.length ? this.asked.shift() : currentFrame);
				if (frames > 0) {
					this.queue.push(chunk);
					this.queued += frames;
					this.fed = true;
				}
				if (wait > this.slowest) this.slowest = wait;
				if (this.need(wait) > this.target) this.target = this.need(wait);
				if (this.dryAt >= 0) {
					this.port.postMessage({ gap: currentFrame - this.dryAt, target: this.target });
					this.dryAt = -1;
				}
			}
			lead(buf) {
				var n = buf.length >> 1;
				var skip = Math.max(0, n - this.queued);
				var at = this.queued - (n - skip);
				var j = skip;
				for (var q = 0; q < this.queue.length && j < n; q++) {
					var c = this.queue[q];
					var first = q === 0 ? this.offset : 0;
					var frames = (c.length >> 1) - first;
					if (at >= frames) {
						at -= frames;
						continue;
					}
					for (var k = first + at; k < (c.length >> 1) && j < n; k++, j++) {
						c[k * 2] += buf[j * 2];
						c[k * 2 + 1] += buf[j * 2 + 1];
					}
					at = 0;
				}
				if (skip) this.port.postMessage({ late: skip });
			}
			drop() {
				this.queue = [];
				this.offset = 0;
				this.queued = 0;
				this.fed = false;
			}
			process(inputs, outputs) {
				var out = outputs[0];
				var left = out[0];
				var right = out.length > 1 ? out[1] : null;
				var n = left.length;
				var i = 0;
				while (i < n && this.queue.length) {
					var c = this.queue[0];
					var frames = c.length >> 1;
					var take = Math.min(n - i, frames - this.offset);
					for (var k = 0, j = this.offset * 2; k < take; k++, j += 2) {
						left[i + k] = c[j];
						if (right) right[i + k] = c[j + 1];
					}
					i += take;
					this.offset += take;
					this.queued -= take;
					if (this.offset >= frames) {
						this.queue.shift();
						this.offset = 0;
					}
				}
				if (i < n && this.fed && this.dryAt < 0) this.dryAt = currentFrame + i;
				if (currentFrame >= this.stretchEnd) {
					if (this.need(this.slowest) < this.target) this.target = this.need(this.slowest);
					this.port.postMessage({ slowest: this.slowest, target: this.target });
					this.slowest = 0;
					this.stretchEnd = currentFrame + 3 * sampleRate;
				}
				var asked = this.asked.length * this.chunk;
				var end = currentFrame + n + this.queued + asked;
				while (this.queued + asked < this.target) {
					this.port.postMessage([this.chunk, end]);
					this.asked.push(currentFrame);
					asked += this.chunk;
					end += this.chunk;
				}
				return true;
			}
		}
		registerProcessor('byopop-output', ByopopOutput);
	};
	var worklet = function () {
		out.kind = 'worklet';
		var url = URL.createObjectURL(new Blob(['(' + processor.toString() + ')();'], { type: 'text/javascript' }));
		ctx.audioWorklet.addModule(url).then(function () {
			URL.revokeObjectURL(url);
			if (out.closed) return;
			var node = new AudioWorkletNode(ctx, 'byopop-output', { numberOfInputs: 0, numberOfOutputs: 1, outputChannelCount: [2],
				processorOptions: { chunk: theChunk, target: theTarget, max: theMax } });
			node.port.onmessage = function (e) {
				var d = e.data;
				if (Array.isArray(d)) {
					pulled();
					Module['audioChunkEar'] = earOf(d[1] / rate);
					var buf = new Float32Array(d[0] * 2);
					buf.set(render(d[0]));
					node.port.postMessage(buf, [buf.buffer]);
					out.mixedEnd = d[1] + d[0];
				} else if ('gap' in d) {
					out.gaps++;
					out.target = d.target;
				} else if ('late' in d) {
					out.late += d.late;
				} else {
					out.slowest = d.slowest;
					out.target = d.target;
				}
			};
			node.connect(ctx.destination);
			out.node = node;
		}).catch(function (e) {
			URL.revokeObjectURL(url);
			console.warn('audio: no AudioWorklet (' + e + '), a ScriptProcessor plays instead');
			script();
		});
	};
	if (Module['audioOutput'] !== 'script' && ctx.audioWorklet && typeof AudioWorkletNode === 'function') worklet(); else script();
});
#endif

namespace Platform
{

static const int kMaxVoices = 48;
static const int kVoiceSerialShift = 8;  // handle = serial << 8 | slot
#ifdef __EMSCRIPTEN__
static const uint32_t kPageDevice = 0xffffffffu;  // mDevice while the web page's own output plays (InitPage): no SDL device
#endif

struct Mixer::Voice
{
	std::shared_ptr<SoundData> mData;
	double mPos = 0.0;
	double mStep = 1.0;
	float mGainL = 1.0f, mGainR = 1.0f;
	bool mLoop = false;
	bool mActive = false;
	int mHandle = 0;
#ifdef __EMSCRIPTEN__
	double mStartedAt = 0.0;  // the page's clock at StartVoice, until the voice's first mix (Module.onVoiceMixed)
#endif
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
	size_t mModuleFileBytes = 0;             // a module's file size, for the memory report (its player holds the module)
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
	float mAmplify = 1.0f;  // BASS's module amplify (SetSongAmplify); streams keep 1

	// The song's last frames before any gain, the two channels averaged, for GetSongFFT (which applies mAmplify).
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
#ifdef __EMSCRIPTEN__
	return InitPage();
#else
	if (!SDL_WasInit(SDL_INIT_AUDIO) && SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
	{
		SDL_Log("audio init failed: %s", SDL_GetError());
		return false;
	}
	const int bufferFrames = 1024;
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
#endif
}

#ifdef __EMSCRIPTEN__
// The page's output (byopop_audio_open, above): chunks of 256 frames (5.3 ms at 48 kHz), a queue of at least 2048 frames
// (43 ms, which covers the game's slower frames: a new voice does not wait for it) and at most 4096. A voice's first
// frames are mixed into the queue from 640 frames (13 ms) ahead of what the context has played: a few render quanta
// ahead, so that they reach the worklet before those frames are played.
static const int kPageChunk = 256;
static const int kPageQueue = 2048;
static const int kPageQueueMax = 4096;
static const int kPageLeadMargin = 640;

bool Mixer::InitPage()
{
	// The context is made here at 48 kHz: at the machine's own rate (often 96 or 192 kHz) every mix would cost two to
	// four times as much. The first key or click lets it play (browsers hold sound until then).
	EM_ASM({
		if (typeof window === 'undefined') return;
		Module['SDL2'] = Module['SDL2'] || {};
		if (!Module['SDL2'].audioContext) {
			var Ctx = window.AudioContext || window.webkitAudioContext;
			if (Ctx) {
				try { Module['SDL2'].audioContext = new Ctx({ sampleRate: 48000, latencyHint: 'interactive' }); }
				catch (e) { Module['SDL2'].audioContext = new Ctx(); }
				var ctx = Module['SDL2'].audioContext;
				var resume = function() { if (ctx.state !== 'running') ctx.resume(); };
				(['pointerdown', 'keydown', 'touchstart']).forEach(function(t) { document.addEventListener(t, resume, true); });
			}
		}
	});
	const int rate = EM_ASM_INT({ var c = Module['SDL2'] && Module['SDL2'].audioContext; return c ? c.sampleRate : 0; });
	if (rate <= 0)
	{
		SDL_Log("audio: the page has no Web Audio");
		return false;
	}
	// Test runs in a browser pane can be hidden, so they must not play sound. The page asks with ?silent=1: the mixer
	// goes on mixing and the output gets silence, which keeps playback timing as it is (like rundesk's dummy driver).
	mSilent = EM_ASM_INT({ return Module['silentAudio'] ? 1 : 0; }) != 0;
	mRate = rate;
	mVoices.clear();
	for (int i = 0; i < kMaxVoices; i++)
		mVoices.emplace_back(new Voice());
	mDevice = kPageDevice;
	byopop_audio_open(this, reinterpret_cast<void*>(&Mixer::PageRender), kPageChunk, kPageQueue, kPageQueueMax, kPageLeadMargin);
	SDL_Log("audio: %d Hz, the page's own output", rate);
	return true;
}

float* Mixer::PageRender(Mixer* theMixer, int theFrames)
{
	std::vector<float>& buf = theMixer->mPageBuffer;
	if ((int)buf.size() < theFrames * 2)
		buf.resize((size_t)theFrames * 2);
	theMixer->Mix(buf.data(), theFrames);
	if (theMixer->mSilent || theMixer->mPageMuted)
		memset(buf.data(), 0, sizeof(float) * 2 * theFrames);
	return buf.data();
}

// A voice just started would wait behind all the sound already queued for the page, so its first frames are mixed now,
// alone, and added to the queued sound from where it is still ahead of the speaker (out.lead); the next chunk goes on
// from there, as it would have from the voice's start.
void Mixer::PageLead(Voice& v)
{
	const int frames = EM_ASM_INT({ var a = Module['byopopAudio']; return a && a.lead ? a.lead() : 0; });
	if (frames <= 0)
		return;
	mLeadBuffer.assign((size_t)frames * 2, 0.0f);
	MixVoice(v, mLeadBuffer.data(), frames);
	if (mSilent || mPageMuted)
		std::fill(mLeadBuffer.begin(), mLeadBuffer.end(), 0.0f);
	EM_ASM({ Module['byopopAudio'].sendLead($0, $1); }, mLeadBuffer.data(), frames);
}
#endif

void Mixer::DropQueued()
{
#ifdef __EMSCRIPTEN__
	if (mDevice == kPageDevice)
		EM_ASM({ var a = Module['byopopAudio']; if (a && a.drop) a.drop(); });
#endif
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
#ifdef __EMSCRIPTEN__
	EM_ASM({ var a = Module['byopopAudio']; if (a) a.close(); });
#else
	SDL_CloseAudioDevice(mDevice);
#endif
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
#ifndef __EMSCRIPTEN__  // the page's output mixes on the page, between the game's frames: nothing to lock
	if (mDevice)
		SDL_LockAudioDevice(mDevice);
#endif
}

void Mixer::Unlock()
{
#ifndef __EMSCRIPTEN__  // the page's output mixes on the page, between the game's frames: nothing to lock
	if (mDevice)
		SDL_UnlockAudioDevice(mDevice);
#endif
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
#ifdef __EMSCRIPTEN__
		v.mStartedAt = emscripten_get_now();
		if (mDevice == kPageDevice && !mGameClocked)
			PageLead(v);
#endif
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

void Mixer::SetGameClocked(bool theGameClocked)
{
	Lock();
	mGameClocked = theGameClocked;
	Unlock();
}

// Moves the voices on as Mix would over theMilliseconds of output (the same step per mixed frame, so pitch changes
// count), and ends a one-shot voice where Mix would: once its position reaches the sound's end.
void Mixer::AdvanceVoices(double theMilliseconds)
{
	Lock();
	if (mGameClocked)
	{
		const double frames = theMilliseconds * mRate / 1000.0;
		for (auto& vp : mVoices)
		{
			Voice& v = *vp;
			if (!v.mActive || !v.mData)
				continue;
			const double len = (double)v.mData->mFrames;
			v.mPos += v.mStep * frames;
			if (v.mPos >= len)
			{
				if (v.mLoop)
					v.mPos = std::fmod(v.mPos, len);
				else
				{
					v.mActive = false;
					v.mData.reset();
				}
			}
		}
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
		song->mModuleFileBytes = file.size();
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
		xmp_set_player(song->mCtx, XMP_PLAYER_INTERP, BYOPOP_XMP_INTERP);  // platform/MusicLevels.h
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

void Mixer::GetSongMemory(int* theCount, size_t* theTrackBytes, size_t* theModuleFileBytes)
{
	*theCount = 0;
	*theTrackBytes = 0;
	*theModuleFileBytes = 0;
	Lock();
	for (auto& s : mSongs)
	{
		if (!s || !s->mLoaded)
			continue;
		(*theCount)++;
		*theTrackBytes += s->mFile.capacity() + s->mDecoded.capacity() * sizeof(float) + (s->mPcm ? s->mPcm->Bytes() : 0);
		*theModuleFileBytes += s->mModuleFileBytes;
	}
	Unlock();
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

void Mixer::SetSongAmplify(int theSong, float theGain)
{
	Lock();
	SONG_FOR_HANDLE(theSong);
	// BASS_MusicSetAttribute fails on a stream, so a mod's Ogg or WAV in place of a module plays unamplified
	if (s && !s->IsStream())
		s->mAmplify = theGain;
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

int Mixer::GetSongTempo(int theSong)
{
	int aTempo = 0;
	Lock();
	SONG_FOR_HANDLE(theSong);
	if (s && s->mModule)
		aTempo = s->mModule->GetTempo();
	Unlock();
	return aTempo;
}

void Mixer::SetSongTempo(int theSong, int theBPM)
{
	Lock();
	SONG_FOR_HANDLE(theSong);
	if (s && s->mModule)
		s->mModule->SetTempo(theBPM);
	Unlock();
}

int Mixer::GetSongChannelVolume(int theSong, int theChannel)
{
	Lock();
	SONG_FOR_HANDLE(theSong);
	int aVolume = s && s->mModule ? s->mModule->GetChannelVolume(theChannel) : -1;
	Unlock();
	return aVolume;
}

void Mixer::SetSongChannelVolume(int theSong, int theChannel, int theVolume)
{
	Lock();
	SONG_FOR_HANDLE(theSong);
	if (s && s->mModule)
		s->mModule->SetChannelVolume(theChannel, theVolume);
	Unlock();
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
			re[i] = s->mHistory[(s->mHistoryPos - n + i) & (Song::kHistoryFrames - 1)] * s->mAmplify;
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
}

// Adds one voice's next theFrames frames to theOut and moves it on (Mix, and on the web PageLead).
void Mixer::MixVoice(Voice& v, float* theOut, int theFrames)
{
#ifdef __EMSCRIPTEN__
	// The latency probe: the page learns when a sound started and that its first frame is in this mix
	if (v.mStartedAt > 0.0)
	{
		EM_ASM({ if (Module['onVoiceMixed']) Module['onVoiceMixed']($0); }, v.mStartedAt);
		v.mStartedAt = 0.0;
	}
#endif
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

void Mixer::Mix(float* theOut, int theFrames)
{
	memset(theOut, 0, sizeof(float) * 2 * theFrames);

	for (auto& vp : mVoices)
	{
		Voice& v = *vp;
		// on the game's clock (SetGameClocked) only AdvanceVoices moves a voice, so the voices are not mixed
		if (!v.mActive || !v.mData || mGameClocked)
			continue;
		MixVoice(v, theOut, theFrames);
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
		float g0 = s.mCurGain * mMusicMaster * s.mTrackGain * s.mAmplify, g1 = s.mGain * mMusicMaster * s.mTrackGain * s.mAmplify;
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

#ifdef __EMSCRIPTEN__
// ---- the site's Music window following the game (docs/WEB_PLAYER.md, "Following a game") ----
// The page asks each animation frame while its Music window is on the screen and this game's window is the one in front; nothing
// here runs otherwise, and nothing changes what the game plays or hears: the views read the players' positions and patterns.

int Mixer::WebFollowedSong()
{
	int aBest = 0;
	float aLoudest = -1.0f;
	Lock();
	for (int i = 0; i < (int)mSongs.size(); i++)
	{
		const Song* s = mSongs[i].get();
		if (!s || !s->mPlaying || s->mPaused)
			continue;
		if (s->mGain > aLoudest)
		{
			aLoudest = s->mGain;
			aBest = i + 1;
		}
	}
	Unlock();
	return aBest;
}

std::string Mixer::WebSongView(int theSong)
{
	std::string aJson;
	Lock();
	SONG_FOR_HANDLE(theSong);
	if (s && s->mPlaying)
	{
		int v[5] = {0, 0, 0, 0, 0};
		std::string aTitle;
		bool aModule = false;
		if (s->mCtx)
		{
			xmp_frame_info anInfo;
			xmp_get_frame_info(s->mCtx, &anInfo);
			xmp_module_info aModuleInfo;
			xmp_get_module_info(s->mCtx, &aModuleInfo);
			v[0] = anInfo.pattern;
			v[1] = anInfo.row;
			v[2] = anInfo.num_rows;
			v[3] = aModuleInfo.mod->chn;
			v[4] = anInfo.pos;
			aTitle = aModuleInfo.mod->name;
			aModule = true;
		}
		else if (s->mModule && s->mModule->GetView(v))
		{
			aTitle = s->mModule->GetTitle();
			aModule = true;
		}
		char aBuf[160];
		if (aModule)
		{
			// the title as a JSON string: its quotes, backslashes and control characters escaped
			std::string aQuoted;
			for (unsigned char c : aTitle)
			{
				if (c == '"' || c == '\\')
				{
					aQuoted += '\\';
					aQuoted += (char)c;
				}
				else if (c < 0x20)
					aQuoted += ' ';
				else
					aQuoted += (char)c;
			}
			snprintf(aBuf, sizeof(aBuf), "{\"song\":%d,\"kind\":\"module\",\"pattern\":%d,\"row\":%d,\"rows\":%d,\"channels\":%d,\"order\":%d,\"title\":\"",
				theSong, v[0], v[1], v[2], v[3], v[4]);
			aJson = aBuf + aQuoted + "\"}";
		}
		else
		{
			snprintf(aBuf, sizeof(aBuf), "{\"song\":%d,\"kind\":\"stream\"}", theSong);
			aJson = aBuf;
		}
	}
	Unlock();
	return aJson;
}

std::string Mixer::WebSongRow(int theSong, int thePattern, int theRow)
{
	std::string aText;
	Lock();
	SONG_FOR_HANDLE(theSong);
	if (s && s->mCtx)
	{
		xmp_module_info aModuleInfo;
		xmp_get_module_info(s->mCtx, &aModuleInfo);
		const xmp_module* aMod = aModuleInfo.mod;
		if (thePattern >= 0 && thePattern < aMod->pat && aMod->xxp[thePattern])
			for (int c = 0; c < aMod->chn; c++)
			{
				static const xmp_event kNone = {};
				const xmp_track* t = aMod->xxt[aMod->xxp[thePattern]->index[c]];
				char aCell[24];
				byopop_xmp_cell(t && theRow >= 0 && theRow < t->rows ? &t->event[theRow] : &kNone, aCell);
				if (c)
					aText += '|';
				aText += aCell;
			}
	}
	else if (s && s->mModule)
		aText = s->mModule->GetRowText(thePattern, theRow);
	Unlock();
	return aText;
}

std::string Mixer::WebSongUsed(int theSong)
{
	unsigned char aUsed[256] = {};
	int aChannels = 0;
	Lock();
	SONG_FOR_HANDLE(theSong);
	if (s && s->mCtx)
	{
		xmp_module_info aModuleInfo;
		xmp_get_module_info(s->mCtx, &aModuleInfo);
		const xmp_module* aMod = aModuleInfo.mod;
		aChannels = aMod->chn;
		for (int p = 0; p < aMod->pat; p++)
		{
			if (!aMod->xxp[p])
				continue;
			for (int c = 0; c < aMod->chn && c < 256; c++)
			{
				const xmp_track* t = aMod->xxt[aMod->xxp[p]->index[c]];
				if (aUsed[c] || !t)
					continue;
				for (int r = 0; r < t->rows; r++)
					if (t->event[r].note >= 1 && t->event[r].note <= 120)
					{
						aUsed[c] = 1;
						break;
					}
			}
		}
	}
	else if (s && s->mModule)
		aChannels = s->mModule->GetUsedChannels(aUsed, 256);
	Unlock();
	// the channels that play a note, by number, separated by commas
	std::string aList;
	for (int c = 0; c < aChannels && c < 256; c++)
		if (aUsed[c])
		{
			if (!aList.empty())
				aList += ',';
			aList += std::to_string(c);
		}
	return aList;
}
#endif

}

#ifdef __EMSCRIPTEN__
// The page's calls (Module.ccall; web/music.js, "Following a game"), linked through -sEXPORTED_FUNCTIONS
// (cmake/ByopopWeb.cmake). A returned string stays valid until the next call.
static std::string gMusicResult;
extern "C"
{
// The song the Music window follows: its handle, 0 when the game plays none.
EMSCRIPTEN_KEEPALIVE int byopop_music_song()
{
	return Platform::Mixer::Get().WebFollowedSong();
}
// Where that song is, as JSON ("" for a song not playing).
EMSCRIPTEN_KEEPALIVE const char* byopop_music_view(int theSong)
{
	gMusicResult = Platform::Mixer::Get().WebSongView(theSong);
	return gMusicResult.c_str();
}
// A row of one of its patterns, the cells separated by '|'.
EMSCRIPTEN_KEEPALIVE const char* byopop_music_row(int theSong, int thePattern, int theRow)
{
	gMusicResult = Platform::Mixer::Get().WebSongRow(theSong, thePattern, theRow);
	return gMusicResult.c_str();
}
// The channels that play a note anywhere in it, by number, separated by commas.
EMSCRIPTEN_KEEPALIVE const char* byopop_music_used(int theSong)
{
	gMusicResult = Platform::Mixer::Get().WebSongUsed(theSong);
	return gMusicResult.c_str();
}
}
#endif
