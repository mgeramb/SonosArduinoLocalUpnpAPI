#include "SonosApi.h"
#include "SonosApiParameterBuilder.h"
#ifdef ARDUINO_ARCH_ESP32   
#include "SonosApiPlayNotification.h"
#endif

const char p_SoapEnvelope[] PROGMEM = "s:Envelope";
const char p_SoapBody[] PROGMEM = "s:Body";


SonosSpeaker::~SonosSpeaker()
{
    auto it = std::find(_sonosApi._allSonosSpeakers.begin(), _sonosApi._allSonosSpeakers.end(), this);
    if (it != _sonosApi._allSonosSpeakers.end())
        _sonosApi._allSonosSpeakers.erase(it);
}

IPAddress& SonosSpeaker::getSpeakerIP()
{
    return _speakerIP;
}

SonosSpeaker::SonosSpeaker(
    SonosApi& sonosApi,
    IPAddress speakerIP
#ifdef USE_ESP_ASNC_WEB_SERVER
    , AsyncWebServer* webServer
#endif
    )
    : _sonosApi(sonosApi), _speakerIP(speakerIP)
{
    _channelIndex = _sonosApi._allSonosSpeakers.size();
#ifdef USE_ESP_ASNC_WEB_SERVER    
    webServer->addHandler(this);
#endif
    _sonosApi._allSonosSpeakers.push_back(this);
    _subscriptionTime = millis() - ((_subscriptionTimeInSeconds - _channelIndex) * 1000); // prevent inital subscription to be at the same time for all channels
}

void SonosSpeaker::xPathOnWifiClient(MicroXPath_P& xPath, WiFiClient& wifiClient, PGM_P* path, uint8_t pathSize, char* resultBuffer, size_t resultBufferSize)
{
    xPath.setPath(path, pathSize);
    while (wifiClient.available() && !xPath.getValue(wifiClient.read(), resultBuffer, resultBufferSize))
        ;
}

#ifdef USE_ESP_ASNC_WEB_SERVER
void SonosSpeaker::setCallback(SonosApiNotificationHandler* notificationHandler)
{
    _notificationHandler = notificationHandler;
}
#endif

void SonosSpeaker::subscribeAll()
{
    Serial.print("Subscribe ");
    Serial.println(millis());
    _subscriptionTime = millis();
    if (_subscriptionTime == 0)
        _subscriptionTime = 1;
    _renderControlSeq = 0;
    subscribeEvents("/MediaRenderer/RenderingControl/Event");
    subscribeEvents("/MediaRenderer/GroupRenderingControl/Event");
    subscribeEvents("/MediaRenderer/AVTransport/Event");
}

void SonosSpeaker::loop()
{
    if (_subscriptionTime > 0)
    {
        if (millis() - _subscriptionTime > _subscriptionTimeInSeconds * 1000)
        {
            subscribeAll();
        }
    }
#ifdef ARDUINO_ARCH_ESP32   
    if (_playNotification != nullptr)
    {
        if (_playNotification->checkFinished())
        {
            delete _playNotification;
            _playNotification = nullptr;
        }
    }
#endif
}
#ifdef USE_ESP_ASNC_WEB_SERVER   
bool SonosSpeaker::canHandle(AsyncWebServerRequest* request)
{
    if (request->url().startsWith("/notification/"))
    {
        if (request->url() == "/notification/" + (String)_channelIndex)
        {
            return true;
        }
    }
    return false;
}
#endif
const char p_PropertySet[] PROGMEM = "e:propertyset";
const char p_Property[] PROGMEM = "e:property";
const char p_LastChange[] PROGMEM = "LastChange";

void charBuffer_xPath(MicroXPath_P& xPath, const char* readBuffer, size_t readBufferLength, PGM_P* path, uint8_t pathSize, char* resultBuffer, size_t resultBufferSize)
{
    xPath.setPath(path, pathSize);
    for (size_t i = 0; i < readBufferLength; i++)
    {
        if (xPath.getValue(readBuffer[i], resultBuffer, resultBufferSize))
            return;
    }
}

boolean readFromEncodeXML(const char* encodedXML, const char* search, char* resultBuffer, size_t resultBufferSize)
{
    if (resultBufferSize > 0)
        resultBuffer[0] = '\0';
    const char* begin = strstr(encodedXML, search);
    if (begin != nullptr)
    {
        begin += strlen(search);
        int i = 0;
        for (; i < resultBufferSize - 1; i++)
        {
            auto c = *begin;
            if (c == '&' || c == '\0')
                break;
            resultBuffer[i] = c;
            begin++;
        }
        resultBuffer[i] = '\0';
        return true;
    }
    return false;
}

SonosApiPlayState SonosSpeaker::getPlayStateFromString(const char* value)
{
    if (strcmp(value, "STOPPED") == 0)
        return SonosApiPlayState::Stopped;
    else if (strcmp(value, "PLAYING") == 0)
        return SonosApiPlayState::Playing;
    else if (strcmp(value, "PAUSED_PLAYBACK") == 0)
        return SonosApiPlayState::Paused_Playback;
    else if (strcmp(value, "TRANSITIONING") == 0)
        return SonosApiPlayState::Transitioning;
    return SonosApiPlayState::Unkown;
}

const char p_PlayModeNormal[] PROGMEM = "NORMAL";
const char p_PlayModeRepeatOne[] PROGMEM = "REPEAT_ONE";
const char p_PlayModeRepeatAll[] PROGMEM = "REPEAT_ALL";
const char p_PlayModeShuffle[] PROGMEM = "SHUFFLE";
const char p_PlayModeShuffleRepeatOne[] PROGMEM = "SHUFFLE_REPEAT_ONE";
const char p_PlayModeShuffleNoRepeat[] PROGMEM = "SHUFFLE_NOREPEAT";

/*
  Normal = 0, // NORMAL
  Repeat = 1, // REPEAT_ONE || REPEAT_ALL || SHUFFLE_REPEAT_ONE || SHUFFLE_NOREPEAT
  RepeatModeAll = 2, // REPEAT_ALL || SHUFFLE
  Shuffle = 4 // SHUFFLE_NOREPEAT || SHUFFLE_NOREPEAT || SHUFFLE || SHUFFLE_REPEAT_ONE
*/

SonosApiPlayMode SonosSpeaker::getPlayModeFromString(const char* value)
{
    if (strcmp(value, p_PlayModeNormal) == 0)
        return SonosApiPlayMode::Normal;
    if (strcmp(value, p_PlayModeRepeatOne) == 0)
        return SonosApiPlayMode::Repeat;
    if (strcmp(value, p_PlayModeRepeatAll) == 0)
        return (SonosApiPlayMode)((byte)SonosApiPlayMode::Repeat | (byte)SonosApiPlayMode::RepeatModeAll);
    if (strcmp(value, p_PlayModeShuffle) == 0)
        return (SonosApiPlayMode)((byte)SonosApiPlayMode::Shuffle | (byte)SonosApiPlayMode::Repeat | (byte)SonosApiPlayMode::RepeatModeAll);
    if (strcmp(value, p_PlayModeShuffleRepeatOne) == 0)
        return (SonosApiPlayMode)((byte)SonosApiPlayMode::Shuffle | (byte)SonosApiPlayMode::Repeat);
    if (strcmp(value, p_PlayModeShuffleNoRepeat) == 0)
        return SonosApiPlayMode::Shuffle;
    else
        return SonosApiPlayMode::Normal;
}

