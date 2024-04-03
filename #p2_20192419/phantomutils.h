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



typedef struct filever {
    char version[MAXDIR];
    int status;     //0 added 1 mod 2 removed
    struct stat statbuf;
    struct filever * next;
}filever;

typedef struct filedir {
    char name[MAXDIR];
    char path[MAXPATH];// version path
    char oripath[MAXPATH]; //original path
    filever * top; //latest

    struct filedir ** childs; //adds when newfile comes in
    int childscnt;
}filedir;

typedef struct commitlog {
    filedir * flink;
    filever * vlink;
    struct commitlog * next;
}commitlog;

typedef struct version_controller { //revert 3 ->  revert files to version that ver < 3 
    char curver[MAXDIR];
    filedir * root;
}control;

control * version_cursor = NULL;

typedef struct stagelog {
    char log[MAXPATH * 2];
    struct stagelog * next;
}stagelog;


stagelog * head = NULL;
stagelog * rear = NULL;
commitlog * commithead = NULL;
commitlog * commitrear = NULL;


stagelog * newlog() {
    stagelog * temp = (stagelog*)malloc(sizeof(stagelog));
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
    stagelog * temp = newlog();
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
    stagelog * temp = head;
    stagelog * prev = NULL;
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
    stagelog * temp = head;
    while(temp) {
        if (!strcmp(target, temp->log)) {
            return 1;
        }
        temp = temp->next;
    }
    return 0;
}
void freelog() {
    stagelog * temp = head;
    stagelog * prev;
    while(temp) {
        prev = temp;
        temp = temp->next;
        free(prev);
    }
}

void addcommitlog(commitlog * t) {
    if (commithead == NULL) {
        commithead = t;
        commitrear = t;
        return;
    }
    commitrear->next = t;
    commitrear = t;
}

filedir * newfile() {
    filedir * temp = (filedir*)malloc(sizeof(filedir));
    temp->top = NULL;
    temp->childs = NULL;
    temp->childscnt = -1;
    return temp;
}
filever * newversion() {
    filever * temp = (filever*)malloc(sizeof(filever));
    temp->next = NULL;
    return temp;
}
commitlog * newcommitlog() { //only directs file
    commitlog * temp = (commitlog*)malloc(sizeof(commitlog));
    temp->next = NULL;
    return temp;
}

// 


int addversion(filedir * f, filever * t) {
    if (f->top == NULL) {
        f->top = t;
        return 0;
    }
    t->next = f->top;
    f->top = t;
    return 0;
}

/**
 * TODO: add stamppath on dir
*/
/// @brief adds filedir by tree search
/// @param root 
/// @param target 
/// @param cur 
/// @return 
filedir * addfiledir(filedir * target) { //always comes file
    filedir * temp = version_cursor->root;
    char * relpath = substr(target->oripath, strlen(temp->oripath) + 1, strlen(target->oripath));

    printf("%s\n", relpath);
    int res;
    char ** args = split(relpath, "/", &res);
    
    char curpath[MAXPATH];
    strcpy(curpath, temp->oripath); //root
    for(int i= 0;i < res; i++) {
        printf("cur : %s\n", temp->oripath);
        strcat(curpath, "/");
        strcat(curpath, args[i]);
        printf("path: %s\n", curpath);
        if (temp->childscnt != -1) {
            for (int j = 0; j < temp->childscnt + 1; j++) {
                printf("member : %s\n", temp->childs[j]->name);
            }
            printf("\n");
            int start = 0;
            int end = temp->childscnt + 1;
            int chk = 0;
            for (int j = 0; j < 20; j++) {
                int mid = start + end >> 1;
                int res = strcmp(temp->childs[mid]->oripath, curpath);
                printf("res : %d %d %d, %s  :  %s\n",res, start, end, temp->childs[mid]->name, args[i]);
                if (res == 0) { //there is same one
                    chk = 1;
                    break;
                }
                if (res > 0) {
                    end = mid;
                }
                else {
                    start = mid;
                }
            }
            if (chk == 1) { 
                //same, dir or file exists
                printf("exists!\n");
                if (i == res - 1) { //file
                    addversion(temp->childs[start], target->top);
                    free(target);
                    return temp->childs[start];
                }
                else { //dir
                    temp = temp->childs[start];
                }
                
            }
            else { //file or dir not exists
                printf("here, %d %d", temp->childscnt, end);
                temp->childs = (filedir **)realloc(temp->childs, sizeof(filedir*) * (temp->childscnt + 2));// 3-> 4ro  5

                for (int j = temp->childscnt; j >= end; j--) {
                    temp->childs[j + 1] = temp->childs[j];
                }
                if (i == res - 1) //no file, adding...
                {
                    temp->childs[end] = target;
                    temp->childscnt++;
                    printf("res\n");
                    return target;
                }
                else { //no dir...
                    printf("seeking\n");
                    filedir * newfiledir = newfile();
                    strcpy(newfiledir->name, args[i]);
                    strcpy(newfiledir->oripath, curpath);
                    temp->childs[end] = newfiledir;
                    temp->childscnt++;
                    temp = temp->childs[end];
                }
                
            }
        }
        else { //cur dir has no child
            if (i == res - 1) {// file {
                printf("adding bare file\n\n");
                temp->childs = (filedir**)malloc(sizeof(filedir*));
                temp->childs[0] = target;
                temp->childscnt = 0;
                return target;
            }
            else { // not yet, keep adding empty dir
                printf("adding new dir\n");
                temp->childs = (filedir**)malloc(sizeof(filedir*));
                filedir * newfiledir = newfile();
                strcpy(newfiledir->name, args[i]);
                strcpy(newfiledir->oripath, curpath);
                temp->childs[0] = newfiledir;
                temp->childscnt = 0;
                temp = temp->childs[0];
            }
        }
    }

}

