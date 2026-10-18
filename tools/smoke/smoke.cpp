// Toolchain smoke test: SDL2 window + renderer, stb_image texture, libxmp S3M playback through SDL audio.
// Usage: smoke <Bejeweled files> [wav]   ("wav" renders 30 s of music offline to smoke.wav and reports peaks)
// The default, assets/bejeweled, is the staged copy (from the repository root; preloaded there on the web).
#include <SDL.h>
#include <stb_image.h>
#include <xmp.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static SDL_Window* gWindow;
static SDL_Renderer* gRenderer;
static SDL_Texture* gGems;
static xmp_context gXmp;
static int gFrame;
static std::vector<short> gTemp;

static void AudioCallback(void*, Uint8* stream, int len)
{
	// device format is F32 stereo at the device's native rate; libxmp renders S16 at that same rate
	int frames = len / (int)(sizeof(float) * 2);
	gTemp.resize(frames * 2);
	if (xmp_play_buffer(gXmp, gTemp.data(), frames * 4, 0) != 0)
	{
		SDL_memset(stream, 0, len);
		return;
	}
	float* out = (float*)stream;
	for (int i = 0; i < frames * 2; i++)
		out[i] = gTemp[i] * (1.0f / 32768.0f);
}

static void Frame()
{
	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		if (e.type == SDL_QUIT)
		{
#ifdef __EMSCRIPTEN__
			emscripten_cancel_main_loop();
#else
			exit(0);
#endif
		}
	}
	SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
	SDL_RenderClear(gRenderer);
	for (int gem = 0; gem < 7; gem++)
	{
		int cel = (gFrame / 3) % 15;
		SDL_Rect src = {cel * 52, gem * 104, 52, 52};
		SDL_Rect dst = {60 + gem * 70, 200, 52, 52};
		SDL_RenderCopy(gRenderer, gGems, &src, &dst);
	}
	SDL_RenderPresent(gRenderer);
	gFrame++;
}

static int RenderWav(const std::string& path)
{
	const int rate = 48000, seconds = 30;
	std::vector<short> pcm(rate * 2 * seconds);
	xmp_start_player(gXmp, rate, 0);
	xmp_play_buffer(gXmp, pcm.data(), (int)pcm.size() * 2, 0);
	long clipped = 0;
	int peak = 0;
	for (short s : pcm)
	{
		peak = std::max(peak, std::abs((int)s));
		if (s == 32767 || s == -32768)
			clipped++;
	}
	printf("offline render: peak %d (%.1f%% FS), clipped samples %ld of %zu\n", peak, peak * 100.0 / 32768, clipped, pcm.size());
	FILE* f = fopen(path.c_str(), "wb");
	int dataBytes = (int)pcm.size() * 2, chunk = 36 + dataBytes, fmtLen = 16, byteRate = rate * 4;
	short fmtTag = 1, channels = 2, align = 4, bits = 16;
	fwrite("RIFF", 1, 4, f); fwrite(&chunk, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f); fwrite(&fmtLen, 4, 1, f);
	fwrite(&fmtTag, 2, 1, f); fwrite(&channels, 2, 1, f); fwrite(&rate, 4, 1, f); fwrite(&byteRate, 4, 1, f);
	fwrite(&align, 2, 1, f); fwrite(&bits, 2, 1, f); fwrite("data", 1, 4, f); fwrite(&dataBytes, 4, 1, f);
	fwrite(pcm.data(), 2, pcm.size(), f);
	fclose(f);
	return 0;
}

int main(int argc, char** argv)
{
	std::string assets = argc > 1 ? argv[1] : "assets/bejeweled";
	gXmp = xmp_create_context();
	if (xmp_load_module(gXmp, (assets + "/music/1980-Autonomus.s3m").c_str()) != 0)
	{
		printf("xmp load failed\n");
		return 1;
	}
	if (argc > 2 && !strcmp(argv[2], "wav"))
		return RenderWav("smoke.wav");

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
	{
		printf("SDL_Init failed: %s\n", SDL_GetError());
		return 1;
	}
	gWindow = SDL_CreateWindow("smoke", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, 0);
	gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	int w, h, n;
	unsigned char* rgb = stbi_load((assets + "/images/gemsheet6.png").c_str(), &w, &h, &n, 4);
	unsigned char* mask = stbi_load((assets + "/images/_gemsheet6.gif").c_str(), &w, &h, &n, 4);
	if (!rgb || !mask)
	{
		printf("image load failed: %s\n", stbi_failure_reason());
		return 1;
	}
	for (int i = 0; i < w * h; i++)
		rgb[i * 4 + 3] = mask[i * 4 + 2];
	gGems = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, w, h);
	SDL_UpdateTexture(gGems, nullptr, rgb, w * 4);
	SDL_SetTextureBlendMode(gGems, SDL_BLENDMODE_BLEND);
	printf("gemsheet %dx%d loaded\n", w, h);

#ifdef __EMSCRIPTEN__
	// SDL's web backend adopts the AudioContext's native rate (often 96/192 kHz) and sizes its
	// ScriptProcessor buffer in frames at that rate, which underruns badly. Pre-create the context
	// at 48 kHz; SDL reuses an existing Module.SDL2.audioContext.
	EM_ASM({
		Module['SDL2'] = Module['SDL2'] || {};
		if (!Module['SDL2'].audioContext) {
			var Ctx = window.AudioContext || window.webkitAudioContext;
			try { Module['SDL2'].audioContext = new Ctx({ sampleRate: 48000 }); }
			catch (e) { Module['SDL2'].audioContext = new Ctx(); }
		}
	});
	const int bufferFrames = 2048;
#else
	const int bufferFrames = 1024;
#endif
	SDL_AudioSpec want = {}, have;
	want.freq = 48000;
	want.format = AUDIO_F32SYS;
	want.channels = 2;
	want.samples = bufferFrames;
	want.callback = AudioCallback;
	SDL_AudioDeviceID dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
	xmp_start_player(gXmp, have.freq, 0);
	SDL_PauseAudioDevice(dev, 0);
	printf("music playing: device %u, %d Hz, %d frames/buffer, format 0x%x\n", dev, have.freq, have.samples, have.format);

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(Frame, 0, 1);
#else
	for (int i = 0; i < 180; i++)
		Frame();
	printf("smoke ok\n");
#endif
	return 0;
}
