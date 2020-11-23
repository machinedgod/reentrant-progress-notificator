#ifndef _CONFIG_H_
#define _CONFIG_H_

// Should test for process name when executed with a PID
const int test_for_process_name = 1;

// Window width
const int width = 256;
// Window height
const int height = 256;

// Value bounds
const long min_value = 0;
const long max_value = 100;

// How long will the window stay up (ms).
const long startlife = 1000;

// Window background color
const unsigned long background_color = 0x70707070;

// Viewport for picture
const double viewport_x = 10;
const double viewport_y = 5;
const double viewport_width = width - 10;
const double viewport_height = height - 40;

// Progressbar position
const int progressbar_bar_width = 6;
const int progressbar_bar_height = 15;
const int progressbar_bar_margin = 4;
const int progressbar_y = 230;


// Image used when no headphones are connected
gchar *speaker_image_path =
    "/usr/share/icons/breeze-dark/devices/64/audio-headphones.svg";

// Image used when headphones _are_ connected
// TODO make this actually useful
//gchar *headphones_image_path =
//    "/usr/share/icons/breeze-dark/devices/64/audio-headphones.svg";


#endif // _CONFIG_H_
