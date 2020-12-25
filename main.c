#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <signal.h>
#include <getopt.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <librsvg/rsvg.h>

// Config
struct
{
	int test_for_process_name;

	// Window geometry
	int x;
	int y;
	int width;
	int height;

	// Value bounds
	long min_value;
	long max_value;

	// How long will the window stay up (ms).
	long startlife;

	// Window background color
	unsigned long background_color;

	// Viewport for picture
	double viewport_x;
	double viewport_y;
	double viewport_width;
	double viewport_height;

	// Progressbar position
	int progressbar_bar_width;
	int progressbar_bar_height;
	int progressbar_bar_margin;
	int progressbar_y;

	// Image used when no headphones are connected
	char *speaker_image_path;

	// Image used when headphones _are_ connected
	// TODO make this actually useful
	//gchar *headphones_image_path =
} app_config =
	{ .test_for_process_name = 0
	, .x = 10
	, .y = 10
	, .width = 256
	, .height = 256
	, .min_value = 0
	, .max_value = 100
	, .startlife = 1000
	, .background_color = 0x70707070
	, .viewport_x = 10
	, .viewport_y = 5
	, .viewport_width = 256 - 10
	, .viewport_height = 256 - 40
	, .progressbar_bar_width = 6
	, .progressbar_bar_height = 15
	, .progressbar_bar_margin = 4
	, .progressbar_y = 230
	, .speaker_image_path = "/usr/share/icons/breeze-dark/devices/64/audio-headphones.svg"
};

char app_short_options[] = "htl:i:";

struct option app_options[] =
	{ { "help"                  , no_argument      , NULL, 0 }
	, { "test-for-process-name" , no_argument      , NULL, 1 }
	, { "x"                     , required_argument, NULL, 2 }
	, { "y"                     , required_argument, NULL, 3 }
	, { "width"                 , required_argument, NULL, 4 }
	, { "height"                , required_argument, NULL, 5 }
	, { "min-value"             , required_argument, NULL, 6 }
	, { "max-value"             , required_argument, NULL, 7 }
	, { "startlife"             , required_argument, NULL, 8 }
	, { "background-color"      , required_argument, NULL, 9 }
	, { "viewport-x"            , required_argument, NULL, 10 }
	, { "viewport-y"            , required_argument, NULL, 11 }
	, { "viewport-width"        , required_argument, NULL, 12 }
	, { "viewport-height"       , required_argument, NULL, 13 }
	, { "progressbar-bar-width" , required_argument, NULL, 14 }
	, { "progressbar-bar-height", required_argument, NULL, 15 }
	, { "progressbar-bar-margin", required_argument, NULL, 16 }
	, { "progressbar-y"         , required_argument, NULL, 17 }
	, { "speaker-image-path"    , required_argument, NULL, 18 }
	, { 0, 0, 0, 0 }
	};


static char *wm_name  = "ReentrantProgressNotificator";
static char *wm_class = "reentrant-progress-notificator";

static long val  = 0;
static long mval = 1;
static long life = 1000;

static long is_first_instance = 1;
static char repeated_op = 0;

void print_usage(void);
int test_process_is_correct(pid_t pid);
int modify_existing(pid_t pid, char modf);
int spawn_new_window(void);
void draw_progress(cairo_t *cr);


