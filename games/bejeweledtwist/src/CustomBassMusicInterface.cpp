// Sexy::CustomBassMusicInterface (0x6ed1ed-0x6eee08): the framework's music with tracks
#include "CustomBassMusicInterface.h"

#include "Common.h"

using namespace Sexy;

CustomBassMusicInterface::CustomBassMusicInterface()
{
}

CustomBassMusicInterface::~CustomBassMusicInterface()
{
	for (auto& aPair : mTracks)
		delete aPair.second;
}

void CustomBassMusicInterface::SetState(int theState)
{
	// The original says "song state = %d" with OutputDebugString when it changes
	if (theState != mState)
		mState = theState;
}

bool CustomBassMusicInterface::LoadMusic(int theSongId, const std::string& theFileName)
{
	if (!SDLMusicInterface::LoadMusic(theSongId, theFileName))
		return false;
	mLoadTempo = GetMusicTempo(theSongId);
	return true;
}

void CustomBassMusicInterface::StopAllMusic()
{
	mCurTrack = -1;
	SDLMusicInterface::StopAllMusic();
}

CustomBassMusicInterface::Track* CustomBassMusicInterface::FindTrack(int theTrackId)
{
	auto it = mTracks.find(theTrackId);
	return it == mTracks.end() ? nullptr : it->second;
}

void CustomBassMusicInterface::PlayTrack(int theTrackId, bool noLoop)
{
	if (mCurTrack == theTrackId)
		return;
	Track* aTrack = FindTrack(theTrackId);
	if (aTrack == nullptr || mSongs.find(aTrack->mSongId) == mSongs.end())
		return;
	// Every channel down to nothing, then the state's tempo and the channels at their targets
	unsigned int i = 0;
	if (aTrack->mChannels.size() != 0) {
		do {
			aTrack->mChannels[i].mVolume = 0;
			SetMusicChannelVolume(aTrack->mSongId, i, 0);
			i++;
		} while (i < aTrack->mChannels.size());
	}
	mTempo = aTrack->mTempo[mState];
	SetMusicTempo(aTrack->mSongId, mTempo);
	i = 0;
	if (aTrack->mChannels.size() != 0) {
		do {
			Channel& aChannel = aTrack->mChannels[i];
			aChannel.mVolume = aChannel.mTarget[mState];
			SetMusicChannelVolume(aTrack->mSongId, i, aChannel.mVolume);
			i++;
		} while (i < aTrack->mChannels.size());
	}
	// port: the original prints "playing song %d,  state=%d, tempo=%d" to the debugger here
	SDLMusicInterface::PlayMusic(aTrack->mSongId, aTrack->mOffset, noLoop);
	mCurTrack = theTrackId;
}

void CustomBassMusicInterface::Update()
{
	SDLMusicInterface::Update();
	Track* aTrack = FindTrack(mCurTrack);
	if (aTrack == nullptr || mSongs.find(aTrack->mSongId) == mSongs.end())
		return;
	aTrack = FindTrack(mCurTrack);
	if (aTrack == nullptr) {
		mCurTrack = -1;
		return;
	}
	int aTempo = aTrack->mTempo[mState];
	if (mTempo != aTempo) {
		mTempo = aTempo;
		// port: the original prints "tempo going to %d" to the debugger here
		SetMusicTempo(aTrack->mSongId, mTempo);
	}
	unsigned int i = 0;
	if (aTrack->mChannels.size() != 0) {
		do {
			Channel& aChannel = aTrack->mChannels[i];
			int aTarget = (int)((float)aChannel.mTarget[mState] * mVolumeFactor);
			bool aSet = aChannel.mDirty;
			if (aChannel.mVolume != aTarget) {
				if (aChannel.mVolume < aTarget)
					aChannel.mVolume++;
				else
					aChannel.mVolume--;
				aSet = true;
			}
			if (aSet) {
				SetMusicChannelVolume(aTrack->mSongId, i, aChannel.mVolume);
				aChannel.mDirty = false;
			}
			i++;
		} while (i < aTrack->mChannels.size());
	}
}

void CustomBassMusicInterface::SetChannelVolume(int theChannel, int theVolume, bool theNow, int theTrackId)
{
	if (theTrackId == -1)
		theTrackId = mCurTrack;
	Track* aTrack = FindTrack(theTrackId);
	if (aTrack == nullptr || mSongs.find(aTrack->mSongId) == mSongs.end())
		return;
	if (theChannel < (int)aTrack->mChannels.size()) {
		Channel& aChannel = aTrack->mChannels[theChannel];
		if (theVolume == -1)
			theVolume = aChannel.mDefault;
		aChannel.mTarget[0] = theVolume;
		aChannel.mTarget[1] = theVolume;
		if (theNow) {
			aChannel.mVolume = theVolume;
			aChannel.mDirty = true;
		}
	}
}

void CustomBassMusicInterface::ResetChannels(int theTrackId)
{
	if (theTrackId == -1)
		theTrackId = mCurTrack;
	Track* aTrack = FindTrack(theTrackId);
	if (aTrack == nullptr || mSongs.find(aTrack->mSongId) == mSongs.end())
		return;
	unsigned int i = 0;
	if (aTrack->mChannels.size() != 0) {
		do {
			SetChannelVolume(i, -1, true, -1);
			i++;
		} while (i < aTrack->mChannels.size());
	}
}

void CustomBassMusicInterface::SetTrackTempo(int theState, int theTempo)
{
	Track* aTrack = FindTrack(mCurTrack);
	if (aTrack == nullptr || mSongs.find(aTrack->mSongId) == mSongs.end())
		return;
	aTrack->mTempo[theState] = theTempo;
}

CustomBassMusicInterface::Track* CustomBassMusicInterface::AddTrack(int theSongId, int theTrackId, int theTempo)
{
	if (mSongs.find(theSongId) == mSongs.end())
		return nullptr;
	Track* aTrack = FindTrack(theTrackId);
	if (aTrack == nullptr) {
		aTrack = new Track(theSongId, theTrackId);
		mTracks[theTrackId] = aTrack;
	}
	if (theTempo == 0)
		theTempo = GetMusicTempo(theSongId);
	// A channel for each the module has (BASS answers -1 past the last), at the module's volume
	int aChannel = 0;
	int aVolume = GetMusicChannelVolume(theSongId, aChannel);
	while (aVolume != -1) {
		Channel aNew;
		aNew.mVolume = aVolume;
		aNew.mTarget[0] = aVolume;
		aNew.mTarget[1] = aVolume;
		aNew.mDefault = aVolume;
		aNew.mDirty = true;
		aTrack->mChannels.push_back(aNew);
		aChannel++;
		aVolume = GetMusicChannelVolume(theSongId, aChannel);
	}
	aTrack->mTempo[0] = theTempo;
	aTrack->mTempo[1] = theTempo;
	return aTrack;
}
