
all:
	mkdir -p bin
	gcc -g src/Pre-Expedite.c src/fsmk.c src/fsperm.c src/fsmount.c  src/fsutils.c -o bin/Pre-Expedite -I include/   -std=c99 -D_XOPEN_SOURCE=500 -D_GNU_SOURCE -Wall
	
clean:
	rm bin/*
