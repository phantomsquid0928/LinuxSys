#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <wait.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <openssl/md5.h>  

#define MAXPATH 4096
#define MAXDIR 255
// #define 
typedef struct backupNode {
  char backupPath[MAXPATH];
  char oripath[MAXPATH];
  struct stat statbuf;
  char stamp[MAXDIR];

  struct backupNode *next;
} backupNode;

typedef struct filedir {
    char path[MAXPATH];
    char name[MAXDIR];
    struct stat statbuf;
    // char backupPath[MAXPATH];
    backupNode * head;
    struct filedir ** childs;
    int childscnt; //file : -1, dir : many
}filedir;

typedef struct dirpoint { //uses for faster dir access
    filedir * node;
    struct dirpoint * next;
}dirpoint;

typedef struct dirList {  //add all file, dir
  struct dirpoint *head;
  struct dirpoint *tail;
  int size;
} dirList;

/**
 * added for test
*/
typedef struct pathpair { //get from log file
    char stamp[4096];
    char oripath[4096];
}pathpair;

static pathpair pairs[10] = { //change to linkedlist
    {"34566669", "home/ph/linuxhw/b"}, //home/ph/b/b.txt ~ t
    {"34434434", "home/ph/linuxhw/b/새 폴더"}, //home/ph/b backukped to 2343434
    {"34566666", "home/ph/linuxhw/b"}, //home/ph/b ~ to 2334356
};
/// @brief added for pairs test
/// @param name 
/// @param res 
/// @return 1 : exists 0 : not exists, return res //
int find_link(char * name, char * res) {
    for (int i =0 ; i< 10; i++) { //10 = pairs cnt 
        if (!strcmp(pairs[i].stamp, name)) {
            strcpy(res, pairs[i].oripath);
            return 1;
        }
    }
    return 0;
}

void addbackup(filedir*t, backupNode * b);
int show_all();
dirList *mainDirList = NULL;
void addDirList(filedir *t) { //중복 들어올 시 백업만 먹고 까버리기 필요  
    // dirpoint * temp = mainDirList->head;
    // dirpoint * point = (dirpoint*)malloc(sizeof(dirpoint));
    // point->node = t;
    // point->next = NULL;
    // if (temp == NULL) { //empty
    //     mainDirList->head = point;
    //     mainDirList->tail = point;
    //     mainDirList->size = 1;
    //     return;
    // }
    // mainDirList->tail->next = point;
    // mainDirList->tail = point;
    // mainDirList->size++;
    printf("called");
    dirpoint * temp = mainDirList->head;
    int flag = 0;
    printf("adding %s\n", t->name);
    if (mainDirList->size == 0) {
        dirpoint *newp = (dirpoint*)malloc(sizeof(dirpoint));
        newp->next = NULL;
        newp->node = t;
        mainDirList->head = newp;
        mainDirList->tail = newp;
        mainDirList->size++;
        printf("ff");
        return;
    }
    char t1[4096];
    char t2[4096];
    while(temp) {
        memset(t1, 0,sizeof(t1));
        memset(t2, 0,sizeof(t2));
        printf(",");
        if (temp->node == NULL) {
            printf("ffffffffff");
        }
        strcpy(t1, temp->node->path);
        strcpy(t2, t->path);
        if (!strcmp(t1, t2)) { //dup
            printf("1");
            flag = 1;
            break;
        }
        temp = temp -> next;
    }
    if (flag == 0) //new
    {
        printf("here");
        dirpoint * newp = (dirpoint*)malloc(sizeof(dirpoint));
        newp->next = NULL;
        newp->node = t;
        mainDirList->tail->next = newp;
        mainDirList->tail = newp;
        mainDirList->size++;
        printf("success %s %s\n", t->name, t->path);
    }
    else { // dup, chk if it is file or dir
        printf("???");
        if (t->childscnt != -1) { // dir, 새 파일차일드만 투포인터로 갱신후 날리기
            filedir * exists = temp->node;
            filedir ** templist = (filedir**)malloc(sizeof(filedir*) * (exists->childscnt + t->childscnt + 2));
            int rescnt = 0;
            for (int i =0 ;i <= exists->childscnt; i++) {
                printf("   %s\n", exists->childs[i]->path);
            }
            for (int i = 0; i <= t->childscnt; i++) {
                printf("+++%s\n", t->childs[i]->path);
            }
            for (int i= 0, j= 0;;) {
                if (i > exists->childscnt) {
                    if (j > t->childscnt) break;
                    templist[rescnt++] = t->childs[j++];
                    continue;
                }
                if (j > t->childscnt) {
                    if (i > exists->childscnt) break;
                    templist[rescnt++] = exists->childs[i++];
                    continue;
                }
                int res = strcmp(exists->childs[i]->path, t->childs[j]->path);
                if (res > 0) {
                    templist[rescnt++] = t->childs[j++];
                }
                if (res == 0) {
                    templist[rescnt++] = exists->childs[i];
                    i++;
                    j++;
                }
                if (res < 0) {
                    templist[rescnt++] = exists->childs[i++];
                }
            }
            printf("DEV] two pointer res \n");
            exists->childscnt = rescnt-1;
            exists->childs = realloc(exists->childs, rescnt);
            for (int i = 0; i< rescnt; i++) {
                exists->childs[i] = templist[i];
                printf("%s\n",templist[i]->name);
            }
            
            /**
             * TODO: add two pointer templist to original childs
            */
            addbackup(exists, t->head);//dir backup add
            free(templist);
            free(t->childs); ///////// HAZARD
            // free(t);
            //two pointer
        }
        else { // file, get new backup and discard filedir
            filedir * exists = temp->node;
            addbackup(exists, t->head);
            free(t);
            free(t->childs);
        }
    }
    
}
void removeDirList(filedir *t) { //file -> 백업다 까버리기 dir -> 그냥삭제, -r bfs에서 재귀 삭제 담당하므로 상관x? 
/**
 * TODO: recursive remove dirlist 정의 필요?
*/
    dirpoint * temp;
    dirpoint * prev;
    temp = mainDirList->head;
    while(temp) {
        if (temp->node == t) break;
        prev = temp;
        temp = temp->next;
    }
    if (temp == NULL) {
        fprintf(stderr, "fatal1");
        exit(1);
    }
    if (prev == NULL) {
        if (temp->node == NULL) {
            mainDirList->head = NULL;
            mainDirList->tail = NULL;
            free(temp);
            return;
        }
        mainDirList->head = mainDirList->head->next;
        free(temp);
        return;
    }
    if (temp -> node == NULL) {
        mainDirList->tail = prev;
        mainDirList->tail->next = NULL;
        free(temp);
        return;
    }
    prev->next = temp->next;
    free(temp);
}
// void searchDirList(filedir *t) {
//     dirpoint * temp = mainDirList->head;
//     while(temp){ 
//         if (temp->node == t)
//     }
// }