const char* SonosSpeaker::getPlayModeString(SonosApiPlayMode playMode)
{
    if (playMode == SonosApiPlayMode::Normal)
        return p_PlayModeNormal;
    if (playMode == SonosApiPlayMode::Repeat)
        return p_PlayModeRepeatOne;
    if (playMode == (SonosApiPlayMode)((byte)SonosApiPlayMode::Repeat | (byte)SonosApiPlayMode::RepeatModeAll))
        return p_PlayModeRepeatAll;
    if (playMode == (SonosApiPlayMode)((byte)SonosApiPlayMode::Shuffle | (byte)SonosApiPlayMode::Repeat | (byte)SonosApiPlayMode::RepeatModeAll))
        return p_PlayModeShuffle;
    if (playMode == (SonosApiPlayMode)((byte)SonosApiPlayMode::Shuffle | (byte)SonosApiPlayMode::Repeat))
        return p_PlayModeShuffleRepeatOne;
    if (playMode == SonosApiPlayMode::Shuffle)
        return p_PlayModeShuffleNoRepeat;
    return p_PlayModeNormal;
}

const char masterVolume[] PROGMEM = "&lt;Volume channel=&quot;Master&quot; val=&quot;";
const char masterMute[] PROGMEM = "&lt;Mute channel=&quot;Master&quot; val=&quot;";
const char masterLoudness[] PROGMEM = "&lt;Loudness channel=&quot;Master&quot; val=&quot;";
const char treble[] PROGMEM = "&lt;Treble val=&quot;";
const char bass[] PROGMEM = "&lt;Bass val=&quot;";
const char transportState[] PROGMEM = "&lt;TransportState val=&quot;";
const char playMode[] PROGMEM = "&lt;CurrentPlayMode val=&quot;";
const char trackURI[] PROGMEM = "&lt;CurrentTrackURI val=&quot;";
const char trackMetaData[] PROGMEM = "&lt;CurrentTrackMetaData val=&quot;";
const char trackDuration[] PROGMEM = "&lt;CurrentTrackDuration val=&quot;";
const char trackNumber[] PROGMEM = "&lt;CurrentTrack val=&quot;";

// const char numberOfTracks[] PROGMEM = "&lt;NumberOfTracks val=&quot;";
// const char currentTrack[] PROGMEM = "&lt;CurrentTrack val=&quot;";
#ifdef USE_ESP_ASNC_WEB_SERVER   
void SonosSpeaker::handleBody(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total)
{
    Serial.print("Notification received ");
    Serial.println(millis());
    // Serial.println(String(data, len));
    if (_notificationHandler != nullptr)
    {
        bool groupCoordinatorChanged = false;
        MicroXPath_P xPath;
        const int bufferSize = 2000;
        char* buffer = new char[bufferSize + 1];
        buffer[bufferSize] = 0;
        buffer[0] = 0;
        PGM_P pathLastChange[] = {p_PropertySet, p_Property, p_LastChange};
        charBuffer_xPath(xPath, (const char*)data, len, pathLastChange, 3, buffer, bufferSize);
        const static int valueBufferSize = 1000;
        SonosTrackInfo* sonosTrackInfo = nullptr;
        char* valueBuffer = new char[1000];
        try
        {
            if (readFromEncodeXML(buffer, masterVolume, valueBuffer, valueBufferSize))
            {
                auto currentVolume = constrain(atoi(valueBuffer), 0, 100);
                Serial.print("Volume: ");
                Serial.println(currentVolume);
                _notificationHandler->notificationVolumeChanged(this, currentVolume);
            }
            if (readFromEncodeXML(buffer, bass, valueBuffer, valueBufferSize))
            {
                auto currentBass = constrain(atoi(valueBuffer), -10, 10);
                Serial.print("Bass: ");
                Serial.println(currentBass);
                _notificationHandler->notificationBassChanged(this, currentBass);
            }
            if (readFromEncodeXML(buffer, treble, valueBuffer, valueBufferSize))
            {
                auto currentTreble = constrain(atoi(valueBuffer), -10, 10);
                Serial.print("Treble: ");
                Serial.println(currentTreble);
                _notificationHandler->notificationTrebleChanged(this, currentTreble);
            }
            if (readFromEncodeXML(buffer, masterMute, valueBuffer, valueBufferSize))
            {
                auto currentMute = valueBuffer[0] == '1';
                Serial.print("Mute: ");
                Serial.println(currentMute);
                _notificationHandler->notificationMuteChanged(this, currentMute);
            }
            if (readFromEncodeXML(buffer, masterLoudness, valueBuffer, valueBufferSize))
            {
                auto currentMute = valueBuffer[0] == '1';
                Serial.print("Loudness: ");
                Serial.println(currentMute);
                _notificationHandler->notificationLoudnessChanged(this, currentMute);
            }
            if (readFromEncodeXML(buffer, transportState, valueBuffer, valueBufferSize))
            {
                auto playState = getPlayStateFromString(valueBuffer);
                Serial.print("Play State: ");
                Serial.println(playState);
                _notificationHandler->notificationPlayStateChanged(this, playState);
            }
            if (readFromEncodeXML(buffer, playMode, valueBuffer, valueBufferSize))
            {
                auto playMode = getPlayModeFromString(valueBuffer);
                Serial.print("Play Mode: ");
                Serial.println(playMode);
                _notificationHandler->notificationPlayModeChanged(this, playMode);
            }
            if (readFromEncodeXML(buffer, trackURI, valueBuffer, valueBufferSize))
            {
                Serial.print("Track URI: ");
                Serial.println(valueBuffer);
                if (sonosTrackInfo == nullptr)
                    sonosTrackInfo = new SonosTrackInfo();
                sonosTrackInfo->uri = valueBuffer;
            }
            if (readFromEncodeXML(buffer, trackMetaData, valueBuffer, valueBufferSize))
            {
                Serial.print("Track Metadata: ");
                Serial.println(valueBuffer);
                if (sonosTrackInfo == nullptr)
                    sonosTrackInfo = new SonosTrackInfo();
                sonosTrackInfo->metadata = valueBuffer;
            }
            if (readFromEncodeXML(buffer, trackDuration, valueBuffer, valueBufferSize))
            {
                Serial.print("Track Duration: ");
                Serial.println(valueBuffer);
                if (sonosTrackInfo == nullptr)
                    sonosTrackInfo = new SonosTrackInfo();
            }
            if (readFromEncodeXML(buffer, trackNumber, valueBuffer, valueBufferSize))
            {
                Serial.print("Track Number: ");
                Serial.println(valueBuffer);
                if (sonosTrackInfo == nullptr)
                    sonosTrackInfo = new SonosTrackInfo();
                sonosTrackInfo->trackNumber = atoi(valueBuffer);
            }
            if (sonosTrackInfo != nullptr)
            {
                _notificationHandler->notificationTrackChanged(this, *sonosTrackInfo);
                if (sonosTrackInfo->uri.startsWith(SonosApi::SchemaMaster))
                {
                    auto newGroupCoordinator = sonosTrackInfo->uri.substring(strlen(SonosApi::SchemaMaster));
                    if (newGroupCoordinator != _groupCoordinatorUuid)
                    {
                        _groupCoordinatorUuid = newGroupCoordinator;
                        groupCoordinatorChanged = true;
                    }
                }
                else
                {
                    if (!_groupCoordinatorUuid.isEmpty())
                    {
                        _groupCoordinatorUuid.clear();
                        groupCoordinatorChanged = true;
                    }
                }
            }
        }
        catch (...)
        {
            delete sonosTrackInfo;
            delete valueBuffer;
            throw;
        }
        delete sonosTrackInfo;
        delete valueBuffer;
        xPath.reset();
        buffer[0] = 0;
        PGM_P pathGroupVolume[] = {p_PropertySet, p_Property, "GroupVolume"};
        charBuffer_xPath(xPath, (const char*)data, len, pathGroupVolume, 3, buffer, bufferSize);
        if (strlen(buffer) > 0)
        {
            auto currentGroupVolume = constrain(atoi(buffer), 0, 100);
            Serial.print("Group Volume: ");
            Serial.println(currentGroupVolume);
            _notificationHandler->notificationGroupVolumeChanged(this, currentGroupVolume);
        }
        xPath.reset();
        buffer[0] = 0;
        PGM_P pathGroupMute[] = {p_PropertySet, p_Property, "GroupMute"};
        charBuffer_xPath(xPath, (const char*)data, len, pathGroupMute, 3, buffer, bufferSize);
        if (strlen(buffer) > 0)
        {
            auto currentGroupMute = buffer[0] == '1';
            Serial.print("Group Mute: ");
            Serial.println(currentGroupMute);
            _notificationHandler->notificationGroupMuteChanged(this, currentGroupMute);
        }
        delete buffer;
        if (groupCoordinatorChanged)
            _notificationHandler->notificationGroupCoordinatorChanged(this);
    }
    // Serial.println(String(data, len));
    request->send(200);
}
#endif

