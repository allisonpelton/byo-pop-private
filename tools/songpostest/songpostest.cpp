// Checks Mixer::GetSongPosition (the BASS_ChannelGetPosition(BASS_POS_MUSIC_ORDER) of MusicInterface::GetMusicPosition):
// renders each module offline, as libxmp-lite or a registered ModulePlayer (MO3 through libopenmpt) plays it, and reads
// its order and row as it goes. A song started at order 0 must report order 0 first, then rows and orders that move on;
// one started at order 2 must report order 2; a stopped one reports nothing.
//   songpostest <gamedir> <music path>...
#include "platform/Audio.h"
#include "platform/VFS.h"
#include "platform/openmpt/OpenMPTPlayer.h"

#include <SDL.h>  // main becomes SDL_main, which SDL2main's entry point calls
#include <cstdio>
#include <set>
#include <utility>
#include <vector>

static bool CheckSong(const char* thePath)
{
	Platform::Mixer& mixer = Platform::Mixer::Get();  // not initialized: renders at the default 48 kHz
	int song = mixer.LoadSong(thePath);
	if (!song)
	{
		printf("songpos: %s: failed to load\n", thePath);
		return false;
	}
	int order = -1, row = -1;
	bool ok = true;
	std::vector<float> out(4800 * 2);  // a tenth of a second per read

	mixer.PlaySong(song, 0, 0, false, true);
	std::set<int> orders;
	std::set<std::pair<int, int>> positions;
	int first = -1;
	for (int i = 0; i < 600 && mixer.IsSongPlaying(song); i++)  // at most a minute
	{
		mixer.RenderOffline(out.data(), 4800);
		if (!mixer.GetSongPosition(song, order, row))
			break;
		if (first < 0)
			first = order;
		orders.insert(order);
		positions.insert(std::make_pair(order, row));
	}
	if (first != 0 || orders.size() < 2 || positions.size() < 8)
	{
		printf("songpos: %s: from order 0: first order %d, %d orders, %d positions\n", thePath, first,
			(int)orders.size(), (int)positions.size());
		ok = false;
	}
	int last = orders.empty() ? -1 : *orders.rbegin();

	mixer.PlaySong(song, 2, 0, false, true);
	mixer.RenderOffline(out.data(), 2400);
	if (!mixer.GetSongPosition(song, order, row) || order != 2)
	{
		printf("songpos: %s: from order 2: reports order %d\n", thePath, order);
		ok = false;
	}

	mixer.StopSong(song);
	if (mixer.GetSongPosition(song, order, row))
	{
		printf("songpos: %s: stopped, still reports order %d row %d\n", thePath, order, row);
		ok = false;
	}
	mixer.UnloadSong(song);
	if (ok)
		printf("songpos: %s: orders 0 to %d, %d positions, start at 2 ok\n", thePath, last, (int)positions.size());
	return ok;
}

int main(int argc, char** argv)
{
	if (argc < 3)
	{
		printf("usage: songpostest <gamedir> <music path>...\n");
		return 2;
	}
	Platform::VFS::Get().SetBaseDir(argv[1]);
	Platform::RegisterOpenMPTModules();
	int failed = 0;
	for (int i = 2; i < argc; i++)
		if (!CheckSong(argv[i]))
			failed++;
	printf("songpos: %d of %d songs ok\n", argc - 2 - failed, argc - 2);
	return failed ? 1 : 0;
}
