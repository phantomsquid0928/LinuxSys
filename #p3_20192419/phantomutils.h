#define OPENSSL_API_COMPAT 0x10100000L
#define _GNU_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <sys/wait.h>
#include <features.h>
#include <utime.h>
#include <ctype.h>
#include <time.h>

#include <openssl/md5.h>

#define MAXPATH 4096
#define MAXDIR 255


/// @brief 
/// @param absolute_path 
/// @return loc of last /, if u wanna get name then use substr(~, func(~) + 1, strlen(~))
int return_last_name(char *absolute_path) {
    int i;
    for (i = strlen(absolute_path) - 1; i >= 0; i--) {
        if (absolute_path[i] == '/') {
            return i;
        }//
    }
    return 0;
}

/// @brief substr of a <=  <= b
/// @param target to slice
/// @param a start
/// @param b end
/// @return sliced one
char * substr(char * target, int a, int b) {
    // printf("%d", strlen(target));
    // char temp[strlen(target) + 100];
    char* temp = (char*)malloc(strlen(target) + 1);
    strncpy(temp, target + a, b - a);
    *(temp + b - a) = '\0';
    return temp;
}

/// @brief 
/// @param command 
/// @param spliter 
/// @param res .
/// @return 
char ** split(char * command, char * spliter, int *res) {
    int cnt = 0;
    // printf("%s", command);
    char ** temp = (char**)malloc(sizeof(char*));
    char * arg = strtok(command, spliter);

    while(arg != NULL) {
        // printf("arg : %s\n", arg);
        temp = (char**)realloc(temp, sizeof(char*) * (cnt + 1));
        temp[cnt] = (char *)malloc(sizeof(char) * (strlen(arg) + 1));
        strcpy(temp[cnt], arg);
        arg = strtok(NULL, spliter);
        cnt++;
    }
    *res = cnt;

    return temp;
}

/// @brief split command smarter
/// @param command 
/// @param spliter 
/// @param res 
/// @param valid 
/// @return 
char ** commandsplit(char * command, char * spliter, int * res, int * valid) {
    int cnt = 0;
    int len = strlen(command);
    
    char ** tok = malloc(sizeof(char*));
    char *temp;
    int idx = 0;
    int chk = 0;
    int chk2 = 0;
    for (int i =0; i < len; i++) {
        if (command[i] == ' ' && chk == 0 && chk2 == 0) {
            tok = realloc(tok, sizeof(char*) * (cnt + 1));
            tok[cnt] = substr(command, idx, i);
            idx = i + 1;
            cnt++;
        }
        if (command[i] == '"') {
            chk ^= 1;
        }
        if (command[i] == '\'') {
            chk2 ^= 1;
        }
    }
    if (idx != len) {
        tok = realloc(tok, sizeof(char*) * (cnt + 1));
        tok[cnt] = substr(command, idx, len);
        cnt++;
    }
    if (chk != 0 || chk2 != 0) {
        *valid = 0;
        *res = -1;
        return NULL;
    }
    *valid = 1;
    *res = cnt;
    return tok;
}

int md5(char *target_path, char *hash_result)
{
	FILE *fp;
	unsigned char hash[MD5_DIGEST_LENGTH];
	unsigned char buffer[16384];
	int bytes = 0;
	MD5_CTX md5;

	if ((fp = fopen(target_path, "rb")) == NULL){
		printf("ERROR: fopen error for %s\n", target_path);
		return 1;
	}

	MD5_Init(&md5);

	while ((bytes = fread(buffer, 1, 16383, fp)) != 0)
		MD5_Update(&md5, buffer, bytes);
	
	MD5_Final(hash, &md5);

	for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
		sprintf(hash_result + (i * 2), "%02x", hash[i]);
	hash_result[32] = 0;

	fclose(fp);

	return 0;
}

/// @brief 
/// @param path 
/// @param path2 
/// @return same : 0 diff : 1 err : -1
int compare_md5(char * path, char * path2) {
	char hash[33];
	char hash2[33];
	if (md5(path, hash)) {
		return -1;
	}
	if (md5(path2, hash2)) {
		return -1;
	}
	if (!strcmp(hash, hash2)) {
		return 0;
	}
	return 1;
}