void SonosSpeaker::writeSoapHttpCall(Stream& stream, const char* soapUrl, const char* soapAction, const char* action, TParameterBuilderFunction parameterFunction, bool addInstanceNode)
{
    uint16_t contentLength = 174 + strlen(action) * 2 + strlen(soapAction);
    if (addInstanceNode)
        contentLength += 26;
    if (parameterFunction != nullptr)
    {
        SonosApiParameterBuilder countCharParameterBuilder(nullptr);
        parameterFunction(countCharParameterBuilder);
        contentLength += countCharParameterBuilder.length();
    }
    // HTTP Method, URL, version
    stream.print("POST ");
    stream.print(soapUrl);
    stream.print(" HTTP/1.1\r\n");
    // Header
    stream.print("HOST: ");
    stream.print(_speakerIP.toString());
    stream.print("\r\n");
    stream.print("Content-Type: text/xml; charset=\"utf-8\"\n");
    stream.printf("Content-Length: %d\r\n", contentLength);
    stream.print("SOAPAction: ");
    stream.print(soapAction);
    stream.print("#");
    stream.print(action);
    stream.print("\r\n");
    stream.print("Connection: close\r\n");
    stream.print("\r\n");
    // Body
    stream.print(F("<s:Envelope xmlns:s='http://schemas.xmlsoap.org/soap/envelope/' s:encodingStyle='http://schemas.xmlsoap.org/soap/encoding/'><s:Body><u:"));
    stream.print(action);
    stream.print(" xmlns:u='");
    stream.print(soapAction);
    stream.print("'>");
    if (addInstanceNode)
        stream.print(F("<InstanceID>0</InstanceID>"));
    if (parameterFunction != nullptr)
    {
        SonosApiParameterBuilder context(&stream);
        parameterFunction(context);
    }
    stream.print("</u:");
    stream.print(action);
    stream.print(F("></s:Body></s:Envelope>"));
}

int SonosSpeaker::postAction(const char* soapUrl, const char* soapAction, const char* action, TParameterBuilderFunction parameterFunction, THandlerWifiResultFunction wifiResultFunction, bool addInstanceNode)
{
    WiFiClient wifiClient;
    if (wifiClient.connect(_speakerIP, 1400) != true)
    {
        Serial.printf("connect to %s:1400 failed\r\n", _speakerIP.toString().c_str());
        return -1;
    }

    writeSoapHttpCall(Serial, soapUrl, soapAction, action, parameterFunction, addInstanceNode);
    Serial.println();
    writeSoapHttpCall(wifiClient, soapUrl, soapAction, action, parameterFunction, addInstanceNode);

    auto start = millis();
    while (!wifiClient.available())
    {
        if (millis() - start > 3000)
        {
            wifiClient.stop();
            return -2;
        }
    }
    if (wifiResultFunction != nullptr)
        wifiResultFunction(wifiClient);
    else
    {
        while (auto c = wifiClient.read())
        {
            if (c == -1)
                break;
            Serial.print((char)c);
        }
    }

    Serial.println();
    wifiClient.stop();
    return 0;
}

const char* renderingControlUrl PROGMEM = "/MediaRenderer/RenderingControl/Control";
const char* renderingControlSoapAction PROGMEM = "urn:schemas-upnp-org:service:RenderingControl:1";

void SonosSpeaker::setVolume(uint8_t volume)
{
    String parameter;
    parameter += F("<Channel>Master</Channel>");
    parameter += F("<DesiredVolume>");
    parameter += volume;
    parameter += F("</DesiredVolume>");
    postAction(renderingControlUrl, renderingControlSoapAction, "SetVolume", [volume](SonosApiParameterBuilder& b) {
        b.AddParameter("Channel", "Master");
        b.AddParameter("DesiredVolume", volume);
    });
}

void SonosSpeaker::setVolumeRelative(int8_t volume)
{
    postAction(renderingControlUrl, renderingControlSoapAction, "SetRelativeVolume", [volume](SonosApiParameterBuilder& b) {
        b.AddParameter("Channel", "Master");
        b.AddParameter("Adjustment", volume);
    });
}

uint8_t SonosSpeaker::getVolume()
{
    uint8_t currentVolume = 0;
    postAction(
        renderingControlUrl, renderingControlSoapAction, "GetVolume", [](SonosApiParameterBuilder& b) { b.AddParameter("Channel", "Master"); }, [=, &currentVolume](WiFiClient& wifiClient) {
        MicroXPath_P xPath;
        PGM_P path[] = {p_SoapEnvelope, p_SoapBody, "u:GetVolumeResponse", "CurrentVolume"};
        char resultBuffer[10];
        xPathOnWifiClient(xPath, wifiClient, path, 4, resultBuffer, sizeof(resultBuffer));
        currentVolume = (uint8_t)constrain(atoi(resultBuffer), 0, 100); });
    return currentVolume;
}

void SonosSpeaker::setMute(boolean mute)
{
    postAction(renderingControlUrl, renderingControlSoapAction, "SetMute", [mute](SonosApiParameterBuilder& b) {
        b.AddParameter("Channel", "Master");
        b.AddParameter("DesiredMute", mute);
    });
}

boolean SonosSpeaker::getMute()
{
    boolean currentMute = 0;
    postAction(
        renderingControlUrl, renderingControlSoapAction, "GetMute", [](SonosApiParameterBuilder& b) { b.AddParameter("Channel", "Master"); }, [=, &currentMute](WiFiClient& wifiClient) {
        MicroXPath_P xPath;
        PGM_P path[] = {p_SoapEnvelope, p_SoapBody, "u:GetMuteResponse", "CurrentMute"};
        char resultBuffer[10];
        xPathOnWifiClient(xPath, wifiClient, path, 4, resultBuffer, sizeof(resultBuffer));
        currentMute = resultBuffer[0] == '1'; });
    return currentMute;
}

