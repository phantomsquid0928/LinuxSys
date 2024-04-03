#define OPENSSL_API_COMPAT 0x10100000L
#define _DEFAULT_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <sys/wait.h>

#include <openssl/md5.h>

#define MAXPATH 4096
#define MAXDIR 256

char commitpath[MAXPATH];
char stagingpath[MAXPATH];
char repopath[MAXPATH];

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

typedef struct node {
	struct node * next;
	void * data;
}node;
typedef struct queue queue;
typedef struct queue {
	node * head;
	node * rear;
	int size;

	void * (*front)(queue *);
	void (*pop)(queue *);
	void (*push)(queue *, void *);
	int (*empty)(queue *);
	void (*clear)(queue *);
}queue;

void * front(queue * self) {
	if (self->head == NULL) {
		return NULL;
	}
	return self->head->data;
}
void pop(queue * self) {
	node * temp = self->head;
	if (self->head == NULL) return;
	
	if (self->head->next == NULL) {
		self->head = NULL;
		self->rear = NULL;
		self->size = 0;
		free(temp);
		return;
	}
	self->head = self->head->next;
	self->size--;
	free(temp);
}
void push(queue * self, void * data) {
	node * temp = (node *)malloc(sizeof(node));
	temp->data = data;
	temp->next = NULL;
	if (self->head == NULL) {
		self->head = temp;
		self->rear = temp;
		self->size++;
		return;
	}
	self->rear->next = temp;
	self->rear = temp;
	self->size++;
}
int empty(queue * self) {
	return self->size == 0 ? 1 : 0;
}
void clear(queue * self) {
	if (self->head == NULL) return;
	node * n = self->head;
	node * temp = n;
	node * prev;
	while(temp) {
		prev = temp;
		temp = temp -> next;
		free(prev);
	}
//	free(prev);
	self->size = 0;
	self->head = NULL;
	self->rear = NULL;
}
queue * initQueue() {
	queue * temp = (queue*)malloc(sizeof(queue));
	temp->head = NULL;
	temp->rear = NULL;
	temp->size = 0;

	temp->front = front;
	temp->pop = pop;
	temp->push = push;
	temp->empty = empty;
	temp->clear = clear;
	temp->size = 0;

	return temp;
}



typedef struct commitlog {
    char version[MAXDIR];
    int status;     //0 added 1 mod 2 removed
    struct stat statbuf;
    struct commitlog * next;
}commitlog;

typedef struct file {
    char name[MAXDIR];
    char path[MAXPATH];// version path
    char oripath[MAXPATH]; //original path
    commitlog * top; //latest

    struct file * next; //adds when newfile comes in

}file;

typedef struct version_controller { //revert 3 ->  revert files to version that ver < 3 
    char curver[MAXDIR];
    file * head;
    file * rear;
}control;

control * version_cursor = NULL;

typedef struct loglist {
    char log[MAXPATH * 2];
    struct loglist * next;
}loglist;
loglist * head = NULL;
loglist * rear = NULL;

loglist * newlog() {
    loglist * temp = (loglist*)malloc(sizeof(loglist));
    if (temp == NULL) {
        printf("FATAL: NO MEM");
        exit(100);
    }
    temp->next = NULL;
    return temp;
}
int islogexists(char * target);
/// @brief 
/// @param target 
/// @return 1 succeed 0 fail
int addlog(char * target) { //target can only be file
    if (islogexists(target)) return 0;
    loglist * temp = newlog();
    strcpy(temp->log, target);
    if (head == NULL) {
        head = temp;
        rear = temp;
        return 1;
    }
    rear->next = temp;
    rear = temp;
    return 1;
}

