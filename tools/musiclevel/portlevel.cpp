// The port's music level (docs/WEB_PLAYER.md, "Music levels"): renders each module offline through Platform::Mixer, as a
// game plays it (libxmp-lite, or libopenmpt for MO3 and OXM), at the song's own gain and the music master's of 1, and prints
// the RMS and peak of its first seconds in dBFS, the two channels together. The originals' side is musiclevel.exe (the
// engines' own DLLs). Native only.
//   portlevel <gamedir> <seconds> <music path>...
#include "platform/Audio.h"
#include "platform/VFS.h"
#include "platform/openmpt/OpenMPTPlayer.h"

#include <SDL.h>  // main becomes SDL_main, which SDL2main's entry point calls
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

int main(int argc, char** argv)
{
	if (argc < 4)
	{
		printf("usage: portlevel <gamedir> <seconds> <music path>...\n");
		return 2;
	}
	Platform::VFS::Get().SetBaseDir(argv[1]);
	Platform::RegisterOpenMPTModules();
	Platform::RegisterOpenMPTOggXM();
	double aSeconds = atof(argv[2]);
	Platform::Mixer& mixer = Platform::Mixer::Get();  // not initialized: renders at the default 48 kHz
	int failed = 0;
	for (int i = 3; i < argc; i++)
	{
		int song = mixer.LoadSong(argv[i]);
		if (!song)
		{
			printf("%s: failed to load\n", argv[i]);
			failed++;
			continue;
		}
		mixer.PlaySong(song, 0, 0, false, true);
		std::vector<float> out(4800 * 2);
		long long frames = (long long)(aSeconds * 48000), done = 0;
		double sum = 0, peak = 0;
		while (done < frames && mixer.IsSongPlaying(song))
		{
			mixer.RenderOffline(out.data(), 4800);
			for (float v : out)
			{
				sum += (double)v * v;
				peak = std::max(peak, (double)std::fabs(v));
			}
			done += 4800;
		}
		mixer.StopSong(song);
		printf("%s: RMS %.2f dBFS peak %.2f dBFS over %.1f s\n", argv[i], 10 * std::log10(sum / std::max(1LL, done * 2)),
			20 * std::log10(peak > 0 ? peak : 1e-9), done / 48000.0);
	}
	return failed ? 1 : 0;
}