const char* devicePropertiesUrl PROGMEM = "/DeviceProperties/Control";
const char* devicePropertiesSoapAction PROGMEM = "urn:schemas-upnp-org:service:DeviceProperties:1";

void SonosSpeaker::setStatusLight(boolean on)
{
    postAction(
        devicePropertiesUrl, devicePropertiesSoapAction, "SetLEDState", [on](SonosApiParameterBuilder& b) {
            b.AddParameter("DesiredLEDState", on ? "On" : "Off");
        },
        nullptr, false);
}

boolean SonosSpeaker::getStatusLight()
{
    boolean currentLEDState = 0;
    postAction(
        devicePropertiesUrl, devicePropertiesSoapAction, "GetLEDState", nullptr, [=, &currentLEDState](WiFiClient& wifiClient) {
            MicroXPath_P xPath;
            PGM_P path[] = {p_SoapEnvelope, p_SoapBody, "u:GetLEDStateResponse", "CurrentLEDState"};
            char resultBuffer[10];
            xPathOnWifiClient(xPath, wifiClient, path, 4, resultBuffer, sizeof(resultBuffer));
            currentLEDState = strcmp(resultBuffer, "On") == 0;
        },
        false);
    return currentLEDState;
}

int8_t SonosSpeaker::getTreble()
{
    int8_t currentTreble = 0;
    postAction(
        renderingControlUrl, renderingControlSoapAction, "GetTreble", [](SonosApiParameterBuilder& b) { b.AddParameter("Channel", "Master"); }, [=, &currentTreble](WiFiClient& wifiClient) {
        MicroXPath_P xPath;
        PGM_P path[] = {p_SoapEnvelope, p_SoapBody, "u:GetTrebleResponse", "CurrentTreble"};
        char resultBuffer[10];
        xPathOnWifiClient(xPath, wifiClient, path, 4, resultBuffer, sizeof(resultBuffer));
        currentTreble = (int8_t)constrain(atoi(resultBuffer), -10, 10); });
    return currentTreble;
}

void SonosSpeaker::setTreble(int8_t treble)
{
    postAction(renderingControlUrl, renderingControlSoapAction, "SetTreble", [treble](SonosApiParameterBuilder& b) {
        b.AddParameter("Channel", "Master");
        b.AddParameter("DesiredTreble", treble);
    });
}

int8_t SonosSpeaker::getBass()
{
    int8_t currentBass = 0;
    postAction(
        renderingControlUrl, renderingControlSoapAction, "GetBass", [](SonosApiParameterBuilder& b) { b.AddParameter("Channel", "Master"); }, [=, &currentBass](WiFiClient& wifiClient) {
        MicroXPath_P xPath;
        PGM_P path[] = {p_SoapEnvelope, p_SoapBody, "u:GetBassResponse", "CurrentBass"};
        char resultBuffer[10];
        xPathOnWifiClient(xPath, wifiClient, path, 4, resultBuffer, sizeof(resultBuffer));
        currentBass = (int8_t)constrain(atoi(resultBuffer), -10, 10); });
    return currentBass;
}

void SonosSpeaker::setBass(int8_t bass)
{
    postAction(renderingControlUrl, renderingControlSoapAction, "SetBass", [bass](SonosApiParameterBuilder& b) {
        b.AddParameter("Channel", "Master");
        b.AddParameter("DesiredBass", bass);
    });
}

void SonosSpeaker::setLoudness(boolean loudness)
{
    postAction(renderingControlUrl, renderingControlSoapAction, "SetLoudness", [loudness](SonosApiParameterBuilder& b) {
        b.AddParameter("Channel", "Master");
        b.AddParameter("DesiredLoudness", loudness);
    });
}

boolean SonosSpeaker::getLoudness()
{
    boolean currentLoudness = 0;
    postAction(
        renderingControlUrl, renderingControlSoapAction, "GetLoudness", [](SonosApiParameterBuilder& b) { b.AddParameter("Channel", "Master"); }, [=, &currentLoudness](WiFiClient& wifiClient) {
        MicroXPath_P xPath;
        PGM_P path[] = {p_SoapEnvelope, p_SoapBody, "u:GetLoudnessResponse", "CurrentLoudness"};
        char resultBuffer[10];
        xPathOnWifiClient(xPath, wifiClient, path, 4, resultBuffer, sizeof(resultBuffer));
        currentLoudness = resultBuffer[0] == '1'; });
    return currentLoudness;
}

const char* renderingGroupRenderingControlUrl PROGMEM = "/MediaRenderer/GroupRenderingControl/Control";
const char* renderingGroupRenderingControlSoapAction PROGMEM = "urn:schemas-upnp-org:service:GroupRenderingControl:1";

void SonosSpeaker::setGroupVolume(uint8_t volume)
{
    auto currentTime = millis();
    if (currentTime - _lastGroupSnapshot > 5000 || _lastGroupSnapshot == 0)
    {
        _lastGroupSnapshot = currentTime;
        postAction(renderingGroupRenderingControlUrl, renderingGroupRenderingControlSoapAction, "SnapshotGroupVolume");
    }
    postAction(renderingGroupRenderingControlUrl, renderingGroupRenderingControlSoapAction, "SetGroupVolume", [volume](SonosApiParameterBuilder& b) {
        b.AddParameter("DesiredVolume", volume);
    });
}

void SonosSpeaker::setGroupVolumeRelative(int8_t volume)
{
    auto currentTime = millis();
    if (currentTime - _lastGroupSnapshot > 5000 || _lastGroupSnapshot == 0)
    {
        _lastGroupSnapshot = currentTime;
        postAction(renderingGroupRenderingControlUrl, renderingGroupRenderingControlSoapAction, "SnapshotGroupVolume");
    }
    postAction(renderingGroupRenderingControlUrl, renderingGroupRenderingControlSoapAction, "SetRelativeGroupVolume", [volume](SonosApiParameterBuilder& b) {
        b.AddParameter("Adjustment", volume);
    });
}

uint8_t SonosSpeaker::getGroupVolume()
{
    uint8_t currentGroupVolume = 0;
    postAction(renderingGroupRenderingControlUrl, renderingGroupRenderingControlSoapAction, "GetGroupVolume", nullptr, [=, &currentGroupVolume](WiFiClient& wifiClient) {
        MicroXPath_P xPath;
        PGM_P path[] = {p_SoapEnvelope, p_SoapBody, "u:GetGroupVolumeResponse", "CurrentVolume"};
        char resultBuffer[10];
        xPathOnWifiClient(xPath, wifiClient, path, 4, resultBuffer, sizeof(resultBuffer));
        currentGroupVolume = (uint8_t)constrain(atoi(resultBuffer), 0, 100);
    });
    return currentGroupVolume;
}

void SonosSpeaker::setGroupMute(boolean mute)
{
    postAction(renderingGroupRenderingControlUrl, renderingGroupRenderingControlSoapAction, "SetGroupMute", [mute](SonosApiParameterBuilder& b) {
        b.AddParameter("DesiredMute", mute);
    });
}

boolean SonosSpeaker::getGroupMute()
{
    boolean currentGroupMute = false;
    postAction(renderingGroupRenderingControlUrl, renderingGroupRenderingControlSoapAction, "GetGroupMute", nullptr, [=, &currentGroupMute](WiFiClient& wifiClient) {
        MicroXPath_P xPath;
        PGM_P path[] = {p_SoapEnvelope, p_SoapBody, "u:GetGroupMuteResponse", "CurrentMute"};
        char resultBuffer[10];
        xPathOnWifiClient(xPath, wifiClient, path, 4, resultBuffer, sizeof(resultBuffer));
        currentGroupMute = resultBuffer[0] == '1';
    });
    return currentGroupMute;
}

