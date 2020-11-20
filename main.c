#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <string.h>

#include <signal.h>

#include "config.h"

void print_usage(void);
void event_received(void);
void timer_fired(void);
int spawn_new_window();
int modify_existing(pid_t pid, char modf);
int test_process_is_correct();

static long val = 0;
static long mval = 1;

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
    	//printf("Increasing on process %d\n", pid);
        break;
    case '-':
        kill(pid, SIGUSR2);
    	//printf("Decreasing on process %d\n", pid);
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
    printf("Modval: %d, Initval: %d\n", mval, val);

	if (signal(SIGUSR1, sig_handler) == SIG_ERR) {
		 printf("\ncan't catch SIGUSR1\n");
	}
	if (signal(SIGUSR2, sig_handler) == SIG_ERR) {
		 printf("\ncan't catch SIGUSR2\n");
	}

	Display *dsp = XOpenDisplay(NULL);
	if (!dsp) {
		return 1;
	}

	int screenNumber = DefaultScreen(dsp);
	unsigned long white = WhitePixel(dsp, screenNumber);
	unsigned long black = BlackPixel(dsp, screenNumber);


	Window win = XCreateSimpleWindow(dsp,
									DefaultRootWindow(dsp),
									50, 50,
									200, 200,
									0, black,
									white);


	// Wait until window is mapped
	XMapWindow(dsp, win);

	long eventMask = StructureNotifyMask;
	XSelectInput(dsp, win, eventMask);

	XEvent evt;
	do {
	   XNextEvent(dsp, &evt); // calls XFlush
	} while (evt.type != MapNotify);


	// Draw the lines
	GC gc = XCreateGC(dsp, win,
					  0,
					  NULL);
	XSetForeground(dsp, gc, black);

	XDrawLine(dsp, win, gc, 10, 10, 190, 190);
	XDrawLine(dsp, win, gc, 10, 190, 190, 10);

	eventMask = ButtonPressMask|ButtonReleaseMask;
	XSelectInput(dsp, win, eventMask);


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
		tv.tv_usec = 0;
		tv.tv_sec = 1;

		// Wait for X Event or a Timer
		int num_ready_fds = select(x11_fd + 1, &in_fds, NULL, NULL, &tv);
		if (num_ready_fds > 0)
			event_received();
		else if (num_ready_fds == 0)
			timer_fired();
		//else
		//	printf("An error occured!\n");

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

void timer_fired(void) {
	printf("Timer, val: %d\n", val);
}
