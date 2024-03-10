#pragma once

#if ARDUINO_ARCH_ESP32
#ifndef USE_ESP_ASNC_WEB_SERVER
#define USE_ESP_ASNC_WEB_SERVER
#endif
#endif

#include "WiFiClient.h"
#include "WiFiClientSecure.h"
#include "WiFi.h"
#ifdef USE_ESP_ASNC_WEB_SERVER
#include <ESPAsyncWebServer.h>
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
#ifdef USE_ESP_ASNC_WEB_SERVER
    AsyncWebServer* _webServer = nullptr;
    uint16_t _port = 28124;
    Stream* _debugSerial = nullptr;
#endif
public:
#ifdef USE_ESP_ASNC_WEB_SERVER
    // Must be called before addSpeaker
    void setWebServer(AsyncWebServer* webServer, uint16_t port);
#endif
    void setDebugSerial(Stream* debugSerial);
    void loop();
    SonosSpeaker* addSpeaker(IPAddress ipAddress); 
};
