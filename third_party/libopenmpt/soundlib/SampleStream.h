/*
 * SampleStream.h
 * --------------
 * byo-pop's addition to libopenmpt (third_party/libopenmpt/BYO-POP.md, "Changes"): a long Ogg Vorbis sample of an MO3
 * kept as its Vorbis data and decoded as it plays, instead of decoded whole when the module loads. The loader keeps the
 * bytes it would have decoded (SampleStreams::Keep) and gives the sample a placeholder of one frame; before each run of
 * the mixer over a channel playing it, CSoundFile::MixChannel asks for the frames that run reads (Window), which are
 * decoded from the start in the loader's own calls and conversion, so that every frame is the one the whole sample would
 * have held, padding included. Off unless byopop_openmpt_stream_samples(1) was called before the module was loaded.
 */

#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "Snd_defs.h"

#include <cstddef>
#include <memory>
#include <vector>

OPENMPT_NAMESPACE_BEGIN

struct ModSample;
struct ModChannel;
struct StreamedSample;

class SampleStreams
{
public:
	SampleStreams();
	~SampleStreams();

	// whether the loader keeps theSample (16-bit, no loops, at least a second at 44.1 kHz) instead of decoding it
	static bool Wanted(const ModSample &theSample);
	// theSample's Vorbis data, as the loader would have given it to stb_vorbis: theHeader (a shared header, empty when the
	// sample's data begins with its own) and theData; the sample gets its placeholder. False when it could not be kept.
	bool Keep(ModSample &theSample, std::vector<std::byte> theHeader, std::vector<std::byte> theData);
	// whether theSample's data is a placeholder (ModSample::PrecomputeLoops leaves it alone)
	bool IsStreamed(const ModSample &theSample) const;
	// for a run of theCount output frames of theChannel playing theSample: the frames it reads, *theShift being the frame
	// the pointer is at (the channel's position is moved back by it for the run); nullptr when theSample is not streamed
	const void *Window(const ModSample &theSample, const ModChannel &theChannel, uint32 theCount, int32 *theShift);

private:
	StreamedSample *Find(const ModSample &theSample) const;
	std::vector<std::unique_ptr<StreamedSample>> m_samples;
};

OPENMPT_NAMESPACE_END

// whether modules loaded from now on stream their long Ogg samples (byo-pop: platform/openmpt/OpenMPTPlayer.cpp)
extern "C" void byopop_openmpt_stream_samples(int theOn);
