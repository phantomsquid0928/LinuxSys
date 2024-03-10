OBJECTS = ssu_backup.o
TARGET = ssu_backup
CC = gcc

$(TARGET) : $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS)

ssu_backup.o: ssu_backup.c
	$(CC) -c ssu_backup.c -lcrypto
