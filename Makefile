all: main
	gcc -o notification-box main.o -lX11

main: main.c
	gcc -c main.c

