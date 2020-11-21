#ifndef _CONFIG_H_
#define _CONFIG_H_

// Image used when no headphones are connected
gchar *speaker_image_path =
    "/usr/share/icons/breeze-dark/devices/64/audio-headphones.svg";

// Image used when headphones _are_ connected
// TODO make this actually useful
gchar *headphones_image_path =
    "/usr/share/icons/breeze-dark/devices/64/audio-headphones.svg";

// How long will the window stay up ms.
static const long startlife = 
    1000;

#endif // _CONFIG_H_
