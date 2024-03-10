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

typedef struct filestr {
    char origin[4096]; //original path
    char *backup[4096]; // home/backup/<timestamp>/...   path, multiple enable with cnt
    char *childs[255];  //childs
    int childscnt;      //orign/backup childs
    char *timestamps[255];// backup route versions
    int stampscnt;        //stamp, backup cnts.
    struct filestr * next;
}filestr;
typedef struct pathpair{ 
    char backup[4096];
    char origin[4096];
}pathpair;

filestr * head = NULL;
// static filestr str[10] = { //change to linekdlist
//     {"home/ph", {""}, {"a.txt", "b", "c"}, 3, {"a"}, 1},
//     {"home/ph/a.txt", {"a.txt"}, {""}, 0, {"a"}, 1},
//     {"home/ph/b", {"b"}, {"a.txt", "b.txt"}, 2, {"a"}, 1},
//     {"home/ph/c", {"c"}, {"c.txt"}, 1, {"a"}, 1},
//     {"home/ph/b/a.txt", {"b/a.txt"}, {""}, 0, {"a"}, 1},
//     {"home/ph/b/b.txt", {"b/b.txt", "b.txt"}, {""}, 0, {"a", "b"}, 2},
//     {"home/ph/c/c.txt", {"c/c.txt"}, {""}, 0, {"a"}, 1},
// };
int strcnt = 0;
static pathpair pairs[10] = { //change to linkedlist
    {"34434434", "home/ph/linuxhw"}, //home/ph/b backukped to 2343434
    {"34566666", "home/ph/linuxhw"}, //home/ph/b ~ to 2334356
    {"34566669", "home/ph/linuxhw/b"}, //home/ph/b/b.txt ~ t
};
int pairscnt = 2;
/**
 * TODO: pairs + backup files => str needed
*/

//pathpair
// a = {"2343434", "home/ph"}; //home/ph backukped to 2343434
// b = {"2334356", "home/ph/b/b.txt"}; //home/ph/b/b.txt ~ to 2334356
//yymmddhhmmss to hash will be fine

char * substr(char * target, int a, int b) {
    // printf("%d", strlen(target));
    // char temp[strlen(target) + 100];
    char* temp = (char*)malloc(strlen(target) - 1);
    return strncpy(temp, target + a, b - a);
}

