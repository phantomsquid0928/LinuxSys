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
        // printf("R> removing fi %s\n", path);
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
            // printf("R> removing fil %s\n", nextpath);
        }
    }
    rmdir(path);
    // printf("R> removing dir %s\n", path);
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

char * pidlogpath;
char * pidrootpath;

typedef struct monitorlist{
    int pid;
    char path[MAXPATH];
    struct monitorlist * next;
}monitorlist;
typedef struct filever{
    time_t vertime;
    int status; //-2 added -1: existing    1 mod 2 removed   indicates action on that version of commit.
    // char path[MAXPATH];
    struct stat statbuf;
    struct filever * next;
}filever;
typedef struct filedir {
    char name[MAXDIR];
    char oripath[MAXPATH];

    filever * head; //oldest
    filever * rear; //latest
    int chk;       //indicates current status compared to latest ver. compared to rear: chk = -1 : existing, no change   chk = 0 / -2 new    chk= 1 modify   chk= 2 remove

    int isreg; //  0 : dir, 1 : file, childscnt is not enough for distinguish file / dir

    struct filedir ** childs; //adds when newfile comes in
    int childscnt;
}filedir;


/**
 * queue
*/

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

////////////////////////queue end

monitorlist * head = NULL;
monitorlist * rear = NULL;

queue q;
queue tracked;

// filever * loghead = NULL; //
// filever * logrear = NULL;

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
void addversion(filedir * f, filever * t) {
    if (f->head == NULL) {
        f->head = t;
        f->rear = t;
        return;
    }
    f->rear->next = t;
    f->rear = t;
    return;
}



filedir * newfile() { //default creation is dir.
    filedir * temp = (filedir*)malloc(sizeof(filedir));
    temp->head = NULL;
    temp->rear = NULL;
    temp->childs = NULL;
    temp->childscnt = -1;
    temp->chk = -1; //not target
    temp->isreg = 0;
    return temp;
}
filever * newversion() {
    filever * temp = (filever*)malloc(sizeof(filever));
    temp->next = NULL;
    temp->status = -1; //existing... 0 -> -1;
    return temp;
}

filedir * addfiledir(filedir * root, filedir * target) { //filedir always comes file, root also can be file when root was tracking file
    // printf("called");
    filedir * temp = root;
    if (!strcmp(root->oripath, target->oripath)) { //must be both file.
        root->isreg = 1;
        addversion(root, target->head);
        free(target);
        return NULL;
    }
    char * relpath = substr(target->oripath, strlen(temp->oripath) + 1, strlen(target->oripath));

    // printf("%s\n", relpath);
    int res;
    char ** args = split(relpath, "/", &res);
    // if (!strcmp(target->oripath, temp->oripath)) { // root is ontrack ????
    //     free(target);
    //     return temp;
    // }
    
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
                // printf("exists!\n");
                if (i == res - 1) { //file
                    // printf("same file!");
                    addversion(temp->childs[mid], target->rear);
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
                    newfiledir->isreg = 0;
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
                newfiledir->isreg = 0;
                strcpy(newfiledir->name, args[i]);
                strcpy(newfiledir->oripath, curpath);
                temp->childs[0] = newfiledir;
    
                temp->childscnt = 0;
                temp = temp->childs[0];
            }
        }
    }

}