void addfdchild(filedir * t, filedir * parent) { //must be added with parent dir
    parent->childs[++parent->childscnt] = t;
    // addDirList(t);
    // if (t->childscnt != -1) //dir
    // {
    //     addDirList(t);
    // }
}
void delfdchild(filedir * t, filedir * parent) { //dirs must be removed after inner childs r all dead
    int flag = 0;
    for (int i = 0; i<= parent->childscnt; i++) {
        if (flag == 1) {
            parent->childs[i-1] = parent->childs[i];
        }
        if (parent->childs[i] == t) {
            flag = 1;
            free(t);
        } 
    }
    /**
     * TODO: realloc maybe?
    */
    parent->childs[parent->childscnt] = NULL; 
    parent->childscnt--;
    // if (t->childscnt != -1) { //dir
    //     // removeDirList(t);
    // }
}
filedir * initfd() {
    filedir * fd = (filedir*)malloc(sizeof(filedir));
    fd->head = NULL;
    fd->childscnt = -1;
    return fd;
}
backupNode * initbackup() {
    backupNode * b = (backupNode * )malloc(sizeof(backupNode));
    b->next = NULL;
    return b;
}
void addbackup(filedir *t, backupNode * b) { //must call when make filedir file
    if (t->head == NULL) {
        t->head = b;
        return;
    }
    backupNode * temp = t->head;
    while(temp->next) {
        temp = temp -> next;
    }
    temp->next = b;
}
void delbackup(filedir *t, char *stamp) {
    backupNode *temp = t->head;
    backupNode * prev;
    while(temp) {
        if (!strcmp(temp->stamp, stamp)) {
            break;
        }
        prev = temp;
        temp = temp -> next;
    }
    if (temp == NULL) {
        exit(1);
    }
    if (prev == NULL) { //1 or first
        if (temp->next == NULL) {
            free(temp);
            t->head = NULL;
            return;
        }
        t->head = temp->next;
        free(temp);
        return;
    }
    if (temp ->next == NULL) {
        prev -> next = NULL;
        free(temp);
        return;
    }
    prev->next = temp->next;
    free(temp);
    
}


/// @brief queue start//////////////
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
void destroyQueue(queue * target) {
	free(target);
}
////////////////////////////////////


//useful

/// @brief 
/// @param absolute_path 
/// @param cnt 
/// @return int that indicates last / location
int return_last_name(char *absolute_path) {
    // char temp[4096];
    // sprintf(temp, "%s", absolute_path);
    // int cnt = strlen(*absolute_path);
    int i;
    // printf("%s",*absolute_path);
    for (i = strlen(absolute_path) - 1; i >= 0; i--) {
        if (absolute_path[i] == '/') {
            return i;
        }//
    }
    return 0;
}
//useful
char * substr(char * target, int a, int b) {
    // printf("%d", strlen(target));
    // char temp[strlen(target) + 100];
    char* temp = (char*)malloc(strlen(target) + 1);
    strncpy(temp, target + a, b - a);
    *(temp + b - a) = '\0';
    return temp;
}