const char* renderingAVTransportUrl PROGMEM = "/MediaRenderer/AVTransport/Control";
const char* renderingAVTransportSoapAction PROGMEM = "urn:schemas-upnp-org:service:AVTransport:1";

void SonosSpeaker::setAVTransportURI(const char* schema, const char* currentURI, const char* currentURIMetaData)
{
    postAction(renderingAVTransportUrl, renderingAVTransportSoapAction, "SetAVTransportURI", [schema, currentURI, currentURIMetaData](SonosApiParameterBuilder& b) {
        b.BeginParameter("CurrentURI");
        b.ParmeterValuePart(schema, SonosApiParameterBuilder::ENCODE_NO);
        b.ParmeterValuePart(currentURI, SonosApiParameterBuilder::ENCODE_XML);
        b.EndParameter();
        b.AddParameter("CurrentURIMetaData", currentURIMetaData);
    });
}

void SonosSpeaker::joinToGroupCoordinator(SonosSpeaker* coordinator)
{
    if (coordinator == nullptr)
        return;
    joinToGroupCoordinator(coordinator->getUID().c_str());
}

void SonosSpeaker::joinToGroupCoordinator(const char* uid)
{
    setAVTransportURI(SonosApi::SchemaMaster, uid);
}

void SonosSpeaker::unjoin()
{
    postAction(renderingAVTransportUrl, renderingAVTransportSoapAction, "BecomeCoordinatorOfStandaloneGroup");
}

SonosApiPlayState SonosSpeaker::getPlayState()
{
    SonosApiPlayState currentPlayState = SonosApiPlayState::Unkown;
    postAction(renderingAVTransportUrl, renderingAVTransportSoapAction, "GetTransportInfo", nullptr, [=, &currentPlayState](WiFiClient& wifiClient) {
        MicroXPath_P xPath;
        PGM_P path[] = {p_SoapEnvelope, p_SoapBody, "u:GetTransportInfoResponse", "CurrentTransportState"};
        char resultBuffer[20];
        xPathOnWifiClient(xPath, wifiClient, path, 4, resultBuffer, sizeof(resultBuffer));
        currentPlayState = getPlayStateFromString(resultBuffer);
    });
    return currentPlayState;
}

void SonosSpeaker::play()
{
    postAction(renderingAVTransportUrl, renderingAVTransportSoapAction, "Play", [](SonosApiParameterBuilder& b) {
        b.AddParameter("Speed", 1);
    });
}

void SonosSpeaker::pause()
{
    postAction(renderingAVTransportUrl, renderingAVTransportSoapAction, "Pause");
}

void SonosSpeaker::stop()
{
    postAction(renderingAVTransportUrl, renderingAVTransportSoapAction, "Stop");
}

void SonosSpeaker::next()
{
    postAction(renderingAVTransportUrl, renderingAVTransportSoapAction, "Next");
}

void SonosSpeaker::previous()
{
    postAction(renderingAVTransportUrl, renderingAVTransportSoapAction, "Previous");
}

SonosApiPlayMode SonosSpeaker::getPlayMode()
{
    SonosApiPlayMode currentPlayMode = SonosApiPlayMode::Normal;
    postAction(renderingAVTransportUrl, renderingAVTransportSoapAction, "GetTransportSettings", nullptr, [=, &currentPlayMode](WiFiClient& wifiClient) {
        MicroXPath_P xPath;
        PGM_P path[] = {p_SoapEnvelope, p_SoapBody, "u:GetTransportSettingsResponse", "PlayMode"};
        char resultBuffer[20];
        xPathOnWifiClient(xPath, wifiClient, path, 4, resultBuffer, sizeof(resultBuffer));
        currentPlayMode = getPlayModeFromString(resultBuffer);
    });
    return currentPlayMode;
}

void SonosSpeaker::setPlayMode(SonosApiPlayMode playMode)
{
    postAction(renderingAVTransportUrl, renderingAVTransportSoapAction, "SetPlayMode", [this, playMode](SonosApiParameterBuilder& b) {
        b.AddParameter("NewPlayMode", this->getPlayModeString(playMode));
    });
}

void SonosSpeaker::setShuffle(bool shuffle)
{
    auto playMode = (byte) getPlayMode();
    auto newPlayMode = playMode;
    if (shuffle)
        newPlayMode |= (byte) SonosApiPlayMode::Shuffle;
    else
        newPlayMode &= ~ (byte)SonosApiPlayMode::Shuffle;
    if (newPlayMode != playMode)
        setPlayMode((SonosApiPlayMode) newPlayMode);
}

bool SonosSpeaker::getShuffle()
{
    auto playMode = getPlayMode();
    return (playMode & SonosApiPlayMode::Shuffle) != 0; 
}

void SonosSpeaker::writeSubscribeHttpCall(Stream& stream, const char* soapUrl)
{
    auto ip = _speakerIP.toString();
    // HTTP Method, URL, version
    stream.print("SUBSCRIBE ");
    stream.print(soapUrl);
    stream.print(" HTTP/1.1\r\n");
    // Header
    stream.print("HOST: ");
    stream.print(ip);
    stream.print("\r\n");
    stream.print("callback: <http://");
    stream.print(WiFi.localIP());
    stream.print(":");
    stream.print(_sonosApi._port);
    stream.print("/notification/");
    stream.print(_channelIndex);
    stream.print(">\r\n");
    stream.print("NT: upnp:event\r\n");
    stream.print("Timeout: Second-");
    stream.print(_subscriptionTimeInSeconds);
    stream.print("\r\n");
    stream.print("Content-Length: 0\r\n");
    stream.print("\r\n");
}

int SonosSpeaker::subscribeEvents(const char* soapUrl)
{
    WiFiClient wifiClient;
    if (wifiClient.connect(_speakerIP, 1400) != true)
    {
        Serial.printf("connect to %s:1400 failed\r\n", _speakerIP.toString().c_str());
        return -1;
    }

    writeSubscribeHttpCall(Serial, soapUrl);
    writeSubscribeHttpCall(wifiClient, soapUrl);

    auto start = millis();
    while (!wifiClient.available())
    {
        if (millis() - start > 3000)
        {
            wifiClient.stop();
            return -2;
        }
    }
    while (auto c = wifiClient.read())
    {
        if (c == -1)
            break;
        Serial.print((char)c);
    }
    Serial.println();
    wifiClient.stop();
    return 0;
}

String SonosSpeaker::getUID(IPAddress ipAddress)
{
    WiFiClient wifiClient;
    if (wifiClient.connect(ipAddress, 1400) != true)
    {
        return String();
    }
    wifiClient.print("GET http://");
    wifiClient.print(ipAddress.toString());
    wifiClient.print(":1400/status/zp HTTP/1.1\r\n");
    // Header
    wifiClient.print("HOST: ");
    wifiClient.print(ipAddress.toString());
    wifiClient.print("\r\n");
    wifiClient.print("Connection: close\r\n");
    wifiClient.print("\r\n");
    ;

    auto start = millis();
    while (!wifiClient.available())
    {
        if (millis() - start > 3000)
        {
            wifiClient.stop();
            return String();
        }
    }

    PGM_P path[] = {"ZPSupportInfo", "ZPInfo", "LocalUID"};
    char resultBuffer[30];
    MicroXPath_P xPath;

    xPathOnWifiClient(xPath, wifiClient, path, 3, resultBuffer, 30);
    wifiClient.stop();
    return resultBuffer;
}

