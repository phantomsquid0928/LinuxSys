// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <sys/types.h>
// #include <sys/stat.h>
// #include <string.h>

// #define MAXPATH 4096

// typedef struct loglist {
//     char log[MAXPATH * 2];
//     struct loglist * next;
// }loglist;
// loglist * head = NULL;
// loglist * rear = NULL;

// loglist * newlog() {
//     loglist * temp = (loglist*)malloc(sizeof(loglist));
//     if (temp == NULL) {
//         printf("FATAL: NO MEM");
//         exit(100);
//     }
//     temp->next = NULL;
//     return temp;
// }
// void addlog(char * target) {
//     loglist * temp = newlog();
//     strcpy(temp->log, target);
//     if (head == NULL) {
//         head = temp;
//         rear = temp;
//         return;
//     }
//     rear->next = temp;
//     rear = temp;
// }
// char * islogexists(char * target) {
//     loglist * temp = head;
//     while(temp) {
//         if (!strcmp(target, temp->log)) {
//             return 
//         }
//     }
//     return NULL;
// }
// void freelog() {
//     loglist * temp = head;
//     loglist * prev;
//     while(temp) {
//         prev = temp;
//         temp = temp->next;
//         free(prev);
//     }
// }

// int load_staging_log() {
//     char * cwd = getcwd(NULL, 0);
//     char stagelogpath[MAXPATH];

//     sprintf(stagelogpath, "%s/.repo/.staging.log");
//     if (access(stagelogpath, F_OK)) return -1;
//     char buf[MAXPATH * 2];
//     FILE * fp;

//     if ((fp = fopen(stagelogpath, "rt")) == NULL) {
//         return -2;
//     }
//     while(1) {
//         int res = fscanf(fp, "%[^\n]", buf);
//         if (res == EOF) break;
        
        
//     }
//     return 0;
// }

#include "phantomutils.h"

int main(int argc, char * argv[]) {
    if (argc == 1) {
        printf("ERROR <PATH> is not include\n");

        /**
         * TODO: usage
        */
        exit(1);
    }
    if (strlen(argv[1]) > MAXPATH) {
        printf("ERROR: '%s' is wrong path\n", argv[1]);
        exit(2);
    }
    if (argc > 2) {
        printf("ERROR: TOO MANY ARGS\n");
        exit(4);
    }
    init();
    int loadres = 0;
    if ((loadres = load_staging_log()) < 0) {
        if (loadres == -1) {
            printf("ERROR: repo didn't initialized, you have to call ssu_repo to init repo first");
        }
        // printf("FATAL: LOG FILE CORRUPTED OR NOT EXISTS");
        exit(3);
    }
    
    char * abpath = realpath(argv[1], NULL);
    if (abpath == NULL) {
        printf("ERROR: '%s' is wrong path\n", argv[1]);
        exit(2);
    }
    char cwd[MAXPATH];
    char relpath[MAXPATH]= ".";
    struct stat statbuf;

    getcwd(cwd, MAXPATH);
    char * chk = strstr(abpath, cwd);
    // printf("%d", (int)(chk - abpath));
    if (chk == NULL || (int)(chk - abpath) != 0) {//CHK IF abpath starts with cwd
        printf("path should not be parent of cwd\n");
        exit(5);
    }
    char * chk2 = strstr(abpath, repopath);
    if (chk2 != NULL || (int)(chk2 - abpath) == 0) {
        printf("path should not be child of ./.repo\n");
        exit(5);
    }

    if (access(abpath, R_OK) != 0 || access(abpath, W_OK) != 0) {
        printf("have no needed permission to path\n");
        exit(7);
    }
    if (lstat(abpath, &statbuf) < 0) {
        printf("lstat err\n");
        exit(6);
    }
    if (!S_ISREG(statbuf.st_mode) && !S_ISDIR(statbuf.st_mode)) {
        printf("this path is not file nor dir\n");
        exit(8);
    }


    //all conditions ok
    char abpath_copy[MAXPATH];
    strcpy(abpath_copy, abpath);

    char * temp = substr(abpath_copy, strlen(cwd), strlen(abpath_copy));
    strcat(relpath, temp);// need mod
    
    if (dellogrecurs(abpath) == 0) { //trying to remove but there was no valid target... //warning abpath becomes NULL, use abpath_copy
        printf("\"%s\" already removed from stage area\n", relpath);
        exit(0);
    }
    //success staging, add log
    printf("remove \"%s\"\n", relpath);
    save_staging_log(abpath_copy, 0);

    // show_staging_log();

    // printf("add \"%s\"\n", argv[1]);
}