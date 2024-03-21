#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

#include <openssl/md5.h>

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
	"backuped to", "removed by", "recovered to", "already backuped to", "not changed with"
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
// typedef struct pathpair { //get from log file
//     char stamp[4096]; //first
//     char oripath[4096]; //second
// }pathpair;


/////////////////// Log struct for .log file//////////

typedef struct Log{
	// unsigned char md[MD5_DIGEST_LENGTH]; //get this when read .log file              if md1, absolute_path1 == md2 abpath2 -> they are same (-y)
	char absolute_path_dir[MAXPATH]; //linked dir
	int timestamp;          //home/backup/<timestamp>
	char pure_path[MAXPATH];    //path til name          timestamp + pure_path == backup_path ((ex) > home/backup/<timestamp>/b/a.txt)
	char purename[MAXDIR];       //-> absolute_path_dir + pure_path == absolute_path   if pure_path == purename -> file else file inside dir
	struct Log * next;
}log;


log * logList; //linked list to save Logs

log * initLog() {

}

log ** searchbystamp(int timestamp) {

}
log ** searchbypath(char * absolute_path) { //absolute_path_dir

}



/////////////////////vv pathpair for bfs ///////////

typedef struct {
	char first[MAXPATH];//?
	char second[MAXPATH];//?
}pathpair;


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