static void
process_options(int opt)
{
	switch(opt)
	{
	case 'h': /* fallthrough */
	case 0: /* help */
		print_usage();
		exit(0);
	case 't': /* fallthrough */
	case 1: /* test_for_process_name */
		app_config.test_for_process_name = 1;
		break;
	case 2: /* x */
		app_config.x = strtol(optarg, NULL, 10);
		break;
	case 3: /* y */
		app_config.y = strtol(optarg, NULL, 10);
		break;
	case 4: /* width */
		app_config.width = strtol(optarg, NULL, 10);
		break;
	case 5: /* height */
		app_config.height = strtol(optarg, NULL, 10);
		break;
	case 6: /* min_value */
		app_config.min_value = strtol(optarg, NULL, 10);
		break;
	case 7: /* max_value */
		app_config.max_value = strtol(optarg, NULL, 10);
		break;
	case 'l': /* fallthrough */
	case 8: /* startlife */
		app_config.startlife = strtol(optarg, NULL, 10);
		life = app_config.startlife;
		break;
	case 9: /* background_color */
		app_config.background_color = strtol(optarg, NULL, 16);
		break;
	case 10: /* viewport_x */
		app_config.viewport_x = strtol(optarg, NULL, 10);
		break;
	case 11: /* viewport_y */
		app_config.viewport_y = strtol(optarg, NULL, 10);
		break;
	case 12: /* viewport_width */
		app_config.viewport_width = strtol(optarg, NULL, 10);
		break;
	case 13: /* viewport_height */
		app_config.viewport_height = strtol(optarg, NULL, 10);
		break;
	case 14: /* progressbar_bar_width */
		app_config.progressbar_bar_width = strtol(optarg, NULL, 10);
		break;
	case 15: /* progressbar_bar_height */
		app_config.progressbar_bar_height = strtol(optarg, NULL, 10);
		break;
	case 16: /* progressbar_bar_margin */
		app_config.progressbar_bar_margin = strtol(optarg, NULL, 10);
		break;
	case 17: /* progressbar_y */
		app_config.progressbar_y = strtol(optarg, NULL, 10);
		break;
	case 'i': /* fallthrough */
	case 18: { /* speaker_image_path */
#define FILENAME_BUFFER_SIZE 1024
		static char image_filename_buffer[FILENAME_BUFFER_SIZE];
		memset(image_filename_buffer, 0, FILENAME_BUFFER_SIZE);
		strncpy(image_filename_buffer, optarg, strlen(optarg));
		
		app_config.speaker_image_path = image_filename_buffer;
#undef FILENAME_BUFFER_SIZE
		break;
		}
	}
}

static long
bounded(long i)
{
    return fmaxl(app_config.min_value, fminl(app_config.max_value, i));
}

static void
get_process_name_by_pid(pid_t pid, char* buf, size_t buffsize)
{
    sprintf(buf, "/proc/%d/cmdline", pid);
    FILE* f = fopen(buf,"r");
    if(f)
    {
        size_t size;
        size = fread(buf, sizeof(char), buffsize, f);
        if(size > 0)
        {
            if('\n' == buf[size-1])
            {
                buf[size-1]='\0';
            }
        }
        fclose(f);
    }
}

static void
sig_handler(int sig)
{
    switch(sig)
    {
        case SIGUSR1:
            val = bounded(val + mval);
            life = app_config.startlife;
            break;
        case SIGUSR2:
            val = bounded(val - mval);
            life = app_config.startlife;
            break;
    }
}

void
print_usage(void)
{
    printf("Usage:\n");
    printf("\treentrant-progress-notificator <options> <mod-val> <init-val>\n");
    printf("\treentrant-progress-notificator (+|-) <pid>\n");
    printf("\n");
    printf("Options:\n");
	printf("\t-h, --help\n");
	printf("\t\tShows this help.\n");
	printf("\t-t, --test-for-process-name\n");
	printf("\t\tWhen second instance is invoked with <pid>, it tests whether process name is the same. Default = 0.\n");
	printf("\t--width\n");
	printf("\t\tWindow width. Default = 256.\n");
	printf("\t--height\n");
	printf("\t\tWindow height. Default = 256.\n");
	printf("\t--min-value\n");
	printf("\t\tMinimum gauge value. Default = 0.\n");
	printf("\t--max-value\n");
	printf("\t\tMaximum gauge value. Default = 100.\n");
	printf("\t-l, --startlife\n");
	printf("\t\tHow long, in ms, will the dialog stay up. Default = 1000.\n");
	printf("\t--background-color\n");
	printf("\t\tBackground color for the dialog in hex. Transparency is supported. Default = 0x70707070.\n");
	printf("\t--viewport-x\n");
	printf("\t\tX position for the image. Default = 10.\n");
	printf("\t--viewport-y\n");
	printf("\t\tY position for the image. Default = 5.\n");
	printf("\t--viewport-width\n");
	printf("\t\tMake image of this width. Default = 246.\n");
	printf("\t--viewport-height\n");
	printf("\t\tMake image of this height. Default = 216.\n");
	printf("\t--progressbar-bar-width\n");
	printf("\t\tWidth of a single bar in the progress bar. Default = 6.\n");
	printf("\t--progressbar-bar-height\n");
	printf("\t\tHeight of a single bar in the progress bar. Default = 15.\n");
	printf("\t--progressbar-bar-margin\n");
	printf("\t\tMargins between bars in the progress bar. Default = 4.\n");
	printf("\t--progressbar-y\n");
	printf("\t\tY position for the progress bar. Default = 230.\n");
	printf("\t-i, --speaker-image-path\n");
	printf("\t\tImage to be displayed. Default = \"/usr/share/icons/breeze-dark/devices/64/audio-headphones.svg\"\n");
}