/// @brief remove file / dir with path... need test.
/// @param path 
/// @return 
int rmdirs(char * path) { //danger?
    struct dirent ** namelist;
    struct stat statbuf;
    
    if (lstat(path, &statbuf) < 0) return -1;
    if (!S_ISDIR(statbuf.st_mode)) {
        remove(path);
        printf("R> removing fi %s\n", path);
        return -2;
    }
    int cnt;
    if ((cnt = scandir(path, &namelist, NULL, alphasort)) < 0) {
        return -1;
    }
    for (int i =0 ;i < cnt; i++) {
        if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;
        char nextpath[MAXPATH];
        strcpy(nextpath, path);
        strcat(nextpath, "/");
        strcat(nextpath, namelist[i]->d_name);
        if (lstat(nextpath, &statbuf) < 0) return -1;
        if (S_ISDIR(statbuf.st_mode)) {
            rmdirs(nextpath);
        }
        else {
            remove(nextpath);
            printf("R> removing fil %s\n", nextpath);
        }
    }
    rmdir(path);
    printf("R> removing dir %s\n", path);
    return 0;
}

void mkdirs(char * path) { //path is always dir
    int res;
    char ** args = split(path, "/", &res);
    char temp[MAXPATH] = "";

    for (int i =0 ;i < res; i++) {
        strcat(temp, "/");
        strcat(temp, args[i]);
        if (access(temp, F_OK)) {
            mkdir(temp, 0777);
        }
    }
}

int isvalidpath(char * path) {
    char * ptr = NULL;
    if ((ptr = strstr(path, getenv("HOME"))) != NULL && (int)(path - ptr) == 0) {
        return 1;
    }
    return 0;
}

void addhelp(void) {
    printf("add <PATH> [OPTION]... : add new daemon process of <PATH> if <PATH> is file\n");
    printf("\t-d : add new daemon process of <PATH> if <PATH> is directory\n");
    printf("\t-r : add new daemon process of <PATH> recursive if <PATH> is directory\n");
    printf("\t-t <TIME> : set daemon process time to <TIME> sec (default : 1sec)\n");
}
void removehelp(void) {
    printf("remove <DAEMON_PID> : delete daemon process with <DAEMON_PID>\n");
}
void listhelp(void) {
    printf("list [DAEMON_PID] : show daemon process list or dir tree\n");
}
void helphelp(void) {
    printf("help [COMMAND] : show commands for program\n");
}
void exithelp(void) {
    printf("exit : exit program\n");
}


const int commandscnt = 5;
char * commandsList[] = {"add", "remove", "list", "help", "exit"};
void (*helpFuncList[])(void) = {addhelp, removehelp, listhelp, helphelp, exithelp};

char * homepath;
char * backuppath;
char logpath[MAXPATH];

typedef struct monitorlist{
    int pid;
    char path[MAXPATH];
    struct monitorlist * next;
}monitorlist;
typedef struct filever{
    time_t time;
    int status;
    char path[MAXPATH];
    struct filever * next;
}filever;
typedef struct filedir {
    char name[MAXDIR];
    char path[MAXPATH];// version path
    char oripath[MAXPATH]; //original path
    filever * head;
    filever * rear;
    int chk;       //indicates current status compared to latest ver. compared to top: chk = -1 : existing, no change   chk = 0 / -2 new    chk= 1 modify   chk= 2 remove
    int istrack;   //chk whether this file is being tracked 0 : no 1 : yesdc    
    
    int isreg; //  0 : dir, 1 : file, childscnt is not enough for distinguish file / dir

    struct filedir ** childs; //adds when newfile comes in
    int childscnt;
}filedir;

monitorlist * head = NULL;
monitorlist * rear = NULL;

filever * loghead = NULL; //
filever * logrear = NULL;

