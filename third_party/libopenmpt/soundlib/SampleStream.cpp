/*
 * SampleStream.cpp
 * ----------------
 * byo-pop's addition to libopenmpt: see SampleStream.h.
 */

#include "stdafx.h"
#include "SampleStream.h"

#include "ModChannel.h"
#include "ModSample.h"
#include "Mixer.h"

#ifdef MPT_WITH_STBVORBIS
#include <stb_vorbis/stb_vorbis.c>
#include "openmpt/soundbase/Copy.hpp"
#endif  // MPT_WITH_STBVORBIS

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static std::atomic<int> gStreamSamples{0};

extern "C" void byopop_openmpt_stream_samples(int theOn)
{
	gStreamSamples = theOn;
}

OPENMPT_NAMESPACE_BEGIN

#ifdef MPT_WITH_STBVORBIS

static bool StreamLog()
{
	static const bool sLog = std::getenv("BYOPOP_STREAM_LOG") != nullptr && std::atoi(std::getenv("BYOPOP_STREAM_LOG")) != 0;
	return sLog;
}

// one way through a streamed sample: a decoder as Load_mo3.cpp's loop (m_offset frames out so far), and the frames
// decoded from frame m_start on that are kept
struct StreamCursor
{
	stb_vorbis *m_vorbis = nullptr;
	const std::byte *m_next = nullptr;
	std::size_t m_left = 0;
	int m_error = VORBIS__no_error;
	SmpLength m_offset = 0;
	bool m_ended = false;
	bool m_started = false;
	std::vector<int16> m_frames;
	SmpLength m_start = 0;
	SmpLength m_count = 0;
	uint64 m_lastUse = 0;

	~StreamCursor() { Close(); }

	void Close()
	{
		if(m_vorbis)
			stb_vorbis_close(m_vorbis);
		m_vorbis = nullptr;
	}
};

struct StreamedSample
{
	ModSample *m_sample = nullptr;
	const void *m_placeholder = nullptr;
	std::vector<std::byte> m_header;
	std::vector<std::byte> m_data;
	SmpLength m_length = 0;
	int m_channels = 1;
	// the frames the padding repeats (ModSample::PrecomputeLoops): frame 0 and the last
	int16 m_first[2] = {0, 0};
	int16 m_last[2] = {0, 0};
	// two ways through the sample: a note and the one it took over from (fading out) read it at two places
	StreamCursor m_cursors[2];
	uint64 m_uses = 0;
	// a run that reaches the padding is copied here with it
	std::vector<int16> m_edge;

	uint32 m_restarts = 0;
	uint64 m_skipped = 0;

	void Restart(StreamCursor &c)
	{
		c.Close();
		m_restarts++;
		c.m_started = true;
		c.m_frames.clear();
		c.m_start = 0;
		c.m_count = 0;
		c.m_offset = 0;
		c.m_ended = false;
		c.m_error = VORBIS__no_error;
		int consumed = 0;
		if(!m_header.empty())
			c.m_vorbis = stb_vorbis_open_pushdata(mpt::byte_cast<const unsigned char *>(m_header.data()), mpt::saturate_cast<int>(m_header.size()), &consumed, &c.m_error, nullptr);
		c.m_next = m_data.data();
		c.m_left = m_data.size();
		if(m_header.empty())
		{
			c.m_vorbis = stb_vorbis_open_pushdata(mpt::byte_cast<const unsigned char *>(c.m_next), mpt::saturate_cast<int>(c.m_left), &consumed, &c.m_error, nullptr);
			c.m_next += consumed;
			c.m_left -= consumed;
		}
		if(!c.m_vorbis)
			c.m_ended = true;
	}

