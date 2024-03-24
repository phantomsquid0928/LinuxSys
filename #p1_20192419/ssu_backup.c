#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>

#include <openssl/md5.h>

#define OPENSSL_API_COMPAT 0x10100000L

// #include <queue.h>
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
	" backuped to ", " removed by ", " recovered to ", " already backuped to ", " not changed with "
};



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
	backupNode * rear;
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

/**f
 * added for test
*/
// typedef struct pathpair { //get from log file
//     char stamp[4096]; //first
//     char oripath[4096]; //second
// }pathpair;


// /////////////////// Log struct for .log file//////////

// typedef struct Log{
// 	// unsigned char md[MD5_DIGEST_LENGTH]; //get this when read .log file              if md1, absolute_path1 == md2 abpath2 -> they are same (-y)
// 	char absolute_path_dir[MAXPATH]; //linked dir
// 	int timestamp;          //home/backup/<timestamp>
// 	char pure_path[MAXPATH];    //path til name          timestamp + pure_path == backup_path ((ex) > home/backup/<timestamp>/b/a.txt)
// 	char purename[MAXDIR];       //-> absolute_path_dir + pure_path == absolute_path   if pure_path == purename -> file else file inside dir
// 	struct Log * next;
// }log;


// log * logList; //linked list to save Logs

// log * initLog() {

// }

// log ** searchbystamp(int timestamp) {

// }
// log ** searchbypath(char * absolute_path) { //absolute_path_dir

// }



/////////////////////vv pathpair for bfs ///////////

char ** split(char * command, char * spliter, int *res);
char * substr(char * target, int a, int b);

typedef struct {
	char first[MAXPATH];//?
	char second[MAXPATH];//?
}pathpair;
typedef struct log {
	pathpair * node;
	struct log * next;
}log;
log * loghead;
void search_and_addLog(pathpair * target) {
	log * head = loghead;
	while(head) { //find same timestamp log, then change path to parent one
		pathpair * t = head->node;
		if (!strcmp(t->first, target->first)) {//same log, save only parent dir
			int res1;
			int res2;
			char ** arg1 = split(t->second, "/", &res1);
			char ** arg2 = split(target->second, "/", &res2);
			int minv = res1 > res2 ? res2 : res1;
			char temp[MAXPATH];
			memset(temp, 0, sizeof(temp));
			for (int i = 0;i < minv; i++) {
				// printf("comparing : %s %s\n", arg1[i], arg2[i]); //debug
				if (!strcmp(arg1[i], arg2[i])) {
					strcat(temp, "/");
					// printf("%d\n", strlen(arg1[i]));
					strncat(temp, arg1[i], strlen(arg1[i]));
					continue;
				}
				break;
			}
			// printf("cat res : %s\n", temp); //debug
			// if (strstr(temp, ".") != NULL) {//file
			// 	char * parent = substr(temp, 0, return_last_name(temp));
			// }
			strcpy(t->second, temp);
			return;
		}
		head = head ->next;
	}

	//no same log, add it
	log * temp = (log *)malloc(sizeof(log));
	temp->node = target;
	temp->next = NULL;
	if (loghead == NULL) {
		loghead = temp;
		return;
	}
	temp->next = loghead;
	loghead = temp;
}
int searchLog(char * name, char * res) {
	log * temp = loghead;
	while(temp) {
		pathpair * t = temp->node;
		if (!strcmp(t->first, name)) {
			strcpy(res, t->second);
			return 1;
		}
		temp = temp->next;
	}
	return 0;
}
///////////////menu, can be changed to dirlist and dirpoint?

typedef struct menu {
    filedir * node;
    int num;
    int lv;
    struct menu * next;
}menu;
typedef struct menulist {
    menu * head;
    menu * rear;
    int cnt;
}menulist;



///////////////////menulist start, maybe can change this to dirlist
/// @brief //
/// @return /
menulist * init_menulist() {
    menulist * temp = (menulist * )malloc(sizeof(menulist));
    temp->cnt = 0;
    // temp->lv = 0;
    temp->head = NULL;
    temp->rear = NULL;
    return temp;
}
void push_menu(menulist * target, filedir * child, int lv) {
    menu * temp = (menu*)malloc(sizeof(menu));
    temp->node = child;
    temp->lv = lv;
    temp->num = target->cnt;
    temp->next = NULL;
    if(target->head == NULL) {
        target->head = temp;
        target->rear = temp;
        target->cnt++;
        return;
    }
    target->rear->next = temp;
    target->rear = temp;
    target->cnt++;
}
void destroy_all(menulist * target) {
    menu * temp = target->head;
    menu * prev;
    while(temp) {
        prev = temp;
        temp = temp->next;
        free(prev);
    }
    free(target);
}



///////////////backunode, filedir, dirpoint, dirlist utils//////////

static pathpair pairs[10] = { //change to linkedlist
    {"34566669", "/home/ph/linuxhw/#p1_20192419/b"}, //home/ph/b/b.txt ~ t
    {"34434434", "/home/ph/linuxhw/#p1_20192419/b/새 폴더"}, //home/ph/b backukped to 2343434
    {"34566666", "/home/ph/linuxhw/#p1_20192419/b"}, //home/ph/b ~ to 2334356
};

/// @brief added for pairs test
/// @param name 
/// @param res 
/// @return 1 : exists 0 : not exists, return res //
int find_link(char * name, char * res) {
    for (int i =0 ; i< 10; i++) { //10 = pairs cnt 
        if (!strcmp(pairs[i].first, name)) {
            strcpy(res, pairs[i].second);
            return 1;
        }
    }
    return 0;
}

void addbackup(filedir*t, backupNode * b);
int show_all();
dirList *mainDirList = NULL;

int return_last_name(char * absolute_path);