monitorlist * newmlog() {
    monitorlist * temp = malloc(sizeof(monitorlist));
    temp->pid = 0;
    temp->next = NULL;
    return temp;
}
void pushmlog(monitorlist * t) {
    if (head == NULL) {
        head = t;
        rear = t;
        return;
    }
    rear->next = t;
    rear = t;
}
int removemlog(int pid) {
    if (head == NULL) return -1;
    monitorlist * temp = head;
    monitorlist * prev = NULL;
    while(temp) {
        if (temp->pid == pid) break;
        prev = temp;
        temp = temp->next;
    }
    if (temp == NULL) return -1;
    if (prev == NULL) {
        head = head->next;
        free(temp);
        return 0;
    }
    if (temp == rear) {
        rear = prev;
        rear->next = NULL;
        free(temp);
        return 0;
    }
    prev->next = temp->next;
    free(temp);
    return 0;
}
void clearmlog() {
    monitorlist * temp = head;
    monitorlist * prev;
    while(temp) {
        prev = temp;
        temp = temp->next;
        free(prev);
    }
}
void pushlog(filever * log) {
    if (loghead == NULL) {
        loghead = log;
        logrear = log;
        return;
    }
    logrear->next = log;
    logrear = log;
}

void init_fs() {
    
}

void init() {
    homepath = getenv("HOME");
    char temp[4096];
    strcpy(temp, homepath);
    strcat(temp, "/backup");
    backuppath = substr(temp, 0, strlen(temp));

    if (access(backuppath, F_OK)) mkdir(backuppath, 0777);

    sprintf(logpath, "%s/%s", backuppath, "monitor_list.log");
    if (access(logpath, F_OK)) {
        if (open(logpath, O_CREAT, 0777) < 0) fprintf(stderr, "creat error");
    }
}

void load_monitor_log() {
    FILE * fp;
    if ((fp = fopen(logpath, "r")) == NULL) {
        fprintf(stderr, "error on fopen");
        exit(1);
    }
    // clearmlog();
    char buf[MAXPATH * 2];
    while(1) {
        int len = fscanf(fp, "%[^\n^\r]", buf);
        if (len == EOF) break;
        fgetc(fp);
        int res = 0;
        char ** temp = split(buf, ":", &res);
        int pid = atoi(temp[0]);
        char * targetpath = substr(temp[1], 1, strlen(temp[1]));
        
        monitorlist * log = newmlog();
        log->pid = pid;
        strcpy(log->path, targetpath);
        pushmlog(log);

        printf("%d %s\n", pid, targetpath);

        free(targetpath);
        free(temp[0]);
        free(temp[1]);
        free(temp);
    }
    fclose(fp);
}
void save_monitor_log() { //truncate
    FILE * fp;
    if ((fp = fopen(logpath, "w")) == NULL) {
        fprintf(stderr, "error on fopen");
        exit(1);
    }
    monitorlist * temp;
    temp = head;

    while(temp) {
        fprintf(fp, "%d : %s\n", temp->pid, temp->path);
        temp = temp->next;
    }

    fclose(fp);
}
void load_pid_log(int pid) {
    char targetpath[MAXPATH];
    FILE * fp;

    sprintf(targetpath, "%s/%d", backuppath, pid);
    if ((fp = fopen(logpath, "r")) == NULL) {
        fprintf(stderr, "error on fopen");
        exit(1);
    }
    char buf[MAXPATH * 2];
    while(1) {
        int len = fscanf(fp, "%[^\n^\r]", buf);
        if (len == EOF) break;
        int res = 0;
        char ** args = split(buf, "[]", &res);
        //args0 = time_t 1 = status 2 = path
    }
}
void save_pid_log(int pid, int status, char * tpath) {
    char targetpath[MAXPATH];
    FILE * fp;

    sprintf(targetpath, "%s/%d", backuppath, pid);
    if ((fp = fopen(logpath, "a+")) == NULL) {
        fprintf(stderr, "error on fopen");
        exit(1);
    }
    time_t t;
    time(&t);
    char * statusstr;
    if (status == 0) {
        statusstr = "create";
    }
    else if (status == 1) {
        statusstr = "modify";
    }
    else if (status == 2) {
        statusstr = "delete";
    } else exit(44);
    fprintf(fp, "[%s][%d][%s]", ctime(&t), status , tpath);
    fclose(fp);
}