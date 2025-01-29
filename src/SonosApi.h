#pragma once
#ifndef SONOS_USE_ESP_ASNC_WEB_SERVER
#define SONOS_USE_ESP_ASNC_WEB_SERVER 0
#endif
#include "vector"
#include "WiFiClient.h"
#include "WiFiClientSecure.h"
#include "WiFi.h"
#ifndef SONOS_DISABLE_CALLBACK
#if SONOS_USE_ESP_ASNC_WEB_SERVER
#include "ESPAsyncWebServer.h"
#else
#include "esp_http_server.h"
#endif
#endif
#include "WebSockets.h"
#include <unordered_set>
#include "SonosApiHelpers.h"
#include "SonosSpeaker.h"

class SonosApi
{
    friend class SonosSpeaker;
public:
      const static char DefaultSchemaInternetRadio[];
      const static char SchemaMaster[];
      const static char SchemaMusicLibraryFile[];
      const static char SchemaLineIn[];
      const static char SchemaTVIn[];
      const static char UrlPostfixTVIn[];

private:
    std::vector<SonosSpeaker*> _allSonosSpeakers;
    Stream* _debugSerial = nullptr;
#ifndef SONOS_DISABLE_CALLBACK
#if SONOS_USE_ESP_ASNC_WEB_SERVER
    AsyncWebServer* _webServer = nullptr;
#else
    httpd_handle_t _webServer = nullptr;
#endif
#endif
    uint16_t _port = 28124;
    bool _lanNetworkConnected = false;
    IPAddress _lanIPAddress = IPAddress();
public:
#ifndef SONOS_DISABLE_CALLBACK
#if SONOS_USE_ESP_ASNC_WEB_SERVER
    // Must be called before addSpeaker
    void setWebServer(AsyncWebServer* webServer, uint16_t port);
#else
    void setWebServer(httpd_handle_t webServer, uint16_t port);
#endif
#endif
    void setLANNetworkConnected(bool connected, IPAddress ipAddress) { _lanNetworkConnected = connected; _lanIPAddress = ipAddress; }
    void setDebugSerial(Stream* debugSerial);
    void loop();
    SonosSpeaker* addSpeaker(IPAddress ipAddress); 
};
