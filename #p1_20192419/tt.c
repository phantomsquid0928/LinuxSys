#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define MAXPATH 4096

typedef struct {
	char first[MAXPATH];//?
	char second[MAXPATH];//?
}pathpair;
char ** split(char * command, char * spliter, int *res) {
    int cnt = 0;
    // printf("%s", command);
    char ** temp = (char**)malloc(sizeof(char*));
    char * arg = strtok(command, spliter);

    while(arg != NULL) {
        // printf("arg : %s\n", arg);
        temp = (char**)realloc(temp, sizeof(char*) * (cnt + 1));
        temp[cnt] = (char *)malloc(sizeof(char) * strlen(arg) + 1);
        strcpy(temp[cnt], arg);
        arg = strtok(NULL, spliter);
        cnt++;
    }
    *res = cnt;

    return temp;
}


int load_log() {
	char *path = "/home/backup/ssubak.log";
	FILE * fp;
	if ((fp = fopen(path, "rt")) == NULL) {
		fprintf(stderr, "got error while opening log");
		exit(1);
	}
	

	int size = 0;
	while(1) {
		char buf[3 * MAXPATH];
		int res = fscanf(fp, "%[^\n]\n", buf);
		if (res == EOF) break;
		printf("log : %s\n", buf);
		
		pathpair temp;
		int rs;
		char ** args = split(buf, "\"", &rs);
		for (int i = 0 ;i< rs; i++) {
			printf("%d:%s", i, args[i]);
		}
		if (!strcmp(args[1], " ff ")) {
			printf("tttt%s %s", args[0], args[2]);
		}


	}
}

int make_log(char* target_path, char*path, int mod) {
	char log[MAXPATH * 3];
	sprintf(log, "\"%s\" %s \"%s\"", path,"ff", target_path);
	// printf("%s\n", log);
	if (mod < 3) { //log file io, add linked list
		FILE * fd;
		if ((fd = fopen("/home/backup/ssubak.log", "a+")) == NULL) {
			fprintf(stderr, "log error, cannot open file");
			exit(1);
		}
		int size;
		fprintf(fd, "%s\n", log);
	}
	
	return 0;
}

int main() {
	make_log("/home/backup/234234/c.txt", "/home/a/c.txt", 1);
	make_log("/home/backup/234334/b.txt", "/home/a/b.txt", 1);
    // make_log("fffffs", "g", 1);
    // make_log("fssssssss", "g", 1);
    // make_log("feeeeeee", "g", 1);
    // make_log("aaaaaf", "g", 1);
    load_log();
}