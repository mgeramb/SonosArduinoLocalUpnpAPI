#include "SonosApiParameterBuilder.h"

//  Original Source of the escape function
//    FILE: XMLWriter.cpp
//  AUTHOR: Rob Tillaart
// VERSION: 0.3.5
//    DATE: 2013-11-06
// PURPOSE: Arduino library for creating XML

const char SonosApiParameterBuilder::cXML[6] = "\"\'<>&";

const char SonosApiParameterBuilder::quote[] PROGMEM = "&quot;";
const char SonosApiParameterBuilder::apostrophe[] PROGMEM = "&apos;";
const char SonosApiParameterBuilder::lessthen[] PROGMEM = "&lt;";
const char SonosApiParameterBuilder::greaterthen[] PROGMEM = "&gt;";
const char SonosApiParameterBuilder::ampersand[] PROGMEM = "&amp;";

const char* const SonosApiParameterBuilder::expandedXML[] PROGMEM =
    {
        quote, apostrophe, lessthen, greaterthen, ampersand};

const char SonosApiParameterBuilder::quote2[] PROGMEM = "&amp;quot;";
const char SonosApiParameterBuilder::apostrophe2[] PROGMEM = "&amp;apos;";
const char SonosApiParameterBuilder::lessthen2[] PROGMEM = "&amp;lt;";
const char SonosApiParameterBuilder::greaterthen2[] PROGMEM = "&amp;gt;";
const char SonosApiParameterBuilder::ampersand2[] PROGMEM = "&amp;amp;";

const char* const SonosApiParameterBuilder::doubleExpandedXML[] PROGMEM =
    {
        quote2, apostrophe2, lessthen2, greaterthen2, ampersand2};

const char SonosApiParameterBuilder::cURL[] PROGMEM = " !\"#$%&'()*+,-./:;<=>?@";

size_t SonosApiParameterBuilder::writeEncoded(const char* str, byte mode)
{
    if (str == nullptr)
        return 0;
    const char* c;
    const char* const* expanded = nullptr;
    switch (mode)
    {
        case ENCODE_XML:
            c = cXML;
            expanded = expandedXML;
            break;
        case ENCODE_DOUBLE_XML:
            c = cXML;
            expanded = doubleExpandedXML;
            break;
        case ENCODE_URL:
            c = cURL;
            break;
        default:
            if (_stream == nullptr)
                return strlen(str);
            else
                return _stream->write(str);
    }
    size_t length = 0;
    char* p = (char*)str;
    while (*p != 0)
    {
        char* q = strchr(c, *p);
        if (q == NULL)
        {
            length++;
            if (_stream != nullptr)
                _stream->write(p, 1);
        }
        else
        {
            char buf[11];
            if (mode == ENCODE_URL)
            {
                sprintf(buf, "%%%02X", (unsigned int)*p);
            }
            else
            {
                strcpy(buf, (expanded[q - c]));
            }
            auto len = strlen(buf);
            length += len;
            if (_stream != nullptr)
                _stream->write(buf, len);
        }
        p++;
    }
    return length;
}

SonosApiParameterBuilder::SonosApiParameterBuilder(Stream* stream)
    : _stream(stream)
{
}

void SonosApiParameterBuilder::AddParameter(const char* name, int32_t value)
{
    char buffer[12];
    itoa(value, buffer, 10);
    AddParameter(name, buffer, ENCODE_NO);
}

void SonosApiParameterBuilder::AddParameter(const char* name, const char* value, byte escapeMode)
{
    if (_stream == nullptr)
    {
        _length += strlen(name) * 2 + 5;
        _length += writeEncoded(value, escapeMode);
    }
    else
    {
        _length += _stream->write('<');
        _length += _stream->write(name);
        _length += _stream->write('>');
        _length += writeEncoded(value, escapeMode);
        _stream->write("</");
        _stream->write(name);
        _stream->write('>');
    }
}

void SonosApiParameterBuilder::BeginParameter(const char* name)
{
    currentParameterName = name;
    if (_stream == nullptr)
    {
        _length += strlen(name) + 2;
    }
    else
    {
        _stream->write('<');
        _stream->write(name);
        _stream->write('>');
    }
}
void SonosApiParameterBuilder::ParmeterValuePart(const char* valuePart, byte escapeMode)
{
    _length += writeEncoded(valuePart, escapeMode);
}

void SonosApiParameterBuilder::EndParameter()
{
    if (_stream == nullptr)
    {
        _length += strlen(currentParameterName) + 3;
    }
    else
    {
        _length += _stream->write("</");
        _length += _stream->write(currentParameterName);
        _length += _stream->write('>');
    }
    currentParameterName = nullptr;
}

size_t SonosApiParameterBuilder::length()
{
    return _length;
}