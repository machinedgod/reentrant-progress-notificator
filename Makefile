libs="x11 cairo glib-2.0 librsvg-2.0"

notification-box: main.c config.h
	gcc -o notification-box main.c $(shell pkg-config --cflags $(libs)) $(shell pkg-config --libs $(libs))


