#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>


//////////////queue
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








typedef struct filedir { 
    char * name;           //real dir
    char * backupname[255]; //versions
    int stampscnt;       //last version : 0, 1, ...
    struct filedir ** childs; //the complete childs
    int childscnt;       //max childs num
}filedir;


//file 뒤지고 log 뒤지는게 빠르것다. ->실행마다 느림
//log 뒤지고 file 뒤지기 -> 빠르긴 하겟다만 힘듬
typedef struct Timestamp {
    int time; //char * time
    char linked_dir[4096];
    char *dir[4096]; //filenames
    int logcnt; //counts logs that has same time stamp, same things will be stored in *dir
}timestamp;

typedef struct filestr {
    char path[4096];
    char *childs[4096];
    int childscnt;
    char *timestamps[255];
    int stampscnt;
}filestr;

filestr str[10] = {
    {"home/ph", {"a.txt", "b", "c"}, 3, {"a"}, 1},
    {"home/ph/a.txt", NULL, 0, {"a"}, 1},
    {"home/ph/b", {"a.txt", "b.txt"}, 2, {"a"}, 1},
    {"home/ph/c", {"c.txt"}, 1, {"a"}, 1},
    {"home/ph/b/a.txt", NULL, 0, {"a"}, 1},
    {"home/ph/b/b.txt", NULL, 0, {"a", "b"}, 2},
    {"home/ph/c/c.txt", NULL, 0, {"a"}, 1},
};

filedir * masterhead = NULL;
// char * readfile[4096];
// int readcnt = 0;
int create_fs(char * real, char * tempdir) { //timestamps will be called in serialized sequence
//real : home/ph/a 
//tempdir : home/backup/timestamp
    struct stat curstat;
    struct dirent ** namelist;
    char tempReal[4096];
    char tempPath[4096];
    
    filedir head = {real, {tempdir}, NULL, 1, 0}; //tempdir for backup file search.
    // check existing root head has 
    // if (masterhead == NULL) {
    //     masterhead = head;
    // }
    // else {
    //     if (strstr(masterhead->name, head.real) > 0) {
    //         //go find masterhead's child that equals head.real
    //     }
    //     else {
    //         //make head master, then make master head's child recursively.
    //     }
    // }

    queue q = *initQueue();
    q.push(&q, &head);

    int cnt;
    while(!q.empty(&q)) {
        filedir * target = q.front(&q);
        char * target_path = target->backupname[target->stampscnt];
        if (lstat(target_path, &curstat) < 0) {
            printf("fatal!");
            return 1;
        }
        if ((cnt = scandir(target_path, &namelist, NULL, alphasort)) == -1) {
            printf("fatal error while making fs");
            return 1;
        }
        int oldcnt = target->childscnt;

        for (int i =0 ; i< cnt; i++) {
            if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
                continue;
            sprintf(tempPath, "%s/%s", target_path, namelist[i]->d_name);
            sprintf(tempReal, "%s/%s", target->name, namelist[i]->d_name);
            if (lstat(tempPath, &curstat) < 0) {
                printf("fatal error while read stat");
                return 1;
            }

            if (S_ISDIR(curstat.st_mode)) {
                // q.push(&q, )
            }
            if (S_ISREG(curstat.st_mode)) {

                // q.push(&q, );
            }
        }
        // char * curtempdir = target->backupname[target->stampscnt];
        
    }
}
int getBackupFiles() {
    char * backup = "/home/backup";
    char tempdir[4096];
    struct dirent **namelist;
    struct stat targetstat;
    int cnt;
    int i;

    // queue q = *initQueue();
    filedir ph = {"/home/ph", NULL, NULL, 0, 0}; //get it from logfile, or 

    if ((cnt = scandir(backup, &namelist, NULL, alphasort)) == -1) {
        return -1;
    }
    for(i = 0;i < cnt; i++) {
        if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
            continue;
        if (!strcmp(namelist[i]->d_name, "backup.log"))
            continue;
        sprintf(tempdir, "%s/%s", backup, namelist[i]->d_name);
        //get matching from log
        // char * real = getLinkedDirFromTimestamp(namelist[i]->d_name);
        // create_fs(real, tempdir); //create file overview for ssu_backup.
                        //routine: bfs first, bfs second
                        //compare head and new, if new is child of head then
                        //bfs from there in opposite case(new is parent) then
                        //just bfs and compare if new has new info, or not just bfs
                        //ssu_backup's fs and real fs both.  
    }

}

