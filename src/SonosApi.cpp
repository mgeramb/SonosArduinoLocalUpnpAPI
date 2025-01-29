#include "SonosApi.h"


const char SonosApi::DefaultSchemaInternetRadio[] PROGMEM = "x-rincon-mp3radio://";
const char SonosApi::SchemaMusicLibraryFile[] PROGMEM = "x-file-cifs:";
const char SonosApi::SchemaLineIn[] PROGMEM = "x-rincon-stream:";
const char SonosApi::SchemaTVIn[] PROGMEM = "x-sonos-htastream:";
const char SonosApi::UrlPostfixTVIn[] PROGMEM = ":spdif";
const char SonosApi::SchemaMaster[] PROGMEM = "x-rincon:";

#ifndef SONOS_DISABLE_CALLBACK
#if SONOS_USE_ESP_ASNC_WEB_SERVER
void SonosApi::setWebServer(AsyncWebServer* webServer, uint16_t port)
{
    _port = port;
    _webServer = webServer;
}
#else
void SonosApi::setWebServer(httpd_handle_t webServer, uint16_t port)
{
    _port = port;
    _webServer = webServer;
}
#endif
#endif

void SonosApi::setDebugSerial(Stream* debugSerial)
{
    _debugSerial = debugSerial;
}
void SonosApi::loop()
{
    bool connected = _lanNetworkConnected || WiFi.status() == WL_CONNECTED;
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
#ifndef SONOS_DISABLE_CALLBACK
#if SONOS_USE_ESP_ASNC_WEB_SERVER
    if (_webServer == nullptr)
    {
        _webServer = new AsyncWebServer(_port);
        _webServer->begin();
    }
    auto speaker = new SonosSpeaker(*this, ipAddress, _webServer);
#else
    if (_webServer == nullptr)
    {
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.server_port = _port;
        httpd_start(&_webServer, &config);
    }
    auto speaker = new SonosSpeaker(*this, ipAddress, _webServer);
#endif
#else
    auto speaker = new SonosSpeaker(*this, ipAddress);
#endif
    return speaker;
}