String& SonosSpeaker::getUID()
{
    if (_uuid.length() == 0)
    {
        _uuid = getUID(_speakerIP);
    }
    return _uuid;
}

uint32_t SonosSpeaker::formatedTimestampToSeconds(char* durationAsString)
{
    const char* begin = durationAsString;
    int multiplicator = 3600;
    uint32_t result = 0;
    for (size_t i = 0;; i++)
    {
        auto c = durationAsString[i];
        if (c == ':' || c == 0)
        {
            durationAsString[i] = 0;
            result += atoi(begin) * multiplicator;
            begin = durationAsString + i + 1;
            if (multiplicator == 1 || c == 0)
                break;
            multiplicator = multiplicator / 60;
        }
    }
    return result;
}

const SonosCurrentTrackInfo SonosSpeaker::getTrackInfo()
{
    SonosCurrentTrackInfo sonosTrackInfo;
    postAction(renderingAVTransportUrl, renderingAVTransportSoapAction, "GetPositionInfo", nullptr, [=, &sonosTrackInfo](WiFiClient& wifiClient) {
        MicroXPath_P xPath;
        const int bufferSize = 2000;
        auto resultBuffer = new char[bufferSize];
        uint16_t trackNumber;
        uint32_t duration;
        uint32_t position;
        String uri;
        String metadata;
        PGM_P pathTrack[] = {p_SoapEnvelope, p_SoapBody, "u:GetPositionInfoResponse", "Track"};
        xPathOnWifiClient(xPath, wifiClient, pathTrack, 4, resultBuffer, bufferSize);
        trackNumber = atoi(resultBuffer);
        PGM_P pathDuration[] = {p_SoapEnvelope, p_SoapBody, "u:GetPositionInfoResponse", "TrackDuration"};
        xPathOnWifiClient(xPath, wifiClient, pathDuration, 4, resultBuffer, bufferSize);
        duration = formatedTimestampToSeconds(resultBuffer);
        PGM_P pathMetadata[] = {p_SoapEnvelope, p_SoapBody, "u:GetPositionInfoResponse", "TrackMetaData"};
        xPathOnWifiClient(xPath, wifiClient, pathMetadata, 4, resultBuffer, bufferSize);
        metadata = resultBuffer;
        PGM_P pathUri[] = {p_SoapEnvelope, p_SoapBody, "u:GetPositionInfoResponse", "TrackURI"};
        xPathOnWifiClient(xPath, wifiClient, pathUri, 4, resultBuffer, bufferSize);
        uri = resultBuffer;
        PGM_P pathRelTime[] = {p_SoapEnvelope, p_SoapBody, "u:GetPositionInfoResponse", "RelTime"};
        xPathOnWifiClient(xPath, wifiClient, pathRelTime, 4, resultBuffer, bufferSize);
        position = formatedTimestampToSeconds(resultBuffer);

        delete resultBuffer;
        sonosTrackInfo.trackNumber = trackNumber;
        sonosTrackInfo.duration = duration;
        sonosTrackInfo.position = position;
        sonosTrackInfo.uri = uri;
        sonosTrackInfo.metadata = metadata;
    });
    return sonosTrackInfo;
}

SonosSpeaker* SonosSpeaker::findGroupCoordinator(bool cached)
{
    if (cached)
    {
        if (_groupCoordinatorCached != nullptr)
            return _groupCoordinatorCached;
        if (_groupCoordinatorUuid.length() == 0)
        {
            _groupCoordinatorCached = this;
            return this;
        }
    }
    else
    {
        auto trackInfo = getTrackInfo();
        if (!trackInfo.uri.startsWith(SonosApi::SchemaMaster))
        {
            if (_groupCoordinatorUuid.length() != 0)
            {
                _groupCoordinatorUuid = "";
                _groupCoordinatorCached = this;
                if (_notificationHandler != nullptr)
                    _notificationHandler->notificationGroupCoordinatorChanged(this);
            }
            return this;
        }
        auto uidCoordinator = trackInfo.uri.substring(9);
        if (uidCoordinator != _groupCoordinatorUuid)
        {
            _groupCoordinatorUuid = uidCoordinator;
            _groupCoordinatorCached = nullptr;
            if (_notificationHandler != nullptr)
                _notificationHandler->notificationGroupCoordinatorChanged(this);
        }
    }
    for (auto iter = _sonosApi._allSonosSpeakers.begin(); iter < _sonosApi._allSonosSpeakers.end(); iter++)
    {
        auto sonosSpeaker = *iter;
        if (sonosSpeaker != this && sonosSpeaker->getUID() == _groupCoordinatorUuid)
        {
            _groupCoordinatorCached = sonosSpeaker;
            return sonosSpeaker;
        }
    }
    return nullptr;
}

SonosSpeaker* SonosSpeaker::findNextPlayingGroupCoordinator()
{
    std::vector<SonosSpeaker*> allGroupCoordinators;
    allGroupCoordinators.reserve(_sonosApi._allSonosSpeakers.size());
    SonosSpeaker* ownCoordinator = nullptr;
    String uidOfOwnCoordinator;
    for (auto iter = _sonosApi._allSonosSpeakers.begin(); iter < _sonosApi._allSonosSpeakers.end(); iter++)
    {
        auto sonosSpeaker = *iter;
        auto trackInfo = sonosSpeaker->getTrackInfo();
        bool isCoordinator = !trackInfo.uri.startsWith(SonosApi::SchemaMaster);
        if (isCoordinator)
        {
            allGroupCoordinators.push_back(sonosSpeaker);
        }
        if (sonosSpeaker == this)
        {
            if (isCoordinator)
            {
                ownCoordinator = this;
            }
            else
            {
                uidOfOwnCoordinator = trackInfo.uri.substring(9);
            }
        }
    }
    bool searchPlayingGroup = false;
    // Searh playing coordinator behind own coordinator
    for (auto iter = allGroupCoordinators.begin(); iter < allGroupCoordinators.end(); iter++)
    {
        auto sonosSpeaker = *iter;
        if (searchPlayingGroup)
        {
            auto playState = sonosSpeaker->getPlayState();
            if (playState == SonosApiPlayState::Playing || playState == SonosApiPlayState::Transitioning)
                return sonosSpeaker; // Next coordinator found
        }
        else
        {
            if (ownCoordinator == sonosSpeaker)
            {
                // Own coordinator found
                searchPlayingGroup = true;
            }
            else if (uidOfOwnCoordinator.length() != 0)
            {
                if (sonosSpeaker->getUID() == uidOfOwnCoordinator)
                {
                    // Own coordinator found
                    searchPlayingGroup = true;
                    ownCoordinator = sonosSpeaker;
                }
            }
        }
    }
    // Searh playing coordinator before own coordinator
    for (auto iter = allGroupCoordinators.begin(); iter < allGroupCoordinators.end(); iter++)
    {
        auto sonosSpeaker = *iter;
        if (sonosSpeaker == ownCoordinator)
            return nullptr; // no other playing coordinator found
        auto playState = sonosSpeaker->getPlayState();
        if (playState == SonosApiPlayState::Playing || playState == SonosApiPlayState::Transitioning)
            return sonosSpeaker; // Next coordinator found
    }
    return nullptr; // no other playing coordinator found
}

