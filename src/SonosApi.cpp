#include "SonosApi.h"


const char SonosApi::DefaultSchemaInternetRadio[] PROGMEM = "x-rincon-mp3radio://";
const char SonosApi::SchemaMusicLibraryFile[] PROGMEM = "x-file-cifs:";
const char SonosApi::SchemaLineIn[] PROGMEM = "x-rincon-stream:";
const char SonosApi::SchemaTVIn[] PROGMEM = "x-sonos-htastream:";
const char SonosApi::UrlPostfixTVIn[] PROGMEM = ":spdif";
const char SonosApi::SchemaMaster[] PROGMEM = "x-rincon:";

#ifdef USE_ESP_ASNC_WEB_SERVER
void SonosApi::setWebServer(AsyncWebServer* webServer, uint16_t port)
{
    _port = port;
    _webServer = webServer;
}
#endif

void SonosApi::setDebugSerial(Stream* debugSerial)
{
    _debugSerial = debugSerial;
}
void SonosApi::loop()
{
    bool connected = WiFi.status() == WL_CONNECTED;
    if (!connected)
        return;
    for (auto iter = _allSonosSpeakers.begin(); iter < _allSonosSpeakers.end(); iter++)
    {
        auto sonosSpeaker = *iter;
        sonosSpeaker->loop();
    }
}

SonosSpeaker* SonosApi::addSpeaker(IPAddress ipAddress)
{
#ifdef USE_ESP_ASNC_WEB_SERVER
    if (_webServer == nullptr)
    {
        _webServer = new AsyncWebServer(_port);
        _webServer->begin();
    }
    auto speaker = new SonosSpeaker(*this, ipAddress, _webServer);
#else
    auto speaker = new SonosSpeaker(*this, ipAddress);
#endif
    return speaker;
}



