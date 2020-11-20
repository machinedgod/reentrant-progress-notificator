#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <signal.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <librsvg/rsvg.h>

#include "config.h"

void print_usage(void);
void event_received(void);
void timer_fired(cairo_t *cr, RsvgHandle *pic);
int spawn_new_window();
int modify_existing(pid_t pid, char modf);
int test_process_is_correct();

static long val = 0;
static long mval = 1;
static char *wm_name  = "NotificationBox";
static char *wm_class = "notification-box";

int main(int argc, char** argv) {
	int retcode = 0;
	if(argc == 3) { // This is to protect from cases like: notification-box + 5 <pid>
        if(argv[1][0] == '+' || argv[1][0] == '-') {
    		pid_t pid = strtol(argv[2], NULL, 10);
    		return modify_existing(pid, argv[1][0]);
        } else {
    		mval = strtol(argv[1], NULL, 10);
    		val = strtol(argv[2], NULL, 10);
    		return spawn_new_window();
        }
	       
	} else {
	   print_usage();
	   return 0;
	}
}

void print_usage(void) {
	printf("Usage:\n");
	printf("\tnotification-box <mod-val> <init-val>\n");
	printf("\tnotification-box (+|-) <pid>\n");
}

int modify_existing(pid_t pid, char modf) {
    if(!test_process_is_correct(pid)) {
        fprintf(stderr, "PID doesn't refer to notification-box process!\n");
        return 1;
    }

    switch(modf) {
    case '+':
        kill(pid, SIGUSR1);
        break;
    case '-':
        kill(pid, SIGUSR2);
        break;
    }

    return 0;
}

static void get_process_name_by_pid(pid_t pid, char* buf) {
    //char* buf = (char*)calloc(1024,sizeof(char));
    sprintf(buf, "/proc/%d/cmdline", pid);
    FILE* f = fopen(buf,"r");
    if(f) {
        size_t size;
        size = fread(buf, sizeof(char), 1024, f);
        if(size > 0){
            if('\n' == buf[size-1]) {
                buf[size-1]='\0';
            }
        }
        fclose(f);
    }
}

int test_process_is_correct(pid_t pid) {
    char buf[1024] = "";
    get_process_name_by_pid(pid, buf);
    printf("Name: %s\n", buf);
    if(strcmp(buf, "./notification-box") == 0 || strcmp(buf, "notification-box"))
        return 1;
    else 
        return 0;
}


void sig_handler(int sig) {
	switch(sig) {
		case SIGUSR1:
			printf("Increase...\n");
			val += mval;
			break;
		case SIGUSR2:
			printf("Decrease...\n");
			val -= mval;
			break;
	}
}

int spawn_new_window() {
    //printf("Modval: %d, Initval: %d\n", mval, val);

    // Signal traps
	if (signal(SIGUSR1, sig_handler) == SIG_ERR) {
		 printf("\ncan't catch SIGUSR1\n");
	}
	if (signal(SIGUSR2, sig_handler) == SIG_ERR) {
		 printf("\ncan't catch SIGUSR2\n");
	}

    // Display
	Display *dsp = XOpenDisplay(NULL);
	if (!dsp) {
		return 1;
	}

    // Window
	int screen = DefaultScreen(dsp);
	unsigned long white = WhitePixel(dsp, screen);
	unsigned long black = BlackPixel(dsp, screen);
	Window root = DefaultRootWindow(dsp);
	Window win = XCreateSimpleWindow(dsp, root,
									 50, 50,
									 200, 200,
									 0, black,
									 white);

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
	do {
	   XNextEvent(dsp, &evt); // calls XFlush
	} while (evt.type != MapNotify);


    // Create Cairo context
    cairo_surface_t *sfc = cairo_xlib_surface_create(dsp, win, DefaultVisual(dsp, screen), 256, 256);
    cairo_xlib_surface_set_size(sfc, 256, 256);
    cairo_t *cr = cairo_create(sfc);

    // Cairo test
    /*
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    cairo_move_to(cr, 20, 20);
    cairo_line_to(cr, 200, 400);
    cairo_line_to(cr, 450, 100);
    cairo_line_to(cr, 20, 20);
    cairo_set_source_rgb(cr, 0, 0, 1);
    cairo_fill_preserve(cr);
    cairo_set_line_width(cr, 5);
    cairo_set_source_rgb(cr, 1, 1, 0);
    cairo_stroke(cr);
    */

	// Draw the image
	GError *gerr = NULL;
    RsvgHandle *pic =rsvg_handle_new_from_file(speaker_image_path, &gerr);
    rsvg_handle_set_dpi (pic, 10);

	// Nonblocking event loop
	// This returns the FD of the X11 display (or something like that)
	int x11_fd = ConnectionNumber(dsp);
	fd_set in_fds;
	struct timeval tv;
	while(1) {
		// Create a File Description Set containing x11_fd
		FD_ZERO(&in_fds);
		FD_SET(x11_fd, &in_fds);

		// Set our timer.  One second sounds good.
		tv.tv_usec = 10000;
		tv.tv_sec = 0;

		// Wait for X Event or a Timer
		int num_ready_fds = select(x11_fd + 1, &in_fds, NULL, NULL, &tv);
		if (num_ready_fds > 0)
			event_received();
		else if (num_ready_fds == 0)
			timer_fired(cr, pic);

		// Handle XEvents and flush the input 
		while(XPending(dsp))
			XNextEvent(dsp, &evt);
	}

	// Bail out
	XDestroyWindow(dsp, win);
	XCloseDisplay(dsp);

	return 0;
}

void event_received(void) {
	//printf("Event Received!\n");
}

void timer_fired(cairo_t *cr, RsvgHandle *pic) {
	printf("Timer, val: %d\n", val);
    rsvg_handle_render_cairo (pic, cr);
}
