
all: fatfs

fat32: fatfs.c
	gcc -Wall -g -o fatfs fatfs.c

clean: 	
	rm -fr *~  fatfs a.out
