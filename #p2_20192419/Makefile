TARGETS = ssu_repo add remove status commit revert log help 
OBJECTS = ssu_repo.o add.o remove.o status.o commit.o revert.o log.o help.o
cc = gcc



all: $(OBJECTS)
	for i in $(TARGETS); \
	do\
		$(cc) -o $$i $$i.o phantomutils.h -lcrypto; \
	done

ssu_repo.o: ssu_repo.c
	$(cc) -c ssu_repo.c
add.o: add.c
	$(cc) -c add.c
remove.o: remove.c
	$(cc) -c remove.c
status.o: status.c
	$(cc) -c status.c
commit.o: commit.c
	$(cc) -c commit.c
revert.o: revert.c
	$(cc) -c revert.c
log.o: log.c
	$(cc) -c log.c
help.o: help.c
	$(cc) -c help.c


clean:
	rm *.o
	for i in $(TARGETS); \
	do\
		rm $$i;\
	done