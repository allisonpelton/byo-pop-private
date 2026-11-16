// Checks Mixer::GetSongPosition (the BASS_ChannelGetPosition(BASS_POS_MUSIC_ORDER) of MusicInterface::GetMusicPosition):
// renders each module offline, as libxmp-lite or a registered ModulePlayer (MO3 through libopenmpt) plays it, and reads
// its order and row as it goes. A song started at order 0 must report order 0 first, then rows and orders that move on;
// one started at order 2 must report order 2; a stopped one reports nothing.
// For modules played by a ModulePlayer (MO3) it also checks the tempo and channel volumes (MusicInterface's
// GetMusicTempo, SetMusicTempo and SetMusicChannelVolume, BASS's BPM and VOL_CHAN music attributes): the module has
// a tempo, twice that tempo (half when twice is over BASS's 255) plays its first order in half the time (twice) and
// reports the tempo set; channels report volume 100 (-1 past the last), tempo and volume keep to BASS 2.2's 1 to 255
// and 0 to 100; and every channel at volume 0 is silence.
// And for MO3s, SetSongPlayersPooled: a player one song gives back and another takes renders what a fresh player does from
// the same position, bit for bit, and a song started again with no position carries on where it stopped.
//   songpostest <gamedir> <music path>...
#include "platform/Audio.h"
#include "platform/VFS.h"
#include "platform/openmpt/OpenMPTPlayer.h"

#include <SDL.h>  // main becomes SDL_main, which SDL2main's entry point calls
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
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
// theTempo it sets that tempo every 10 ms, so that the tempo commands in the patterns do not undo it while it measures
// (Bejeweled Twist's music sets it only when its state's tempo changes).
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

	int channels = 0;
	while (mixer.GetSongChannelVolume(song, channels) == 100)
		channels++;
	if (channels == 0 || mixer.GetSongChannelVolume(song, channels) != -1)
	{
		printf("tempo: %s: channel %d's volume is %d, not 100 or -1 past the last\n", thePath, channels,
			mixer.GetSongChannelVolume(song, channels));
		ok = false;
	}
	mixer.SetSongChannelVolume(song, 0, 200);
	mixer.SetSongTempo(song, 300);
	if (mixer.GetSongChannelVolume(song, 0) != 100 || mixer.GetSongTempo(song) != 255)
	{
		printf("tempo: %s: 200 gives channel volume %d and 300 BPM %d, not BASS's 100 and 255\n", thePath,
			mixer.GetSongChannelVolume(song, 0), mixer.GetSongTempo(song));
		ok = false;
	}
	mixer.SetSongTempo(song, bpm);
	for (int i = 0; i < channels; i++)
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
		printf("tempo: %s: %d BPM, first order %d frames, %.2f of that at %d BPM, %d silent channels ok\n",
			thePath, bpm, normal, ratio, tempo, channels);
	return ok;
}

// SetSongPlayersPooled (platform/Audio.h): song c plays elsewhere and stops, its player goes to the pool, song d takes it
// and plays from order 3; that must be the audio a fresh player gives for order 3 (song b, made before pooling was on).
// libopenmpt's renders differ a little from one fresh player to the next (rms 0.009 of a full scale of 1 here: some
// instruments swing their volume and pan at random, and libopenmpt draws the swings anew for each player), so the pooled
// player's difference from a fresh one is held against two fresh players' difference from each other. A pooled player
// that kept the mix plugins' state (Bejeweled Twist's music has an echo and a chorus) differed from a fresh one by four
// times that. Then c, started again with no position, carries on from the order and row where it stopped, and a song
// never played, started with no position, takes a pooled player and starts at the beginning as a fresh one does.
// Runs last: the mixer stays pooled.
static std::vector<float> RenderBlocks(Platform::Mixer& mixer, int theBlocks)
{
	std::vector<float> all;
	std::vector<float> out(4800 * 2);
	for (int i = 0; i < theBlocks; i++)
	{
		mixer.RenderOffline(out.data(), 4800);
		all.insert(all.end(), out.begin(), out.end());
	}
	return all;
}

static double RmsDifference(const std::vector<float>& theLeft, const std::vector<float>& theRight)
{
	double aSum = 0;
	for (size_t i = 0; i < theLeft.size(); i++)
		aSum += ((double)theLeft[i] - theRight[i]) * ((double)theLeft[i] - theRight[i]);
	return std::sqrt(aSum / theLeft.size());
}

