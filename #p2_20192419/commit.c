
#include "phantomutils.h"

int main(int argc, char * argv[]) {
    if (argc == 1) {
        printf("ERROR <NAME> is not include\n");
        printf("Usage: ");
        helpfuncs[3]();
        /**
         * TODO: usage
        */
        exit(1);
    }
    if (strlen(argv[1]) > MAXDIR) {
        printf("ERROR: '%s' is wrong commit name\n", argv[1]);
        exit(2);
    }
    //init is essential
    init();
    init_version_controller();
    initstatus();
    
    int loadres;
    if ((loadres = load_staging_log()) < 0) {
        if (loadres == -1) {
            printf("ERROR: repo didn't initialized, you have to call ssu_repo to init repo first");
        }
        printf("FATAL: LOG FILE CORRUPTED OR NOT EXISTS");
        exit(3);
    }
    if ((loadres = load_commit_log()) < 0) {
        if (loadres == -1) {
            printf("ERROR: repo didn't initialized, you have to call ssu_repo to init repo first");
        }
        printf("FATAL: LOG FILE CORRUPTED OR NOT EXISTS");
        exit(3);
    }

    printf("helo");
    char targetpath[MAXPATH];
    strcpy(targetpath, repopath);
    printf("%s\n", repopath);
    printf("%s\n", staginglogpath);
    printf("%s\n", commitlogpath);
    char purename[MAXDIR];
    if (strstr(argv[1], "\"") || strstr(argv[1], "'")) {
        sprintf(purename, "%s", substr(argv[1], 1, strlen(argv[1]) - 1));
    }
    else {
        sprintf(purename, "%s", argv[1]);
    }
    strcat(targetpath, "/");
    strcat(targetpath, purename);

    printf("backup named; %s\n", targetpath);
    if (access(targetpath, F_OK) == 0) {
        printf("%s is already exist in repo\n", purename);
        exit(3);
    }
    
    
    stagedtofs();
    int err = 0;
    if ((err = makestatus()) < 0) {
        printf("error %d", err);
        exit(100);
    }

    if (tracked.empty(&tracked) == 1) //empty //test
    {
        printf("Nothing to commit\n");
        exit(0);
    }

    char * cwd = getcwd(NULL, 0);
    int len = strlen(cwd);

    mkdir(targetpath, 0777);
    printf("%s is under mkdir\n",targetpath);


    while(!tracked.empty(&tracked)) {
        filedir * f = tracked.front(&tracked);
        tracked.pop(&tracked);

        int originfd, commitfd;
        if ((originfd = open(f->oripath, O_RDONLY)) < 0) {
            printf("error while open file\n");
            close(originfd);
            exit(1);
        }
        char curpath[MAXPATH];
        char relpath[MAXPATH];
        sprintf(relpath, "%s", substr(f->oripath, len + 1, strlen(f->oripath)));

        // printf("%s\n", relcdpath);


        strcpy(curpath, targetpath);
        strcat(curpath, "/");
        strcat(curpath, relpath);

        // printf("%s is under working\n", curpath);
        
        mkdirs(substr(curpath, 0, return_last_name(curpath)));
        if ((commitfd = open(curpath, O_WRONLY | O_CREAT)) < 0) {
            printf("failed to create commit");
            printf("removing %s\n", targetpath);
            // rmdirs(targetpath);
            close(originfd);
            close(commitfd);
            exit(1);
        }

        /**
         * TODO: mkdirs curpath...
        */
        char buf[4096];
        int len;
        while((len = read(originfd, buf, sizeof(buf))) > 0) {
            write(commitfd, buf, len);
        }
        struct utimbuf temptime;
        struct stat statbuf;
        if (lstat(f->oripath, &statbuf) < 0) {
            printf("failed to do lstat");
            // rmdirs(targetpath);
            close(commitfd);
            close(originfd);
            exit(1);
        }
        temptime.modtime = statbuf.st_mtime;
        temptime.actime = statbuf.st_atime;
        utime(curpath, &temptime);
        
        close(originfd);
        close(commitfd);

        /**
         * TODO: save_commit_log queue, flush 형태로 바꾸기?
        */

        if (save_commit_log(purename, f->oripath, f->chk) < 0) {
            printf("failed to write log");
            printf("%s %s %d\n", purename, f->oripath, f->chk);
            //rmdirs(targetpath);
            printf("removing %s\n", targetpath);
            close(commitfd);
            close(originfd);
            exit(1);
        }
        

    }

    // file * temp = version_cursor->head;
    // while(temp) {
        
    // }

}