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

#include <openssl/md5.h>

#define MAXPATH 4096
#define MAXDIR 256

char commitlogpath[MAXPATH];
char staginglogpath[MAXPATH];
char repopath[MAXPATH];

const int commandscnt = 8;

char * commandsList[] = {"add", "remove", "status", "commit", "revert", "log", "help", "exit"};

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
char * purifypath(char * path) {
    int cnt = 0;
    int mod = 0;
    char * newpath = (char*)malloc(sizeof(char) * (sizeof(path) + 1));
    
    if (strstr(path, "\"") == NULL && strstr(path, "'") == NULL) {
        return realpath(path, 0);
    }
    if (strstr(path, ".") != NULL) //relpath
    {
        mod = 1;
    }
    printf("here");
    strcpy(newpath, "");
    char * arg = strtok(path, "/");
    char * argtemp;
    int chk = 1;
    while(arg != NULL) {
    	if (strstr(arg, "\"") != NULL || strstr(arg, "'") != NULL) {
    		char * erased = substr(arg, 1, strlen(arg) - 1);
            if (mod == 0 || chk == 0) {
    		    strcat(newpath, "/");
                mod = 1;
            }
            chk = 0;
    		strcat(newpath, erased);
    		arg = strtok(NULL, "/");
    		continue;
		}
        if (mod == 0 || chk == 0) {
		    strcat(newpath, "/");
            mod = 1;
        }
        chk = 0;
        
		strcat(newpath, arg);
    	arg = strtok(NULL, "/");
	}
    return newpath;
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
    int status;     //-2 added -1: existing    1 mod 2 removed   indicates action on that version of commit.
    struct stat statbuf;
    struct filever * next;
}filever;

