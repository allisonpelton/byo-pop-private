#ifndef __PLATFORM_JP2DECODER_H__
#define __PLATFORM_JP2DECODER_H__

// JPEG 2000 images (.jp2 files, and raw .j2k codestreams) through OpenJPEG (third_party/openjpeg), for the
// games whose images are JPEG 2000 (Bookworm Adventures, ...; docs/POPCAP_GAMES_SURVEY.md 3.6, "Shared
// components"). It plugs into Platform::LoadGameImage / DecodeImage through Platform::RegisterImageFormat
// (platform/ImageLoader.h), so image names, alpha masks ("name_.jp2" beside "name.jp2"), HD variants and mods work as for any other format.
//
// Built as its own library (sexyopenjpeg, framework/CMakeLists.txt): a game that wants it links it and
// calls RegisterOpenJPEGImages() once before loading images. Games that do not never link OpenJPEG.

namespace Platform
{

// Registers ".jp2" and ".j2k" with Platform::RegisterImageFormat. Safe to call more than once.
void RegisterOpenJPEGImages();

}

#endif
