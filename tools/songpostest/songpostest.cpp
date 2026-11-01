// Checks Mixer::GetSongPosition (the BASS_ChannelGetPosition(BASS_POS_MUSIC_ORDER) of MusicInterface::GetMusicPosition):
// renders each module offline, as libxmp-lite or a registered ModulePlayer (MO3 through libopenmpt) plays it, and reads
// its order and row as it goes. A song started at order 0 must report order 0 first, then rows and orders that move on;
// one started at order 2 must report order 2; a stopped one reports nothing.
// For modules played by a ModulePlayer (MO3) it also checks the tempo and channel volumes (MusicInterface's
// GetMusicTempo, SetMusicTempo and SetMusicChannelVolume, BASS's BPM and VOL_CHAN music attributes): the module has
// a tempo, twice that tempo (half when twice is over BASS's 255) plays its first order in half the time (twice) and
// reports the tempo set, and every channel at volume 0 is silence.
//   songpostest <gamedir> <music path>...
#include "platform/Audio.h"
#include "platform/VFS.h"
#include "platform/openmpt/OpenMPTPlayer.h"

#include <SDL.h>  // main becomes SDL_main, which SDL2main's entry point calls
#include <algorithm>
#include <cmath>
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

// Frames rendered from order 0 until the song reports another order (0 when it never does within a minute). With
// theTempo it sets that tempo every 10 ms, as Bejeweled Twist's music Update does every frame, so that the tempo
// commands in the patterns do not undo it.
static int FramesInFirstOrder(Platform::Mixer& mixer, int song, int theTempo)
{
	std::vector<float> out(480 * 2);
	mixer.PlaySong(song, 0, 0, false, true);
	int order = 0, row = 0;
	for (int frames = 0; frames < 48000 * 60; frames += 480)
	{
		if (theTempo)
			mixer.SetSongTempo(song, theTempo);
		mixer.RenderOffline(out.data(), 480);
		if (mixer.GetSongPosition(song, order, row) && order != 0)
			return frames + 480;
	}
	return 0;
}

static bool CheckTempo(const char* thePath)
{
	Platform::Mixer& mixer = Platform::Mixer::Get();
	int song = mixer.LoadSong(thePath);
	if (!song)
		return false;
	int bpm = mixer.GetSongTempo(song);
	if (bpm <= 0)
	{
		// not a ModulePlayer song (libxmp): nothing to check
		mixer.UnloadSong(song);
		return true;
	}
	bool ok = true;
	// BASS takes 1 to 255 BPM: twice the tempo, or half of it when twice is out of range
	int tempo = bpm * 2 <= 255 ? bpm * 2 : bpm / 2;
	int normal = FramesInFirstOrder(mixer, song, bpm);
	mixer.SetSongTempo(song, tempo);
	int reported = mixer.GetSongTempo(song);
	int changed = FramesInFirstOrder(mixer, song, tempo);
	double ratio = normal > 0 ? (double)changed / normal : 0, expected = (double)bpm / tempo;
	if (normal <= 0 || ratio < expected * 0.95 || ratio > expected * 1.05 || reported != tempo)
	{
		printf("tempo: %s: %d BPM, first order %d frames, %d at %d BPM (reports %d BPM)\n", thePath, bpm, normal,
			changed, tempo, reported);
		ok = false;
	}
	mixer.SetSongTempo(song, bpm);

	for (int i = 0; i < 256; i++)
		mixer.SetSongChannelVolume(song, i, 0);
	std::vector<float> out(48000 * 2);
	mixer.PlaySong(song, 0, 0, false, true);
	mixer.RenderOffline(out.data(), 48000);
	float peak = 0;
	for (float v : out)
		peak = std::max(peak, std::fabs(v));
	if (peak > 1e-4f)
	{
		printf("tempo: %s: every channel at 0 still peaks at %g\n", thePath, peak);
		ok = false;
	}
	mixer.UnloadSong(song);
	if (ok)
		printf("tempo: %s: %d BPM, first order %d frames, %.2f of that at %d BPM, silent channels ok\n", thePath,
			bpm, normal, ratio, tempo);
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
		if (!CheckSong(argv[i]) || !CheckTempo(argv[i]))
			failed++;
	printf("songpos: %d of %d songs ok\n", argc - 2 - failed, argc - 2);
	return failed ? 1 : 0;
}