int addlogrecurs(char * target) {
    int res = 0;
    struct stat statbuf;
    struct dirent **namelist;
    int cnt;
    queue q = *initQueue();

    char * cwd = getcwd(NULL, 0);

    q.push(&q, target);
    while(!q.empty(&q)) {
        char * cur = q.front(&q);
        q.pop(&q);
        if (lstat(cur, &statbuf) < 0) return -1;
        if (S_ISREG(statbuf.st_mode)) {
            res = addlog(cur);
            free(cur);
            continue;
        }
        if (!S_ISDIR(statbuf.st_mode)) continue;
        if ((cnt = scandir(cur, &namelist, NULL, alphasort)) < 0) return -1;
        for (int i = 0; i< cnt; i++) {
            if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;
            if (!strcmp(namelist[i]->d_name, ".repo")) continue;
            char newpath[MAXPATH];
            sprintf(newpath, "%s/%s", cur, namelist[i]->d_name);
            if (lstat(newpath, &statbuf) < 0) return -1;
            if (S_ISREG(statbuf.st_mode)) {
                res = addlog(newpath);
                continue;
            }
            if (!S_ISDIR(statbuf.st_mode)) continue;
            char * nextdir = (char *)malloc(sizeof(char) * MAXPATH);
            strcpy(nextdir, newpath);
            q.push(&q, nextdir);
        }
        free(cur);
    }
    return res;
}
/// @brief 
/// @param target 
/// @return 1 succeed 0 fail
int dellog(char * target) { //target can only be file
    loglist * temp = head;
    loglist * prev = NULL;
    // if (islogexists(target) == 0) return 0;
    // if (temp == NULL) return 0;
    while(temp) {
        if (!strcmp(temp->log, target)) {
            break;
        }
        prev = temp;
        temp = temp->next;
    }
    if (temp == NULL) return 0;
    if (prev == NULL) {
        head = temp->next;
        free(temp);
        return 1;
    }
    if (temp == rear) {
        rear = prev;
        prev->next = NULL;
        free(temp);
        return 1;
    }
    prev->next = temp->next;
    free(temp);
    return 1;
}
/// @brief 
/// @param target 
/// @return 0 fail 1 succeed -> if 0 then no target exists 1 -> make log
int dellogrecurs(char * target) {
    int res = 0;
    struct stat statbuf;
    struct dirent **namelist;
    int cnt;
    queue q = *initQueue();

    char * cwd = getcwd(NULL, 0);

    q.push(&q, target);

    while(!q.empty(&q)) {
        char * cur = q.front(&q);
        q.pop(&q);
        if (lstat(cur, &statbuf) < 0) return -1;
        if (S_ISREG(statbuf.st_mode)) {
            res = dellog(cur);
            free(cur);
            continue;
        }
        if (!S_ISDIR(statbuf.st_mode)) continue;
        if ((cnt = scandir(cur, &namelist, NULL, alphasort)) < 0) return -1;
        for (int i = 0; i< cnt; i++) {
            if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;
            if (!strcmp(namelist[i]->d_name, ".repo")) continue;
            char newpath[MAXPATH];
            sprintf(newpath, "%s/%s", cur, namelist[i]->d_name);
            if (lstat(newpath, &statbuf) < 0) return -1;
            if (S_ISREG(statbuf.st_mode)) {
                res = dellog(newpath);
                continue;
            }
            if (!S_ISDIR(statbuf.st_mode)) continue;
            char * nextdir = (char *)malloc(sizeof(char) * MAXPATH);
            strcpy(nextdir, newpath);
            q.push(&q, nextdir);
        }
        free(cur);
    }
    return res;
}
int islogexists(char * target) {
    loglist * temp = head;
    while(temp) {
        if (!strcmp(target, temp->log)) {
            return 1;
        }
        temp = temp->next;
    }
    return 0;
}
void freelog() {
    loglist * temp = head;
    loglist * prev;
    while(temp) {
        prev = temp;
        temp = temp->next;
        free(prev);
    }
}

file * newfile() {
    file * temp = (file*)malloc(sizeof(file));
    temp->top = NULL;
    temp->next = NULL;
    return temp;
}
commitlog * newversion() {
    commitlog * temp = (commitlog*)malloc(sizeof(commitlog));
    temp->next = NULL;
    return temp;
}
int addfile(file * t) {
    if (version_cursor == NULL) return -1;
    if (version_cursor->head == NULL) {
        version_cursor->head = t;
        version_cursor->rear = t;
        return 0;
    }
    version_cursor->rear->next = t;
    version_cursor->rear = t;
    return 0;
}
int addversion(file * f, commitlog * t) {
    if (f->top == NULL) {
        f->top = t;
        return 0;
    }
    t->next = f->top;
    f->top = t;
    return 0;
}
file * searchExistingFile(char * path) {
    file * temp = version_cursor->head;
    while(temp) {
        if (!strcmp(temp->oripath, path)) {
            return temp;
        }
        temp = temp->next;
    }
    return NULL;
}






/// @brief adds only FILE ****
/// @return success 0 else minus values
int load_staging_log() {
    char * cwd = getcwd(NULL, 0);
    // printf("%s", stagelogpath);
    if (access(stagingpath, F_OK)) return -1;
    char buf[MAXPATH * 2];
    FILE * fp;

    if ((fp = fopen(stagingpath, "rt")) == NULL) {
        return -2;
    }
    while(1) {
        int res = fscanf(fp, "%[^\n]", buf);
        if (res == EOF) break;
        fgetc(fp);
        int argc = 0;
        // printf("%s", buf);
        char ** args = split(buf, "\"", &argc);
        if (!strcmp(args[0], "add ")) {
            addlogrecurs(args[1]);
        }
        else if (!strcmp(args[0], "remove ")) {
            dellogrecurs(args[1]);
        }
        else {
            return -10;
        }
        // printf("\n");
    }
    return 0;
}