	// the next Vorbis frame's frames appended, as the loader would have stored them; false at the end
	bool DecodeFrame(StreamCursor &c)
	{
		if(c.m_ended || !((c.m_error == VORBIS__no_error || (c.m_error == VORBIS_need_more_data && c.m_left > 0)) && c.m_offset < m_length))
		{
			c.m_ended = true;
			return false;
		}
		int channels = 0, decodedSamples = 0;
		float **output;
		int consumed = stb_vorbis_decode_frame_pushdata(c.m_vorbis, mpt::byte_cast<const unsigned char *>(c.m_next), mpt::saturate_cast<int>(c.m_left), &channels, &output, &decodedSamples);
		c.m_next += consumed;
		c.m_left -= consumed;
		LimitMax(decodedSamples, mpt::saturate_cast<int>(m_length - c.m_offset));
		if(decodedSamples > 0)
		{
			const std::size_t at = c.m_frames.size();
			c.m_frames.resize(at + static_cast<std::size_t>(decodedSamples) * m_channels, 0);
			if(channels == m_channels)
				CopyAudio(mpt::audio_span_interleaved(c.m_frames.data() + at, m_channels, decodedSamples), mpt::audio_span_planar(output, channels, decodedSamples));
			if(c.m_offset == 0)
				std::copy(c.m_frames.begin() + at, c.m_frames.begin() + at + m_channels, m_first);
			if(c.m_offset + decodedSamples == m_length)
				std::copy(c.m_frames.end() - m_channels, c.m_frames.end(), m_last);
			c.m_count += decodedSamples;
		}
		c.m_offset += decodedSamples;
		c.m_error = stb_vorbis_get_error(c.m_vorbis);
		return true;
	}

	// the cursor that reaches frames theFirst to theLast with the least decoding: one already at or before them, or
	// the one used least lately started again
	StreamCursor &Choose(int32 theFirst, int32 theLast)
	{
		StreamCursor *best = nullptr;
		int64 bestCost = 0;
		for(StreamCursor &c : m_cursors)
		{
			if(!c.m_started || theFirst < static_cast<int32>(c.m_start))
				continue;
			const int64 cost = std::max<int64>(0, static_cast<int64>(theLast) + 1 - static_cast<int64>(c.m_start + c.m_count));
			if(!best || cost < bestCost)
			{
				best = &c;
				bestCost = cost;
			}
		}
		if(best)
			return *best;
		StreamCursor *oldest = &m_cursors[0];
		for(StreamCursor &c : m_cursors)
			if(!c.m_started || (oldest->m_started && c.m_lastUse < oldest->m_lastUse))
				oldest = &c;
		if(StreamLog())
			std::fprintf(stderr, "openmpt stream: sample of %u frames started again for %d\n", (unsigned)m_length, (int)theFirst);
		Restart(*oldest);
		return *oldest;
	}

	// frames theFirst to theLast (theFirst may be up to the padding before 0, theLast up to the padding after the end)
	const int16 *Frames(int32 theFirst, int32 theLast)
	{
		const int32 realFirst = std::max(theFirst, int32(0));
		const int32 realLast = std::min(theLast, static_cast<int32>(m_length) - 1);
		StreamCursor &c = Choose(realFirst, realLast);
		c.m_lastUse = ++m_uses;
		// frames before the run are not kept
		if(realFirst > static_cast<int32>(c.m_start))
		{
			const SmpLength drop = std::min(static_cast<SmpLength>(realFirst) - c.m_start, c.m_count);
			if(drop >= 8192 || drop == c.m_count)
			{
				c.m_frames.erase(c.m_frames.begin(), c.m_frames.begin() + static_cast<std::size_t>(drop) * m_channels);
				c.m_start += drop;
				c.m_count -= drop;
			}
		}
		while(realLast >= static_cast<int32>(c.m_start + c.m_count))
		{
			if(!DecodeFrame(c))
				break;
			if(c.m_count > 0 && static_cast<int32>(c.m_start + c.m_count) <= realFirst)
			{
				// decoded only to be passed over
				m_skipped += c.m_count;
				c.m_frames.clear();
				c.m_start += c.m_count;
				c.m_count = 0;
			}
		}
		// past the decoder's end the whole sample held silence
		if(c.m_ended && realLast >= static_cast<int32>(c.m_start + c.m_count) && c.m_start + c.m_count < m_length)
		{
			const SmpLength upTo = std::min(static_cast<SmpLength>(realLast) + 1, m_length);
			if(c.m_count == 0 && c.m_start < static_cast<SmpLength>(realFirst))
				c.m_start = realFirst;
			c.m_frames.resize(static_cast<std::size_t>(upTo - c.m_start) * m_channels, 0);
			c.m_count = upTo - c.m_start;
		}
		if(theFirst >= 0 && theLast < static_cast<int32>(m_length))
			return c.m_frames.data() + static_cast<std::size_t>(theFirst - c.m_start) * m_channels;
		// the padding: frame 0 before the start, the last frame after the end (ModSample::PrecomputeLoops)
		m_edge.resize(static_cast<std::size_t>(theLast - theFirst + 1) * m_channels);
		for(int32 f = theFirst; f <= theLast; f++)
		{
			const int16 *frame = (f < 0) ? m_first : (f >= static_cast<int32>(m_length)) ? m_last : c.m_frames.data() + static_cast<std::size_t>(f - c.m_start) * m_channels;
			std::copy(frame, frame + m_channels, m_edge.begin() + static_cast<std::size_t>(f - theFirst) * m_channels);
		}
		return m_edge.data();
	}
};

