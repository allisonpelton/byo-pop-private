# Tremor (integer Ogg Vorbis decoder)

Xiph.Org's Tremor 2002 sources, BSD-style license in `COPYING`, as shipped in the `ogg/` folder of
SexyAppFramework 1.2 ([sandboxmo-de/SexyAppFramework-v1.2](https://github.com/sandboxmo-de/SexyAppFramework-v1.2)
at 3f57764). One change: `os_types.h` has a MinGW branch with `<stdint.h>` types (its `_WIN32` + `__GNUC__`
Cygwin branch needs `_G_config.h`). 1.2-era PopCap games (Big Money Deluxe) decoded their Ogg sounds with it;
`src/platform/Audio.cpp` uses it for files stb_vorbis cannot open (Vorbis floor 0).
