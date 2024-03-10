#pragma once
#include "SonosApiHelpers.h"
#ifdef USE_ESP_ASNC_WEB_SERVER
#include <ESPAsyncWebServer.h>
#else 
#include "WebServer.h"
#endif

#include <MicroXPath.h>
#include <MicroXPath_P.h>

class SonosApiParameterBuilder;
class SonosApiPlayNotification;

class SonosSpeaker 
#ifdef USE_ESP_ASNC_WEB_SERVER
: private AsyncWebHandler
#endif
{
    friend class SonosApi;
  private:
    SonosApi& _sonosApi;
    const static uint32_t _subscriptionTimeInSeconds = 600;
    String _uuid;
    String _groupCoordinatorUuid;
    SonosSpeaker* _groupCoordinatorCached = nullptr;
    IPAddress _speakerIP;
    uint16_t _channelIndex;
    uint32_t _renderControlSeq = 0;
    unsigned long _lastGroupSnapshot = 0;
    unsigned long _subscriptionTime = 0;
    SonosApiNotificationHandler* _notificationHandler = nullptr;
    SonosApiPlayNotification* _playNotification = nullptr;
  
    const std::string logPrefix()
    {
      return "Sonos.API";
    }
    static uint32_t formatedTimestampToSeconds(char* durationAsString);
    static void xPathOnWifiClient(MicroXPath_P& xPath, WiFiClient& wifiClient, PGM_P* path, uint8_t pathSize, char* resultBuffer, size_t resultBufferSize);
    static const char* xPathOnString(MicroXPath_P& xPath, const char* bufferToSearch, PGM_P* path, uint8_t pathSize, char* resultBuffer, size_t resultBufferSize, bool xmlDecode);

    typedef std::function<void(SonosApiParameterBuilder&)> TParameterBuilderFunction;
    void writeSoapHttpCall(Stream& stream, const char* soapUrl, const char* soapAction, const char* action, TParameterBuilderFunction parameterFunction, bool addInstanceNode);
    void writeSubscribeHttpCall(Stream& stream, const char* soapUrl);
    
    SonosApiPlayState getPlayStateFromString(const char* value);
    SonosApiPlayMode getPlayModeFromString(const char* value);
    const char* getPlayModeString(SonosApiPlayMode playMode);

    typedef std::function<void(WiFiClient&)> THandlerWifiResultFunction;
    int postAction(const char* soapUrl, const char* soapAction, const char* action, TParameterBuilderFunction parameterFunction = nullptr, THandlerWifiResultFunction wifiResultFunction = nullptr, bool addInstanceNode = true);
    int subscribeEvents(const char* soapUrl);
    void subscribeAll();

#ifdef USE_ESP_ASNC_WEB_SERVER
    // AsyncWebHandler
    bool canHandle(AsyncWebServerRequest *request) override final;
    void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) override final;
#endif   
  private:
#ifdef USE_ESP_ASNC_WEB_SERVER  
    SonosSpeaker(SonosApi& sonosApi, IPAddress speakerIP, AsyncWebServer* webServer);
#else
    SonosSpeaker(SonosApi& sonosApi, IPAddress speakerIP);
#endif
    ~SonosSpeaker();
    void loop();

  public:
    String& getUID();
    static String getUID(IPAddress ipAddress);

    IPAddress& getSpeakerIP();
#ifdef USE_ESP_ASNC_WEB_SERVER
    void setCallback(SonosApiNotificationHandler* notificationHandler);
#endif
    void setVolume(uint8_t volume);
    void setVolumeRelative(int8_t volume);
    uint8_t getVolume();
    void setMute(boolean mute);
    boolean getMute();
    void setLoudness(boolean loudness);
    boolean getLoudness();
    void setTreble(int8_t treble);
    int8_t getTreble();
    void setBass(int8_t bass);
    int8_t getBass();
    void setGroupVolume(uint8_t volume);
    void setGroupVolumeRelative(int8_t volume);
    uint8_t getGroupVolume();
    void setGroupMute(boolean mute);
    boolean getGroupMute();
    SonosApiPlayState getPlayState();
    void play();
    void pause();
    void stop();
    void next();
    void previous();
    void gotTrack(uint32_t trackNumber);
    SonosApiPlayMode getPlayMode();
    void setPlayMode(SonosApiPlayMode playMode);
    void setShuffle(bool shuffle);
    bool getShuffle();
    const SonosCurrentTrackInfo getTrackInfo();
    SonosSpeaker* findGroupCoordinator(bool cached = false);
    SonosSpeaker* findNextPlayingGroupCoordinator();
    void setAVTransportURI(const char* schema, const char* uri, const char* metadata = nullptr);
    void playInternetRadio(const char* streamingUrl, const char* radionStationName, const char* imageUrl = nullptr, const char* schema = nullptr);
    void playFromHttp(const char* url);
    void playMusicLibraryFile(const char* mediathekFilePath);
    void playMusicLibraryDirectory(const char* mediathekDirectory);
    void playSonosPlaylist(const char* playListTitle);
    void playLineIn();
    void playTVIn();
    void playQueue();
    void playQueue(bool shuffle);
    void joinToGroupCoordinator(SonosSpeaker* coordinator);
    void joinToGroupCoordinator(const char* uid);
    void unjoin();
    SonosSpeaker* findFirstParticipant(bool cached = false);
    void delegateGroupCoordinationTo(SonosSpeaker* sonosApi, bool rejoinGroup);
    void delegateGroupCoordinationTo(const char* uid, bool rejoinGroup);
    void gotoTrack(uint16_t trackNumber);
    void gotoTime(uint8_t hour, uint8_t minute, uint8_t second);
    void addTrackToQueue(const char *scheme, const char *address, const char* metadata = nullptr, uint16_t desiredTrackNumber = 0, bool enqueueAsNext = true);
    void removeAllTracksFromQueue();
    void setStatusLight(boolean on);
    boolean getStatusLight();
    const SonosApiBrowseResult browse(const char* objectId, uint32_t index, uint32_t* totalNumberOfItems);
    const SonosApiBrowseResult search(const char* objectId, const char* titleToSearch);
#ifdef ARDUINO_ARCH_ESP32     
    void playNotification(const char* url, uint8_t volume);
#endif
};