SonosSpeaker* SonosSpeaker::findFirstParticipant(bool cached)
{
    auto uid = getUID();
    for (auto iter = _sonosApi._allSonosSpeakers.begin(); iter < _sonosApi._allSonosSpeakers.end(); iter++)
    {
        auto sonosSpeaker = *iter;
        if (sonosSpeaker != this)
        {
            if (findGroupCoordinator(cached) == this)
                return sonosSpeaker;
        }
    }
    return nullptr;
}

void SonosSpeaker::delegateGroupCoordinationTo(SonosSpeaker* sonosSpeaker, bool rejoinGroup)
{
    if (sonosSpeaker == nullptr)
        return;
    delegateGroupCoordinationTo(sonosSpeaker->getUID().c_str(), rejoinGroup);
}

void SonosSpeaker::delegateGroupCoordinationTo(const char* uid, bool rejoinGroup)
{
    postAction(renderingAVTransportUrl, renderingAVTransportSoapAction, "DelegateGroupCoordinationTo", [uid, rejoinGroup](SonosApiParameterBuilder& b) {
        b.AddParameter("NewCoordinator", uid);
        b.AddParameter("RejoinGroup", rejoinGroup);
    });
}

void SonosSpeaker::gotoTrack(uint16_t trackNumber)
{
    postAction(renderingAVTransportUrl, renderingAVTransportSoapAction, "Seek", [trackNumber](SonosApiParameterBuilder& b) {
        b.AddParameter("Unit", "TRACK_NR");
        b.AddParameter("Target", trackNumber);
    });
}

void SonosSpeaker::gotoTime(uint8_t hour, uint8_t minute, uint8_t second)
{
    postAction(renderingAVTransportUrl, renderingAVTransportSoapAction, "Seek", [hour, minute, second](SonosApiParameterBuilder& b) {
        char time[12];
        sprintf_P(time, "%d:%02d:%02d", hour, minute, second);
        b.AddParameter("Unit", "REL_TIME");
        b.AddParameter("Target", trackNumber);
    });
}

void SonosSpeaker::addTrackToQueue(const char* scheme, const char* address, const char* metadata, uint16_t desiredTrackNumber, bool enqueueAsNext)
{
    postAction(renderingAVTransportUrl, renderingAVTransportSoapAction, "AddURIToQueue", [scheme, address, metadata, desiredTrackNumber, enqueueAsNext](SonosApiParameterBuilder& b) {
        b.BeginParameter("EnqueuedURI");
        b.ParmeterValuePart(scheme, SonosApiParameterBuilder::ENCODE_NO);
        b.ParmeterValuePart(address);
        b.EndParameter();
        b.AddParameter("EnqueuedURIMetaData", metadata);
        b.AddParameter("DesiredFirstTrackNumberEnqueued", desiredTrackNumber);
        b.AddParameter("EnqueueAsNext", enqueueAsNext);
    });
}

void SonosSpeaker::removeAllTracksFromQueue()
{
    postAction(renderingAVTransportUrl, renderingAVTransportSoapAction, "RemoveAllTracksFromQueue");
}

const char radioMetadataBeginTitle[] PROGMEM = "<DIDL-Lite xmlns:dc='http://purl.org/dc/elements/1.1/' xmlns:upnp='urn:schemas-upnp-org:metadata-1-0/upnp/' xmlns:r='urn:schemas-rinconnetworks-com:metadata-1-0/' xmlns='urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/'><item id='R:0/0/46' parentID='R:0/0' restricted='true'><dc:title>";
const char radioMetadataEndTitleBeginImageUrl[] PROGMEM = "</dc:title><upnp:albumArtURI>";
const char radioMetadataEndImageUrl[] PROGMEM = "</upnp:albumArtURI><upnp:class>object.item.audioItem.audioBroadcast</upnp:class><desc id='cdudn' nameSpace='urn:schemas-rinconnetworks-com:metadata-1-0/'>SA_RINCON65031_</desc></item></DIDL-Lite>";


void SonosSpeaker::playInternetRadio(const char* streamingUrl, const char* radioStationName, const char* imageUrl, const char* schema)
{
    if (schema == nullptr)
        schema = SonosApi::DefaultSchemaInternetRadio;
    postAction(renderingAVTransportUrl, renderingAVTransportSoapAction, "SetAVTransportURI", [streamingUrl, radioStationName, imageUrl, schema](SonosApiParameterBuilder& b) {
        b.BeginParameter("CurrentURI");
        b.ParmeterValuePart(schema, SonosApiParameterBuilder::ENCODE_NO);
        b.ParmeterValuePart(streamingUrl, SonosApiParameterBuilder::ENCODE_XML);
        b.EndParameter();
        b.BeginParameter("CurrentURIMetaData");
        b.ParmeterValuePart(radioMetadataBeginTitle, SonosApiParameterBuilder::ENCODE_XML);
        b.ParmeterValuePart(radioStationName, SonosApiParameterBuilder::ENCODE_DOUBLE_XML);
        b.ParmeterValuePart(radioMetadataEndTitleBeginImageUrl, SonosApiParameterBuilder::ENCODE_XML);
        b.ParmeterValuePart(imageUrl, SonosApiParameterBuilder::ENCODE_DOUBLE_XML);
        b.ParmeterValuePart(radioMetadataEndImageUrl, SonosApiParameterBuilder::ENCODE_XML);
        b.EndParameter();
    });
    play();
}

void SonosSpeaker::playFromHttp(const char* url)
{
    setAVTransportURI(nullptr, url);
    play();
}



void SonosSpeaker::playMusicLibraryFile(const char* mediathekFilePath)
{
    postAction(renderingAVTransportUrl, renderingAVTransportSoapAction, "SetAVTransportURI", [mediathekFilePath](SonosApiParameterBuilder& b) {
        b.BeginParameter("CurrentURI");
        b.ParmeterValuePart(SonosApi::SchemaMusicLibraryFile, SonosApiParameterBuilder::ENCODE_NO);
        b.ParmeterValuePart(mediathekFilePath, SonosApiParameterBuilder::ENCODE_XML);
        b.EndParameter();
        b.AddParameter("CurrentURIMetaData", nullptr);
    });
    play();
}

void SonosSpeaker::playMusicLibraryDirectory(const char* mediathekDirectory)
{
    removeAllTracksFromQueue();
    String dir = mediathekDirectory;
    if (dir.endsWith("/"))
        dir = dir.substring(0, dir.length() - 1);
    const char* cDir = dir.c_str();

    postAction(renderingAVTransportUrl, renderingAVTransportSoapAction, "AddURIToQueue", [this, cDir](SonosApiParameterBuilder& b) {
        b.BeginParameter("EnqueuedURI");
        b.ParmeterValuePart("x-rincon-playlist:", SonosApiParameterBuilder::ENCODE_NO);
        b.ParmeterValuePart(this->getUID().c_str(), SonosApiParameterBuilder::ENCODE_NO);
        b.ParmeterValuePart("#S:", SonosApiParameterBuilder::ENCODE_NO);
        b.ParmeterValuePart(cDir, SonosApiParameterBuilder::ENCODE_NO);
        b.EndParameter();
        b.AddParameter("EnqueuedURIMetaData");
        b.AddParameter("DesiredFirstTrackNumberEnqueued", (int32_t) 0);
        b.AddParameter("EnqueueAsNext", true);
    });
    playQueue();
}



