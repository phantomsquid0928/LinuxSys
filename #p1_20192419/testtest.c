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

typedef struct filedir {
    char path[MAXPATH];
    char name[MAXDIR];
    struct stat statbuf;
    // char backupPath[MAXPATH];
    // backupNode * head;
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

dirList * mainList = NULL;
dirList * initMainList() {
  dirList * temp = (dirList*)malloc(sizeof(dirList));
  temp->head = NULL;
  temp->tail = NULL;
  temp->size = 0;
}
void addList(filedir* t) {
    dirpoint * temp = mainList->head;
    dirpoint * point = (dirpoint*)malloc(sizeof(dirpoint));
    point->node = t;
    point->next = NULL;
    if (temp == NULL) { //empty
        mainList->head = point;
        mainList->tail = point;
        mainList->size = 1;
        return;
    }
    mainList->tail->next = point;
    mainList->tail = point;
    mainList->size++;
}

void show_list() {
    dirpoint *temp = mainList->head;
    while(temp) {
        printf("%s\n", temp->node->name);
        temp = temp->next;
    }
}
int main() {
  mainList = initMainList();
  filedir *f1 = (filedir*)malloc(sizeof(filedir));
  filedir *f2 = (filedir*)malloc(sizeof(filedir));
  filedir *f3 = (filedir*)malloc(sizeof(filedir));
  filedir *f4 = (filedir*)malloc(sizeof(filedir));
  filedir *f5 = (filedir*)malloc(sizeof(filedir));
  filedir *f6 = (filedir*)malloc(sizeof(filedir));

  addList(f1);
  addList(f2);
  addList(f3);
  addList(f4);
  addList(f5);
  addList(f6);
  strcpy(f1->name, "f1");
  strcpy(f2->name, "f2");
  strcpy(f3->name, "f3");
  strcpy(f4->name, "f4");
  strcpy(f5->name, "f5");
  strcpy(f6->name, "f6");

  show_list();
}