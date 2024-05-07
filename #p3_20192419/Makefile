cc = gcc

test: ssu_sync.o
	$(cc) -o ssu_sync ssu_sync.o phantomutils.h -lcrypto

ssu_sync.o: ssu_sync.c
	$(cc) -c ssu_sync.c
clean:
	rm *.o
	rm ssu_sync