SampleStreams::SampleStreams() {}

SampleStreams::~SampleStreams()
{
	if(StreamLog())
		for(const auto &s : m_samples)
			std::fprintf(stderr, "openmpt stream: sample of %u frames: %u starts, %llu frames decoded only to be passed over\n", (unsigned)s->m_length, (unsigned)s->m_restarts, (unsigned long long)s->m_skipped);
}

bool SampleStreams::Wanted(const ModSample &theSample)
{
	return gStreamSamples != 0 && theSample.uFlags[CHN_16BIT] && !theSample.uFlags[CHN_LOOP | CHN_SUSTAINLOOP] && theSample.nLength >= 44100 && theSample.GetNumChannels() <= 2;
}

bool SampleStreams::Keep(ModSample &theSample, std::vector<std::byte> theHeader, std::vector<std::byte> theData)
{
	void *placeholder = ModSample::AllocateSample(1, theSample.GetBytesPerSample());
	if(!placeholder)
		return false;
	theSample.FreeSample();
	theSample.pData.pSample = placeholder;
	auto s = std::make_unique<StreamedSample>();
	s->m_sample = &theSample;
	s->m_placeholder = placeholder;
	s->m_header = std::move(theHeader);
	s->m_data = std::move(theData);
	s->m_length = theSample.nLength;
	s->m_channels = theSample.GetNumChannels();
	m_samples.push_back(std::move(s));
	return true;
}

StreamedSample *SampleStreams::Find(const ModSample &theSample) const
{
	for(const auto &s : m_samples)
		if(s->m_sample == &theSample && s->m_placeholder == theSample.samplev())
			return s.get();
	return nullptr;
}

bool SampleStreams::IsStreamed(const ModSample &theSample) const
{
	return Find(theSample) != nullptr;
}

const void *SampleStreams::Window(const ModSample &theSample, const ModChannel &theChannel, uint32 theCount, int32 *theShift)
{
	StreamedSample *s = Find(theSample);
	if(!s)
		return nullptr;
	const SamplePosition from = theChannel.position;
	const SamplePosition to = theChannel.position + theChannel.increment * theCount;
	const int32 first = std::min(from.GetInt(), to.GetInt()) - static_cast<int32>(InterpolationLookaheadBufferSize);
	const int32 last = std::max(from.GetInt(), to.GetInt()) + static_cast<int32>(InterpolationLookaheadBufferSize) + 1;
	const int32 lowest = -static_cast<int32>(InterpolationLookaheadBufferSize);
	const int32 highest = static_cast<int32>(s->m_length) + static_cast<int32>(InterpolationLookaheadBufferSize) - 1;
	const int32 clippedFirst = std::max(first, lowest), clippedLast = std::min(last, highest);
	if(clippedFirst > clippedLast)
		return nullptr;
	*theShift = clippedFirst;
	return s->Frames(clippedFirst, clippedLast);
}

#else  // !MPT_WITH_STBVORBIS

struct StreamedSample {};
SampleStreams::SampleStreams() {}
SampleStreams::~SampleStreams() {}
bool SampleStreams::Wanted(const ModSample &) { return false; }
bool SampleStreams::Keep(ModSample &, std::vector<std::byte>, std::vector<std::byte>) { return false; }
StreamedSample *SampleStreams::Find(const ModSample &) const { return nullptr; }
bool SampleStreams::IsStreamed(const ModSample &) const { return false; }
const void *SampleStreams::Window(const ModSample &, const ModChannel &, uint32, int32 *) { return nullptr; }

#endif  // MPT_WITH_STBVORBIS

OPENMPT_NAMESPACE_END
