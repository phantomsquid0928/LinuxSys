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
        printf("ERROR <NAME> is not include\n");

        /**
         * TODO: usage
        */
        exit(1);
    }
    if (strlen(argv[1]) > MAXDIR) {
        printf("ERROR: '%s' is wrong path\n", argv[1]);
        exit(2);
    }
    //init is essential
    init();

    char targetpath[MAXPATH];
    strcpy(targetpath, repopath);
    sprintf(targetpath, "/%s", argv[1]);
    if (access(targetpath, F_OK) == 0) {
        printf("%s is already exist in repo\n", argv[1]);
        exit(3);
    }
    if (head == NULL) //empty //test
    {
        printf("there is no staged file\n");
        exit(4);
    }


    //all conditions ok
    char abpath_copy[MAXPATH];
    strcpy(abpath_copy, abpath);

    char * temp = substr(abpath_copy, strlen(cwd), strlen(abpath_copy));
    strcat(relpath, temp);// need mod
    
    if (addlogrecurs(abpath) == 0) { //trying to stage but all files were same... //warning abpath becomes NULL, use abpath_copy
        printf("\"%s\" already exist in stage area\n", relpath);
        exit(0);
    }
    //success staging, add log
    printf("add \"%s\"\n", relpath);
    save_staging_log(abpath_copy, 1);

    // show_staging_log();

    // printf("add \"%s\"\n", argv[1]);
}