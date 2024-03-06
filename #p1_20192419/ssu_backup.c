#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#define MAXPATH 4096
#define MAXDIR 255
#define COMMAND_ENT 5
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
commandsMap * conversion[COMMAND_ENT];
char backup_path[MAXPATH];
///////////////////^^^^^^^^// declare zone//////////////////

//////////////////vv// init, func zone //////////////////
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
void initBackupDir() {
	sprintf(backup_path, "%s/backup", getenv("HOME"));
	//printf("%s", backup_path);
	if (access(backup_path, F_OK))
		mkdir(backup_path, 0777);
}
void initBackupList() {

}
/////////////////// maker_zone (io zone) /////////////////////
//makers gets fds as parameter, then do file io.
//make log, copy(backup) of files...
//make_log gets paths cuz fds r already opened
int make_backup(int targetfd, int fd) {
	int len;
	char buf[1024];
	while((len = read(fd, buf, 1024)) > 0) {
		write(targetfd, buf, len);
	}
	if (!errno || errno == 2) return 0;
	else {
		printf("errno : %d", errno);
		return -1;
	}
}
int make_log(char* target_path, char*path) {
	char log[2048];
	sprintf(log, "%s backed up to %s", path, target_path);
	return 0;
}

//////////////////// worker zone ////////////////////
//workers ex) backup remove... are here, all functional actions are 
//occur here
//workers only produce path, open file. throws opened fds from open() to
//makers
int do_backup(char * path, int mod) {
	int fd;
	int target_fd;
	char * time = "34434434";
	char cwd[1024];
	struct stat info;

	if (getcwd(cwd, 1024) == NULL)
		return -3;
	if (lstat(path, &info) < 0) {
		return -1; //lstat error
	}
	//make path for backup
	char target_path[MAXPATH];
	//get time

	//tokenize provided path : /a/b/c.txt -> c.txt 
	//to get pure file name
	char *ptr = strtok(path, "/");
	char *tmp;
	char purename[MAXDIR];
	if (ptr == NULL) {
		sprintf(purename, "%s", ptr);
	}
	else {
		while (1) {
			tmp = ptr;
			ptr = strtok(NULL, "/");
			if (ptr == NULL) break;
		}
		sprintf(purename, "%s", tmp);
	}
	//sprintf(target_path, "%s/%s/%s", backup_path, time, purename);
	strcpy(target_path, backup_path);
	strcat(target_path, "/");
	strcat(target_path, time);
	printf("%s", target_path);
	if (access(target_path, F_OK))
		mkdir(target_path, 0777);
	strcat(target_path, "/");
	strcat(target_path, purename);
	//home/backup/<time>/a.txt
	printf("%s\n", target_path);
	//printf("%d\n", strlen(target_path));
	if (S_ISREG(info.st_mode)) { //file
		if ((fd = open(path, O_RDONLY)) < 0)
			return -7; //open error
		
		if ((target_fd = open(target_path, O_RDWR|O_CREAT, 777)) < 0)
			return -2; //open error
		if (make_backup(target_fd, fd) < 0) {
			close(target_fd);
			close(fd);
			remove(target_path);
			return -4; //make_backup error
		}
		//call logger
		if (make_log(path, target_path) < 0) //TODO : get path from fd is needed
			return -5; //logger error
	}
	else if (S_ISDIR(info.st_mode)) { //dir
		if ((fd = open(path, O_RDONLY)) < 0) return -2; //open error
	}
	/*if ((fd = open(path, )) < 0) {
		return -1;
	}*/
	return 0;
}

///////////////////vvvvvvv/ func zone vvvvvv/////////////////////
//slices commands and manage bad inputs
/**
* backup -d
*/
void backup_func(int argc, char * argv[]) {
	int i = 0;
	printf("here");
	if (argc < 3) {
		//error
		return;
	}
	char path[MAXPATH];
	sprintf(path, "%s", argv[2]);
	int mod = 0;
	
	if (argc > 3){
		for (i = 3; i < argc; i++) {
			if (!strcmp(argv[i], "-d"))
				mod = mod | 1;
			else if (!strcmp(argv[i], "-r"))
				mod = mod | 2;
			else if (!strcmp(argv[i], "-a"))
				mod = mod | 4;
			else { //err wrong flags
				printf("wrong flag!\n");
				exit(1);
			}
		}
	}
	int errcode = 0;
	printf("path : %s mod : %d\n", path, mod);
	if ((errcode = do_backup(path, mod)) < 0) //failure of backup 
	{
		printf("error no : %d", errcode);
		return;
		//error, make stderr
	}
	printf("hfffffff");
}
void remove_func() {
	
}
void recover_func() {
	printf("hello recover");
}
void list_func() {
	printf("hello list");
}
void help_func() {
	printf("hello help");
}

//////////////////////////vvvvvvvv// main //////////////////////
int main(int argc, char * argv[]) {
	int i = 0;
	initEnum();	
	initBackupDir();
	initBackupList();

	//for (i = 0; i < argc; i++) {
	//	printf("%s\n", argv[i]);
	//}
	//printf("%d", argc);
	switch(str2enum(argv[1])) {
		case backup:
			backup_func(argc, argv);
			break;
		case remove_enum:
			remove_func(argc, argv);
			break;
		case recover:
			recover_func(argc, argv);
			break;
		case list:
			list_func(argc, argv);
			break;
		case help:
			help_func();
			break;
		default: //errorcom 
			{
				break;
			}
	}
}

