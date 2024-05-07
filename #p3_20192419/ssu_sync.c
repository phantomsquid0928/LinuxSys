#include "phantomutils.h"

void addfunc(int argc, char * argv[]) {
    int c = -1;
    int mod = 0;
    extern char *optarg;
    extern int optind;
    int period = 1; //default period

    optind = 0; //important!

    if (argc == 1) {
        //err
        addhelp();
        return;
    }
    if (strlen(argv[1]) > MAXPATH) { //BADpath
        printf("bad path");
        return;
    }
    char path[MAXPATH];
    char * temp = realpath(argv[1], NULL);
    if (temp == NULL) { //badpath2
        printf("bad path");
        return;
    }
    strcpy(path, temp);
    
    printf("path : %s\n", path);
    while((c = getopt(argc, argv, "drt:")) != -1) {
        switch(c) {
            case 'd':
                mod |= 1;
                break;
            case 'r':
                mod |= 2;
                break;
            case 't':
                mod |= 4;
                for (int i= 0; i < strlen(optarg); i++) {
                    if (isdigit(optarg[i]) == 0) {
                        printf("bad arg of -t");
                        addhelp();
                        //not good
                        // printf("eeee");
                        return;
                    }
                }
                period = atoi(optarg);
                break;
            case '?':
                if (optopt == 't')
                    printf("no time value in t\n");
                addhelp();
                return;
            default:
                addhelp();
                return;
        }
    }



    printf("path : %s\t mod : %d %d\n", path, mod, period);
}   
void removefunc(int argc, char * argv[]) {
    if (argc != 2) {
        removehelp();
        return;
    }
    for (int i = 0; i < strlen(argv[1]); i++) {
        if (isdigit(argv[1][i]) == 0) {
            removehelp();
            return;
        }
    }
    int pid = atoi(argv[1]);


}
void listfunc(int argc, char * argv[]) {
    if (argc > 2) {
        removehelp();
        return;
    }
    int pid = -1;
    if (argc ==2) {
        for (int i = 0; i < strlen(argv[1]); i++) {
            if (isdigit(argv[1][i]) == 0) {
                    removehelp();
                    return;
            }
        }
        pid = atoi(argv[1]);
    }
    
    if (pid == -1) {

    }
    else {//tree

    }


}
void helpfunc(int argc, char * argv[]) {
    if (argc == 2) {
        for(int i= 0 ;i < commandscnt; i++) {
            if (!strcmp(commandsList[i], argv[1])) {
                printf("Usage:\n");
                helpFuncList[i]();
                printf("\n");
                return;
            }
        }
    }
    printf("Usage:\n");
    for (int i = 0;i < commandscnt; i++) {
        printf("   > ");
        helpFuncList[i]();
    }
}
void exitfunc(int argc, char * argv[]) {
    exit(0);
}

void (*funcList[5])(int, char **) = {addfunc, removefunc, listfunc, helpfunc, exitfunc};

int main(void) {
    char input[4096];
    init();
    printf("%s\n", homepath);
    
    printf("%s\n", backuppath);
    
    printf("%s\n", logpath);

    while(1) {
        printf("20192419>");
        memset(input, 0, sizeof(input));
        scanf("%[^\n]", input);
        getchar();
        // fgets(input, 4096, stdin);
        int res = 0;
        int valid = 1;
        char ** args = commandsplit(input, " ", &res, &valid);
        for (int i =0 ;i < res; i++) {
            printf("%d : %s \n", i, args[i]);
        }
        if (res == 0) continue;
        if (valid != 0) {
            int chk = 0;
            for (int i = 0; i < commandscnt; i++) {
                if (!strcmp(args[0], commandsList[i])) {
                    chk = 1;
                    funcList[i](res, args);
                    break;
                }
            }
            if (chk == 0) {
                helpfunc(0, NULL);
            }
        }
        else {
            helpfunc(0, NULL);
            //wrong
        }
        for (int i =0 ;i < res; i++) {
            free(args[i]);
        }
        free(args);
        args = NULL;
    }
}