void show_fs(filedir * cur, char * padding) {
    if (cur->childscnt == -1) { //file
        printf(" file");
        printf(" latest : %s\n", cur->top->version);
        return;
    }
    printf("%s", cur->name);
    printf("  dir\n");
    for (int i = 0;i < cur->childscnt + 1; i++) {
        char curpad[1000];
        char nextpad[1000];
        if (i == cur->childscnt) {
            sprintf(curpad, "%s   └", padding);
            sprintf(nextpad, "%s    ", padding);
        }
        else {
            sprintf(curpad, "%s   ├", padding);
            sprintf(nextpad, "%s   ", padding);
        }
        
        if (cur->childs[i]->childscnt == -1) { //file
            printf("%s/%s", curpad, cur->childs[i]->name);
            printf(" file");
            printf(" latest : %s\n", cur->childs[i]->top->version);
            continue;
        }
        else {
            printf("%s/", curpad);
            show_fs(cur->childs[i], nextpad);
        }
        
    }
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
        
        
        filedir * f = newfile();
        filever * v = newversion();
        commitlog * c = newcommitlog();
        // if (lstat(version_path, &statbuf) < 0 && status != 2) {
        //     return -1; //.repo corrupted
        // }
        strcpy(f->name, name);
        strcpy(f->oripath, target_path);
        strcpy(f->path, version_path);
        strcpy(v->version, commit_name);
        v->status = status;
        addversion(f, v);
        if (status != 2)
            v->statbuf = statbuf;
        // filedir * exists = searchExistingFile(target_path);
        filedir * exists = addfiledir(f);
        
        c->flink = exists;
        c->vlink = v;
        addcommitlog(c);
        
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
    stagelog* temp = head;
    while(temp) {
        printf("%s\n", temp->log);
        temp = temp->next;
    }
}
void show_commit_log() {
    commitlog * temp = commithead;
    char curver[MAXDIR];
    strcpy(curver, commithead->vlink->version);
    while(temp) {
        printf("name : %s\n", temp->vlink->version);
        // printf("path : %s\n", temp->path);
        // printf("oripath : %s\n", temp->oripath);
        // filever * t2 = temp->top;
            strcpy(curver, temp->vlink->version);
            printf("   status : %d\n", temp->vlink->status);
            printf("   log : %s\n", temp->flink->oripath);
            // printf("   action : %d\n", temp->);
            // printf("   size : %ld\n\n", t2->statbuf.st_size);
            temp = temp->next;
    }
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
    filedir * root = newfile();
    char cwd[MAXDIR];
    strcpy(cwd, getcwd(NULL, 0));
    char * name = substr(cwd, return_last_name(cwd) + 1, strlen(cwd));
    strcpy(root->name, name);
    strcpy(root->oripath, cwd);

    version_cursor->root = root;
}