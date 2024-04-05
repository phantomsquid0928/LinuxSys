
#include "phantomutils.h"

queue q;
queue revert_q;

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
    int loadres;
    if ((loadres = load_staging_log()) < 0) {
        if (loadres == -1) {
            printf("ERROR: repo didn't initialized, you have to call ssu_repo to init repo first");
        }
        // printf("FATAL: LOG FILE CORRUPTED OR NOT EXISTS");
        exit(3);
    }
    if ((loadres = load_commit_log()) < 0) {
        if (loadres == -1) {
            printf("ERROR: repo didn't initialized, you have to call ssu_repo to init repo first");
        }
        // printf("FATAL: LOG FILE CORRUPTED OR NOT EXISTS");
        exit(3);
    }


    char targetpath[MAXPATH];
    strcpy(targetpath, repopath);
    char purename[MAXDIR];
    if (strstr(argv[1], "\"") || strstr(argv[1], "'")) {
        sprintf(purename, "%s", substr(argv[1], 1, strlen(argv[1]) - 1));
    }
    else {
        sprintf(purename, "%s", argv[1]);
    }
    sprintf(targetpath, "/%s", purename); //.repo/~~ commit/
    if (access(targetpath, F_OK)) { //not exists
        printf("not existing version of commit\n");
        exit(4);
    }

    filedir * temp = version_cursor->root;
    commitlog * selected = commithead;

    struct stat statbuf;
    /**
     * TODO: tree traverse of root(bfs) and recover to latest to version.
     *       version exists -> recover version. if already exists then err: commit head is on this version.
     *       not exists -> recover latest.
    */
    q = *initQueue();
    q.push(&q, temp);

    
    int errorcode = 0;
    while(!q.empty(&q)) {
        filedir * t = q.front(&q);
        q.pop(&q);
        for(int i = 0; i <= t->childscnt; i++) {
            filedir * n = t->childs[i];
            filever * v = n->top;
            if (n->childscnt != -1) //dir
            {
                q.push(&q, n);
                continue;
            }
            if (lstat(n->oripath, &statbuf) < 0) {
                exit(0);
            }
            if (!strcmp(n->top->version, purename)) {
                //check if there is same file or not
                if (access(n->oripath, F_OK)) { //deleted, just add to revert section
                    revert_q.push(&revert_q, n);
                    continue;
                }
                
                if (n->top->statbuf.st_mtime != statbuf.st_mtime) { //exists but modified, revert to version
                    revert_q.push(&revert_q,n);
                }
                else { //it is currently top now , err
                    errorcode = 1;
                    break;
                }
            }
            else if (statbuf.st_mtime != n->top->statbuf.st_mtime) {
                revert_q.push(&revert_q, n); //recover to latest, not deleted commit section
            }

        }
        if (errorcode != 0) break;
    }
    q.clear(&q);
    if (errorcode == 0) { //normal, revert.
        while(!revert_q.empty(&revert_q)) {
            filedir * t = revert_q.front(&revert_q);
            revert_q.pop(&revert_q);
            // printf("seleced %s: v.<%s>\n", t->name, t->);
        }
    }
    while(selected) {
        if (selected->vlink->version)
        selected = selected->next;
    }

}