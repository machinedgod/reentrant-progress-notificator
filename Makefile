libs="x11 cairo glib-2.0 librsvg-2.0"

reentrant-progress-notificator: main.c
	gcc -ggdb -o reentrant-progress-notificator main.c $(shell pkg-config --cflags $(libs)) $(shell pkg-config --libs $(libs))


