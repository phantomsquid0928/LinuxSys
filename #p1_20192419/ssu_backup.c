#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

typedef enum commands {
	backup, remove_enum, recover, list, help, errorcom
}commands;
static const char *commandList[] = {
	"backup", "remove", "recover", "list", "help",
};
typedef struct {
    commands val;
	const char *str;
} commandsMap;
static int commandSize = sizeof(commandList) / sizeof(commandList[0]);
commandsMap * conversion[5];

void initEnum() {
	int i= 0;

	for (i = 0;i < commandSize; i++) {
		conversion[i] = (commandsMap*)malloc(sizeof(commandsMap));
		conversion[i]->val = (commands)i;
		conversion[i]->str = commandList[i];
	}
}
commands str2enum(char * str) {
	int i;

	for (i = 0;i < commandSize; i++) {
		if (strcmp(str, conversion[i]->str) == 0)
			return conversion[i]->val;
	}
	return errorcom;
}
int main(int argc, char * argv[]) {
	int i = 0;
	initEnum();
	for (i = 0; i < argc; i++) {
		printf("%s\n", argv[i]);
	}
	printf("%d\n", str2enum("backup"));
	printf("%s   ", argv[1]);
	printf("%d\n", str2enum(argv[1]));
}
