
#include "phantomutils.h"

//////////////////////// 

char addpath[MAXPATH];
char removepath[MAXPATH];
char statuspath[MAXPATH];
char commitpath[MAXPATH];
char revertpath[MAXPATH];
char logpath[MAXPATH];
char helppath[MAXPATH];

int repo_init(char * programpath) {
    char * cwd = getcwd(NULL, 0);
    char repopath[MAXPATH];
    char stagingpath[MAXPATH];
    char commitlogpath[MAXPATH];
    sprintf(repopath, "%s/.repo", cwd);
    if (access(repopath, F_OK))
        if (mkdir(repopath, 0777) < 0) return -1;
    sprintf(staginglogpath, "%s/.repo/.staging.log", cwd);
    sprintf(commitlogpath, "%s/.repo/.commit.log", cwd);
    int fd, fd2;
    if ((fd = open(staginglogpath, O_CREAT| O_RDONLY, 0777)) < 0) 
        return -1;
    if ((fd2 = open(commitlogpath, O_CREAT| O_RDONLY, 0777)) < 0)
        return -1;
    close(fd);
    close(fd2);

    sprintf(addpath, "%s/add", programpath);
    sprintf(removepath, "%s/remove", programpath);
    sprintf(statuspath, "%s/status", programpath);
    sprintf(commitpath, "%s/commit", programpath);
    sprintf(revertpath, "%s/revert", programpath);
    sprintf(logpath, "%s/log", programpath);
    sprintf(helppath, "%s/help", programpath);


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
        char * exec_args1[] = {addpath, NULL};
        char * exec_args2[] = {addpath, path, NULL};
        if (argc == 1) {
            execv(addpath, exec_args1);
        }
        else {
            execv(addpath, exec_args2);
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
        char * exec_args1[] = {removepath, NULL};
        char * exec_args2[] = {removepath, path, NULL};
        if (argc == 1) {
            execv(removepath, exec_args1);
        }
        else {
            execv(removepath, exec_args2);
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
        char * exec_args1[] = {statuspath, NULL};
        execv(statuspath, exec_args1); 
    }
    else {
        int code;
        int pid = wait(&code);
        //error
    }
}
void commit_exec(int argc, char ** argv) {
    // printf("hello commit");
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
        char * exec_args1[] = {commitpath, NULL};
        char * exec_args2[] = {commitpath, path, NULL};
        if (argc == 1) {
            execv(commitpath, exec_args1);
        }
        else {
            execv(commitpath, exec_args2);
        }
    }
    else {
        int code;
        int pid = wait(&code);
       //error
    }
}
void revert_exec(int argc, char ** argv) {
    // printf("hello revert");
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
        char * exec_args1[] = {revertpath, NULL};
        char * exec_args2[] = {revertpath, path, NULL};
        if (argc == 1) {
            execv(revertpath, exec_args1);
        }
        else {
            execv(revertpath, exec_args2);
        }
    }
    else {
        int code;
        int pid = wait(&code);
       //error
    }
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
        char * exec_args1[] = {logpath, NULL};
        char * exec_args2[] = {logpath, path, NULL};
        if (argc == 1) {
            execv(logpath, exec_args1);
        }
        else {
            execv(logpath, exec_args2);
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
int main(int argc, char * argv[]) {


    char * programpath = substr(argv[0], 0, return_last_name(argv[0]));
    //init
    repo_init(programpath);
    // printf("%s", getcwd(NULL, 0));

    while(1) {
        char input[10000];
        char garbage[10];
        printf("20192419>");
        memset(input, 0, sizeof(input));

        scanf("%[^\n]", input);
        // printf("%s", input);
        getc(stdin); //remove useless \n

        int argc = 0;
        int chk = 0;
        char ** argv = split(input, " ", &argc);
        if (argc == 0) continue;
        for (int i = 0; i < commandscnt; i++) {
            if (!strcmp(commandsList[i], argv[0])) {
                chk = 1;
                exec_link[i](argc, argv);
            }
        }
        for (int i = 0; i < argc; i++) {
            free(argv[i]);
        }
        if (chk == 0) {
            printf("Usage: \n");
            for (int i = 0; i < commandscnt; i++) {
                printf("  >");
                helpfuncs[i]();
            }
        }

    }
}