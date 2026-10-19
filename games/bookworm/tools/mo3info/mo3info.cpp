// mo3info: prints an MO3 (or any libopenmpt) module's orders, subsongs and the loudness of each order,
// without an audio device. Used to document BookWorm's music sections (games/bookworm/docs/GAME_NOTES.md).
//
//   mo3info <file.mo3> [order ...]    with orders: renders 2 s from each order and prints its RMS level
#include <libopenmpt/libopenmpt.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "usage: mo3info <file> [order ...]\n");
		return 2;
	}
	FILE* f = fopen(argv[1], "rb");
	if (!f)
	{
		perror(argv[1]);
		return 1;
	}
	std::vector<char> data;
	char buf[65536];
	size_t got;
	while ((got = fread(buf, 1, sizeof(buf), f)) > 0)
		data.insert(data.end(), buf, buf + got);
	fclose(f);
	int err = 0;
	const char* msg = nullptr;
	openmpt_module* mod = openmpt_module_create_from_memory2(data.data(), data.size(), openmpt_log_func_silent, nullptr,
		openmpt_error_func_default, nullptr, &err, &msg, nullptr);
	if (!mod)
	{
		fprintf(stderr, "load failed: %d %s\n", err, msg ? msg : "");
		return 1;
	}
	const char* keys[] = {"type", "type_long", "tracker", "title", "artist"};
	for (const char* k : keys)
	{
		const char* v = openmpt_module_get_metadata(mod, k);
		printf("%s: %s\n", k, v);
		openmpt_free_string(v);
	}
	int orders = openmpt_module_get_num_orders(mod);
	printf("orders %d, patterns %d, channels %d, instruments %d, samples %d, subsongs %d, duration %.1f s\n", orders,
		openmpt_module_get_num_patterns(mod), openmpt_module_get_num_channels(mod), openmpt_module_get_num_instruments(mod),
		openmpt_module_get_num_samples(mod), openmpt_module_get_num_subsongs(mod), openmpt_module_get_duration_seconds(mod));
	for (int o = 0; o < orders; o++)
		printf("order 0x%02x: pattern %d (%d rows)\n", o, openmpt_module_get_order_pattern(mod, o),
			openmpt_module_get_pattern_num_rows(mod, openmpt_module_get_order_pattern(mod, o)));
	for (int i = 0; i < openmpt_module_get_num_subsongs(mod); i++)
	{
		const char* n = openmpt_module_get_subsong_name(mod, i);
		printf("subsong %d: %s\n", i, n);
		openmpt_free_string(n);
	}
	openmpt_module_set_repeat_count(mod, -1);
	std::vector<float> pcm(48000 * 2 * 2);
	for (int a = 2; a < argc; a++)
	{
		int order = (int)strtol(argv[a], nullptr, 0);
		openmpt_module_set_position_order_row(mod, order, 0);
		size_t n = openmpt_module_read_interleaved_float_stereo(mod, 48000, 96000, pcm.data());
		double sum = 0;
		for (size_t i = 0; i < n * 2; i++)
			sum += pcm[i] * pcm[i];
		printf("order 0x%02x: %zu frames, rms %.4f, now at order 0x%02x row %d\n", order, n,
			n ? sqrt(sum / (n * 2)) : 0.0, openmpt_module_get_current_order(mod), openmpt_module_get_current_row(mod));
	}
	openmpt_module_destroy(mod);
	return 0;
}
