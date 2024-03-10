#pragma once
#include "Arduino.h"

class SonosApiParameterBuilder
{
  private:
    static const char cXML[];

    static const char quote[];
    static const char apostrophe[];
    static const char lessthen[];
    static const char greaterthen[];
    static const char ampersand[];

    static const char* const expandedXML[];

    static const char quote2[];
    static const char apostrophe2[];
    static const char lessthen2[];
    static const char greaterthen2[];
    static const char ampersand2[];

    static const char* const doubleExpandedXML[];

    static const char cURL[];
  public:
    static const int ENCODE_NO = 0;
    static const int ENCODE_XML = 1;
    static const int ENCODE_DOUBLE_XML = 2;
    static const int ENCODE_URL = 3;
  private:
    Stream* _stream;
    size_t _length = 0;
    const char* currentParameterName = nullptr;
    size_t writeEncoded(const char* buffer, byte escapeMode);
    SonosApiParameterBuilder(SonosApiParameterBuilder& b) {} // Prevent copy constructor usage
  public:
    size_t length();
    SonosApiParameterBuilder(Stream* stream);
    void AddParameter(const char* name, const char* value = nullptr, byte escapeMode = ENCODE_XML);
    void AddParameter(const char* name, int32_t value);
    void BeginParameter(const char* name);
    void ParmeterValuePart(const char* valuePart = nullptr, byte escapeMode = ENCODE_XML);
    void EndParameter();
};