/// @brief 
/// @param root file or dir
/// @param starttime 
/// @param mod 0 then targetpath is filepath 1 or 2 then dirpath
/// @param targetpath 
/// @return 
int makeUnionofMockReal(filedir * root, time_t starttime, int mod, char * targetpath) { 
    struct stat statbuf;
    struct dirent ** namelist;
    int cnt;

    if ((mod & 3) == 0) {
        root->isreg = 1;
        filever * v = newversion();
        if (root->rear == NULL) {
            root->chk = -2;
            v->status = -2;
            v->vertime = starttime;
            if (lstat(root->oripath, &statbuf) < 0) {
                fprintf(stderr, "lstat error, have no permission?\n");
                exit(2);
            }
            v->statbuf = statbuf;
            addversion(root, v);
            
            return 1;
        }
        if (access(root->oripath, F_OK)) {
            if (root->rear->status == 2) {
                return 1;
            }
            root->chk = 2;
            v->status = 2;
            v->vertime = starttime;
            addversion(root, v);

            return 1;
        }
        if (lstat(root->oripath, &statbuf) < 0) {
            fprintf(stderr, "lstat error, have no permission?\n");
            exit(2);
        }
        if (root->rear->statbuf.st_size != statbuf.st_size) {
            root->chk = 1;
            v->statbuf = statbuf;
            v->status = 1;
            v->vertime = starttime;
            addversion(root, v);
            return 1;
        }
        else {
            char verpath[MAXPATH];
            
            strcpy(verpath, pidrootpath);
            strcat(verpath, "/");

            char relpath[MAXPATH];
            strcat(verpath, substr(root->oripath, return_last_name(root->oripath) + 1, strlen(root->oripath)));
            strcat(verpath, "_");

            char mask[200];
            struct tm *tmt;
            tmt = localtime(&root->rear->vertime);
            if (strftime(mask, sizeof(mask), "%Y%m%d%H%M%S", tmt) == 0) {
                fprintf(stderr, "failed to make time\n");
                exit(3);
            }
            strcat(verpath, mask);

            int res = compare_md5(root->oripath, verpath);

            if (res < 0) {
                fprintf(stderr, "error while comparing files... md5\n");
                // exit(1);ddd
            }
            if (res == 0) {
                //same
                root->chk = -1; 
                // tracked.push(&tracked, f->childs[i]); //tracked newfil
                return 1;
                // printf("%s got no change\n", child->name);
            }
            else {
                root->chk = 1; //mod
                filever * v = newversion();
                v->statbuf = statbuf;
                v->status = 1;
                v->vertime = starttime;
                addversion(root, v);
                return 1;
            }
        }
        return 1;
    }


    q.clear(&q);
    q.push(&q, root); 
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
                v->vertime = starttime;
                addversion(n, v);
                if (S_ISREG(statbuf.st_mode)) {
                    n->isreg = 1;
                    j++;
                    continue;
                }
                else if (S_ISDIR(statbuf.st_mode)) { //dir contains new
                    n->isreg = 0;
                    if ((mod & 2) != 0) //-r exists
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
                if (f->childs[i]->isreg == 0) {
                    if ((mod & 2) != 0)
                        q.push(&q, f->childs[i]);
                    i++;
                    continue;
                }

                if (f->childs[i]->rear->status == 2) {
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
                v->vertime = starttime;
                addversion(f->childs[i], v);

                i++;
                continue;
            }


            // printf("WHY");
            // printf("compareing %s %s\n", f->childs[i]->name, namelist[j]->d_name);
            int res = strcmp(f->childs[i]->name, namelist[j]->d_name);
            if (res == 0) {
                tchild[rescnt] = f->childs[i];
                rescnt++;
                
                
                if (lstat(f->childs[i]->oripath, &statbuf) < 0) {
                    printf("fk");
                    return -1;
                }
                if (S_ISDIR(statbuf.st_mode)) { //dir
                    if ((mod & 2) != 0) //-r exists
                        q.push(&q, f->childs[i]);
                    i++;
                    j++;
                    continue;
                }
                if (f->childs[i]->rear->status == 2) { //file that has same name with commited, deleted  come back here...
                    f->childs[i]->chk = -2; //new! 
                    filever * v = newversion();
                    v->statbuf = statbuf;
                    v->status = -2;
                    v->vertime = starttime;
                    addversion(f->childs[i], v);
                    i++;
                    j++;
                    continue;
                }
            


                if (statbuf.st_size != f->childs[i]->rear->statbuf.st_size) {//modified
                    f->childs[i]->chk = 1; //mod
                    filever * v = newversion();
                    v->statbuf = statbuf;
                    v->status = 1;
                    v->vertime = starttime;
                    addversion(f->childs[i], v);
                    i++;
                    j++;
                    continue;
                }
                else {

                    // printf("hle");
                    char verpath[MAXPATH];

                    strcpy(verpath, pidrootpath);
                    strcat(verpath, "/");

                    char relpath[MAXPATH];
                    sprintf(relpath, "%s", substr(f->childs[i]->oripath, strlen(root->oripath) + 1, strlen(f->childs[i]->oripath)));
                    strcat(verpath, relpath);
                    strcat(verpath, "_");

                    char mask[200];
                    struct tm *tmt;
                    tmt = localtime(&f->childs[i]->rear->vertime);
                    if (strftime(mask, sizeof(mask), "%Y%m%d%H%M%S", tmt) == 0) {
                        fprintf(stderr, "failed to make time\n");
                        exit(3);
                    }
                    strcat(verpath, mask);

                    int res = compare_md5(f->childs[i]->oripath, verpath);

                    if (res < 0) {
                        fprintf(stderr, "error while comparing files... md5\n");
                        // exit(1);
                    }
                    if (res == 0) {
                        //same
                        f->childs[i]->chk = -1; 
                        // tracked.push(&tracked, f->childs[i]); //tracked newfile
                        i++;
                        j++;
                        continue;
                        // printf("%s got no change\n", child->name);
                    }
                    else {
                        f->childs[i]->chk = 1; //mod
                        filever * v = newversion();
                        v->statbuf = statbuf;
                        v->status = 1;
                        v->vertime = starttime;
                        addversion(f->childs[i], v);
                        i++;
                        j++;
                        continue;
                    }
                }
            }
            if (res < 0) { //removed staged / unstaged
                tchild[rescnt] = f->childs[i];
                rescnt++;
                if (f->childs[i]->isreg == 0) { //dir that contains removed
                    if ((mod & 2) != 0)
                        q.push(&q, f->childs[i]);
                    i++;
                    continue;
                }

                if (f->childs[i]->rear->status == 2) {
                    f->childs[i]->chk = -1; //removed long time ago and keeping its state
                    i++;
                    continue;
                }

                f->childs[i]->chk = 2; //removed right before.

                filever * v = newversion();
                v->status = 2;
                v->vertime = starttime;
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
                v->vertime = starttime;
                addversion(n, v);
                if (S_ISREG(statbuf.st_mode)) {
                    n->isreg = 1;
                    n->chk = -2;
                    j++;
                    continue;
                }
                else if (S_ISDIR(statbuf.st_mode)) { //dir contains new
                    n->isreg = 0;
                    if ((mod & 2) != 0)
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
    }
    // free(&q);
    return 1;
}

/// @brief final call to store filedirs to tracked / untracked queue. initstatus() -
/// @brief -> makeUnionofMockReal -> managelogrecurs is must to call
/// @param mod mod 0 : for status 1 for commit
/// @return succed
int store2pockets(filedir * root) {
    // queue q = *initQueue();
    
    if (root->isreg == 1){
        if(root->chk != -1) 
            tracked.push(&tracked, root);
        return 1;
    }
    //root is ignored.
    q.clear(&q);
    q.push(&q, root);

    while(!q.empty(&q)) {
        filedir * cur = q.front(&q);
        q.pop(&q);

        for (int i= 0; i<= cur->childscnt; i++) {
            filedir * child = cur->childs[i];
            if (child->isreg == 0) { //dir
                q.push(&q, child);
                continue;
            }
            
            if (child->chk == -1) continue;
            tracked.push(&tracked, child);
        }
    }
    // free(&q);
    return 1;
}

// void show_fs(filedir * cur, char * padding) {
//     // if (cur->childscnt == -1) { //file
//     //     printf(" file");
//     //     printf(" latest : %s\n", cur->top->version);
//     //     return;
//     // }
//     printf("%s", cur->name);
//     printf("  dir %d %p\n", cur->childscnt, cur);
//     for (int i = 0;i < cur->childscnt + 1; i++) {
//         char curpad[1000];
//         char nextpad[1000];
//         if (i == cur->childscnt) {
//             sprintf(curpad, "%s   └─", padding);
//             sprintf(nextpad, "%s    ", padding);
//         }
//         else {
//             sprintf(curpad, "%s   ├─", padding);
//             sprintf(nextpad, "%s   │ ", padding);
//         }
        
//         if (cur->childs[i]->isreg == 1) { //file
//             printf("%s%s", curpad, cur->childs[i]->name);
//             printf(" file");
//             printf(" latest : %s %p    chk ;%d status: %d\n", ctime(&cur->childs[i]->rear->vertime)
//             , cur->childs[i], cur->childs[i]->chk, cur->childs[i]->rear->status);
//             continue;
//         }
//         else {
//             printf("%s/", curpad);
//             show_fs(cur->childs[i], nextpad);
//         }
        
//     }
// }

void show_fs_all_mod(filedir * cur, char * padding, int first) {
    if (cur->isreg == 1) { //file
        if (first == 1) printf("%s\n", cur->oripath);
        else printf("%s\n", cur->name);
        // printf(" file");
        // printf(" latest : %s\n", cur->rear-);
        filever * v = cur->head;
        while(v) {
            char lastpad[1000];
            if (v->next == NULL) {
                sprintf(lastpad, "%s   └─", padding);
            }
            else {
                sprintf(lastpad, "%s   ├─", padding);
            }
            char buf[200];
            struct tm * tmt;
            tmt = localtime(&v->vertime);
            if (strftime(buf, sizeof(buf), "%Y-%m-%d %T", tmt) == 0) {
                fprintf(stderr, "failed to make time\n");
                exit(3);
            }
            printf("%s  %s : %d %ld\n", lastpad, buf, v->status, v->vertime);
            v = v->next;
        }
        return;
    }
    if (cur->isreg == 0 && cur->childscnt != -1) {
        if (first == 1) printf("%s\n", cur->oripath);
        else printf("%s\n", cur->name);
        // printf("  dir\n");
    }
    for (int i = 0;i < cur->childscnt + 1; i++) {
        char curpad[1000];
        char nextpad[1000];
        if (i == cur->childscnt) {
            sprintf(curpad, "%s   └─", padding);
            sprintf(nextpad, "%s     ", padding);
        }
        else {
            sprintf(curpad, "%s   ├─", padding);
            sprintf(nextpad, "%s   │ ", padding);
        }
        
        if (cur->childs[i]->isreg == 1) { //file
            printf("%s", curpad);
        }
        else {
            if (cur->childs[i]->childscnt == -1) continue;
            printf("%s/", curpad);
        }
        show_fs_all_mod(cur->childs[i], nextpad, 0); 
    }
}

void init_fs() {
    q = *initQueue();
    tracked = *initQueue();
}
void init_pid(int pid) {
    char temproot[4096];
    sprintf(temproot, "%s/%d", backuppath, pid);
    if (access(temproot, F_OK)) mkdir(temproot, 0777);
    pidrootpath = NULL;
    pidrootpath = substr(temproot, 0, strlen(temproot));
    strcat(temproot, ".log");
    pidlogpath = NULL;
    pidlogpath = substr(temproot, 0, strlen(temproot));
}

void init() {
    homepath = getenv("HOME");
    char temp[4096];
    strcpy(temp, homepath);
    strcat(temp, "/backup");
    backuppath = substr(temp, 0, strlen(temp));

    if (access(backuppath, F_OK)) mkdir(backuppath, 0777);

    sprintf(logpath, "%s/%s", backuppath, "monitor_list.log");

    int fd;
    if (access(logpath, F_OK)) {
        if ((fd = open(logpath, O_CREAT, 0777)) < 0) fprintf(stderr, "creat error");
        close(fd);
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
    char line[MAXPATH * 2];
    while(1) {
        if (fgets(line, MAXPATH * 2, fp) == NULL) break;
        sscanf(line, "%[^\n^\r]", buf);
        // int len = fscanf(fp, "%[^\n^\r]", buf);
        // if (len == EOF) break;
        // fgetc(fp);
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
void load_pid_log(filedir * root, int pid) { ///for list
    char targetpath[MAXPATH];
    FILE * fp;

    sprintf(targetpath, "%s/%d.log", backuppath, pid);
    if ((fp = fopen(targetpath, "r")) == NULL) {
        fprintf(stderr, "error on fopen");
        exit(1);
    }
    char buf[MAXPATH * 2];
    while(1) {
        memset(buf, 0, sizeof(buf));
        int len = fscanf(fp, "%[^\n^\r]", buf);
        if (len == EOF) break;
        fgetc(fp);
        int res = 0;

        char ** argv = split(buf, "[]", &res);
        //args0 = time_t 1 = status 2 = path
        filedir * f = newfile();
        filever * v = newversion();
        char * name = substr(argv[2], return_last_name(argv[2]) + 1, strlen(argv[2]));
        strcpy(f->oripath, argv[2]);
        strcpy(f->name, name);
        f->isreg = 1;
        if(!strcmp(argv[1], "create")) v->status = -2;
        if(!strcmp(argv[1], "modify")) v->status = 1;
        if(!strcmp(argv[1], "delete")) v->status = 2;

        struct tm tmt = {0};
        if (strptime(argv[0], "%Y-%m-%d %T", &tmt) == NULL) {
            fprintf(stderr, "log file corrupted, time\n");
            exit(100);
        }
        v->vertime = mktime(&tmt);

        addversion(f, v);
        addfiledir(root, f);
        for (int i =0 ;i < res; i++) {
            free(argv[i]);
        }
    }
    fclose(fp);
}
void save_pid_log(int pid, int status, char * tpath, char * timemask) { //for pid daemon
    FILE * fp;

    if ((fp = fopen(pidlogpath, "a+")) == NULL) {
        fprintf(stderr, "error on fopen\n");
        exit(1);
    }
    time_t t;
    time(&t);
    char * statusstr;
    if (status == -2) {
        statusstr = "create";
    }
    else if (status == 1) {
        statusstr = "modify";
    }
    else if (status == 2) {
        statusstr = "delete";
    } else exit(44);

    fprintf(fp, "[%s][%s][%s]\n", timemask, statusstr, tpath);
    fclose(fp);
}