filedir * initfd();
backupNode * initbackup();
filedir * addDirList(filedir * t, int chklost);
void addfdchild(filedir * t, filedir * parent);
void find_lost_link(filedir * t) {
	dirpoint * temp = mainDirList->head;
	filedir * sibling;
	int chk_both_par_lost = 0; //둘이 다 고아이면 1
	
	char * parent_path = substr(t->path, 0, return_last_name(t->path));

	while(temp) {
		filedir * node = temp->node;

		char * nodespar = substr(node->path, 0, return_last_name(node->path));
		if (!strcmp(nodespar, t->path)) { //found lost child dir of t
			int chk = 0;
			for (int i = 0; i <= t->childscnt; i++) {
				if (!strcmp(t->name, t->childs[i]->name)) {
					chk = 1;
					break;
				}
			}
			if (chk == 0) { //not in list
				// printf("fllsfsfsf");
				//add child by alphasort... o(n)
				// t->childscnt++;
				t->childs = (filedir**)realloc(t->childs, sizeof(char*) * (t->childscnt + 2));
				if (strcmp(t->childs[t->childscnt]->path, node->path) < 0) {
					t->childs[t->childscnt + 1] = node;
				}
				else {
					for (int i = t->childscnt; i >= 0; i--) {
						t->childs[i + 1] = t->childs[i];
						if (strcmp(t->childs[i]->path, node->path) < 0) {
							t->childs[i] = node;
							break;
						}
						if (i == 0) t->childs[i] = node;
					}
				}
			
				t->childscnt++;
			}
		}
		if (!strcmp(parent_path, node->path)) { //found lost chlids of node
			// printf("2;l22222\n");
			//add child on parent
			// node->childscnt++;
			chk_both_par_lost = -1;
			int chk = 0;
			for (int i = 0; i <= node->childscnt; i++) {
				if (!strcmp(node->childs[i]->name, t->name)) {
					chk = 1;
					break;
				}
				// printf("   _f__%s\n", node->childs[i]->name);
			}
			if (chk == 0) { //not in list
				node->childs = (filedir**)realloc(node->childs, sizeof(char*) * (t->childscnt + 2));
				if (strcmp(node->childs[node->childscnt]->path, t->path) < 0) {
					node->childs[node->childscnt + 1] = t;
				}
				else {
					for (int i = node->childscnt; i >= 0; i--) {
						node->childs[i + 1] = node->childs[i];
						if (strcmp(node->childs[i]->path, t->path) < 0) {
							node->childs[i] = t;
							break;
						}
						if (i == 0) node->childs[i] = t;
					}
				}
				node->childscnt++;
			}
			
		}
		if (!strcmp(nodespar, parent_path) && chk_both_par_lost != -1 && strcmp(t->path, node->path) != 0) { //같은놈아닌데 부모가 같음 -> 형제인데 부모가 없나?
			chk_both_par_lost = 1;
			sibling = node;
		}
		temp = temp->next;
	}
	if (chk_both_par_lost == 1) { //sibling (1) exists, make parent dir for t and sibling
		temp = mainDirList->head;
		filedir * parentdir= initfd();
		backupNode * backup = initbackup();
		char * parent_name = substr(parent_path, return_last_name(parent_path) + 1, strlen(parent_path));
		char * parent_oripath = substr(t->head->oripath, 0, return_last_name(t->head->oripath));
		strcpy(parentdir->path, parent_path);
		strcpy(parentdir->name, parent_name);

		strcpy(backup->backupPath, parent_path);
		strcpy(backup->oripath, parent_oripath);

		addbackup(parentdir, backup);
		parentdir->childs = (filedir**)malloc(sizeof(filedir *) * 2);
		if (strcmp(t->name, sibling->name) < 0) {
			addfdchild(t, parentdir);
			addfdchild(sibling, parentdir);
		}
		else {
			addfdchild(sibling, parentdir);
			addfdchild(t, parentdir);
		}

		addDirList(parentdir, 0);
	}
	
}
/// @brief 
/// @param t 
/// @param chklost : mod that determines whether chk lost_link or not 0->no 1->yes
/// @return filedir that changed filedir *, : if there is dup, then older one returns, if not, returns same filedir from param
filedir * addDirList(filedir *t, int chklost) { //중복 들어올 시 백업만 먹고 까버리기 필요
    // printf("called");
    dirpoint * temp = mainDirList->head;
    int flag = 0;
    // printf("adding %s\n", t->name); //debug
    if (mainDirList->size == 0) {
        dirpoint *newp = (dirpoint*)malloc(sizeof(dirpoint));
        newp->next = NULL;
        newp->node = t;
        mainDirList->head = newp;
        mainDirList->tail = newp;
        mainDirList->size++;
        // printf("ff");
        return t;
    }
    char t1[4096];
    char t2[4096];
    while(temp) {
        memset(t1, 0,sizeof(t1));
        memset(t2, 0,sizeof(t2));
        // printf(",");
        // if (temp->node == NULL) {
        //     printf("ffffffffff");
        // }
        strcpy(t1, temp->node->path);
        strcpy(t2, t->path);
        if (!strcmp(t1, t2)) { //dup
            // printf("1");
            flag = 1;
            break;
        }
        temp = temp -> next;
    }
    if (flag == 0) //new
    {
        // printf("NEW");
        dirpoint * newp = (dirpoint*)malloc(sizeof(dirpoint));
        newp->next = NULL;
        newp->node = t;
        mainDirList->tail->next = newp;
        mainDirList->tail = newp;
        mainDirList->size++;
		/**
		 * TODO: /#p1_20192419 들어왓는데 a.txt 만 커맨드로 받은 상태에서 #p1_20192419/b 디렉토리 잇다면 연결해야함
		 * 		반대로 -d 옵션 후 안에 dir을 그냥 추가시 ->부모 링크 없음
		 * 		부모가 내 (dir)링크 없는 경우, 내(dir)가 자식링크 없는경우
		 * 		무결성검사필요
		 * 		자식만 추가되서 부모가 없는경우
		*/
		if (t->childscnt != -1 && chklost == 1) {
			// printf("lost?");
			find_lost_link(t);
		}
		 //strcmp 로 alphabetic add 필요
        // printf("success %s %s\n", t->name, t->path); //debug
		return t;
    }
    else { // dup, chk if it is file or dir
        // printf("???");
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
					printf("adding %p", exists->childs[i]);
                    templist[rescnt++] = exists->childs[i++];
                    continue;
                }
				printf("%s : %s\n", exists->childs[i]->path, t->childs[j]->path);
                int res = strcmp(exists->childs[i]->path, t->childs[j]->path);
                if (res > 0) {
                    templist[rescnt++] = t->childs[j++];
                }
                if (res == 0) {
					printf("same!");
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
            exists->childs = realloc(exists->childs, sizeof(char *) * rescnt);
            for (int i = 0; i< rescnt; i++) {
                exists->childs[i] = templist[i];
                printf("%s : %p\n",templist[i]->name, templist[i]);
            }
            
        
            addbackup(exists, t->head);//add new backup
            // free(templist);
            // free(t->childs); ///////// HAZARD
            //two pointer
        }
        else { // file, get new backup and discard filedir
            filedir * exists = temp->node;
            addbackup(exists, t->head);
            free(t);
            free(t->childs);
        }
		return temp->node;
    }
    
}
void removeDirList(filedir *t) { //file -> 백업다 까버리기 dir -> 그냥삭제, -r bfs에서 재귀 삭제 담당하므로 상관x? 

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
	dirpoint * temp = mainDirList->head;
	while(temp) {
		filedir * exist = temp->node;
		if (!strcmp(t->path, exist->path)) {
			parent->childs[++parent->childscnt] = temp->node;
			return;
		}
		temp = temp->next;
	}
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
            // free(t); ???????????
        } 
    }
 
    parent->childs[parent->childscnt] = NULL; 
    parent->childscnt--;

}
filedir * initfd() {
    filedir * fd = (filedir*)malloc(sizeof(filedir));
    fd->head = NULL;
	fd->rear = NULL;
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
		t->rear = b;
        return;
    }
    t->rear->next = b;
	t->rear = b;
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
	target->head = NULL;
	target->rear = NULL;

	target->clear = NULL;
	target->empty = NULL;
	target->front = NULL;
	target->push = NULL;
	target->pop = NULL;
}
////////////////////////////////////