void SonosSpeaker::playLineIn()
{
    setAVTransportURI(SonosApi::SchemaLineIn, getUID().c_str());
    play();
}

void SonosSpeaker::playTVIn()
{
    String url = getUID();
    url += SonosApi::UrlPostfixTVIn;
    setAVTransportURI(SonosApi::SchemaTVIn, url.c_str());
    play();
}

void SonosSpeaker::playQueue()
{
    String url = getUID() + "#0";
    setAVTransportURI("x-rincon-queue:", url.c_str());
    play();
}

const char* contentDirectoryUrl PROGMEM = "/MediaServer/ContentDirectory/Control";
const char* contentDirectorySoapAction PROGMEM = "urn:schemas-upnp-org:service:ContentDirectory:1";

const char* SonosSpeaker::xPathOnString(MicroXPath_P& xPath, const char* bufferToSearch, PGM_P* path, uint8_t pathSize, char* resultBuffer, size_t resultBufferSize, bool xmlDecode)
{
    const char* xmlEncodeStart = nullptr;
    char c;
    while ((c = *bufferToSearch) != 0)
    {
        bufferToSearch++;
        if (xmlDecode)
        {
            if (c == '&')
            {
                // XML Encode start
                xmlEncodeStart = bufferToSearch;
                continue;
            }
            else if (xmlEncodeStart != nullptr)
            {
                if (c != ';')
                    continue;
                // XML Encode end
                if (strncmp(xmlEncodeStart, "lt", 2) == 0)
                    c = '<';
                else if (strncmp(xmlEncodeStart, "gt", 2) == 0)
                    c = '>';
                else if (strncmp(xmlEncodeStart, "amp", 3) == 0)
                    c = '&';
                else if (strncmp(xmlEncodeStart, "quot", 4) == 0)
                    c = '"';
                else if (strncmp(xmlEncodeStart, "apos", 4) == 0)
                    c = '\'';
                else
                    continue; // Invalid encode sequence
                xmlEncodeStart = nullptr;
            }
        }
        if (xPath.getValue(c, resultBuffer, resultBufferSize))
            return bufferToSearch;
    }
    return bufferToSearch;
}

const SonosApiBrowseResult SonosSpeaker::browse(const char* objectId, uint32_t index, uint32_t* totalNumberOfItems)
{
    SonosApiBrowseResult browseResult;
    postAction(
        contentDirectoryUrl, contentDirectorySoapAction, "Browse", [index, objectId, totalNumberOfItems](SonosApiParameterBuilder& b) { 
            b.AddParameter("ObjectID", objectId); 
            b.AddParameter("BrowseFlag", "BrowseDirectChildren"); 
            b.AddParameter("Filter", "*"); 
            b.AddParameter("StartingIndex", (int32_t) index); 
            b.AddParameter("RequestedCount", "1"); 
            b.AddParameter("SortCriteria", "+dc:title"); },
        [=, &browseResult](WiFiClient& wifiClient) {
            MicroXPath_P xPath;
            PGM_P path[] = {p_SoapEnvelope, p_SoapBody, "u:BrowseResponse", "Result"};
            const static size_t bufferSize = 3000;
            auto resultBuffer = new char[bufferSize];
            try
            {
                xPathOnWifiClient(xPath, wifiClient, path, 4, resultBuffer, bufferSize);

                MicroXPath_P xPathMetadata;
                const char* metadata = resultBuffer;
                PGM_P pathTitle[] = {"DIDL-Lite", "container", "dc:title"};
                xPathMetadata.setPath(pathTitle, 3);
                metadata = xPathOnString(xPathMetadata, metadata, pathTitle, 3, resultBuffer, bufferSize, true);
                browseResult.title = resultBuffer;

                PGM_P pathRes[] = {"DIDL-Lite", "container", "res"};
                xPathMetadata.setPath(pathRes, 3);
                metadata = xPathOnString(xPathMetadata, metadata, pathTitle, 3, resultBuffer, bufferSize, true);
                browseResult.uri = resultBuffer;

                PGM_P path3[] = {p_SoapEnvelope, p_SoapBody, "u:BrowseResponse", "TotalMatches"};
                xPathOnWifiClient(xPath, wifiClient, path3, 4, resultBuffer, bufferSize);
                *totalNumberOfItems = (uint32_t)atoi(resultBuffer);             
            }
            catch (...)
            {
                delete resultBuffer;
                throw;
            }
            delete resultBuffer;
        });
    return browseResult;
}

const SonosApiBrowseResult SonosSpeaker::search(const char* objectId, const char* titleToSearch)
{
    // get first entry to get the total number of items
    uint32_t searchIndex = 0;
    uint32_t totalNumberOfItems;
    auto result = browse(objectId, searchIndex, &totalNumberOfItems);
    auto cmp = strcasecmp(result.title.c_str(), titleToSearch);
    if (cmp == 0)
    {
        // coincidentally found
        return result;
    }
    if (totalNumberOfItems > 1)
    {
        // Try to find with binary search,
        // but this is not 100% safe because the sort order of sons is base on unicode while strmp uses a simple byte compare
        int32_t lowerLimit = 1;
        int32_t upperLimit = totalNumberOfItems - 1;
        std::unordered_set<uint32_t> checkedindex;
        while (lowerLimit <= upperLimit)
        {
            auto searchIndex = (lowerLimit + upperLimit) / 2;
            result = browse(objectId, searchIndex, &totalNumberOfItems);
            Serial.println("***");
             Serial.println(searchIndex);
            Serial.println(totalNumberOfItems);
            cmp = strcasecmp(result.title.c_str(), titleToSearch);
            if (cmp == 0)
            {
                return result;
            }
            else if (cmp < 0)
                lowerLimit = searchIndex + 1;
            else
                upperLimit = searchIndex - 1;
            checkedindex.insert(searchIndex);
        }
        // Because binary search usage is not safe, try it again with linear search
        uint32_t startValue = 1; // 0 index already checked
        int32_t excludedEndValue = totalNumberOfItems;
        int8_t loopOffset = 1;
        if (strncasecmp(titleToSearch, "m", 1) > 0)
        {
            // Start search from end
            startValue = totalNumberOfItems - 1;
            excludedEndValue = 0;  // 0 is already checked
            loopOffset = -1;
        }
        for (auto searchIndex = startValue; searchIndex != excludedEndValue; searchIndex += loopOffset)
        {
            if (checkedindex.find(searchIndex) != checkedindex.end())
                continue;
            auto result = browse(objectId, searchIndex, &totalNumberOfItems);
            cmp = strcasecmp(result.title.c_str(), titleToSearch);
            if (cmp == 0)
            {
                return result;
            }
        }
    }      
    return SonosApiBrowseResult();
}

void SonosSpeaker::playSonosPlaylist(const char* playListTitle)
{
    auto result = search("SQ:", playListTitle);
    if (result.uri.length() != 0)
    {
        removeAllTracksFromQueue();
        addTrackToQueue(nullptr, result.uri.c_str());
        playQueue();
    }
}

#ifdef ARDUINO_ARCH_ESP32   
void SonosSpeaker::playNotification(const char* url, uint8_t volume)
{
    if (_playNotification != nullptr)
    {
        auto temp = _playNotification;
        _playNotification = nullptr;
        delete temp;
    }
    _playNotification = new SonosApiPlayNotification(_speakerIP, url, volume, getUID());
}
#endif