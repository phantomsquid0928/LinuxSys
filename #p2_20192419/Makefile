TARGETS = ssu_repo add remove status
OBJECTS = ssu_repo.o add.o remove.o status.o
cc = gcc



all: $(OBJECTS)
	for i in $(TARGETS); \
	do\
		$(cc) -o $$i $$i.o phantomutils.h; \
	done

ssu_repo.o: ssu_repo.c
	$(cc) -c ssu_repo.c
add.o: add.c
	$(cc) -c add.c
remove.o: remove.c
	$(cc) -c remove.c
status.o: status.c
	$(cc) -c status.c

clean:
	rm *.o
	for i in $(TARGETS); \
	do\
		rm $$i;\
	done