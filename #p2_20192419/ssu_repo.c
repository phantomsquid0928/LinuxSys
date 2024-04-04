// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <sys/types.h>
// #include <sys/stat.h>
// #include <string.h>

// /// @brief 
// /// @param command 
// /// @param spliter 
// /// @param res 
// /// @return 
// char ** split(char * command, char * spliter, int *res) {
//     int cnt = 0;
//     // printf("%s", command);
//     char ** temp = (char**)malloc(sizeof(char*));
//     char * arg = strtok(command, spliter);

//     while(arg != NULL) {
//         // printf("arg : %s\n", arg);
//         temp = (char**)realloc(temp, sizeof(char*) * (cnt + 1));
//         temp[cnt] = (char *)malloc(sizeof(char) * (strlen(arg) + 1));
//         strcpy(temp[cnt], arg);
//         arg = strtok(NULL, spliter);
//         cnt++;
//     }
//     *res = cnt;

//     return temp;
// }
#include "phantomutils.h"

//////////////////////// 

int repo_init() {
    char * cwd = getcwd(NULL, 0);
    char repopath[MAXPATH];
    char stagingpath[MAXPATH];
    char commitpath[MAXPATH];
    sprintf(repopath, "%s/.repo", cwd);
    if (access(repopath, F_OK))
        if (mkdir(repopath, 0777) < 0) return -1;
    sprintf(stagingpath, "%s/.repo/.staging.log", cwd);
    sprintf(commitpath, "%s/.repo/.commit.log", cwd);
    int fd, fd2;
    if ((fd = open(stagingpath, O_CREAT| O_RDONLY, 0777)) < 0) 
        return -1;
    if ((fd2 = open(commitpath, O_CREAT| O_RDONLY, 0777)) < 0)
        return -1;
    close(fd);
    close(fd2);
    return 0;
}
void add_exec(int argc, char ** argv) { //fork, exec 
    // printf("hello add");
    char path[MAXPATH] = "";
    memset(path, 0, sizeof(path));
    if (argc != 1) {
        strcpy(path, argv[1]);
    }
    if (argc > 2) {
        for (int i = 2; i < argc; i++) {
            strcat(path, " ");
            strcat(path, argv[i]);
        }
    }

    int pid = fork();
    if (pid == 0) {
        char * exec_args1[] = {"./add", NULL};
        char * exec_args2[] = {"./add", path, NULL};
        if (argc == 1) {
            execv("add", exec_args1);
        }
        else {
            execv("add", exec_args2);
        }
    }
    else {
        int code;
        int pid = wait(&code);
        //error
    }
}
void remove_exec(int argc, char ** argv){ //fork, exec
    // printf("hello remove");
    char path[MAXPATH] = "";
    memset(path, 0, sizeof(path));
    if (argc != 1) {
        strcpy(path, argv[1]);
    }
    if (argc > 2) {
        for (int i = 2; i < argc; i++) {
            strcat(path, " ");
            strcat(path, argv[i]);
        }
    }

    int pid = fork();
    if (pid == 0) {
        char * exec_args1[] = {"./remove", NULL};
        char * exec_args2[] = {"./remove", path, NULL};
        if (argc == 1) {
            execv("remove", exec_args1);
        }
        else {
            execv("remove", exec_args2);
        }
    }
    else {
        int code;
        int pid = wait(&code);
        //error
    }
}
void status_exec(int argc, char ** argv) { //
    // printf("hello status");/
    int pid = fork();
    if (pid == 0) {
        char * exec_args1[] = {"status", NULL};
        execv("status", exec_args1); 
    }
    else {
        int code;
        int pid = wait(&code);
        //error
    }
}
void commit_exec(int argc, char ** argv) {
    printf("hello commit");
}
void revert_exec(int argc, char ** argv) {
    printf("hello revert");
}
void log_exec(int argc, char ** argv) { 
    // printf("hello log");
    char path[MAXPATH] = "";
    memset(path, 0, sizeof(path));
    if (argc != 1) {
        strcpy(path, argv[1]);
    }
    if (argc > 2) {
        for (int i = 2; i < argc; i++) {
            strcat(path, " ");
            strcat(path, argv[i]);
        }
    }

    int pid = fork();
    if (pid == 0) {
        char * exec_args1[] = {"./log", NULL};
        char * exec_args2[] = {"./log", path, NULL};
        if (argc == 1) {
            execv("log", exec_args1);
        }
        else {
            execv("log", exec_args2);
        }
    }
    else {
        int code;
        int pid = wait(&code);
        //error
    }
}
void help_exec(int argc, char ** argv){
    printf("");
}
void exit_exec(int argc, char ** argv) {
    exit(0);
}

/// @brief //////////////function pointer

void (*exec_link[])(int, char**) = {add_exec, remove_exec, status_exec, commit_exec, revert_exec, log_exec, help_exec, exit_exec};
int main() {

    //init
    repo_init();

    while(1) {
        char input[10000];
        char garbage[10];
        printf("20192419>");
        memset(input, 0, sizeof(input));

        scanf("%[^\n]", input);
        // printf("%s", input);
        getc(stdin); //remove useless \n

        int argc = 0;
        char ** argv = split(input, " ", &argc);
        if (argc == 0) continue;
        for (int i = 0; i < commandscnt; i++) {
            if (!strcmp(commandsList[i], argv[0])) {
                exec_link[i](argc, argv);
            }   
        }
        for (int i = 0; i < argc; i++) {
            free(argv[i]);
        }
    }
}