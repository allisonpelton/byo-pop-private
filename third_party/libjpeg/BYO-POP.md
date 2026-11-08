# IJG libjpeg 6b (JPEG images)

The Independent JPEG Group's JPEG software, release 6b of 27-Mar-1998, from `https://www.ijg.org/files/jpegsrc.v6b.tar.gz`
(613,261 bytes, MD5 `dbd5f3b47ed13132f04c685d608a7547`, SHA-256
`75c3ec241e9996504fe02a9ed4d12f16b74ade713972f3db9e65ce95cd27e35d`), downloaded on 2026-11-07 with the user's approval (their
choice of 2026-11-07: add IJG libjpeg rather than list stb_image's rounding as a deviation). Its license is in `README`
(the IJG's, "LEGAL ISSUES"; `web/licenses/libjpeg.txt` carries it to the games' copyright pages, with the acknowledgement it
asks for: "this software is based in part on the work of the Independent JPEG Group").

## Why 6b

Every original game exe that decodes JPEGs carries libjpeg 6b: its version string `6b  27-Mar-1998` and its error texts are in
Bonnie's Bookstore's (through SDL_image), BookWorm's, Bejeweled Deluxe's, Bejeweled Twist's, both Bookworm Adventures games', Seven
Seas', Peggle Extreme's, TipTop's, Ning Po's, both Mahjong Escape games', Atomica's, Pixelus', Rocket Mania's, Dynomite's, Noah's
Ark's, Mummy Maze's and Word Harmony's exes (checked 2026-11-07). PopCap's SexyAppFramework source ships the same library: 36 of
the 37 decoder files of its `ImageLib/jpeg` are byte for byte the files here (line ends aside); the 37th, `jmorecfg.h`, differs
only in Visual C++ linkage macros and in `boolean` being `unsigned char`. stb_image's inverse DCT and chroma upsampling round
differently from libjpeg's (up to 3 levels on about 1% to 6% of a picture's pixels), which the screen comparisons with the
originals flag.

## What is here

Copied unchanged from the package: `README`, and the decompressor's sources `jcomapi.c jutils.c jerror.c jmemmgr.c jmemnobs.c
jdapimin.c jdapistd.c jdatasrc.c jdmaster.c jdinput.c jdmarker.c jdhuff.c jdphuff.c jdmainct.c jdcoefct.c jdpostct.c jddctmgr.c
jidctint.c jidctfst.c jidctflt.c jidctred.c jdsample.c jdcolor.c jquant1.c jquant2.c jdmerge.c` and the headers `jdhuff.h jdct.h
jerror.h jinclude.h jmemsys.h jmorecfg.h jpegint.h jpeglib.h jversion.h`. `jmemnobs.c` is the memory manager without temporary
files (the images fit in memory). Left out: the compressor, the transcoder (`jdtrans.c`), the command-line programs and
their format readers and writers, the other memory managers, the documentation, the build files and the other platforms'
`jconfig.*`. No original file was changed.

`jconfig.h` is ours: the settings `configure` picks for a plain ANSI C compiler (as in the `jconfig.h` PopCap's copy ships with),
for llvm-mingw and Emscripten; it changes nothing the decoder computes. `third_party/CMakeLists.txt` builds the library as
`ijgjpeg`.

`test/` holds three of the package's own files: `testorig.jpg` (baseline) and `testprog.jpg` (progressive), and
`testimg.ppm`, the reference output of `djpeg -dct int -ppm` on both, which the package's `make test` compares byte for byte.
`tools/jpegtest` runs the decoder on them.

## How the games call it

`platform/JPEGDecoder.cpp` is the one caller, with the calls and settings of PopCap's `ImageLib::GetJPEGImage`: `jpeg_read_header`,
`jpeg_start_decompress` and `jpeg_read_scanlines` with nothing set in between, so the defaults of 6b apply: the accurate integer
IDCT (`JDCT_ISLOW`), fancy upsampling, `JCS_RGB` output for colour files and one component for grey. (SDL_image's `IMG_jpg.c`,
which Bonnie's original used, sets only `out_color_space = JCS_RGB` and `quantize_colors = FALSE`, which are the defaults too.)
`platform/ImageLoader.cpp` routes JPEGs through it; stb_image keeps every other format, and any JPEG libjpeg refuses (four
components, CMYK).
