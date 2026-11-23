#ifndef __SPROUT2_THEORAREADER_H__
#define __SPROUT2_THEORAREADER_H__

// The frames of an animation's Theora file read one at a time (the reader AnimationData::loadFrames keeps on its stack,
// 0x108 bytes; its constructor 0x415ba0, open 0x47b930 and close 0x418e70 (feedingfrenzy2)). The file is not an Ogg
// stream but the Theora packets one after another, each behind a header of 28 bytes: its length, its first-packet and
// last-packet flags (32 bits each) and its granule position and number (64 bits each), little-endian. The first three
// packets are the stream's headers; each packet after them is a frame. The original links libtheora's old API; the port
// decodes with libtheora 1.1.1 (third_party/libtheora), whose output is the same, Theora's decoding being exact.

#include "Object.h"

#include <string>
#include <vector>

#include <theora/theoradec.h>

namespace engine
{

class InputStream;
class Surface;

class TheoraReader
{
public:
	TheoraReader();
	~TheoraReader();

	// 0x47b930 (feedingfrenzy2): the file opened from the archives and its three headers read and decoded, the decoder
	// made, the frame count at 0 and the file's folder kept for the frames' names
	void open(const std::string& thePath);
	// The last packet read was the stream's last (the packet's +0xa4 (feedingfrenzy2))
	bool atEnd() const { return mPacket.e_o_s != 0; }
	// +0xe4 (feedingfrenzy2): the frames read so far
	unsigned int getFrameIndex() const { return mFrameIndex; }
	// 0x45f400 (feedingfrenzy2): the next frame read; null at the end
	Surface* readFrame();
	// 0x474d00 (feedingfrenzy2): the next frame read and decoded, and dropped
	void skipFrame();
	// 0x415b40 (feedingfrenzy2): the frames before the one at the index decoded and dropped (up to the stream's end),
	// then the next read; null at the end
	Surface* readFrameAt(unsigned int theFrame)
	{
		while (mFrameIndex < theFrame && !atEnd())
			skipFrame();
		return readFrame();
	}

private:
	// 0x45f130 (feedingfrenzy2): the next packet's header and bytes
	void readPacket();
	// 0x45fd90 (feedingfrenzy2): a new surface of the picture's size with the decoded frame in it
	Surface* convertFrame();

	std::string mFolder;                // +0x00: the file's folder, as _splitpath gives it (its separator last)
	th_info mInfo;                      // +0x1c: the stream's header
	th_comment mComment;
	th_setup_info* mSetup;
	th_dec_ctx* mDecoder;               // +0x80
	ogg_packet mPacket;                 // +0x98
	std::vector<unsigned char> mBytes;  // +0x98 and +0xb8: the packet's bytes and their buffer's size
	th_ycbcr_buffer mFrame;             // +0xbc: the last frame decoded
	Ref<InputStream> mStream;           // +0xe0
	unsigned int mFrameIndex;           // +0xe4
};

} // namespace engine

#endif
