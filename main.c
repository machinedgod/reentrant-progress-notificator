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

static const int width = 256;
static const int height = 256;
static char *wm_name  = "NotificationBox";
static char *wm_class = "notification-box";

static long val  = 0;
static long mval = 1;
static long life = startlife;


void print_usage(void);
void event_received(void);
void timer_fired(cairo_t *cr);
int spawn_new_window(void);
int modify_existing(pid_t pid, char modf);
int test_process_is_correct(pid_t pid);
void clear(cairo_t *cr);
void draw_image(cairo_t *cr, RsvgHandle *pic, RsvgRectangle *vp);
void draw_progress(cairo_t *cr);


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
			life = startlife;
			break;
		case SIGUSR2:
			printf("Decrease...\n");
			val -= mval;
			life = startlife;
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

	XVisualInfo vinfo;
	XMatchVisualInfo(dsp, screen, 32, TrueColor, &vinfo);

	XSetWindowAttributes attr;
	attr.colormap = XCreateColormap(dsp, root, vinfo.visual, AllocNone);
	attr.border_pixel = 0;
	attr.background_pixel = 0x70707070;

	//Window win = XCreateSimpleWindow(dsp, root,
	//								 0, 0,	  // x, y
	//								 width, height,
	//								 0, black,
	//								 white);
	Window win = XCreateWindow(dsp, root, 
							   0, 0,  // x, y
							   width, height,
							   0, vinfo.depth, 
							   InputOutput, vinfo.visual, 
							   CWColormap | CWBorderPixel | CWBackPixel, 
							   &attr);

	Atom wm_delete_window = XInternAtom(dsp, "WM_DELETE_WINDOW", 0);
	XSetWMProtocols(dsp, win, &wm_delete_window, 1);


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
	cairo_surface_t *sfc = cairo_xlib_surface_create(dsp, win, vinfo.visual, width, height);
	cairo_xlib_surface_set_size(sfc, width, height);
	cairo_t *cr = cairo_create(sfc);

	// Get info image
	GError *gerr = NULL;
	RsvgHandle *pic =rsvg_handle_new_from_file(speaker_image_path, &gerr);
	rsvg_handle_set_dpi (pic, 75);
	RsvgRectangle viewport;
	viewport.x = 10;
	viewport.y = 5;
	viewport.width = width - 10;
	viewport.height = height - 40;

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
	while(life > 0) {
		// Create a File Description Set containing x11_fd
		FD_ZERO(&in_fds);
		FD_SET(x11_fd, &in_fds);

		// Set our timer.  One second sounds good.
		tv.tv_usec = 1000;
		tv.tv_sec = 0;

		// Wait for X Event or a Timer
		int num_ready_fds = select(x11_fd + 1, &in_fds, NULL, NULL, &tv);
		if (num_ready_fds > 0)
			event_received();
		else if (num_ready_fds == 0)
			timer_fired(cr);

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

void timer_fired(cairo_t *cr) {
	printf("Life: %d, val: %d\n", life, val);
	life--;

	draw_progress(cr);
}

void draw_progress(cairo_t *cr) {
    const int y = 230;
    const int w = 6;
	const int m = 4;
	const int h = 15;
	const int maxrectcount = 24;

    // Erase progressbar part
	cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 0.5);
    cairo_rectangle(cr, m + m, y - 2, (w + m) * maxrectcount, h + 4);
	cairo_fill (cr);

	int howmany = ((float) val / 100.0) * maxrectcount;
	int i;
	for(i = 1; i < howmany + 1; i++) {
		cairo_rectangle (cr, (w + m) * i, y, w, h);
	}

	cairo_set_source_rgba (cr, 0, 0.3, 1, 0.7);
	cairo_fill (cr);
}
