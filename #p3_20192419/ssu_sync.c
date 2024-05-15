#include "phantomutils.h"

int cnt = 3;
void modify_checker(int pid, filedir * root, char * timemask) { //1초 내에 끝나야 함????
    if (tracked.empty(&tracked)) return;
    while(!tracked.empty(&tracked)) {
        filedir * f = tracked.front(&tracked);
        tracked.pop(&tracked);

        int originfd, commitfd;
        
        char curpath[MAXPATH];
        // char relpath[MAXPATH];

        char * relpath =  substr(f->oripath, strlen(root->oripath) + 1, strlen(f->oripath));

        if (f->chk != 2) {

            struct stat statbuf;
            if (lstat(f->oripath, &statbuf) < 0) {
                printf("failed to do lstat");
                // rmdirs(targetpath);
                close(commitfd);
                close(originfd);
                exit(1);
            }
            
            sprintf(curpath, "%s/%s_%s", pidrootpath, relpath, timemask);

            mkdirs(substr(curpath, 0, return_last_name(curpath)));
            

            
            if ((commitfd = open(curpath, O_WRONLY | O_CREAT, 0777)) < 0) {
                printf("failed to create commit, %s\n", curpath);
                // printf("removing %s\n", targetpath);
                // rmdirs(targetpath);
                close(originfd);
                close(commitfd);
                exit(1);
            }

            if ((originfd = open(f->oripath, O_RDONLY)) < 0) {
                printf("error while open file\n");
                // printf("removing %s\n", targetpath);
                // rmdirs(targetpath);
                close(originfd);
                close(originfd);
                continue;
                // exit(1);
            }

            char buf[4096];
            int len;
            while((len = read(originfd, buf, sizeof(buf))) > 0) {
                write(commitfd, buf, len);
            }
            struct utimbuf temptime;
    
            temptime.modtime = statbuf.st_mtime;
            temptime.actime = statbuf.st_atime;
            utime(curpath, &temptime);
            
            close(originfd);
            close(commitfd);
        }

        /**
         * TODOOLD: save_commit_log queue, flush 형태로 바꾸기?
         * i dunt know wal :P
        */
        save_pid_log(pid, f->chk, f->oripath, timemask);
    }
}
static void killer() {
    exit(0);
}
int make_daemon() {
    pid_t pid;
    int fd, maxfd;
    if ((pid = fork()) < 0) {
        fprintf(stderr, "fork error\n");
        return -1;
    }
    else if (pid != 0) return pid;
    // if ((pid = fork()) < 0) {
    //     fprintf(stderr, "fork error\n");
    //     return -1;
    // }
    // else if (pid != 0) exit(0);

    pid = getpid();

    printf("%d is daemon\n", pid);
    setsid();
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    // maxfd = getdtablesize();

    // for (fd = 0; fd < maxfd; fd++) 
    //     close(fd);
    
    umask(0);
    chdir("/");
    fd = open("/dev/null", O_RDWR);
    dup(0);
    dup(0);
    return 0;
}
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
    if (period <= 0) {
        addhelp();
        return;
    }

    struct stat statbuf;
    if (lstat(path, &statbuf) < 0) {
        fprintf(stderr, "lstat error\n");
        exit(1);
    }

    if ((mod & 3) == 0 && S_ISDIR(statbuf.st_mode)) { //dir but no flag
        //wrogn
        addhelp();
        return;
    }
    if ((mod & 3) != 0 && S_ISREG(statbuf.st_mode)) { //file but flag exists
        //wrong
        addhelp();
        return;
    }

    char * rootpath;
    if (S_ISDIR(statbuf.st_mode)) {
        rootpath = path;
    }
    else {
        rootpath = substr(path, 0, return_last_name(path));
    }

    
    /**
     * TODO: opt manage. DONE
     * TODO: dup manage 
    */

    monitorlist * newmon = newmlog();
    int pid = -1;
    if ((pid = make_daemon()) < 0) {
        fprintf(stderr, "make daemon failed\n");
        return;
    }
    if (pid == 0) { //daemon, need preprocessing before entering while loop
        signal(SIGUSR1, killer);
        int daemon_pid = getpid();
        filedir * root = newfile();
        strcpy(root->oripath, rootpath);
        init_pid(daemon_pid);
        sleep(10);
        init_fs();
        // execlp("./a.out", )
        // int t = 0;
        while(1) {
            time_t t;
            time(&t);
            struct tm* tmt;
            tmt = localtime(&t);
            char timemask[200];
            if (strftime(timemask, sizeof(timemask), "%Y%m%d%H%M%S", tmt) == 0) {
                fprintf(stderr, "failed to make time\n");
                exit(3);
            }
            makeUnionofMockReal(root, t, mod, path);
            // printf("fff");
            store2pockets(root);
            modify_checker(daemon_pid, root, timemask);

            show_fs_all(root, "");
            sleep(10);
        }
        exit(0); //wIERD
    }
    else {
        newmon->pid = pid;
        strcpy(newmon->path, path);
        pushmlog(newmon);
        save_monitor_log();
        printf("path : %s\t mod : %d %d\n", path, mod, period);
        return;
    }
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

    if (removemlog(pid) < 0) {
        fprintf(stderr, "failed to remove pid as pid is invalid\n");
        return;
    }
    /**
     * TODO: KILL PROCESS PID
    */
   //
    save_monitor_log();

    kill(pid, SIGUSR1);
    //rmdirs
    char targetpath[MAXPATH];
    char targetlogpath[MAXPATH];
    sprintf(targetpath, "%s/%d", backuppath, pid);
    sprintf(targetlogpath, "%s/%d.log", backuppath, pid);
    rmdirs(targetpath);
    remove(targetlogpath);

    //remove pid(kill) from process, list, files
}
void listfunc(int argc, char * argv[]) {
    if (argc > 2) {
        listhelp();
        return;
    }
    int pid = -1;
    if (argc ==2) {
        for (int i = 0; i < strlen(argv[1]); i++) {
            if (isdigit(argv[1][i]) == 0) {
                listhelp();
                return;
            }
        }
        pid = atoi(argv[1]);
    }

    if (pid == -1) {
        //showmlog
        monitorlist * temp = head;
        while(temp) {
            printf("%d : %s\n", temp->pid, temp->path);
            temp = temp->next;
        }
        return;
    }
    else {
        /**
         * TODO: search pid from mlog and get root
         * TODO: if not exists then err or continue
         * TODO: from root make fs?
         * TODO: then showfs will be clear as pid folder is dirty.
        */
        monitorlist * temp = head;
        while(temp) {
            if (temp->pid == pid) {
                break;
            }
            temp = temp->next;
        }
        if (temp == NULL) {
            printf("no such pid is under process\n");
            return;
        }
        char targetpath[MAXPATH];
        sprintf(targetpath, "%s/%d.log", backuppath, pid);
        if (access(targetpath, F_OK)) {
            fprintf(stderr, "backup file corrupted");
            exit(10);
        }
        


    }
    
    //tree


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

    load_monitor_log();

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
            printf("input %d : %s \n", i, args[i]);
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