int add_filestr(filestr * target) { 
    /**
     * TODO: 중복 dir 들어왓을 때 childs에 추가 필요.
    */
    // str[strcnt] = *target;
    // strcnt++;
    // printf("\norigin : %s\n backup : %s\n childs : \n", target->origin, target->backup[0]);
    // for (int i = 0;i <= target->childscnt; i++) {
    //     printf("   %s\n", target->childs[i]);
    // }
    // printf("%d ", target->childscnt);
    // printf("\n");

    if (head == NULL){
        head = target;
        return 1;
    } 
    filestr * ptr = head;
    filestr * prev;
    char assembled_route[4096];
    int flag = 0;
    // sprintf(assembled_route, "%s/%s", target->origin, target->childs[0]);
    // printf("%s\n", target->origin);
    while(ptr) { //여기까지 하면 O(ptrlen * (n + m))
        // printf("   %s\n", ptr->origin);
        if (!strcmp(ptr->origin, target->origin)) { //중복!
            // printf("hello");
            ptr->stampscnt++;
            ptr->backup[ptr->stampscnt] = (char*)malloc(sizeof(char) * 4096);
            ptr->timestamps[ptr->stampscnt] = (char*)malloc(sizeof(char) * 255);
            strcpy(ptr->backup[ptr->stampscnt], target->backup[0]);
            strcpy(ptr->timestamps[ptr->stampscnt], target->timestamps[0]);
            int i = 0, j = 0;
            
            //정렬되잇으므로 투포인터 ^^ O(n + m) 그는 신이야...
            char ** rest = (char**)malloc(sizeof(char*) * 4096);

            int rescnt = 0;
            if (ptr->childscnt > 0 || target->childscnt > 0) {
                for (int i = 0; i <= ptr->childscnt; i++) {
                    printf("a : %s\n", ptr->childs[i]);
                }
                printf("\n");
                for (int i = 0; i <= target->childscnt; i++) {
                    printf("a : %s\n", target->childs[i]);
                }
                for (i = 0, j = 0;;) {
                    // printf("%d %d\n", i, j);
                    if (i >= ptr->childscnt) {
                        rest[rescnt] = (char*)malloc(sizeof(char) * 255);
                        strcpy(rest[rescnt], target->childs[j]);
                        rescnt++;
                        // res[rescnt++] = target->childs[j];
                        j++;
                        if (j >= target->childscnt) break;
                        continue;
                    }
                    if (j >= target->childscnt) {
                        rest[rescnt] = (char*)malloc(sizeof(char) * 255);
                        strcpy(rest[rescnt], ptr->childs[i]);
                        rescnt++;
                        // res[rescnt++] = ptr->childs[i];
                        i++;
                        if (i >= ptr->childscnt) break;
                        continue;
                    }
                    int res = strcmp(ptr->childs[i], target->childs[j]);
                    if (res == 0) {
                        rest[rescnt] = (char*)malloc(sizeof(char) * 255);
                        strcpy(rest[rescnt], ptr->childs[i]);
                        rescnt++;
                        // res[rescnt++] = ptr->childs[i];
                        i++;
                        j++;
                    }
                    if (res < 0) {
                        rest[rescnt] = (char*)malloc(sizeof(char) * 255);
                        strcpy(rest[rescnt], ptr->childs[i]);
                        rescnt++;
                        // res[rescnt++] = ptr->childs[i];
                        i++;
                    }
                    if (res > 0) {
                        rest[rescnt] = (char*)malloc(sizeof(char) * 255);
                        strcpy(rest[rescnt], target->childs[j]);
                        rescnt++;
                        // res[rescnt++] = target->childs[j];
                        j++;
                    }
                }
                ptr->childscnt = rescnt - 1;
                for (int i = 0; i < rescnt; i++) {
                    if (ptr->childs[i] == NULL) {
                        ptr->childs[i] = (char*)malloc(sizeof(char) * 255);
                    }
                    strcpy(ptr->childs[i], rest[i]);
                    printf("---- %s\n", rest[i]);
                }
                free(rest);
            }
            // free(ptr->childs);
            // realloc(ptr->childs, sizeof(char*) * rescnt);
            
            // ptr->childs = rest;
            prev = ptr;
            ptr = ptr->next;
            flag = 1;
            continue;
        }
        prev = ptr;
        ptr = ptr->next;
    }
    if (flag == 0){
        prev->next = target;
    }
    else {
        free(target);
    }
    // ptr->next = target;
}
void show_all() {
    filestr * ptr = head;
    while(ptr) {
        printf("%s\n", ptr->childscnt == -1 ? "file": "dir");
        printf("origin : %s\n backup : \n", ptr->origin);
        for (int i = 0; i <= ptr->stampscnt; i++) {
            printf("   %s : st> %s\n", ptr->backup[i], ptr->timestamps[i]);
        }
        printf("childs : \n");
        for (int i = 0;i <= ptr->childscnt; i++) {
            printf("   %s\n", ptr->childs[i]);
        }
        printf("%d \n", ptr->childscnt);
        printf("%d\n", ptr->stampscnt);
        printf("\n");
        ptr = ptr ->next;
    }
}

