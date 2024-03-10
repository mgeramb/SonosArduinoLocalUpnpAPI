#pragma once
#include "Arduino.h"

class SonosApi;
class SonosSpeaker;

enum SonosApiPlayState : byte
{
  Unkown,
  Stopped, // STOPPED
  Transitioning, // TRANSITIONING
  Playing, // PLAYING
  Paused_Playback, // PAUSED_PLAYBACK
};

enum SonosApiPlayMode : byte
{
  Normal = 0, // NORMAL
  Repeat = 1, // REPEAT_ONE || REPEAT_ALL || SHUFFLE_REPEAT_ONE || SHUFFLE_NOREPEAT
  RepeatModeAll = 2, // REPEAT_ALL || SHUFFLE
  Shuffle = 4 // SHUFFLE_NOREPEAT || SHUFFLE_NOREPEAT || SHUFFLE || SHUFFLE_REPEAT_ONE
};

class SonosTrackInfo
{
  public:
    uint16_t trackNumber;
	  uint32_t duration;
	  String uri;
    String metadata;
};

class SonosCurrentTrackInfo : public SonosTrackInfo
{
  public:
	  uint32_t position;
};


class SonosApiNotificationHandler
{
  public:
    virtual void notificationVolumeChanged(SonosSpeaker* speaker, uint8_t volume) {}
    virtual void notificationTrebleChanged(SonosSpeaker* speaker, int8_t treble) {}
    virtual void notificationBassChanged(SonosSpeaker* speaker, int8_t bass) {}
    virtual void notificationMuteChanged(SonosSpeaker* speaker, boolean mute) {}
    virtual void notificationLoudnessChanged(SonosSpeaker* speaker, boolean loudness) {}
    virtual void notificationGroupVolumeChanged(SonosSpeaker* speaker, uint8_t volume) {};
    virtual void notificationGroupMuteChanged(SonosSpeaker* speaker, boolean mute) {};
    virtual void notificationPlayStateChanged(SonosSpeaker* speaker, SonosApiPlayState playState) {};
    virtual void notificationPlayModeChanged(SonosSpeaker* speaker, SonosApiPlayMode playMode) {};
    virtual void notificationTrackChanged(SonosSpeaker* speaker, SonosTrackInfo& trackInfo) {};
    virtual void notificationGroupCoordinatorChanged(SonosSpeaker* speaker) {};
};


class SonosApiBrowseResult
{
  public:
    String title;
    String uri;
};