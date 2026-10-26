// Checks Mixer::GetSongFFT (the BASS_ChannelGetData FFT of MusicInterface::GetMusicFFT) against what BASS 2.3.0.1
// (Bookworm Adventures' bass.dll) returns for the same signals, measured on a decoding stream: a full-scale sine centred
// on a bin reads 1.0 there and 0.5 in each neighbour (the Hann window), half the amplitude half the value, a sine on
// one channel half (the channels are averaged), a sine half way between two bins about 0.8488 in both, a constant
// 0.5 reads 1.0 in bin 0, and silence zeros. The song's gain must not change it, and a stopped song has none.
//   spectrumtest
#include "platform/Audio.h"

#include <SDL.h>  // main becomes SDL_main, which SDL2main's entry point calls
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

namespace
{

class TonePlayer : public Platform::ModulePlayer
{
public:
	TonePlayer(double theBin, int theSamples, double theLeft, double theRight, double theConstant)
		: mStep(2.0 * 3.14159265358979323846 * theBin / theSamples), mLeft(theLeft), mRight(theRight),
		  mConstant(theConstant)
	{
	}

	void SetPosition(int, int) override { mPhase = 0; }

	bool Render(float* theOut, int theFrames, bool) override
	{
		for (int i = 0; i < theFrames; i++)
		{
			double s = std::sin(mPhase);
			mPhase += mStep;
			theOut[i * 2] = (float)(s * mLeft + mConstant);
			theOut[i * 2 + 1] = (float)(s * mRight + mConstant);
		}
		return true;
	}

private:
	double mStep, mLeft, mRight, mConstant;
	double mPhase = 0;
};

int gChecks = 0;
int gFailed = 0;

void Expect(const char* theCase, const char* theWhat, double theGot, double theWanted, double theTolerance)
{
	gChecks++;
	if (std::fabs(theGot - theWanted) > theTolerance)
	{
		gFailed++;
		printf("spectrum: %s: %s is %.6f, not %.6f\n", theCase, theWhat, theGot, theWanted);
	}
}

// Plays a tone through the mixer and checks its spectrum: theBins lists (bin, value) pairs; every other bin must be
// below theFloor.
void CheckTone(const char* theCase, int theSamples, double theBin, double theLeft, double theRight, double theConstant,
	std::vector<std::pair<int, double>> theBins, double theFloor = 1e-3)
{
	Platform::Mixer& mixer = Platform::Mixer::Get();
	int song = mixer.AddSong(std::unique_ptr<Platform::ModulePlayer>(
		new TonePlayer(theBin, theSamples, theLeft, theRight, theConstant)));
	mixer.PlaySong(song, 0, 0, true, true);
	mixer.SetSongGain(song, 0.3f);  // before the gain: the spectrum does not change with it
	// 4096 frames: the analysed frames then start where the tone's phase is 0, as in the BASS measurements, which
	// matters for the leakage into bin 0
	std::vector<float> out(4096 * 2);
	mixer.RenderOffline(out.data(), 4096);
	std::vector<float> fft(theSamples / 2, -1.0f);
	gChecks++;
	if (!mixer.GetSongFFT(song, fft.data(), theSamples))
	{
		gFailed++;
		printf("spectrum: %s: no spectrum\n", theCase);
	}
	else
	{
		std::vector<bool> named(fft.size(), false);
		for (auto& b : theBins)
		{
			char what[32];
			snprintf(what, sizeof(what), "bin %d", b.first);
			Expect(theCase, what, fft[b.first], b.second, 1e-3);
			named[b.first] = true;
		}
		int loud = 0;
		for (size_t k = 0; k < fft.size(); k++)
			if (!named[k] && fft[k] > theFloor)
				loud++;
		Expect(theCase, "bins above the floor", loud, 0, 0);
	}
	mixer.StopSong(song);
	gChecks++;
	if (mixer.GetSongFFT(song, fft.data(), theSamples))
	{
		gFailed++;
		printf("spectrum: %s: a stopped song still has a spectrum\n", theCase);
	}
	mixer.UnloadSong(song);
}

}

int main(int argc, char** argv)
{
	CheckTone("sine 1.0 at bin 40 of 2048", 2048, 40, 1, 1, 0, {{39, 0.5}, {40, 1.0}, {41, 0.5}});
	CheckTone("sine 0.5 at bin 40 of 2048", 2048, 40, 0.5, 0.5, 0, {{39, 0.25}, {40, 0.5}, {41, 0.25}});
	CheckTone("sine 1.0 at bin 20 of 1024", 1024, 20, 1, 1, 0, {{19, 0.5}, {20, 1.0}, {21, 0.5}});
	CheckTone("sine on the left only", 2048, 40, 1, 0, 0, {{39, 0.25}, {40, 0.5}, {41, 0.25}});
	CheckTone("sine at bin 1 of 2048", 2048, 1, 1, 1, 0, {{0, 0.0}, {1, 1.0}, {2, 0.5}});
	CheckTone("constant 0.5", 2048, 0, 0, 0, 0.5, {{0, 1.0}, {1, 0.5}});
	CheckTone("silence", 2048, 40, 0, 0, 0, {}, 1e-6);
	// half way between two bins the window's scalloping leaves 0.848827 in both (BASS: 0.848827 and 0.848826); the
	// leakage beyond them is wider, so only those two are checked
	CheckTone("sine at bin 40.5 of 2048", 2048, 40.5, 1, 1, 0, {{40, 0.848827}, {41, 0.848827}}, 1.0);

	printf("spectrum: %d checks, %d failed\n", gChecks, gFailed);
	return gFailed ? 1 : 0;
}