void bfs_fs_maker(char * path, char * oripath, char * stamp) {//if file comes in, it will cause err
    char curname[4096];
    char nextpath[4096] = "";
    char nextoripath[4096] = "";
    struct dirent **namelist;
    struct stat curstat;
    int cnt;

    queue q = *initQueue();
    filedir* dir = initfd();//
    backupNode * b = initbackup();
    strcpy(b->backupPath, path);
    strcpy(b->oripath, oripath);
    strcpy(b->stamp, stamp);
    
    strcpy(dir->path, oripath);
    // strcpy(dir->, path);
    // printf("%d", strlen(oripath));
    // printf(" %d", return_last_name(oripath));
    // printf("-------%s\n", substr(oripath, return_last_name(oripath), strlen(oripath)));
    sprintf(dir->name, "%s", substr(oripath, return_last_name(oripath) + 1, strlen(oripath))); // find name plz

    addbackup(dir, b);
    // addDirList(dir);
    printf("%s || %s || %s\n", path,oripath, stamp);
    q.push(&q, dir);
    while(!q.empty(&q)) {
        filedir * target = (filedir*)q.front(&q);
        q.pop(&q);

        if (lstat(target->head->backupPath, &curstat) < 0) exit(1);
        if (S_ISREG(curstat.st_mode)) {
            target->statbuf = curstat;
            addDirList(target);
            continue;
        }
        free(namelist);
        if ((cnt = scandir(target->head->backupPath, &namelist, NULL, alphasort)) < 0) {
            exit(1);
        }
       
        // printf("running bfs");
        // printf("%s %s\n", target->head->oripath, target->head->backupPath);
        
        target->statbuf = curstat;
        // target->childscnt = cnt - 1;
        target->childs = (filedir**)malloc(sizeof(filedir*) * cnt);
        for (int i =0 ; i< cnt;i++) {
            if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;
            // if (!strcmp(namelist[i]->d_name, "ssubak.log")) continue;
            
            sprintf(nextpath, "%s/%s", target->head->backupPath, namelist[i]->d_name); //it is adding sequence, so there is only 1 backup,so this code works
            sprintf(nextoripath, "%s/%s", target->head->oripath, namelist[i]->d_name);
            printf("%s %s\n", nextoripath, nextpath);
            filedir * next = initfd();
            backupNode * back = initbackup();
            strcpy(next->path, nextoripath);
            strcpy(back->oripath, nextoripath);
            strcpy(back->backupPath, nextpath);
            strcpy(back->stamp, stamp);
            
            strcpy(next->name, namelist[i]->d_name);
            // strcpy(next->name, namelist[i]->d_name); 
            // strcpy(next->path, nextoripath);
            // strcpy(next->backupPath, nextpath);
            if (lstat(nextpath, &curstat) < 0) {
                continue;
            }
            next->statbuf = curstat;
            back->statbuf = curstat;

            addfdchild(next, target);
            // show_all();
            addbackup(next, back);
            
            q.push(&q, next);
        }
        addDirList(target);
    }
}

int load_backup(){
    char *path = "/home/backup";
    char curname[4096];
    char origin[4096];
    struct dirent ** namelist;
    int cnt;
    if ((cnt = scandir(path, &namelist, NULL, alphasort)) < 0) return -1;
    printf("%d\n", cnt);
    for (int i =0 ; i < cnt; i++) {
        if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;
        sprintf(curname, "%s/%s", path, namelist[i]->d_name);
        printf("%s is on progress\n", curname);
        int res = find_link(namelist[i]->d_name, origin);
        
        if (!res) {
            printf("fked");
            continue;
        }
        
        bfs_fs_maker(curname, origin, namelist[i]->d_name);
        show_all();
    }
    return 1;
}
int show_all() {
    dirpoint *temp = mainDirList->head;
    printf("\n\n");
    while(temp) {
        filedir * target = temp->node;
        printf("%s\n",target->name);
        printf("path : %s\n", target->path);
        printf("backups : ");
        backupNode * b = target->head;
        while(b) {
            printf(" %s,", b->stamp);
            b = b ->next;
        }
        printf("\n");
        printf("childs : ");
        for (int i = 0; i <= target->childscnt; i++) {
            printf("%s,", target->childs[i]->name);
        }
        printf("\n");
        printf("size : %ld\n", target->statbuf.st_size);
        temp = temp->next;
    }
    return 1;
}
int main() {
    mainDirList = (dirList*)malloc(sizeof(dirList));
    mainDirList->head = NULL;
    mainDirList->tail = NULL;
    mainDirList->size = 0;
    int i =load_backup();
    printf("\n\n");
    while(1) {
        char input[4000];
        scanf("%s", input);
        if (!strcmp(input, "exit")) break;
        if (!strcmp(input, "show")) show_all();
    }
}