int
test_process_is_correct(pid_t pid)
{
    char buf[256] = "";
    get_process_name_by_pid(pid, buf, 256);
    printf("Name: %s\n", buf);
    if(strcmp(buf, "./reentrant-progress-notificator") == 0 || strcmp(buf, "reentrant-progress-notificator") == 0)
        return 1;
    else
        return 0;
}

int
modify_existing(pid_t pid, char modf)
{
    if(app_config.test_for_process_name)
    {
        if(!test_process_is_correct(pid))
        {
            fprintf(stderr, "PID doesn't refer to reentrant-progress-notificator process!\n");
            return 1;
        }
    }

    switch(modf)
    {
    case '+':
        kill(pid, SIGUSR1);
        break;
    case '-':
        kill(pid, SIGUSR2);
        break;
    }

    return 0;
}


int
spawn_new_window()
{
    // Signal traps
    if (signal(SIGUSR1, sig_handler) == SIG_ERR)
    {
         printf("\ncan't catch SIGUSR1\n");
    }
    if (signal(SIGUSR2, sig_handler) == SIG_ERR)
    {
         printf("\ncan't catch SIGUSR2\n");
    }

    // Display
    Display *dsp = XOpenDisplay(NULL);
    if (!dsp)
    {
        return 1;
    }


    // Window
    int screen = DefaultScreen(dsp);
    unsigned long white = WhitePixel(dsp, screen);
    unsigned long black = BlackPixel(dsp, screen);
    Window root = DefaultRootWindow(dsp);

    XVisualInfo vinfo;
    XMatchVisualInfo(dsp, screen, 32, TrueColor, &vinfo);

    XSetWindowAttributes attr;
    attr.colormap = XCreateColormap(dsp, root, vinfo.visual, AllocNone);
    attr.border_pixel = 0;
    attr.background_pixel = app_config.background_color;
    attr.override_redirect = 1;

    Window win = XCreateWindow(dsp, root,
                               app_config.x, app_config.y,
                               app_config.width, app_config.height,
                               0, vinfo.depth,
                               InputOutput, vinfo.visual,
                               CWOverrideRedirect | CWColormap | CWBorderPixel | CWBackPixel,
                               &attr);

    Atom wm_delete_window = XInternAtom(dsp, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(dsp, win, &wm_delete_window,                                                                                                 1);


    // Set WM_CLASS
    XClassHint *classhint = XAllocClassHint();
    classhint->res_name  = wm_name;
    classhint->res_class = wm_class;
    XSetClassHint(dsp, win, classhint);


    // Wait until window is mapped
    XMapWindow(dsp, win);

    long eventMask = StructureNotifyMask;
    XSelectInput(dsp, win, eventMask);
    XEvent evt;
    /*
    do
    {
       XNextEvent(dsp, &evt); // calls XFlush
    } while (evt.type != MapNotify);
    */


    // Create Cairo context
    cairo_surface_t *sfc = cairo_xlib_surface_create(dsp, win, vinfo.visual, app_config.width, app_config.height);
    cairo_xlib_surface_set_size(sfc, app_config.width, app_config.height);
    cairo_t *cr = cairo_create(sfc);

    // Get info image
    GError *gerr = NULL;
    RsvgHandle *pic =rsvg_handle_new_from_file(app_config.speaker_image_path, &gerr);
    rsvg_handle_set_dpi (pic, 75);
    RsvgRectangle viewport;
    viewport.x = app_config.viewport_x;
    viewport.y = app_config.viewport_y;
    viewport.width = app_config.viewport_width;
    viewport.height = app_config.viewport_height;

    // Draw image
    rsvg_handle_render_document(pic, cr, &viewport, &gerr);

    // Draw the initial progress
    draw_progress(cr);

    // Get everything displayed
    XFlush(dsp);

    // Nonblocking event loop
    int x11_fd = ConnectionNumber(dsp);
    fd_set in_fds;
    struct timeval tv;
    while(life > 0)
    {
        // Create a File Description Set containing x11_fd
        FD_ZERO(&in_fds);
        FD_SET(x11_fd, &in_fds);

        // One ms timer (probably too much, but whatevs)
        tv.tv_usec = 1000;
        tv.tv_sec = 0;

        // Wait for X Event or a Timer
        int num_ready_fds = select(x11_fd + 1, &in_fds, NULL, NULL, &tv);
        if (num_ready_fds > 0)
        {
            //printf("Event Received!\n");
        } else if (num_ready_fds == 0)
        {
            //printf("Life: %, life, val);
            life--;
            draw_progress(cr);
        }

        // Handle XEvents and flush the input
        while(XPending(dsp))
            XNextEvent(dsp, &evt);
    }

    // Bail out
    XDestroyWindow(dsp, win);
    XCloseDisplay(dsp);

    return 0;
}

void
draw_progress(cairo_t *cr)
{
	// TODO Calc this using app_config settings
    const int maxrectcount = 24;

    // Erase progressbar part
    cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 0.5);
    cairo_rectangle(cr, app_config.progressbar_bar_margin * 2,
	                app_config.progressbar_y - 2,
	                (app_config.progressbar_bar_width + app_config.progressbar_bar_margin) * maxrectcount,
	                app_config.progressbar_bar_height + 4);
    cairo_fill (cr);

    int howmany = ((float) val / (app_config.max_value - app_config.min_value)) * maxrectcount;
    int i;
    for(i = 1; i < howmany + 1; i++)
    {
        cairo_rectangle (cr,
        				 (app_config.progressbar_bar_width + app_config.progressbar_bar_margin) * i,
        				 app_config.progressbar_y,
        				 app_config.progressbar_bar_width,
        				 app_config.progressbar_bar_height);
    }

    cairo_set_source_rgba (cr, 0, 0.3, 1, 0.7); // TODO progressbar color in options
    cairo_fill (cr);
}

int
main(int argc, char** argv)
{
    int retcode = 0;
	int opt = getopt_long(argc, argv, app_short_options, app_options, NULL);
	do
	{
		process_options(opt);
		opt = getopt_long(argc, argv, app_short_options, app_options, NULL);
	} while (opt != -1);


	if(argv[optind] == NULL || argv[optind + 1] == NULL) {
       print_usage();
       exit(0);
	}

	printf("opts: %s - %s\n", argv[optind], argv[optind+1]);

    if(argv[optind][0] == '+' || argv[optind][0] == '-')
    {
        pid_t pid = strtol(argv[optind + 1], NULL, 10);
        return modify_existing(pid, argv[optind][0]);
    } else
    {
        mval = bounded(strtol(argv[optind], NULL, 10));
        val  = bounded(strtol(argv[optind + 1], NULL, 10));
        return spawn_new_window();
    }
}