/// @brief 
typedef struct filedir {
    char name[MAXDIR];
    char path[MAXPATH];// version path
    char oripath[MAXPATH]; //original path
    filever * top; //latest
    int chk;       //indicates whether it is target of commit: compared to top: chk = -1 : existing, no change   chk = 0 new    chk= 1 modify   chk= 2 remove
    int istrack;   //chk whether this file is being tracked 0 : no 1 : yes
    filever * target; //use when make revert?

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
    char latestver[MAXDIR];
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


filedir * search_filedir(char * target);

/// @brief addlogre + dellogre = managelogre with bitmasking, 
/// @brief istrack = add->or with 1  del -> and op with 0. only for loadstaginglog.
/// @details get dir/filepath, root(cwd) chk if curpath == target-> access childs-> keep doing...
/// @param target filepath or dirpath
/// @param mod 0 add 1 del
/// @return succeed?
int managelogrecurs(char * target, int mod) {
    int res = 0;
    int cnt;
    queue q = *initQueue();

    // char * cwd = getcwd(NULL, 0);
    filedir * f = search_filedir(target); //logw logn
    // printf("foudn : %s\n\n", f->oripath);
    if (f == NULL) return 0;

    if (f->childscnt == -1) {
        if(mod == 0) {//add
            res += (f->istrack == 0) ? 1 : 0;
            f->istrack = 1;
        } 
        if (mod == 1) {//remove
            res += f->istrack;
            f->istrack = 0;
        }
        
        // printf("~~ing %s\n", f->oripath);
        return res;
    }
    q.push(&q, f);
    while(!q.empty(&q)) {
        filedir * cur = q.front(&q);
        q.pop(&q);
        
        for (int i =0 ; i<= cur->childscnt; i++) {
            filedir * next = cur->childs[i];
            
            if (next->childscnt != -1) {
                q.push(&q, next);
                continue;
            }
            if(mod == 0) {//add
                res += (next->istrack == 0) ? 1 : 0;
                next->istrack = 1;
                // printf("adding %s\n", next->oripath);
                addlog(next->oripath);
            }
            if (mod == 1) {//remove
                res += next->istrack;
                next->istrack = 0;
                // printf("removing %s\n", next->oripath);
                dellog(next->oripath);
            }
            
        }
    }

    return res;
}

/// @deprecated no longer used, replaced to managelogrecurs routine.
/// @brief addlog recursively which exists in realfs.
/// @param target 
/// @return succeed?
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

/// @deprecated no longer used
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
    temp->chk = -1; //not target
    temp->istrack = 0; //no tracking
    return temp;
}
filever * newversion() {
    filever * temp = (filever*)malloc(sizeof(filever));
    temp->next = NULL;
    temp->status = -1; //existing... 0 -> -1;
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
/// @param target 
/// @return 
/// @todo mod
filedir * addfiledir(filedir * target) { //always comes file
    // printf("called");
    filedir * temp = version_cursor->root;
    char * relpath = substr(target->oripath, strlen(temp->oripath) + 1, strlen(target->oripath));

    // printf("%s\n", relpath);
    int res;
    char ** args = split(relpath, "/", &res);
    if (!strcmp(target->oripath, temp->oripath)) { // root is ontrack
        temp->istrack = 1;
        free(target);
        return temp;
    }
    
    char curpath[MAXPATH];
    strcpy(curpath, temp->oripath); //root
    for(int i= 0;i < res; i++) {
        // printf("cur : %s\n", temp->oripath);
        strcat(curpath, "/");
        strcat(curpath, args[i]);
        // printf("path: %s\n", curpath);
        if (temp->childscnt != -1) {
            // for (int j = 0; j < temp->childscnt + 1; j++) {
            //     printf("member : %s\n", temp->childs[j]->name);
            // }
            // printf("\n");
            int start = 0;
            int end = temp->childscnt + 1;
            int chk = 0;
            for (int j = 0; j < 30; j++) { //can handle 2 ^ 30filedirs in same dir, may cause err
                int mid = start + end >> 1;
                int res = strcmp(temp->childs[mid]->name, args[i]);
                // printf("res : %d %d %d, %s  :  %s\n",res, start, end, temp->childs[mid]->name, args[i]);
                if (res == 0) { //there is same one, found real quick
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
                int mid = start + end >> 1;
                //same, dir or file exists
                printf("exists!\n");
                if (i == res - 1) { //file
                    addversion(temp->childs[mid], target->top);
                    free(target);
                    return temp->childs[mid];
                }
                else { //dir
                    temp = temp->childs[mid];
                }
                
            }
            else { //file or dir not exists
                // printf("here, %d %d", temp->childscnt, end);
                temp->childs = (filedir **)realloc(temp->childs, sizeof(filedir*) * (temp->childscnt + 2));// 3-> 4ro  5

                for (int j = temp->childscnt; j >= end; j--) {
                    temp->childs[j + 1] = temp->childs[j];
                }
                if (i == res - 1) //no file, adding...
                {
                    // if (mod == 0) {//from commit
                    temp->childs[end] = target;
                    temp->childscnt++;
                    // printf("res\n");
                    return target;
                }
                else { //no dir...
                    // printf("seeking\n");
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
                // printf("adding bare file\n\n");
                temp->childs = (filedir**)malloc(sizeof(filedir*));
                temp->childs[0] = target;
                
                temp->childscnt = 0;
                return target;
            }
            else { // not yet, keep adding empty dir
                // printf("adding new dir\n");
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
/// @brief find filedir that has oripath, O(logwlogn)
/// @param oripath from staginglog, search filedir. -> no file : newfile staged, yes file : just staged.
/// @return fildir 
filedir * search_filedir(char * oripath) { 
    filedir * temp = version_cursor->root;
    char * relpath = substr(oripath, strlen(temp->oripath), strlen(oripath));

    // printf("ffffs]%s\n", relpath);
    int res;
    char ** args = split(relpath, "/", &res);
    
    char curpath[MAXPATH];
    strcpy(curpath, temp->oripath); //root
    for (int i = 0; i < res; i++) {
        // printf("cur : %s\n", temp->oripath);
        strcat(curpath, "/");
        strcat(curpath, args[i]);
        // printf("path: %s\n", curpath);

        int start = 0;
        int end = temp->childscnt + 1;
        int chk = 0;
        for (int j = 0; j < 30; j++) {
            int mid = start + end >> 1;
            int res = strcmp(temp->childs[mid]->name, args[i]);
            // printf("res : %d %d %d, %s  :  %s\n",res, start, end, temp->childs[mid]->name, args[i]);
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
            int mid = start + end >> 1;
            if (i == res - 1) { //found file / dir
                return temp->childs[mid]; //found result
            }
            else {
                temp = temp->childs[mid];
            }
        }
        else {
            return NULL;
        }
    }
    return temp; //root, as root is res = 0, comes right here
}



void show_fs(filedir * cur, char * padding) {
    // if (cur->childscnt == -1) { //file
    //     printf(" file");
    //     printf(" latest : %s\n", cur->top->version);
    //     return;
    // }
    printf("%s", cur->name);
    printf("  dir %p\n", cur);
    for (int i = 0;i < cur->childscnt + 1; i++) {
        char curpad[1000];
        char nextpad[1000];
        if (i == cur->childscnt) {
            sprintf(curpad, "%s   └─", padding);
            sprintf(nextpad, "%s    ", padding);
        }
        else {
            sprintf(curpad, "%s   ├─", padding);
            sprintf(nextpad, "%s   │ ", padding);
        }
        
        if (cur->childs[i]->childscnt == -1) { //file
            printf("%s%s", curpad, cur->childs[i]->name);
            printf(" file");
            printf(" latest : %s %p    chk ;%d status: %d istracking : %d\n", cur->childs[i]->top->version
            , cur->childs[i], cur->childs[i]->chk, cur->childs[i]->top->status, cur->childs[i]->istrack);
            continue;
        }
        else {
            printf("%s/", curpad);
            show_fs(cur->childs[i], nextpad);
        }
        
    }
}


queue q;
queue tracked;
int plus;
int minus;
queue untracked;
void initstatus(){
    q = *initQueue();
    tracked = *initQueue();
    untracked = *initQueue();
    plus = 0;
    minus = 0;
}

/// @brief initstatus() is essential, cmps log files and real files, then merge them into 1
/// @return 
int makeUnionofMockReal() {
    struct stat statbuf;
    struct dirent ** namelist;
    int cnt;


    filedir * root = version_cursor->root;
    
    q = *initQueue();
    tracked = *initQueue();
    untracked = *initQueue();


    q.push(&q, root); //commit 한번 돌기-> commit할 파일 표시 -> 다시 돌기 -> 실제파일과 비교
    while(!q.empty(&q)) { // two pointer 사용 필요
        filedir * f = q.front(&q);
        q.pop(&q);

        int isbonked = 0;
        if (access(f->oripath, F_OK)) { //dir / file bonked...
            isbonked = 1;
        }
        if ((cnt = scandir(f->oripath, &namelist, NULL, alphasort)) < 0) { //whole dir erased
            if (isbonked != 1) {
                return -1;
            }
            cnt = 0;
        }

        //
        int i = 0, j = 0;
        int rescnt = 0;
        filedir ** tchild = (filedir **)malloc(sizeof(filedir *) * (f->childscnt + 1 + cnt)); //temp size.
        while(1) {
            if (j < cnt && (!strcmp(namelist[j]->d_name, ".") || !strcmp(namelist[j]->d_name, ".."))) {
                j++;
                continue;
            }
            if (j < cnt && !strcmp(namelist[j]->d_name, ".repo") && !strcmp(f->oripath, getcwd(NULL, 0))) {
                j++;
                continue;
            }
            if (i > f->childscnt) { //new
                if (j == cnt) break;
                // printf("new!");
                filedir * n = newfile();
                n->chk = -2;
                char nextpath[MAXPATH];
                strcpy(nextpath, f->oripath);
                strcat(nextpath, "/");
                strcat(nextpath, namelist[j]->d_name);
                strcpy(n->oripath, nextpath);
                strcpy(n->name, namelist[j]->d_name);

                filever * v = newversion();
                tchild[rescnt] = n;

                rescnt++;

                if (lstat(nextpath, &statbuf) < 0) {
                    printf("sssssss");
                    return -1;
                }
                v->statbuf = statbuf;
                v->status = -2;
                addversion(n, v);
                if (S_ISREG(statbuf.st_mode)) {
                    j++;
                    continue;
                }
                else if (S_ISDIR(statbuf.st_mode)) { //dir contains new
                    q.push(&q, n);
                    j++;
                    continue;
                }
                else {
                    printf("sss");
                    return -1;
                }
            }
            if (j == cnt) { // removed, i
                if (i > f->childscnt) break;
                tchild[rescnt] = f->childs[i]; 
                rescnt++;
                if (f->childs[i]->childscnt != -1) {
                    q.push(&q, f->childs[i]);
                    i++;
                    continue;
                }

                if (f->childs[i]->top->status == 2) {
                    f->childs[i]->chk = -1; //removed long time ago and keeping its state
                    i++;
                    continue;
                }
                else {
                    f->childs[i]->chk = 2; //removed right before.
                }
                f->childs[i]->chk = 2;

                filever * v = newversion();
                v->status = 2;
                addversion(f->childs[i], v);

                i++;
                continue;
            }


            // printf("WHY");
            printf("compareing %s %s\n", f->childs[i]->name, namelist[j]->d_name);
            int res = strcmp(f->childs[i]->name, namelist[j]->d_name);
            if (res == 0) {
                tchild[rescnt] = f->childs[i];
                rescnt++;
                if (f->childs[i]->childscnt != -1) { //dir
                    q.push(&q, f->childs[i]);
                    i++;
                    j++;
                    continue;
                }
                // if (access(f->childs[i]->oripath, F_OK)) { //removed
                //     printf("YOU CALLED?");
                //     if (f->childs[i]->top->status == 2) {
                //         f->childs[i]->chk = -1; //removed long time ago and keeping its state
                //         i++;
                //         j++;
                //         continue;
                //     }
                //     else {
                //         f->childs[i]->chk = 2; //removed right before.
                //     }
                    
                //     filever * v = newversion();
                //     v->status = 2;
                //     addversion(f->childs[i], v);
                //     i++;
                //     j++;
                //     continue;
                // }
                if (lstat(f->childs[i]->oripath, &statbuf) < 0) {
                    printf("fk");
                    return -1;
                }
                if (f->childs[i]->top->status == 2) { //file that has same name with commited, deleted  come back here...
                    f->childs[i]->chk = -2; //new! 
                    filever * v = newversion();
                    v->statbuf = statbuf;
                    v->status = -2;
                    addversion(f->childs[i], v);
                    i++;
                    j++;
                    continue;
                }
                if (statbuf.st_mtime != f->childs[i]->top->statbuf.st_mtime) {//modified
                    f->childs[i]->chk = 1; //mod
                    filever * v = newversion();
                    v->statbuf = statbuf;
                    v->status = 1;
                    addversion(f->childs[i], v);
                    i++;
                    j++;
                    continue;
                }
                else {
                    /**
                     * TODO: md5 here
                    */
                    f->childs[i]->chk = -1; 
                    // tracked.push(&tracked, f->childs[i]); //tracked newfile
                    i++;
                    j++;
                    continue;
                }
            }
            if (res < 0) { //removed staged / unstaged
                tchild[rescnt] = f->childs[i];
                rescnt++;
                if (f->childs[i]->childscnt != -1) { //dir that contains removed
                    q.push(&q, f->childs[i]);
                    i++;
                    continue;
                }

                if (f->childs[i]->top->status == 2) {
                    f->childs[i]->chk = -1; //removed long time ago and keeping its state
                    i++;
                    continue;
                }

                f->childs[i]->chk = 2; //removed right before.

                filever * v = newversion();
                v->status = 2;
                addversion(f->childs[i], v);

                i++;
                continue;
            }
            if (res > 0) { //new unstaged.
                filedir * n = newfile();
                char nextpath[MAXPATH];
                strcpy(nextpath, f->oripath);
                strcat(nextpath, "/");
                strcat(nextpath, namelist[j]->d_name);
                strcpy(n->oripath, nextpath);
                strcpy(n->name, namelist[j]->d_name);
                filever * v = newversion();

                tchild[rescnt] = n;
                rescnt++;

                if (lstat(nextpath, &statbuf) < 0) {
                    printf("ff");
                    exit(1);
                }
                v->statbuf = statbuf;
                v->status = -2;
                addversion(n, v);
                if (S_ISREG(statbuf.st_mode)) {
                    n->chk = -2;
                    j++;
                    continue;
                }
                else if (S_ISDIR(statbuf.st_mode)) { //dir contains new
                    q.push(&q, n);
                    j++;
                    continue;
                }
                else {
                    printf("faa");
                    exit(1);
                }
            }
        }

        //store result tchild to f->childs.

        f->childs = (filedir**)realloc(f->childs, sizeof(filedir*) * rescnt);
        f->childscnt = rescnt - 1;
        for (int i =0 ;i < rescnt; i++) {
            f->childs[i] = tchild[i];
            // printf("%d ", f->childs[i]->chk);
        }
        // free(tchild);
    }
    // free(&q);
    return 1;
}
/// @brief final call to store filedirs to tracked / untracked queue. initstatus() -
/// @brief -> makeUnionofMockReal -> managelogrecurs is must to call.
/// @return succed
int store2pockets() {
    filedir * root = version_cursor->root;

    q = *initQueue();
    q.push(&q, root);
    //root is ignored.

    while(!q.empty(&q)) {
        filedir * cur = q.front(&q);
        q.pop(&q);

        for (int i= 0; i<= cur->childscnt; i++) {
            filedir * child = cur->childs[i];
            if (child->childscnt != -1) { //dir
                q.push(&q, child);
                continue;
            }
            
            if (child->istrack == 1) {
                if (child->chk == -1) continue;
                tracked.push(&tracked, child);
                if (child->chk == -2) {//new
                    plus += child->top->statbuf.st_size;
                }
                if (child->chk == 1) { //mod
                    filever * prev = child->top->next;
                    int diff = child->top->statbuf.st_size - prev->statbuf.st_size;
                    if (diff > 0) {
                        plus += diff;
                    }
                    else {
                        minus -= diff;
                    }
                }
                if (child->chk == 2) {//del
                    filever * prev = child->top->next;
                    minus += prev->statbuf.st_size;
                }
            }
            else {
                if (child->chk == -1) continue;
                untracked.push(&untracked, child);
            }
        }
    }
    return 1;
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

int rmdirs(char * path) { //danger?
    struct dirent ** namelist;
    struct stat statbuf;
    
    if (lstat(path, &statbuf) < 0) return -1;
    if (S_ISREG(statbuf.st_mode)) {
        // remove(path);
        printf("R> removing fi %s\n", path);
    }
    if (!S_ISDIR(statbuf.st_mode)) {
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
            // remove(nextpath);
            printf("R> removing fil %s\n", nextpath);
        }
    }
    // return rmdir(path);
    printf("R> removing dir %s\n", path);
    return 0;
}


/// @brief NOW U HAVE TO CALL THIS AFTER make()  231 routine
/// @return success 0 else minus values
int load_staging_log() {
    char * cwd = getcwd(NULL, 0);
    // printf("%s", stagelogpath);
    if (access(staginglogpath, F_OK)) return -1;
    char buf[MAXPATH * 2];
    FILE * fp;

    if ((fp = fopen(staginglogpath, "rt")) == NULL) {
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
            managelogrecurs(args[1], 0);
        }
        else if (!strcmp(args[0], "remove ")) {
            managelogrecurs(args[1], 1);
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
    if (access(commitlogpath, F_OK)) return -1;
    char buf[MAXPATH * 2];
    FILE * fp;

    if ((fp = fopen(commitlogpath, "rt")) == NULL) {
        return -2;
    }
    while(1) {
        int res = fscanf(fp, "%[^\n]", buf);
        if (res == EOF) break;
        fgetc(fp);
        int argc = 0;
        // printf("%s\n", buf);
        char ** args;
        args = split(buf, "-", &argc);

        char * commit_name = substr(args[0], 9, strlen(args[0]) - 2);
        char * target_temp = NULL;
        int status = -1;
        // printf("commit_name : :%s:\n", commit_name);
        if (strstr(args[1], "new file:") != NULL) {
            target_temp = substr(args[1], 11, strlen(args[1]));
            // printf("  new : :%s:\n", target_name);
            // char * name = substr(target_path, );
            status = -2;
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
        if (lstat(version_path, &statbuf) < 0 && status != 2) {
            return -1; //.repo corrupted
        }
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
    if (access(staginglogpath, F_OK)) return -1;
    char buf[MAXPATH * 2];
    FILE * fp;

    if ((fp = fopen(staginglogpath, "at")) == NULL) {
        return -2;
    }
    fprintf(fp, "%s \"%s\"\n", mod == 1 ? "add" : "remove", abpath);
}
int save_commit_log(char * stamp, char * oripath, int mod) {
    FILE * fp = fopen(commitlogpath, "a+");
    char * msg;
    if (mod == -2) msg = "new file: ";
    else if (mod == 1) msg = "modified: ";
    else if (mod == 2) msg = "removed: ";
    else {
        return -1;
    }
    fprintf(fp, "commit: \"%s\" - %s%s\n", stamp, msg, oripath);
    fclose(fp);
    return 0;
}
void show_staging_log() {
    stagelog* temp = head;
    while(temp) {
        printf("%s\n", temp->log);
        temp = temp->next;
    }
}
int show_commit_log(char * version) {
    commitlog * temp = commithead;
    
    struct stat statbuf;
    char curver[MAXDIR];
    int chk = 0;


    while(temp) {
        strcpy(curver, temp->vlink->version);
        // printf("%s %s", version, curver);
        if (version == NULL || !strcmp(version, curver)) {
            chk = 1;
            printf("commit: %s\n", temp->vlink->version);
            while(1) {
                if (temp == NULL || strcmp(curver, temp->vlink->version) != 0) break;
                strcpy(curver, temp->vlink->version);
                char tempstr[1000];
                if (temp->vlink->status == 0) strcpy(tempstr, "new file");
                if (temp->vlink->status == 1) strcpy(tempstr, "modified");
                if (temp->vlink->status == 2) strcpy(tempstr, "removed");
                printf("-  %s: %s\n", tempstr, temp->flink->oripath);
                // printf("   log : %s\n", temp->flink->oripath);
                // printf("   p : %p\n", temp->flink);
                // printf("   f : %p\n", temp->vlink);
                // printf("   action : %d\n", temp->);
                // printf("   size : %ld\n\n", t2->statbuf.st_size);
                temp = temp->next;
            }
            printf("\n");
        }
        if (version != NULL) temp = temp->next;
    }
    if (chk == 0) return 0; //no commit log
    return 1;
}

void addhelp() {
    printf("add <PATH> : record path to staging area, path will tracking modification\n");;
}
void removehelp() {
    printf("remove <PATH> : record path to staging area, path will not tracking modification\n");
}
void statushelp() {
    printf("status : show staging area status\n");
}
void commithelp() {
    printf("commit <NAME> : backup staging area with commit name\n");
}
void reverthelp() {
    printf("revert <NAME> : recover commit version with commit name\n");
}
void loghelp() {
    printf("log : show commit log\n");
}
void helphelp() {
    printf("help : show commands for program\n");
}
void exithelp() {
    printf("exit : exit program\n");
}

void (*helpfuncs[])(void) = {addhelp, removehelp, statushelp, commithelp, reverthelp, loghelp, helphelp, exithelp};

void init() {
    char * cwd = getcwd(NULL, 0);
    if (cwd == NULL) exit(100);
    sprintf(commitlogpath, "%s/.repo/.commit.log", cwd);
    sprintf(staginglogpath, "%s/.repo/.staging.log", cwd);
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