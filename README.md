# ArduinoSonosLocalUpnpAPI
Sonos-ESP inspired successor library for controlling Sonos speakers over the unofficial local UPnP API.

## Warning
This library uses an unofficial API. 
The API can be shutdown by Sonos without any announcment, which would cause the library stop working.

## Features

### Play sources

- MP3 from http url
- Http radion stream
- File from media library
- Directory from media library
- Sonos playlist
- TV
- Line-In

### Sound overlay

- Play notification sound mixed over the current music without stopping them (audio clip)

### Sound control

- Volume
- Relative volume
- Treble
- Bass
- Loudness
- Group volume
- Relative group volume
- Mute

### Play control

- Play
- Pause
- Stop
- Next
- Previous
- Shuffle (random)
- Repeat one
- Repead all
- Goto Track

### Group control

- Join group
- Leave group
- Find and join next currently activ playing group

### Play state

- Track number
- Track url
- Track metadata

### Notification callbacks

- Volume change
- Treble change 
- Bass change
- Mute change
- Loudness change
- Group volume change
- Group mute change
- Play state change
- Play mode change
- Track change
- Group coordinator change

### Dependencies

This library used the following libs:

- ESP Async WebServer (for notification callback)
- ArduinoWebsockets (for playing sound overlay)
- ArduinoHttpClient (for sonos api calls)
- MicroXPath (for XML parsing)

# Using the library

``` C++

SonosApi sonosApi;
SonosSpeaker speakerLivingroom;
SonosSpeaker speakerKitchen;

void setup()
{
    // Setup WIFI
    (...)

    // declare all sonsos (For stereo speaker pairs, add only one)
    speakerLivingroom = sonosApi.addSpeaker("192.168.0.101");
    speakerKitchen = sonosApi.addSpeaker("192.168.0.102");

    // Start playing radion in living room as standalone group controller
    speakerLivingroom->playInternetRadio(
        "https://orf-live.ors-shoutcast.at/wie-q2a.m3u", // station audio stream url
        "Radio Wien", // station name
        "https://cdn-profiles.tunein.com/s44255/images/logod.jpg"); // station logo image url
    
    // joing group living room
    speakerKitchen->joinToGroupCoordinator(speakerLivingroom); 

    // change the volume of the group
    speakerLivingroom->setGroupVolume(10); 
}

void loop()
{
    sonosApi.loop();
}

```


# Licence

This library is [MIT license](LICENSE).

# Credits

This library is base on the following libraries and documentations:

[tmittet/sonos](https://github.com/tmittet/sonos)

[javos65/Sonos-ESP32](https://github.com/javos65/Sonos-ESP32)

[Unofficial Sonos Docs](https://sonos.svrooij.io)

[KilianB/Java-Sonos-Controller](https://github.com/KilianB/Java-Sonos-Controller)

[tmittet/microxpath](https://github.com/tmittet/microxpath)