static bool CheckPool(const char* thePath)
{
	Platform::Mixer& mixer = Platform::Mixer::Get();
	int a = mixer.LoadSong(thePath);
	int b = mixer.LoadSong(thePath);
	if (!a || !b || (mixer.GetSongTempo(a) <= 0))
	{
		// libxmp's songs have no player of a registered format to pool
		mixer.UnloadSong(a);
		mixer.UnloadSong(b);
		return true;
	}
	mixer.PlaySong(b, 3, 0, true, true);
	std::vector<float> aFresh = RenderBlocks(mixer, 20);
	mixer.StopSong(b);
	mixer.PlaySong(a, 3, 0, true, true);
	std::vector<float> aFreshAgain = RenderBlocks(mixer, 20);
	mixer.StopSong(a);
	double aFreshRms = RmsDifference(aFresh, aFreshAgain);
	mixer.UnloadSong(a);
	mixer.UnloadSong(b);
	int aStartOrder = -1, aStartRow = -1;
	{
		int fresh = mixer.LoadSong(thePath);
		mixer.PlaySong(fresh, 0, 0, true, false);   // never played, no position: the beginning
		RenderBlocks(mixer, 1);
		mixer.GetSongPosition(fresh, aStartOrder, aStartRow);
		mixer.StopSong(fresh);
		mixer.UnloadSong(fresh);
	}

	mixer.SetSongPlayersPooled(true);
	int c = mixer.LoadSong(thePath);
	int d = mixer.LoadSong(thePath);
	bool ok = c && d;
	int order = -1, row = -1, saved = -1, savedRow = -1;
	double aPooledRms = 0;
	if (ok)
	{
		mixer.PlaySong(c, 7, 4, true, true);
		RenderBlocks(mixer, 15);
		mixer.GetSongPosition(c, saved, savedRow);
		mixer.StopSong(c);       // c's player goes to the pool
		mixer.PlaySong(d, 3, 0, true, true);   // d takes it
		std::vector<float> aPooled = RenderBlocks(mixer, 20);
		aPooledRms = RmsDifference(aPooled, aFresh);
		if (aPooledRms > aFreshRms * 1.5 + 1e-4)
		{
			printf("pool: %s: from order 3 a pooled player differs from a fresh one by %g (rms), two fresh ones by %g\n", thePath, aPooledRms, aFreshRms);
			ok = false;
		}
		mixer.StopSong(d);
		mixer.PlaySong(c, 0, 0, true, false);  // no position: where it stopped
		RenderBlocks(mixer, 1);
		if (!mixer.GetSongPosition(c, order, row) || (order != saved) || (row < savedRow) || (row > savedRow + 4))
		{
			printf("pool: %s: stopped at order %d row %d, started again with no position it is at %d row %d\n", thePath, saved,
				savedRow, order, row);
			ok = false;
		}
		mixer.StopSong(c);       // c's player, at the order and row where it stopped, goes to the pool again
		int e = mixer.LoadSong(thePath);
		mixer.PlaySong(e, 0, 0, true, false);  // e was never played: it takes that player and starts at the beginning
		RenderBlocks(mixer, 1);
		int anEndOrder = -1, anEndRow = -1;
		if (!mixer.GetSongPosition(e, anEndOrder, anEndRow) || (anEndOrder != aStartOrder) || (anEndRow != aStartRow))
		{
			printf("pool: %s: a fresh player started with no position is at order %d row %d, a pooled one at order %d row %d\n", thePath,
				aStartOrder, aStartRow, anEndOrder, anEndRow);
			ok = false;
		}
		mixer.StopSong(e);
		mixer.UnloadSong(e);
	}
	mixer.UnloadSong(c);
	mixer.UnloadSong(d);
	mixer.SetSongPlayersPooled(false);
	if (ok)
		printf("pool: %s: from order 3 a pooled player differs from a fresh one by %g (rms), two fresh ones by %g; started again with no position it carries on from order %d row %d ok\n", thePath,
			aPooledRms, aFreshRms, saved, savedRow);
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
		if (!CheckSong(argv[i]) || !CheckTempo(argv[i]) || !CheckPool(argv[i]))
			failed++;
	printf("songpos: %d of %d songs ok\n", argc - 2 - failed, argc - 2);
	return failed ? 1 : 0;
}
