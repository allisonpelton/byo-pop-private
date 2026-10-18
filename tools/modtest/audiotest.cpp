// Renders a game music track offline through the real audio path (VFS + mod remaps + Mixer streaming)
// and writes raw float stereo for comparison with a reference decode.
//   audiotest <gamedir> <modsdir> <userdir> <game music path> <seconds> <out.f32>
#include "platform/Audio.h"
#include "platform/ModManager.h"
#include "platform/VFS.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

int main(int argc, char** argv)
{
	if (argc < 7)
	{
		printf("usage: audiotest <gamedir> <modsdir> <userdir> <music path> <seconds> <out.f32>\n");
		return 2;
	}
	Platform::VFS::Get().SetBaseDir(argv[1]);
	Platform::VFS::Get().SetUserDir(argv[3]);
	Platform::ModManager::Get().SetModsDir(argv[2]);
	Platform::ModManager::Get().Refresh();

	Platform::Mixer& mixer = Platform::Mixer::Get();  // not initialized: renders at the default 48 kHz
	int song = mixer.LoadSong(argv[4]);
	if (!song)
	{
		printf("failed to load %s\n", argv[4]);
		return 1;
	}
	const Platform::VFSRemap* remap = Platform::VFS::Get().GetRemap(argv[4]);
	printf("%s -> %s (start %.6f s, loop %.6f s)\n", argv[4], remap ? remap->mRealPath.c_str() : "(no remap)",
		remap ? remap->mStartTime : 0.0, remap ? remap->mLoopStart : 0.0);
	mixer.PlaySong(song, 0, 0, true, true);

	size_t frames = (size_t)(atof(argv[5]) * mixer.GetRate());
	std::vector<float> out(frames * 2);
	const size_t chunk = 2048;
	for (size_t pos = 0; pos < frames; pos += chunk)
		mixer.RenderOffline(&out[pos * 2], (int)std::min(chunk, frames - pos));
	FILE* f = fopen(argv[6], "wb");
	fwrite(out.data(), sizeof(float), out.size(), f);
	fclose(f);
	printf("rendered %zu frames\n", frames);
	return 0;
}
