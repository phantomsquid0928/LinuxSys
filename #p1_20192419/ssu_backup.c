#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <queue.h>
#define MAXPATH 4096
#define MAXDIR 255
#define COMMAND_ENT 5
typedef enum commands {
	backup, remove_enum, recover, list, help, errorcom
}commands;
static const char *commandList[] = {
	"backup", "remove", "recover", "list", "help",
};
static const char *logModList[] = {
	"backuped to", "removed by", "recovered to", "already backuped to", "not changed with"
};
typedef struct Log{
	char absolute_path[MAXPATH];
	char backup_path[MAXPATH];
	int timestamp;
	char purename[MAXPATH];
	struct Log * next;
}log;
log * logList;
typedef struct {
	char first[MAXPATH];
	char second[MAXPATH];
}pathpair;
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
	//sprintf(backup_path, "%s/backup", getenv("HOME"));
	sprintf(backup_path, "/home/backup");
	//printf("%s", backup_path);
	if (access(backup_path, F_OK))
		mkdir(backup_path, 0777);
}
void initBackupList() { //io log file

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
	return 0;
	if (!errno || errno == 2) return 0;
	else {
		printf("errno : %d", errno);
		return -1;
	}
}
//changes log info by mod
//mod 0 : backuped to
//mod 1 : removed by
//mod 2 : recovered to
//mod 3 : already backuped to
//mod 4 : not changed with
//TODO need more parameter for struct Log, expecially timestamp?
int make_log(char* target_path, char*path, int mod) {
	char log[10000];
	sprintf(log, "\"%s\" %s \"%s\"", path, logModList[mod], target_path);
	printf("%s\n", log);
	if (mod < 3) { //log file io, add linked list

	}
	return 0;
}

//////////////////// worker zone ////////////////////
//workers ex) backup remove... are here, all functional actions are 
//occur here
//workers only produce path, open file. throws opened fds from open() to
//makers
int bfs_worker(char * target_path, char * path, void * func) {
	queue<pathpair> q;///TODO cpp abstract code
	char tempname[MAXDIR];
	char temppath[MAXPATH];
	struct stat tempstat;
	struct dirent * dentry;
 	mkdir(target_path);
	pathpair node;
	sprintf(node.first, "%s", path);
    sprintf(node.second, "%s", target_path);	
	q.push(node);
	while(!q.empty()) {
		char* curpath =q.front().first;
		char* target_curpath = q.front().second;
		DIR * x; 
		if ((x = opendir(curpath)) == NULL || chdir(curpath) == -1) {
			return -12; //bfs err
		}
		q.pop();
		opendir(x);
		while((dentry = readdir(x)) != NULL) {
			if (dentry -> d_ino == 0)
				continue;
			memcpy(tempname, dentry -> dname, MAXDIR);
			memset(temppath, 0, sizeof(temppath));
			if (stat(tempname, &tempstat) == -1) {
				return -13; //stat err
			}
			if (S_ISREG(tempstat.st_mode)) {
				sprintf(temppath, "%s/%s", curpath, tempname);
				//save file in backup
				if ((fd = open(temppath, O_RDONLY)) < 0) { //path need modi
					return -7; //open er
				}
				if ((target_fd = open(target_path, O_WRONLY, 0777)) < 0) {
					return -2; //open er, target_path also need mod
				}
				if (make_backup(target_fd, fd) < 0) { //change func by func
					close(fd);
					close(target_fd);
					remove();
					printf("error");
					return -4;
				}
				if (make_log(path, target_path) < 0) {
					return -5; //log error
				}
				close(fd);
				close(target_fd);
			}
			if (S_ISDIR(tempstat.st_mode) && mod & 2 != 0) {
				//mkdir in backup

				q.push(x);
			}
		}
	}
}
int do_backup(char * path, int mod) {
	int fd;
	int target_fd;
	char * time = "34434434";
	char cwd[1024];
	struct stat info;
	struct dirent *dentry;
	DIR *dirp;

	if (getcwd(cwd, 1024) == NULL)
		return -3;
	if (lstat(path, &info) < 0) {
		return -1; //lstat error, file/dir not exists
	}
	//make path for backup
	char target_path[MAXPATH];
	//get time

	//tokenize provided path : /a/b/c.txt -> c.txt 
	//to get pure file name
	char temp[MAXPATH];
	sprintf(temp, "%s", path);
	char *ptr = strtok(temp, "/");
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
	printf("::::::::path : %s\n", path);
	if (S_ISREG(info.st_mode)) { //file
		if (mod & 3 != 0) { //-d -r flag but file
			return -10; //flag error
		}
		if ((fd = open(path, O_RDONLY)) < 0)
			return -7; //open error
		if ((target_fd = open(target_path, O_WRONLY|O_CREAT, 0777)) < 0) {
			printf("errno: %d", errno);
			return -2; //open error
		}
		if (make_backup(target_fd, fd) < 0) {
			close(target_fd);
			close(fd);
			remove(target_path);
			return -4; //make_backup error
		}
		//call logger
		if (make_log(path, target_path, 0) < 0) //TODO : get path from fd is needed
			return -5; //logger error
	}
	else if (S_ISDIR(info.st_mode)) { //path is dir
		if (mod & 3 == 0)
			return -11; //flag error 2
		//if ((fd = open(path, O_RDONLY)) < 0) return -2; //open error
			
	}
	/*if ((fd = open(path, )) < 0) {
		return -1;
	}*/
	chdir(cwd);  //make working dir points orig dir may be useless
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
	char strict_path[MAXPATH];
	realpath(path, strict_path);
	printf("%s\n", path);
	printf("%s\n", strict_path);
	int mod = 0;
	
	if (argc > 3){
		for (i = 3; i < argc; i++) {
			if (!strcmp(argv[i], "-d"))
				mod = mod | 1;
			else if (!strcmp(argv[i], "-r"))
				mod = mod | 2;
			else if (!strcmp(argv[i], "-y"))
				mod = mod | 4;
			else { //err wrong flags
				printf("wrong flag!\n");
				exit(1);
			}
		}
	}
	int errcode = 0;
	printf("path : %s mod : %d\n", path, mod);
	if ((errcode = do_backup(strict_path, mod)) < 0) //failure of backup 
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