//useful
int return_last_name(char *absolute_path, int cnt) {
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
    char* temp = (char*)malloc(strlen(target) - 1);
    return strncpy(temp, target + a, b - a);
}
void search_targets(timestamp * t, char * absolute_path) {//bfs timestamp... how? dir must have / on last
    int i;
    int sep = return_last_name(absolute_path, strlen(absolute_path));

    char dir[4096];
    // char name[255];
    sprintf(dir, "%s", substr(absolute_path, 0, sep));
    char *name = substr(absolute_path, sep + 1, strlen(absolute_path));
    printf("%s    %s\n", dir, name);
    // int targets[1000];
    timestamp ** targets = (timestamp**)malloc(sizeof(timestamp) * 1000);
    int cnt = 0;
    int chk = 0;
    for(i = 0; i < 6; i++) {
        printf("%s || %s\n", dir, t[i].linked_dir);
        if (strstr(dir, t[i].linked_dir) != NULL) {
            if (strcmp(dir, t[i].linked_dir) != 0 && t[i].logcnt == 1) continue;
            targets[cnt] = &t[i];
            cnt++;
        }
    }

    queue q = *initQueue();
    for (i = 0; i < cnt ;i++) {
        q.push(&q, &targets[i]);
        printf("%d", targets[i]->time);
    }
    q.push(&q, targets);
    while(!q.empty(&q)) {
        timestamp * t = q.front(&q);
        q.pop(&q);
    
        // scan
    }
    // while(1) {
    //     int sep = return_last_name(dir, strlen(dir));
    //     for (i = 0; i < 4; i++) {
    //         // printf("%d\n", t[i].logcnt);
    //         // if (t[i].logcnt == 1) {
    //         //     // printf("%s / %s\n", t[i].linked_dir, t[i].dir[0]);
    //         //     if (chk == 0 &&  strcmp(t[i].dir[0], name) == 0) {
    //         //         targets[cnt] = i;
    //         //         cnt++;
    //         //     }
    //         //     continue;
    //         // }
    //         // printf("%s _==_ %s\n", t[i].linked_dir, dir);
    //         if (strcmp(t[i].linked_dir, dir) == 0) {
    //             if (t[i].logcnt == 1 && chk == 1) continue; 
    //             targets[cnt] = i;
    //             cnt++;
    //         }
    //     }
    //     chk = 1;
    //     printf("%s\n", dir);
    //     strcpy(dir, substr(dir, 0, sep));
    //     // printf("%s\n\n", dir);
    //     if (sep == 0) break;
    // }
    // for (i = 0; i< cnt; i++) {
    //     printf("%d ", targets[i]);
    // }

    
    // for (i = 0; i < 5; i++) {
    //     if (t->logcnt == 1) continue;
    //     if (strcmp(t->linked_dir)
    // }
}
int main() {
    char * temp = "/home/ph/linuxhw/#p1_20192419/b/b.txt";
    char temp2[10] = "hello/e/fs.e";
    printf("%d", return_last_name(temp2, 7));
    printf("%s", substr(temp, 0, return_last_name(temp, 3)));
    // getBackupFiles();

    // filedir f[6] = {
    //     {"R",{"", ""}, NULL, 2, 3},
    //     {"a.txt",{"", ""}, NULL, 2, 0},
    //     {"b.txt",{"", ""}, NULL, 2, 0},
    //     {"a",{"", ""}, NULL, 2, 1},
    //     {"c.txt", {"", ""},NULL, 2, 0},
    // };
    // f[0].childs = (filedir**)malloc(sizeof(filedir) * 3);
    // f[0].childs[0] = &f[1];
    // f[0].childs[1] = &f[2];
    // f[0].childs[2] = &f[3];
    // f[3].childs = (filedir**)malloc(sizeof(filedir));
    // f[3].childs[0] = &f[4];
    // // f[3].childs[0] = &f[4];
    // f[0].childs[0] = &f[1];
    // f[0].childs[1] = &f[2];
    // f[0].childs[2] = &f[3];
    // int i;
    // queue qq = *initQueue();
    // qq.push(&qq, &f[0]);
    // while(!qq.empty(&qq)) {
    //     printf("\n");
    //     filedir * target = qq.front(&qq);
    //     qq.pop(&qq);
    //     for (i = 0; i < target->childscnt; i++) {
    //         if (target->childs[i]->childscnt == 0) {
    //             printf("  %s", target->childs[i]->name);
    //             continue;
    //         }
    //         qq.push(&qq, target->childs[i]);
            
    //     }
    // }


    /// read backup file, for timestamps dir, read file inside with bfs, scandir alphasort, stores in timestamp.
    //as we dont know about how many timestamps exists, so timestamp must be changed as linkedlist or * expression
    // timestamp t[6] = {
    //     {1, "home/ph", {"a.txt"}, 1},  //dir/a.txt
    //     {2, "home/ph", {"a.txt", "b.txt", "b/a.txt"}, 3}, //dir  //right_log - <timestamp dir expression> = puredir
    //                                                 //left_log - puredir == linked dir
    //     // {2, "b.txt"}, 
    //     // {2, "b/a.txt"}, 
    //     {3, "home/ph", {"a.txt"}, 1}, //dir/a.txt
    //     {4, "home/ph/b", {"a.txt"}, 1},
    //     {5, "home", {"ph/a.txt", "ph/b.txt", "ph/b/a.txt"}, 3},
    //     {6, "home/ph/b", {"a.txt"}, 1}}; //dir/b/a.txt   

            //path          child cnt, ver, vercnt
    //     {"home/ph/b/a.txt, {}, 0, {1}, 1"}
    //     {"home/ph/b, {a.txt}, 1, {1}, 1"}             


    // for (int i = 0;i < 6; i++) { //change to node iter
    //     int cnt = t[i].logcnt;
    //     printf("%d\n", t[i].time);
    //     for (int j = 0; j < cnt; j++){
    //         printf("   ㄴ%s\n", t[i].dir[j]);
    //     }
    //     printf("\n");
    // }
    // search_targets(t, "home/ph/");
    // char *tmp = "hello/a.txt";
    // // int sub = dir_name_substr(tmp);
    // int sub = return_last_name(tmp, strlen(tmp));
    // printf("%d\n", sub);
    // char * name = tmp + sub;
    // char dir[4096];
    // strncpy(dir, tmp, sub);
    // printf("%s", name);
    // printf("\n%s ", dir);
    // // printf("%d", strlen(tmp));
    // char * str = substr(tmp, 3, 11);
    // printf("\n%s", str);
    // printf("\n%s", substr(tmp, 0, 4));
    // free(str);
}