//useful

/// @brief return start of /, so if u wanna use this on substr, u have to return_last_name() + 1 when on first param,
///       just use return_last_name() on second param of substr
///       
/// 
/// @param absolute_path 
/// @param cnt 
/// @return int that indicates last / location
int return_last_name(char *absolute_path) {
    int i;
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
int get_slash_cnt(char * t) {
	char temp[MAXPATH];
	strcpy(temp, t);
    int res = 0;
    for (int i=0 ;i < strlen(temp); i++) {
        if (t[i] == '/') res++;
    }
    return res;
}
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

/////////////////
typedef struct {
    commands val;
	const char *str;
} commandsMap;
static int commandSize = sizeof(commandList) / sizeof(commandList[0]);
commandsMap * conversion[COMMAND_ENT];
char backup_path[MAXPATH];
char logfile_path[MAXPATH];


///////////////////^^^^^^^^// declare , util zone///////////////////////

//////////////////vv// init zone //////////////////

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
	strcpy(logfile_path, backup_path);
	strcat(logfile_path, "/");
	strcat(logfile_path, "ssubak.log");
	// sprintf(logfile_path, "%s/%s", backup_path, "ssubak.log");
	if (access(logfile_path, F_OK))
		creat(logfile_path, 0777);
}
// void initBackupLog() { //io log file
// 	//get log from logfile_path, 
// }
void initDirList() {
	mainDirList = (dirList*)malloc(sizeof(dirList));
    mainDirList->head = NULL;
    mainDirList->tail = NULL;
    mainDirList->size = 0;
}
void check_valid_runloc() {
	char * cwd = getcwd(NULL, 0);
	if (strstr(cwd, "/home") == NULL || get_slash_cnt(cwd) < 2) {
		fprintf(stderr, "this command must be ran on user (/home/<username>) above");
		exit(1);
	}
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

int make_log(char* target_path, char*path, char * stamp, int mod) { //0 backup, 1 remove, 2 recover, 3not backuped, 4 not changed
	char log[MAXPATH * 3];
	sprintf(log, "\"%s\"%s\"%s\"", target_path, logModList[mod], path);
	printf("%s\n", log);
	sprintf(log, "%s : \"%s\"%s\"%s\"", stamp, target_path, logModList[mod], path);
	if (mod < 3) { //log file io, add linked list
		FILE * fd;
		if ((fd = fopen("/home/backup/ssubak.log", "a+")) == NULL) {
			fprintf(stderr, "log error, cannot open file");
			exit(1);
		}
		int size;
		fprintf(fd, "%s\n", log);
	}
	
	return 0;
}


void bfs_fs_maker(char * path, char * oripath, char * stamp) {//if file comes in, it will cause err, but 
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
    // printf("%s || %s || %s\n", path,oripath, stamp); //debug
    q.push(&q, dir);
    while(!q.empty(&q)) {
        filedir * target = (filedir*)q.front(&q);
        q.pop(&q);

        if (lstat(target->head->backupPath, &curstat) < 0) exit(1);
        if (S_ISREG(curstat.st_mode)) {
            target->statbuf = curstat;
            addDirList(target, 1);
            continue;
        }
		// if (!S_ISDIR(curstat.st_mode)) continue;
        // free(namelist);
        if ((cnt = scandir(target->head->backupPath, &namelist, NULL, alphasort)) < 0) {
            exit(1);
        }
       
        // printf("running bfs"); //debug
        // printf("%s %s\n", target->head->oripath, target->head->backupPath);
        
        target->statbuf = curstat;
        // target->childscnt = cnt - 1;
        target->childs = (filedir**)malloc(sizeof(filedir*) * cnt);
        for (int i =0 ; i< cnt;i++) {
            if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;
            // if (!strcmp(namelist[i]->d_name, "ssubak.log")) continue;
            strcpy(nextpath, target->head->backupPath);
			strcat(nextpath, "/");
			strcat(nextpath, namelist[i]->d_name);

			strcpy(nextoripath, target->head->oripath);
			strcat(nextoripath, "/");
			strcat(nextoripath, namelist[i]->d_name);

            // sprintf(nextpath, "%s/%s", target->head->backupPath, namelist[i]->d_name); //it is adding sequence, so there is only 1 backup,so this code works
            // sprintf(nextoripath, "%s/%s", target->head->oripath, namelist[i]->d_name);
            // printf("%s %s\n", nextoripath, nextpath); //debug
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
        addDirList(target, 1);
    }
	// destroyQueue(&q);
}


//useful
// int mkdirs(char * path) {// 
// 	char * top = getenv("HOME"); ///home/ph

/// @brief KILL ALL BELOW PATH AND ITSELF HAHA, can be used when backup is ruined
/// @param path file or dir path
/// @param mod 0 for remove all, 1 for only clears empty dirs
/// @return 
int rmdirs(char * dir, int mod) { 
	struct stat statbuf;
	// char temp[MAXPATH];
	// strcpy(temp, dir);
	int cnt;
	struct dirent **namelist;
	int res;
	if (lstat(dir, &statbuf) < 0) {

		return errno;
	}
	if (S_ISREG(statbuf.st_mode) || S_ISLNK(statbuf.st_mode)) {
		if (mod == 0) {
			remove(dir);
		}
	}
	if (!S_ISDIR(statbuf.st_mode)) return 0;
	if ((cnt = scandir(dir, &namelist, NULL, alphasort)) < 0) {
		return -1;
	}
	for (int i= 0;i < cnt; i++) {
		if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;
		char nextpath[MAXPATH];
		sprintf(nextpath, "%s/%s", dir, namelist[i]->d_name);
		int res = rmdirs(nextpath, mod);
		// printf("%d", res);
	}
	if (cnt == 2) rmdir(dir);
}

/// @brief lower cost than rmdirs, get lowest(child) dir path and clear it 
/// @param dir 
/// @return 
int clear_empty_dirs(char * dir) {	
	char *stop = "/home/backup";
	char temp[MAXPATH];
	struct dirent ** namelist;
	int res = 0;
	strcpy(temp, dir);
	while(1) {
		int cnt = 0;
		
		printf("chekcing %s is empty dir\n", temp);
		if (!strcmp(stop, temp)) break;
		if ((cnt = scandir(temp, &namelist, NULL, alphasort)) < 0) return -1;
		if (cnt == 2) rmdir(temp);
		strcpy(temp, substr(temp, 0, return_last_name(temp)));
	}
	
}

/**
 * TODO: if all things are completed arged next : pathpair log
 * 		then remove annotation on remove(t->)
 * 
*/
int remove_backup(backupNode * t, char * actiontime, int funcmod) {
	if (remove(t->backupPath)) {
		fprintf(stderr, "error while removing target %s\n", t->backupPath);
	}
	char *upperdir = substr(t->backupPath, 0, return_last_name(t->backupPath));
	int cnt;
	
	printf("target remove path : %s\n", t->backupPath);
	printf("target remove dir : %s\n ", upperdir);
	printf("removed %s!\n", t->stamp);
	if (!funcmod) make_log(t->backupPath, t->oripath, actiontime, 1); //need sth change
	clear_empty_dirs(upperdir);
}

int compare_md5_backups(char * path);
/// @brief just restore file from backup(no bfs), bfs is called by outer caller(bfs_worker_mockfs), only file comes in
/// @param t backupnode that FILE !!!111
/// @param newpath if mod has 8, then path is newpath
/// @param root root of the path, t->oripath - root = relpath
/// @param mod 
/// @return 
int restore_backup(backupNode * t, char * newpath, char * root, char * stamp, int mod) { //if mod & 8 != 0 -> path is not null, path is dir or file
	char restorepath[MAXPATH];
	
	char backup_path[MAXPATH];
	strcpy(backup_path, t->backupPath);
	// printf(":::::::; %s ::::::::\n", backup_path);
	// printf(":::::::: %s ::::::::\n", t->oripath);
	char * filename = substr(backup_path, return_last_name(backup_path) + 1, strlen(backup_path));
	char * command_root = substr(root, 0, (strstr(root, ".") == NULL) ? strlen(root) : return_last_name(root));
	char * relpath = substr(t->oripath, strlen(command_root) + 1, strlen(t->oripath));
	// printf("}}%s\n", filename);
	// printf("++%s\n", newpath); //dir that file will be stored.
	// printf("---%s\n", root); //where the command issued
	// printf("UU %s\n", command_root);
	// printf("||%s\n",relpath);

	// if (access(command_root, F_OK) && (mod & 8) == 0) mkdir(command_root, 0777);
	int res1, res2;
	char ** restorepath_args = NULL;
	char target_path[MAXPATH] = "";
	// char stored_new[MAXPATH];
	// char stored_rel[MAXPATH];

	// strcpy(stored_new, newpath);
	// strcpy(stored_rel, relpath);
	if ((mod & 8) != 0)
		restorepath_args = split(newpath, "/", &res1);
	else {
		restorepath_args = split(command_root, "/", &res1);
	}
	char ** relpath_args = split(relpath, "/", &res2);
	for (int i =0 ; i< res1; i++) {
		// sprintf(target_path, "%s/%s", target_path, restorepath_args[i]);
		strcat(target_path, "/");
		strcat(target_path, restorepath_args[i]);
		if (access(target_path, F_OK)) mkdir(target_path, 0777);
	}
	for (int i = 0; i < res2 - 1; i++) {
		// sprintf(target_path, "%s/%s", target_path, restorepath_args[i]);
		strcat(target_path, "/");
		strcat(target_path, relpath_args[i]);
		if (access(target_path, F_OK)) mkdir(target_path, 0777);
	}
	strcat(target_path, "/");
	strcat(target_path, relpath_args[res2 - 1]);

	int fd, tofd;

	if (!access(target_path, F_OK)) { //check if same file exists in dir if exists then dont restore
		int chk = compare_md5_backups(target_path);
		if (chk == -1) {
			printf("md5 chk error");
			return -1;
		}
		if (chk == 1 && (mod & 8) == 0) { //same file exists also no -n -> no need to recover backup
			if (make_log(target_path, t->backupPath, stamp, 4) < 0) { //md5
				printf("log error");
				// errorcode = -3;				
				//rollback
				return -1;
			}
			return 1;
		}

	}


	if ((fd = open(t->backupPath, O_RDONLY)) < 0) {
		fprintf(stderr, "open error");
		exit(1);
	}
	
	// printf("restore location: %s\n", target_path);
	if ((tofd = open(target_path, O_TRUNC|O_CREAT|O_WRONLY, 0777)) < 0) {
		fprintf(stderr, "failed to open restore path");
		exit(1);
	}
	int len;
	char buf[1024];
	while((len = read(fd, buf, 1024)) > 0) {
		write(tofd, buf, len);
	}
	make_log(t->backupPath, target_path, stamp, 2);
	return 0;
	// printf("restored %s!", t->stamp); 
}

//////////////////// worker(special util) zone ////////////////////
/**
 * functions that similar to utils but not works with "basic" operations on structs ...
 *  like : queue pop front ... etc
 * workers : ex) backup remove... are here, all functional actions are occur here
 *          workers only produce path, open file. throws opened fds from open() to
 *          makers
 * gets
 * show ers : are workers that made to only show things
 * search  : search target files from maindirlist
 * compare_md5 : md5 compare util
*/


int compare_md5(char * path, char * path2);
int compare_md5_backups(char * path);

/// @brief TIMESTAMP PPAKURI
/// @return 12 len timestamp
char *getDate() {
	char *date = (char *)malloc(sizeof(char) * 14);
	time_t timer;
	struct tm *t;

	timer = time(NULL);	
	t = localtime(&timer);

  sprintf(date, "%02d%02d%02d%02d%02d%02d",t->tm_year %100, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
  
  return date;
}
char * getHome() {
	char * cwd = getcwd(NULL, 0);
	int res;
	char ** args = split(cwd, "/", &res);
	char * home = (char*)malloc(sizeof(char) * (MAXPATH));
	strcpy(home, "/");
	strcat(home, args[0]);
	strcat(home, "/");
	strcat(home, args[1]);
	free(cwd);
	free(args);
	return home;
}
/// @brief make dfs menulist to select
/// @param menus 
/// @param target 
/// @param lv 
void dfs_worker(menulist * menus, filedir * target, int lv) {
    for (int i=0 ;i <= target->childscnt; i++) {
        filedir * child = target-> childs[i];
        push_menu(menus, child, lv);
        if (child->childscnt != -1) //dir;
            dfs_worker(menus, child, lv + 1);
    }
}


filedir * search_from_dirlist(char * goodpath);

/**
 * TODO: 324 get return from restore, : fail or suc
 * 		if suc, remove if fail no remove
*/
/// @brief get file / dir path, do backup or recover by funcmod, controls actions by mod
/// @param target_path from path
/// @param newpath   to path, when remove, it is null/ when recover and mod has -n, then has new path
/// @param funcmod 0 remove, 1 recover
/// @param mod     0 nothin, &1 -d, &2 -r, &4 -l or -a, &8  -n
/// @return succeed?
int bfs_worker_mockfs(char * target_path, char * newpath, int funcmod, int mod) { //funcmod 0 remove only 1 recover, 
	//mod1 then path is not null
	//assume target_path is good_path
	queue q = *initQueue();
	// filedir * head;
	char temp[MAXPATH]; //root path
	strcpy(temp, target_path);
	filedir * head = search_from_dirlist(temp);
	if (head == NULL) return -1;
	// char * stamp = "34434434"; //timestamp
	char * stamp = getDate();

	q.push(&q, head);
	while(!q.empty(&q)) {
		filedir * t = q.front(&q);
		q.pop(&q);
		printf("target : %s", t->name);
		if (t->childscnt == -1) {//file
			if (t->head->next == NULL) {//only 1 backup exists
				int res = 0;
				if (funcmod) res = restore_backup(t->head, newpath, temp, stamp, mod);
				if (!res) remove_backup(t->head, stamp, funcmod);
				else if (res < 0) {
					printf("got error while doing recover!");
					exit(1);
				}
				continue;
			}
			backupNode * backups = t->head;
			backupNode * prev;
			int i = 0;
			if ((mod & 4) == 0) { //no -l or -a option
				printf("backup files of %s\n", t->path);
				printf("0. exit\n");
			}
			while(backups) {
				i++;
				if ((mod & 4) != 0 && funcmod == 0) //remove -a option
					remove_backup(backups, stamp, funcmod);
				if ((mod & 4) == 0) {
					printf("%d    %s    %ldbytes\n", i, backups->stamp, backups->statbuf.st_size);
				}
				prev = backups;
				backups = backups->next;
			}
			if ((mod & 4) == 0) { // no -l or -a option, select target
				int input = 0;
				printf(">>");
				scanf("%d", &input);
				if (input == 0) exit(0);
				backups = t->head;
				i = 0;
				while(backups) {
					i++;
					if (i == input) break;
					backups = backups->next;
				}
				int res = 0;
				if (funcmod) res = restore_backup(backups, newpath, temp, stamp, mod);
				if (!res) remove_backup(backups, stamp, funcmod);
				else if (res < 0) {
					printf("got error while doing recover!");
					exit(1);
				}
				continue;
			}


			if ((mod & 4) != 0 && funcmod == 0) //remove -a option
				continue;
			if ((mod & 4) != 0 && funcmod == 1) {//recover -l option
				int res = restore_backup(prev, newpath, temp, stamp, mod);
				if (!res) remove_backup(prev, stamp, funcmod);
				else if (res < 0) {
					printf("got error while doing recover!");
					exit(1);
				}
			}
			
			continue;
		}
		//dir
		for (int i = 0; i <= t->childscnt; i++) {
			if (t->childs[i]->childscnt != -1) {//dir
				if ((mod & 2) == 0) continue;
			}
			q.push(&q, t->childs[i]);
		}
	}
	// destroyQueue(&q);
}

/// @brief contains bfs, can manage file / dir backup
/// @param path must be good path, which means return from realpath()
/// @param mod backup action mod, mod |= 1 : -d    |= 2 : -r    |= 4 : -y
/// @return 0 succeed minusts -> errocode
int bfs_worker_realfs(char * path, int mod) { 
	int fd;
	int target_fd;
	int cnt = 0;
	int errorcode = 0;
	char * stamp = getDate(); //stamp

	struct stat statbuf;
	struct dirent ** namelist;

	
	char backup_path[MAXPATH];
	char linked_dir_path[MAXPATH]; // -> if path is txt, then it is parent dir, dir then dir itself
	//path below is ROOT
	//CAN BE USED IN RMDIRS TO KILL ALL
	char root_path[MAXPATH];

	char purename[MAXDIR];

	char temp_path[MAXDIR] = "/";
	
	sprintf(purename, "%s", substr(path, return_last_name(path) + 1, strlen(path)));
	printf("\n-------%s--------\n", purename);

	sprintf(backup_path, "%s/%s%s", "/home/backup", stamp, (strstr(path, ".") == NULL) ? "" : strcat(temp_path, purename)); //
	sprintf(root_path, "%s/%s", "/home/backup", stamp);
	// //file -> /home/backup/stamp/a.txt
	// //dir -> /home/backup/stamp/
	sprintf(linked_dir_path, "%s", substr(path, 0, (strstr(path, ".") != NULL) ? strlen(path) : return_last_name(path)));
	// //file -> b/a.txt
	// //dir -> b
	printf("backup : %s\n root : %s \n linked : %s\n", backup_path, root_path, linked_dir_path);
	if (mkdir(root_path, 0777) < 0) {
		fprintf(stderr, "NO SUDO");
		exit(1);
	}
	

	queue q = *initQueue();
	pathpair *p = (pathpair*)malloc(sizeof(pathpair));
	sprintf(p->first, "%s", backup_path); // ./<stamp>/purename
	sprintf(p->second, "%s", path);      // file or dir
	q.push(&q, p);
	while(!q.empty(&q)) {
		pathpair *pt = q.front(&q);
		char * bpath = pt->first;   //backup
		char * lpath = pt->second;  //ori
		// printf("watching : %s %s ", bpath, lpath);
		q.pop(&q);
		if (lstat(lpath, &statbuf) < 0) {
			printf("lstat error, it is not real file");
			//rollback all
			errorcode = -1;
			break;
		}
		if (S_ISREG(statbuf.st_mode)) {//backup
			int chk = compare_md5_backups(lpath);
			if (chk == -1) {
				printf("md5 chk error");
				errorcode = -2;
				break;
			}
			if (chk == 1 && (mod & 4) == 0) { //same file exists but no -y
				if (make_log(lpath, bpath, stamp, 3) < 0) { //md5
					printf("log error");
					errorcode = -3;				
					//rollback
					break;
				}
				continue;
			}
			if ((fd = open(lpath, O_RDONLY)) < 0) {
				printf("open error");
				errorcode = -4;				
				//rollback
				break;
				//rollback
			}
			if ((target_fd = open(bpath, O_WRONLY | O_CREAT, 0666)) < 0) {
				printf("open error");
				errorcode = -4;				
				break;
			}
			if (make_backup(target_fd, fd) < 0) {
				printf("backup error");
				errorcode = -4;				
				break;
			}
			if (make_log(lpath, bpath, stamp, 0) < 0) {
				printf("log error");
				errorcode = -3;			
				break;
			}
			continue;
		}
		
		if (!S_ISDIR(statbuf.st_mode)) continue;
		if (mkdir(bpath, 0777) < 0) {
			if (errno != EEXIST) {
				printf("mkdir");
				errorcode = -5;				
				break;
				//rmdirs(root_path);
			}
		}
		if ((cnt = scandir(lpath, &namelist, NULL, alphasort)) < 0) {
			printf("scan error");
			errorcode = -3;				
			break;
			//rollback all, rmdirs will be best. provide /home/backup/<stamp> dir
		}
		for (int i = 0;i < cnt; i++) {
			if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;
			char next_bpath[MAXPATH];
			char next_lpath[MAXPATH];
			sprintf(next_bpath, "%s/%s", bpath, namelist[i]->d_name);
			sprintf(next_lpath, "%s/%s", lpath, namelist[i]->d_name);
			if (lstat(next_lpath, &statbuf) < 0) {
				printf("lstat 2");
				errorcode = -1;				
				break;
				//rollback
			}
			if ((mod & 2) == 0 && S_ISDIR(statbuf.st_mode))  {// -r not exists
				continue;
			}
			pathpair * nextp = (pathpair*)malloc(sizeof(pathpair));
			strcpy(nextp->first, next_bpath);
			strcpy(nextp->second, next_lpath);

			q.push(&q, nextp);
		}
	}
	if (errorcode < 0) rmdirs(root_path, 0);
	else {
		printf("here?");

		printf("target : %s\n", root_path);
		rmdirs(root_path, 1);
		// printf("%d\n", errno);
	}
	// // destroyQueue(&q);
}

int load_log() {
	char *path = "/home/backup/ssubak.log";
	FILE * fp;
	if ((fp = fopen(path, "rt")) == NULL) {
		fprintf(stderr, "got error while opening log");
		exit(1);
	}
	
	while(1) {
		char buf[3 * MAXPATH];
		int read = fscanf(fp, "%[^\n]\n", buf);
		if (read == EOF) break;
		// printf("log : %s\n", buf);
		
		int rs;
		char ** args = split(buf, "\"", &rs);
		if (!strcmp(args[2], logModList[0])) { //backuped to
			// printf("%s %s", args[0], args[2]);
			int padding = strlen("/home/backup/");
			char * stamp = substr(args[3], padding, padding + 12); //12
			
			// printf("%s %s\n", stamp, args[1]);
			pathpair * temp = (pathpair *)malloc(sizeof(pathpair));

			strcpy(temp->first, stamp);
			char * parent = substr(args[1], 0, return_last_name(args[1]));
			strcpy(temp->second, parent);

			// printf("saved!\n");
			search_and_addLog(temp);
		}
	}
}

/// @brief use bfs_fs_maker() and make filesystem
/// @return succeed?
int load_backup(){ 
    char *path = "/home/backup";
    char curname[4096];
    char origin[4096];
    struct dirent ** namelist;
    int cnt;
    if ((cnt = scandir(path, &namelist, NULL, alphasort)) < 0) return -1;
    // printf("%d\n", cnt);
    for (int i =0 ; i < cnt; i++) {
        if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;
        sprintf(curname, "%s/%s", path, namelist[i]->d_name);
        // printf("%s is on progress\n", curname);
        // int res = find_link(namelist[i]->d_name, origin);
		int res = searchLog(namelist[i]->d_name, origin);
        
        if (!res) {
            // printf("fked");
            continue;
        }
        
        bfs_fs_maker(curname, origin, namelist[i]->d_name);
		// printf("\n\n\n\n");
		// show_all();
    }
    return 1;
}


//////////////////showers, similar to do but for showing things
/// @brief show all things in mainDirList
/// @return --
int show_all() {
    dirpoint *temp = mainDirList->head;
    printf("\n\n");
    while(temp) {
		printf("\n");
        filedir * target = temp->node;
        printf("%s : %p\n",target->name, target);
        printf("path : %s\n", target->path);
        printf("backups : ");
        backupNode * b = target->head;
        while(b) {
            printf(" %s,", b->stamp);
            b = b ->next;
        }
        printf("\n");
		printf("childscnt : %d\n", target->childscnt);
        printf("childs : ");
        for (int i = 0; i <= target->childscnt; i++) {
            printf("%s: %p, ", target->childs[i]->name, target->childs[i]);

        }
        printf("\n");
        printf("size : %ld\n", target->statbuf.st_size);
        temp = temp->next;
    }
    return 1;
}

int show_log() {
	log * temp = loghead;
	while(temp) {
		pathpair * node = temp->node;
		printf("%s %s\n", node->first, node->second);
		temp = temp->next;
	}
}

void show_backup_help() {
	printf("\t> backup <PATH> [OPTION]...\n\t");
	printf("  <none> : backup file if <PATH> is file\n\t");
	printf("  -d : backup files in directory if <PATH> is directory\n\t");
	printf("  -r : backup files in directory recursive if <PATH> is directory\n\t");
	printf("  -y : backup file although already backuped\n");
}
void show_remove_help() {
	printf("\t> remove <PATH> [OPTION]...\n\t");
	printf("  <none> : remove backuped file if <PATH> is file\n\t");
	printf("  -d : remove backuped files in directory if <PATH> is directory\n\t");
	printf("  -r : remove backuped files in directory recursive if <PATH> is directory\n\t");
	printf("  -a : remove all backuped files\n");
}
void show_recover_help() {
	printf("\t> recover <PATH> [OPTION]...\n\t");
	printf("  <none> : recover backuped file if <PATH> is file\n\t");
	printf("  -d : recover backuped files in directory if <PATH> is directory\n\t");
	printf("  -r : recover backuped files in directory recursive if <PATH> is directory\n\t");
	printf("  -l : recover latest backuped file\n\t");
	printf("  -n <NEW_PATH> : recover backuped file with new path\n");
}
void show_list_help() {
	printf("\t> list [PATH]\n\t");
	printf("  >> rm <INDEX> [OPTION]...\n\t");
	printf("  >> rc <INDEX> [OPTION]...\n\t");
	printf("  >> vi(m) <INDEX>\n");
}

	//purify path
int get_good_path(char * p, char * goodpath) {
	int mod = 0;
	char abstract_path[MAXPATH];
	strcpy(abstract_path, p);
	char * cwd = getcwd(NULL, 0);
	char * respath = (char *)malloc(sizeof(char) * MAXPATH);
	char * truepath = realpath(abstract_path, NULL);
	// int cnt;
	// char ** args = split(abstract_path, "/", &cnt);
	if (truepath == NULL) {
		printf("not exists");
		int cnt = 0;
		char ** args = split(abstract_path, "/", &cnt);
		for (int i =0 ; i< cnt; i++) {
			printf("args : %s\n", args[i]);
		}
		
		mod = 1;
		if (!strcmp(args[0], ".") || !strcmp(args[0], "..")) //rel path
		{
			printf("real");
			strcpy(respath, cwd);
			for (int i= 1; i < cnt; i++) {
				if (!strcmp(args[i], ".")) continue;
				else if (!strcmp(args[i], "..")) {
					char * temp = substr(respath, 0, return_last_name(respath));
					printf("%s\n", temp);
					strcpy(respath, temp);
				}
				else {
					strcat(respath, "/");
					strcat(respath, args[i]);
				}
			}
			printf("result : %s\n", respath);
			strcpy(goodpath, respath);
		}
		else { //abpath
			printf("HERE");
			strcpy(respath, p);
		}
		printf("path ;; %s\n", respath);
		char * res = strstr(respath, getHome());
		if (res == NULL || (int)(res - respath) != 0) {
			return -1;
		}
		// printf("path ;; %s\n", respath);
		strcpy(goodpath, respath);
		return 0;
	}
	else {
		printf("truepath\n");
		printf("%s %s\n", truepath, getHome());
		char * home = getHome();
		char * res = strstr(truepath, home);
		printf("%p %p\n", res, truepath);
		if (res == NULL || (int)(res - truepath) != 0) {
			printf("bad");
			return -1;
		}
		printf("path ; %s\n", truepath);
		strcpy(goodpath, truepath);
		return 0;
	}
}

/**
 * need to call in show_list_command
*/
int remove_func(int argc, char*argv[]);
int recover_func(int argc, char*argv[]);
/**
 * TODO: change function form needed...
*/
filedir * search_target_dir(char * targetpath) {
	dirpoint * temp = mainDirList->head;
	dirpoint * top = mainDirList->head;
	filedir * res = NULL;
	int flag = 0;
	while(temp) {
		filedir * node = temp->node;
		if (targetpath != NULL && !strcmp(targetpath, node->path)) {
			res = node;
			flag = 1;
			break;
		}
		if (get_slash_cnt(top->node->path) > get_slash_cnt(node->path)) {
			top = temp;
		}
		temp = temp->next;
	}
	if (flag) {
		// printf("returning res");
		return res;
	}
	else {
		// printf("returning top");
		return top->node; // returns head of fs
	}
	
}
int show_list_command(char * path) { //4 : list
    // menulist * templist = (menulist *)malloc(sizeof(menulist));
    menulist * templist = init_menulist();
    char * targetpath = NULL; // /home/ph/ 
    if (path) {
		targetpath = realpath(path, NULL);
		if (targetpath == NULL) {
			fprintf(stderr, "bad path");
			exit(1);
		}
		char * ff = strstr(targetpath, getHome());
		if(ff == NULL || (int)(ff - targetpath) != 0) { // parent of home/ph or other root path
			fprintf(stderr, "invalid path");
			exit(1);
		}
	}
	if (mainDirList->head == NULL) {
		printf("empty!");
		return -1;
	}
    filedir * target = search_target_dir(targetpath);
	
	if (target == NULL) {
		printf("fe");
		return -1;
	}
	/**
	 * TODO: just modify dfs_worker to print file / dir correctly and list more better
	*/
		// backupNode * temp = target->head;
		// while(temp) {
		// 	menu * t = (menu*)malloc(sizeof(menu));
		// 	t->node = temp;
		// 	t->next = NULL;
		// 	push_menu(templist, t, 0);
		// 	printf("%s       %ldbytes")
		// 	temp = temp->next;
		// }
	
	push_menu(templist, target, 0); //segfault
	dfs_worker(templist, target, 0);
	
	//print templist of target (tree)
	menu * temp = templist->head;
	int numcnt = 0;
	printf("%d %s\n", temp->num, temp->node->path);
	temp = temp->next;
	while(temp -> next) {
		filedir * target = temp->node;
		printf("%d ", temp->num);
		for (int i =0 ; i< temp->lv; i++) printf("| ");
		if (target->childscnt == -1)
			printf("├ %.20s%20s  %10ldbytes\n", target->name, target->rear->stamp, target->rear->statbuf.st_size);
		else {
			printf("├ %s\n", target->name);
		}
		temp = temp->next;
	}
	printf("%d ", temp->num);
	for (int i =0 ; i< temp->lv; i++) printf("| ");
	printf("└ %.20s%20s   %10ldbytes\n", temp->node->name, temp->node->rear->stamp, temp->node->rear->statbuf.st_size);
	numcnt = temp->num + 1;
	
    // filedir * target = mainDirList->head->node;
	// printf("selected : %s\n", target->name);
    

    printf("\n>>");
    //scanf command
    char command[2048];
    int arglen = 0;
    scanf("%[^\n]s", command);
    printf("%s", command);
    char ** argstemp = split(command, " ", &arglen); //utils function, strtok all and return args
    char ** args = (char**)malloc(sizeof(char *) * (arglen + 1));
	args[0] = "";
	for (int i =1 ;i < arglen + 1; i++) {
		args[i] = argstemp[i - 1];
	}


    if (arglen < 2) {
		printf("wrong command");
		exit(1);
	}

	int sel = atoi(args[2]);
	if (sel >= numcnt) { //checking sel num error
		printf("wrong number");
		exit(1);
	}
	if (sel == 0 && strcmp(args[2], "0") != 0) {
		printf("wrong number");
		exit(1);
	}
	temp = templist->head;
	while(temp) {
		if (temp->num == sel) break;
		temp = temp->next;
	}
	filedir * select = temp->node;
	args[2] = select->path;
	printf("%s", args[2]);
    if (!strcmp(args[1], "rm")) {     //call remove_func with args
        remove_func(arglen + 1, args);
		return 0;
    }
    else if (!strcmp(args[1], "rc")) { //call recover_func with args
		recover_func(arglen + 1, args);
		return 0;
    }
    else if (!strcmp(args[1], "vi")) {
		//exec fork vi
		// printf("hello vi");
		return 0;
    }
	else {
		printf("wrong comand");
		// break;
	}
    destroy_all(templist);
	exit(0);
}

filedir * search_from_dirlist(char * goodpath) {
	dirpoint * temp = mainDirList->head;
	while(temp) {
		filedir * node = temp->node;
		if (!strcmp(goodpath, node->path)) {
			return node;
		}
		temp = temp ->next;
	}
	return NULL;
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

/// @brief compare md5
/// @param path 
/// @param path2 
/// @return 0 -> same file 1 -> diff file
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
/// @brief check whether there is backup that has same path, file
/// @param path : must be real
/// @return 0 no backup that same 1 : there exists backup that has same
int compare_md5_backups(char * path) {
	filedir * exists = search_from_dirlist(path);
	if (exists != NULL) {
		backupNode * temp = exists->head;
		int chk = 0;
		while(temp) {
			int compres = 0;
			if ((compres = compare_md5(temp->backupPath, path)) < 0) {
				return -1;
			}
			if (compres == 0) {//same file
				return 1;
			}
			temp = temp->next;
		}
	}
	return 0;
}



///////////////////vvvvvvv/ func zone vvvvvv/////////////////////
/**
 * commands r named like ~_func()
*/
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
			else {	 //err wrong flags
				printf("wrong flag!\n");
				exit(1);
			}
		}
	}
	int errcode = 0;
	printf("path : %s mod : %d\n", strict_path, mod);
	if (strstr(path, ".") == NULL) {//dir
		if ((mod & 3) == 0) {
			fprintf(stderr, "wrong flag");
			exit(1);
		}
	}
	else {
		if ((mod & 3) != 0) {
			fprintf(stderr, "target is file but flag is about dir");
			exit(1);
		}
	}
	if ((errcode = bfs_worker_realfs(strict_path, mod)) < 0) //failure of backup 
	{
		printf("error no : %d", errcode);
		return;
		//error, make stderr
	}
}
int remove_func(int argc, char*argv[]) {
	if (argc <= 2) { //경로미입력
		fprintf(stderr, "USAGE");
		exit(1);
	}
	if (strlen(argv[2]) > MAXPATH) exit(1); //길이제한
	int mod = 0;
	if (argc > 3){
		for (int i = 3; i < argc; i++) {
			if (!strcmp(argv[i], "-d"))
				mod = mod | 1;
			else if (!strcmp(argv[i], "-r"))
				mod = mod | 2;
			else if (!strcmp(argv[i], "-a"))
				mod = mod | 4;
			else { //err wrong flags
				printf("wrong flag!\n");
				exit(1);
			}
		}
	}
	
	char good_path[MAXPATH];
	if (get_good_path(argv[2], good_path) < 0) { //bad path or path is parent of "HOME"
		// fprintf(stderr, "this dir cannot be removed or backuped!");
		printf("BADABADBADBfff");
		exit(1);
	}
	char * target_name = substr(good_path, return_last_name(good_path) + 1, strlen(good_path));
	printf("[[]]target : %s\n",target_name);
	printf("mod : %d\n", mod);
	if (strstr(target_name, ".") == NULL) {
		if ((mod & 3) == 0) {
			printf("dir but no flag needed");
			exit(1);
		}
	}
	else { //file
		if ((mod & 3) != 0) {
			printf("file but flag for dir exists");
			exit(1);
		}
	}

	if (bfs_worker_mockfs(good_path, NULL, 0, mod) < 0) {
		printf("file not exists in backup\n");
		exit(1);
	}
}
int recover_func(int argc, char* argv[]) {
	// printf("hello recover");
	if (argc <= 2) { //경로미입력
		fprintf(stderr, "USAGE");
		exit(1);
	}
	if (strlen(argv[2]) > MAXPATH) exit(1); //길이제한
	int mod = 0;
	int newpathidx = 0;
	char newname[MAXDIR];
	if (argc > 3){
		for (int i = 3; i < argc; i++) {
			if (!strcmp(argv[i], "-d"))
				mod = mod | 1;
			else if (!strcmp(argv[i], "-r"))
				mod = mod | 2;
			else if (!strcmp(argv[i], "-l"))
				mod = mod | 4;
			else if (!strcmp(argv[i], "-n")) {
				mod = mod | 8;
				i = i + 1;
				if (i + 1 > argc) { //no path after -n
					fprintf(stderr, "no path after -n");
					exit(1);
				}
				newpathidx = i;
				sprintf(newname, "%s", argv[i]);
			}
			else { //err wrong flags
				printf("wrong flag!\n");
				exit(1);
			}
		}
	}
	
	char good_path[MAXPATH];
	if (get_good_path(argv[2], good_path) < 0) { //bad path or path is parent of "HOME"
		// fprintf(stderr, "this dir cannot be removed or backuped!");
		printf("BADABADBADB");
		exit(1);
	}

	//chk flag was right
	char * target_name = substr(good_path, return_last_name(good_path) + 1, strlen(good_path));
	printf("[[]]target : %s\n",target_name);
	printf("here");
	if (strstr(target_name, ".") == NULL) {//dir 
		if ((mod & 3) == 0) {
			printf("dir but no flag needed");
			exit(1);
		}
	}
	else { //file
		if ((mod & 3) != 0) {
			printf("file but flag for dir exists");
			exit(1);
		}
	}
	 


	if ((mod & 8) != 0) {
		char good_newpath[MAXPATH];
		if (get_good_path(argv[newpathidx], good_newpath) < 0) {
			fprintf(stderr, "bad path");
			exit(1);
		}
		printf("newpath : %s\n", good_newpath);
		if (bfs_worker_mockfs(good_path, good_newpath, 1, mod) < 0) {
			fprintf(stderr, "file not exists in backup");
			exit(1);
		}
		return 0;
	}
	
	if (bfs_worker_mockfs(good_path, NULL, 1, mod) < 0) {
		printf("file not exists in backup");
		exit(1);
	}
	
}
void list_func(int argc, char *argv[]) {
	// printf("hello list");
	if (argc > 3) {
		fprintf(stderr, "usage");
		exit(1);
	}
	if (argc == 2) {
		show_list_command(NULL);
		exit(0);
	}
	show_list_command(argv[2]);
}
void help_func(int argc, char *argv[]) {
	// printf("hello help");
	int flag = 0;
	if (argc > 3) {
		fprintf(stderr, "e");
		exit(1);
	}
	if (argc == 3) {
		flag |= (!strcmp(argv[2], "backup")) ? 1 : 0;
		flag |= (!strcmp(argv[2], "remove")) ? 2 : 0;
		flag |= (!strcmp(argv[2], "recover")) ? 4 : 0;
		flag |= (!strcmp(argv[2], "list")) ? 8 : 0;
	}
	printf("Usage: ");
	if (flag == 0) printf("\n");
	if ((flag & 1) != 0 || flag == 0) {
		show_backup_help();
		if (flag != 0) return;
	}
	if ((flag & 2) != 0 || flag == 0) {
		show_remove_help();
		if (flag != 0) return;
	}
	if ((flag & 4) != 0 || flag == 0) {
		show_recover_help();
		if (flag != 0) return;
	}
	if ((flag & 8) != 0 || flag == 0) {
		show_list_help();
		if (flag != 0) return;
	}	
	if (flag == 0) printf("\t> help [COMMAND]\n");

}

//////////////////////////vvvvvvvv// main //////////////////////
int main(int argc, char * argv[]) {
	int i = 0;
	check_valid_runloc();
	// printf("HOME : %s\n", getHome());
	initEnum();	
	initBackupDir();

	//make mock filesystem in program and save it to mainDirList, maybe dont need when just do backup
	initDirList();
	load_log();

	// printf("---------------------\n");
	// show_log(); // log, debug
	
	load_backup();
	// show_all();//show result of load_backup, debug


	// char *temp = "/home/backup";
	// rmdirs(temp);

	//for (i = 0; i < argc; i++) {
	//	printf("%s\n", argv[i]);f
	//}
	//printf("%d", argc);
	if (argc < 2) {
		printf("ERROR: wrong input\n");
		printf("%s help : show commands for program\n", argv[0]);
		exit(1);
	}
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
			help_func(argc, argv);
			break;
		default: //errorcom 
			{
				printf("WRONG command");
				break;
			}
	}
}