filestr* init_filestr(char * origin, char * backup, char * stamp) { //modify this after change to linked
    
    filestr * temp = (filestr*)malloc(sizeof(filestr));
    strcpy(temp->origin, origin);
    temp->backup[0] = (char*)malloc(sizeof(char) * strlen(backup));
    strcpy(temp->backup[0], backup);
    temp->timestamps[0] = (char*)malloc(sizeof(char) * strlen(stamp));
    strcpy(temp->timestamps[0], stamp);
    temp->next = NULL;
    temp->childscnt = -1;
    temp->stampscnt = 0;
    return temp;
}
int bfs_fileload(char * origin, char * stampdir, char * stamp) {
    int cnt;
    char temp_backup[4096];
    char temp_dir[4096];
    char cur_backup[4096];
    struct stat curstat;
    struct dirent ** namelist;
    
    // if (lstat(stampdir, &curstat) < 0) {
    //     return -1;
    // }
    // if (S_ISREG(curstat.st_mode)) {
    //     // make_filestr(origin, )
    //     // filestr temp = {*origin, {*stampdir} , NULL, -1, {*stamp}, 0};
    //     filestr * temp = init_filestr(origin, stampdir, stamp);
    //     printf("helo?");
    //     add_filestr(temp);
    //     return 1;
    // }
    // filestr temp = {*origin, {*stampdir}, NULL, -1, {*stamp}, 0};
    filestr * temp = init_filestr(origin, stampdir, stamp);

    queue q = *initQueue();
    q.push(&q, temp);
    int xcnt = 0;
    while(!q.empty(&q)) {
        filestr *target = q.front(&q);
        q.pop(&q);
        // target->backup[target->stampscnt] = 
        xcnt++; 
        sprintf(cur_backup, "%s/%s/%s", "/home/backup", stamp ,target->backup[target->stampscnt]);
        if (lstat(cur_backup, &curstat) < 0) {
            printf("\n%s\n\n", cur_backup);
            printf("here");
            return -1;
        }
        if (S_ISREG(curstat.st_mode)) {
            add_filestr(target);
            // printf("UHH");
            continue;
        }
        if ((cnt = scandir(cur_backup, &namelist, NULL, alphasort)) < 0) {
            printf("UUHUHUH");
            return -1;
        }
        // if (xcnt == 3) break;
        // target->childs = (char**)malloc(sizeof(char*) * cnt);
        // target->childs[0]
        for (int i = 0;i < cnt; i++) {
            if (!strcmp(namelist[i]->d_name,".") || !strcmp(namelist[i]->d_name, "..")) continue;
            sprintf(temp_backup, "%s/%s", target->backup[target->stampscnt], namelist[i]->d_name);
            sprintf(temp_dir, "%s/%s", target->origin, namelist[i]->d_name);
            // printf("%s :: %s\n\n", temp_backup, temp_dir);
            target->childs[++target->childscnt] = (char*)malloc(sizeof(char) * 255);
            strcpy(target->childs[target->childscnt], namelist[i]->d_name);
            // target->childs[target->childscnt] = namelist[i]->d_name;
            // target->childscnt++;

            // q.push(&q, &)
            // printf("ehre");
            // filestr child = {temp_dir, {temp_backup}, NULL, -1, {*stamp}, 0};
            filestr* child = init_filestr(temp_dir,temp_backup, stamp);
            // add_filestr(temp_dir, temp_backup, stamp);
            q.push(&q, child);
        }
        add_filestr(target);
    }
    q.clear(&q);
}
int find_link(char * name, char * res) {
    for (int i =0 ; i< 10; i++) { //10 = pairs cnt 
        if (!strcmp(pairs[i].backup, name)) {
            strcpy(res, pairs[i].origin);
            return 1;
        }
    }
    return 0;
}
int load_backup(char * backup) {
    char tempdir[4096];
    char oripath[4096];
    char res[4096];
    struct stat curstat;
    struct dirent ** namelist;
    int cnt;
    
    if ((cnt = scandir(backup, &namelist, NULL, alphasort)) < 0) {
        return 1;
    }
    for (int i =0 ;i < cnt; i++) {
        if (!strcmp(namelist[i]->d_name,".") || !strcmp(namelist[i]->d_name, "..")) continue;
        sprintf(tempdir, "%s/%s", backup, namelist[i]->d_name);
        if (!find_link(namelist[i]->d_name, res)) {
            printf("failed to find such backup\n");
            continue;
        }
        sprintf(oripath, "/%s", res);
        if (lstat(tempdir, &curstat)<0) {
            printf("err");
            continue;
        }
        if (S_ISREG(curstat.st_mode)){
            printf("log file, continue...");
            continue;
        }
        printf("names \n %s \n%s \n%s\n\n", oripath, tempdir, namelist[i]->d_name);
        bfs_fileload(oripath, "", namelist[i]->d_name); 
                // /home/ph    /home/backup/34434434   34434434
    }
}
int main() {
    char * backup = "/home/backup";
    // load_log(); //log load for match log - real file system link

    filestr *child =(filestr*)malloc(sizeof(filestr));
    child->backup[0] = (char*)malloc(sizeof(char) * 255);
    strcpy(child->backup[0], "hi");
    // strcpy(child->backup[1], "ff");
    strcpy(child->origin, "hellO");

    printf("%s", child->origin);
    printf("%s", child->backup[0]);
    // printf("%s", child->backup[1]);

    load_backup(backup);
    show_all();
    // for (int i =0 ; i < 7; i++) {
    //     printf("%s\n", str[i].origin);
    //     for (int j = 0; j < str[i].childscnt; j++) {
    //         printf("   %s :", str[i].childs[j]);
    //     }
    //     for (int j = 0; j < str[i].stampscnt; j++) {
    //         printf(" %s \n", str[i].timestamps[j]);
    //     }
    // }
}