int load_commit_log() {
    struct stat statbuf;

    char * cwd = getcwd(NULL, 0);

    // printf("%s", stagelogpath);
    if (access(commitpath, F_OK)) return -1;
    char buf[MAXPATH * 2];
    FILE * fp;

    if ((fp = fopen(commitpath, "rt")) == NULL) {
        return -2;
    }
    while(1) {
        int res = fscanf(fp, "%[^\n]", buf);
        if (res == EOF) break;
        fgetc(fp);
        int argc = 0;
        printf("%s\n", buf);
        char ** args;
        args = split(buf, "-", &argc);

        char * commit_name = substr(args[0], 8, strlen(args[0]) - 1);
        char * target_temp = NULL;
        int status = -1;
        // printf("commit_name : :%s:\n", commit_name);
        if (strstr(args[1], "new file:") != NULL) {
            target_temp = substr(args[1], 11, strlen(args[1]));
            // printf("  new : :%s:\n", target_name);
            // char * name = substr(target_path, );
            status = 0;
        }
        else if (strstr(args[1], "modified:") != NULL) {
            target_temp = substr(args[1], 11, strlen(args[1]));
            // printf("  modified : :%s:\n", target_name);
            status = 1;
        }
        else if (strstr(args[1], "removed:") != NULL) {
            target_temp = substr(args[1], 10, strlen(args[1]));
            // printf("  removed : :%s:\n", target_name);
            status = 2;
        } else {
            printf("FATAL : corrupted log file\n");
            exit(100);
        }
        char target_path[MAXPATH];
        strcpy(target_path, target_temp);
        
        char * relpath = substr(target_path, strlen(cwd) + 1, strlen(target_path));
        char version_path[MAXPATH];
        char name[MAXDIR];
        sprintf(version_path, "%s/.repo/%s/%s", cwd, commit_name, relpath);
        sprintf(name, "%s", substr(target_path, return_last_name(target_path) + 1, strlen(target_path)));
        
        
        file * f = newfile();
        commitlog * v = newversion();
        // if (lstat(version_path, &statbuf) < 0 && status != 2) {
        //     return -1; //.repo corrupted
        // }
        strcpy(f->name, name);
        strcpy(f->oripath, target_path);
        strcpy(f->path, version_path);
        strcpy(v->version, commit_name);
        v->status = status;
        if (status != 2)
            v->statbuf = statbuf;
        file * exists = searchExistingFile(target_path);
        if (exists != NULL) {
            free(f);
            addversion(exists, v);
        }
        else {
            addfile(f);
            addversion(f, v);
        }
        // printf("\n\n");
    }
    return 0;
}
/// @brief call init() first to make path of .repo logs
/// @param relpath 
/// @param mod 
/// @return 
int save_staging_log(char * abpath, int mod) {
    char * cwd = getcwd(NULL, 0);
    // char stagelogpath[MAXPATH];

    // sprintf(stagelogpath, "%s/.repo/.staging.log", cwd);
    if (access(stagingpath, F_OK)) return -1;
    char buf[MAXPATH * 2];
    FILE * fp;

    if ((fp = fopen(stagingpath, "at")) == NULL) {
        return -2;
    }
    fprintf(fp, "%s \"%s\"\n", mod == 1 ? "add" : "remove", abpath);
}
int save_commit_log() {

}
void show_staging_log() {
    loglist* temp = head;
    while(temp) {
        printf("%s\n", temp->log);
        temp = temp->next;
    }
}
void show_commit_asfile() {
    file * temp = version_cursor->head;
    while(temp) {
        printf("name : %s\n", temp->name);
        printf("path : %s\n", temp->path);
        printf("oripath : %s\n", temp->oripath);
        commitlog * t2 = temp->top;
        while(t2) {
            printf("   version : %s\n", t2->version);
            printf("   action : %d\n", t2->status);
            // printf("   size : %ld\n\n", t2->statbuf.st_size);
            t2 = t2 ->next;
        }
        temp = temp->next;
    }
}
void show_commit_log() {
    //new struct just for log
}
void init() {
    char * cwd = getcwd(NULL, 0);
    if (cwd == NULL) exit(100);
    sprintf(commitpath, "%s/.repo/.commit.log", cwd);
    sprintf(stagingpath, "%s/.repo/.staging.log", cwd);
    sprintf(repopath, "%s/.repo", cwd);
    return;
}

/**
 * TODO: make cursor->curver have value
*/
/// @brief init controler. call ` after calling this
void init_version_controller() {
    version_cursor = (control*)malloc(sizeof(control));
    version_cursor->head = NULL;
    version_cursor->rear = NULL;
}