/// @brief 
/// @param t 
/// @return filedir that changed filedir *, : if there is dup, then older one returns, if not, returns same filedir from param
filedir * addDirList(filedir *t) { //중복 들어올 시 백업만 먹고 까버리기 필요
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
        return t;
    }
    char t1[4096];
    char t2[4096];
    while(temp) {
        memset(t1, 0,sizeof(t1));
        memset(t2, 0,sizeof(t2));
        printf(",");
        // if (temp->node == NULL) {
        //     printf("ffffffffff");
        // }
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
		return t;
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
            exists->childs = realloc(exists->childs, rescnt);
            for (int i = 0; i< rescnt; i++) {
                exists->childs[i] = templist[i];
                printf("%s : %p\n",templist[i]->name, templist[i]);
            }
            
            /**
             * TODO: add two pointer templist to original childs
            */
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
            // free(t); ???????????
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
int get_slash_cnt(char * t) {
    int res = 0;
    for (int i=0 ;i < strlen(t); i++) {
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
        temp[cnt] = (char *)malloc(sizeof(char) * strlen(arg) + 1);
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
	sprintf(logfile_path, "%s/%s", backup_path, "ssubak.log");
	if (access(logfile_path, F_OK))
		creat(logfile_path, 0777);
}
void initBackupLog() { //io log file
	//get log from logfile_path, 
}
void initDirList() {
	mainDirList = (dirList*)malloc(sizeof(dirList));
    mainDirList->head = NULL;
    mainDirList->tail = NULL;
    mainDirList->size = 0;
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
//TODO need more parameter for struct Log, expecially timestamp?
int make_log(char* target_path, char*path, int mod) {
	char log[10000];
	sprintf(log, "\"%s\" %s \"%s\"", path, logModList[mod], target_path);
	printf("%s\n", log);
	if (mod < 3) { //log file io, add linked list

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

int remove_backup(backupNode * t) {
	printf("removed %s!", t->stamp);
}
int restore_backup(backupNode * t, char * path, int mod) { //if mod & 8 != 0 -> path is not null
	if ((mod & 8) != 0) {
		printf("restored %s to new dir %s", t->stamp, path);
		return 1;
	}
	printf("restored %s!", t->stamp);
}

//////////////////// worker(special util) zone ////////////////////
/**
 * functions that similar to utils but not works with "basic" operations on structs ...
 *  like : queue pop front ... etc
 * workers : ex) backup remove... are here, all functional actions are occur here
 *          workers only produce path, open file. throws opened fds from open() to
 *          makers
 * 
 * show ers : are workers that made to only show things
 * search  : search target files from maindirlist
 * compare_md5 : md5 compare util
*/

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

int bfs_worker_backup(char * target_path, char * path, int mod) {//mod not needed, only for backup
	int fd, target_fd;
	queue q;
	q = *initQueue();// cpp abstract code
	char tempname[MAXDIR];
	char temppath[MAXPATH];
	char tempTargetPath[MAXPATH];
	struct stat tempstat;
	struct dirent * dentry;

	pathpair pair;
	
	mkdir(target_path, 0777);
	sprintf(pair.first, "%s", path);
    sprintf(pair.second, "%s", target_path);	
	q.push(&q, &pair);

	while(!q.empty(&q)) {
		pathpair* temp = (pathpair *)q.front(&q);
		char* curpath = temp->first;
		char* target_curpath = temp->second;
		DIR * x; 
		printf("watching: %s, %s\n", curpath, target_curpath);
		if ((x = opendir(curpath)) == NULL || chdir(curpath) == -1) {
			printf("fked\n");
			return -12; //bfs err
		}
		q.pop(&q);
		while((dentry = readdir(x)) != NULL) {
			if (dentry -> d_ino == 0)
				continue;
			memcpy(tempname, dentry->d_name, MAXDIR);
			if (!strcmp(tempname, ".") || !strcmp(tempname, ".."))
				continue;
			memset(temppath, 0, sizeof(temppath));
			memset(tempTargetPath, 0, sizeof(tempTargetPath));
			if (stat(tempname, &tempstat) == -1) {
				printf("fked2\n");
				return -13; //stat err
			}
			
			sprintf(temppath, "%s/%s", curpath, tempname); //path
			sprintf(tempTargetPath, "%s/%s", target_curpath, tempname); //backup 
			printf("spreading \n %s \n%s\n\n", temppath, tempTargetPath);
			// strcat(target_path, "/");
			// strcat(target_path, tempname);
			if (S_ISREG(tempstat.st_mode)) {
				
				//save file in backup
				if ((fd = open(temppath, O_RDONLY)) < 0) { //path need modi
					printf(";\n");
					return -7; //open er
				}
				/**
				 *  TODO: mod y for existing check here, same on above. 
				 */
				if ((target_fd = open(tempTargetPath, O_WRONLY | O_CREAT, 0777)) < 0) {
					printf("=2\n");
					return -2; //open er, target_path also need mod , no sudo
				}
				if (make_backup(target_fd, fd) < 0) { //change func by func
					close(fd);
					close(target_fd);
					// remove();
					printf("error");
					return -4;
				}
				if (make_log(temppath, tempTargetPath, mod) < 0) {
					return -5; //log error
				}
				close(fd);
				close(target_fd);
			}
			if (S_ISDIR(tempstat.st_mode) && (mod & 2) != 0) {
				//mkdir in backup
				mkdir(tempTargetPath, 0777);
				pathpair temp2;
				sprintf(temp2.first, "%s", temppath);
				sprintf(temp2.second, "%s", tempTargetPath);
				q.push(&q, &temp2);
			}
		}
	}
}


filedir * search_from_dirlist(char * goodpath);
/// @brief get file / dir path, do backup or recover by funcmod, controls actions by mod
/// @param target_path from path
/// @param path   to path, when remove, it is null/ when recover and mod has -n, then has new path
/// @param funcmod 0 remove, 1 recover
/// @param mod     0 nothin, &1 -d, &2 -r, &4 -l or -a, &8  -n
/// @return succeed?
int bfs_worker_mod(char * target_path, char * path, int funcmod, int mod) { //funcmod 0 remove only 1 recover, 
	//mod1 then path is not null
	//assume target_path is good_path
	queue q = *initQueue();
	// filedir * head;
	char temp[MAXPATH];
	strcpy(temp, target_path);
	filedir * head = search_from_dirlist(temp);
	if (head == NULL) return -1;
	q.push(&q, head);
	while(!q.empty(&q)) {
		filedir * t = q.front(&q);
		q.pop(&q);
		printf("target : %s", t->name);
		if (t->childscnt == -1) {//file
			if (t->head->next == NULL) {//only 1 backup exists
				remove_backup(t->head);
				if (funcmod) restore_backup(t->head, path, mod);
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
					remove_backup(backups);
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
				backups = t->head;
				i = 0;
				while(backups) {
					i++;
					if (i == input) break;
					backups = backups->next;
				}
				remove_backup(backups);
				if (funcmod) restore_backup(prev, path, mod);
				continue;
			}


			if ((mod & 4) != 0 && funcmod == 0) //remove -a option
				continue;
			if ((mod & 4) != 0 && funcmod == 1) {//recover -l option
				remove_backup(prev);
				restore_backup(prev, path, mod);
			}
			
			continue;
		}
		//dir
		printf("fffsfsf\n\n");
		for (int i = 0; i <= t->childscnt; i++) {
			if (t->childs[i]->childscnt != -1) {//dir
				if ((mod & 2) == 0) continue;
			}
			q.push(&q, t->childs[i]);
		}

	}
}
int do_backup(char * path, int mod) {
	int fd;
	int target_fd;
	char * time = "34434434";
	// char * time = "34566666";
	// char * time = "34566669";
	char cwd[1024];
	struct stat info;
	struct dirent *dentry;
	DIR *dirp;

	if (getcwd(cwd, 1024) == NULL)
		return -3;
	if (lstat(path, &info) < 0) {
		return -1; //lstat error, file/dir not exists
	}
	//make path for backup
	char target_path[MAXPATH];
	//get time

	//tokenize provided path : /a/b/c.txt -> c.txt 
	//to get pure file name
	char temp[MAXPATH];
	sprintf(temp, "%s", path);
	char *ptr = strtok(temp, "/");
	char *tmp;
	char purename[MAXDIR];
	sprintf(purename, "%s", substr(temp, return_last_name(path) + 1, strlen(path)));
	printf("\n-------%s--------\n", purename);
	// if (ptr == NULL) {
	// 	sprintf(purename, "%s", ptr);
	// }
	// else {
	// 	while (1) {
	// 		tmp = ptr;
	// 		ptr = strtok(NULL, "/");
	// 		if (ptr == NULL) break;
	// 	}
	// 	sprintf(purename, "%s", tmp);
	// }
	//sprintf(target_path, "%s/%s/%s", backup_path, time, purename);
	strcpy(target_path, backup_path);
	strcat(target_path, "/");
	strcat(target_path, time);
	printf("%s", target_path);
	if (access(target_path, F_OK))
		mkdir(target_path, 0777);
	
	//printf("%d\n", strlen(target_path));
	printf("::::::::path : %s\n", path);
	if (S_ISREG(info.st_mode)) { //file
		strcat(target_path, "/");
		strcat(target_path, purename);
		//home/backup/<time>/a.txt
		printf("%s\n", target_path);
		if ((mod & 3) == 1 || (mod & 3) == 2) { //-d -r flag but file
			return -10; //flag error
		}
		if ((fd = open(path, O_RDONLY)) < 0)
			return -7; //open error
		if ((target_fd = open(target_path, O_WRONLY|O_CREAT, 0777)) < 0) {
			printf("errno: %d", errno);
			return -2; //open error, no sudo
		}
		if (make_backup(target_fd, fd) < 0) {
			close(target_fd);
			close(fd);
			remove(target_path);
			return -4; //make_backup error
		}
		//call logger
		if (make_log(path, target_path, 0) < 0) //TODO : get path from fd is needed
			return -5; //logger error
	}
	else if (S_ISDIR(info.st_mode)) { //path is dir
		if ((mod & 3) != 1 && (mod & 3) != 2) 
 			return -11; //flag error 2
		//if ((fd = open(path, O_RDONLY)) < 0) return -2; //open error
		printf("bfs called");
		return bfs_worker_backup(target_path, path, mod);
	}
	/*if ((fd = open(path, )) < 0) {
		return -1;
	}*/
	chdir(cwd);  //make working dir points orig dir may be useless
	return 0;
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
		printf("\n\n\n\n");
		show_all();
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
            printf("%s: %p,", target->childs[i]->name, target->childs[i]);

        }
        printf("\n");
        printf("size : %ld\n", target->statbuf.st_size);
        temp = temp->next;
    }
    return 1;
}

int get_good_path(char * p, char * goodpath) {
	//purify path
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
		char * res = strstr(respath, getenv("HOME"));
		if (res == NULL || (int)(res - respath) != 0) {
			return -1;
		}
		printf("path ;; %s\n", respath);
		strcpy(goodpath, respath);
		return 0;
	}
	else {
		char * res = strstr(truepath, getenv("HOME"));
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
void remove_func(int argc, char*argv[]);
void recover_func(int argc, char*argv[]);
/**
 * TODO: change function form needed...
*/
int show_list_command(char * path) { //4 : list
    // menulist * templist = (menulist *)malloc(sizeof(menulist));
    menulist * templist = init_menulist();
	char * curpath = getcwd(NULL, 0);
    char * targetpath = getenv("HOME"); // /home/ph/
    if (path) targetpath = realpath(path, NULL);
    // filedir * target = search_target_dir(targetpath);
    filedir * target = mainDirList->head->node;
    
    push_menu(templist, target, 0); //segfault
    dfs_worker(templist, target, 0);
    
    //print templist of target
    menu * temp = templist->head;
    while(temp) {
        filedir * target = temp->node;
        printf("%d %s\n", temp->num, target->name);
        temp = temp->next;
    }
    printf("\n>>");
    //scanf command
    char command[2048];
    int arglen = 0;
    scanf("%[^\n]s", command);
    // printf("%s", command);
    char ** args = split(command, " ", &arglen); //utils function, strtok all and return args
    
    if (arglen < 2) return -1;
    if (!strcmp(args[0], "rm")) {     //call remove_func with args
        
    }
    else if (!strcmp(args[0], "rc")) { //call recover_func with args

    }
    else if (!strcmp(args[0], "vi")) {
		
    }
    else { //err
        fprintf(stderr, "fucked internal command");
    }
    destroy_all(templist);
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
			else { //err wrong flags
				printf("wrong flag!\n");
				exit(1);
			}
		}
	}
	int errcode = 0;
	printf("path : %s mod : %d\n", path, mod);
	// if ((errcode = do_backup(strict_path, mod)) < 0) //failure of backup 
	// {
	// 	printf("error no : %d", errcode);
	// 	return;
	// 	//error, make stderr
	// }
	printf("hfffffff");
}
void remove_func(int argc, char*argv[]) {
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
		printf("BADABADBADB");
		exit(1);
	}
	char * target_name = substr(good_path, return_last_name(good_path) + 1, strlen(good_path));
	printf("[[]]target : %s\n",target_name);
	printf("mod : %d\n", mod);
	if (strstr(target_name, ".") == NULL) {//dir {
		if ((mod & 3) == 0) return -1;
	}
	else { //file
		printf("file");
		if ((mod & 3) != 0) return -1;
	}
	/**
	 * TODO: mod needed
	*/
	if (bfs_worker_mod(good_path, NULL, 0, mod) < 0) {
		printf("file not exists in backup\n");
		exit(1);
	}
}
void recover_func(int argc, char* argv[]) {
	printf("hello recover");
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
	if (strstr(target_name, ".") == NULL) {//dir 
		if ((mod & 3) == 0) return -1;
	}
	else { //file
		if ((mod & 3) != 0) return -1;
	}

	//chk newpath(path) is null


	if ((mod & 8) != 0) {
		bfs_worker_mod(good_path, argv[newpathidx], 1, mod);
		return;
	}
	
	if (bfs_worker_mod(good_path, NULL, 1, mod) < 0) {
		printf("file not exists in backup");
		exit(1);
	}
	/**
	 * TODO: mod needed
	*/
	
}
void list_func() {
	printf("hello list");
	show_list_command(NULL);
}
void help_func() {
	printf("hello help");
}

//////////////////////////vvvvvvvv// main //////////////////////
int main(int argc, char * argv[]) {
	int i = 0;
	initEnum();	
	initBackupDir();
	initBackupLog(); //get backuplog and make pathpair or sth

	//make mock filesystem in program and save it to mainDirList, maybe dont need when just do backup
	initDirList();
	load_backup();
	show_all();//show result of load_backup,

	//for (i = 0; i < argc; i++) {
	//	printf("%s\n", argv[i]);
	//}
	//printf("%d", argc);
	if (argc < 2) exit(1);
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
			help_func();
			break;
		default: //errorcom 
			{
				break;
			}
	}
}

