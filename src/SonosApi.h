#pragma once

#include "vector"
#include "WiFiClient.h"
#include "WiFiClientSecure.h"
#include "WiFi.h"
#ifndef DISABLE_CALLBACK
#include "ESPAsyncWebServer.h"
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
#ifdef USE_ESP_ASNC_WEB_SERVER
    AsyncWebServer* _webServer = nullptr;
#endif
    uint16_t _port = 28124;
public:
#ifdef USE_ESP_ASNC_WEB_SERVER
    // Must be called before addSpeaker
    void setWebServer(AsyncWebServer* webServer, uint16_t port);
#endif
    void setDebugSerial(Stream* debugSerial);
    void loop();
    SonosSpeaker* addSpeaker(IPAddress ipAddress); 
};
