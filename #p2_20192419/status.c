
#include "phantomutils.h"

queue q;
queue untracked;
queue tracked;


int main(int argc, char * argv[]) {
    if (argc != 1) {
        printf("ERROR no args allowed\n");
        printf("Usage: ");
        helpfuncs[2]();
        /**
         * TODO: usage
        */
        exit(1);
    }
    int loadres;

    //call init and init_version_controller to make path and cursor
    init();
    init_version_controller();
    initstatus(); //must call this func to call makestatus,  initstatus -> stagedtofs -> makestatus routine

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
    // show_commit_log(NULL);
    // show_staging_log();
    // show_fs(version_cursor->root, "");

    /**
     * TODO: push dirty junks below into header
    */

    stagedtofs();
    // stagelog * temp = head;

    // while(temp) { //o logn logn
    //     char * oripath = temp->log;
    //     char * name = substr(oripath, return_last_name(oripath) + 1, strlen(oripath));
    //     filedir * f = newfile();
    //     strcpy(f->oripath, oripath);
    //     strcpy(f->name, name);
    //     //as this is temp file, there is no path (commitpath)
    //     f->istrack = 1;
    //     f->chk = -2; //newfile newflag
    //     filedir * found = addfiledir(f, 1);
    //     // if (found == NULL) { //staged new file, not 
    //     //     temp->isnew = 1;
    //     // }
        
    //     temp = temp->next;
    // }
    show_fs(version_cursor->root, "");
    int errcode = 0;
    if ((errcode = makestatus()) < 0) {
        printf("%d error", errcode);
    }
    
    // printf("HELLO");
    if (!tracked.empty(&tracked)) {
        printf("\nChanges to be commited:\n");
    }
    while(!tracked.empty(&tracked)) {
        filedir * temp = tracked.front(&tracked);
        tracked.pop(&tracked);
        char type[MAXDIR];

        if (temp->chk == -2) sprintf(type, "%s", "   new file : ");
        else if (temp->chk == 1) sprintf(type, "%s", "   modified : ");
        else if (temp->chk == 2) sprintf(type, "%s", "   deleted : ");
        else continue;
        printf("%s\"%s\" \n", type, temp->oripath);
    }
    printf("\n");

    printf("Untracked files: \n");
    while(!untracked.empty(&untracked)) {
        filedir * temp = untracked.front(&untracked);
        untracked.pop(&untracked);
        char type[MAXDIR];

        if (temp->chk == -2) sprintf(type, "%s", "   new file : ");
        if (temp->chk == 1) sprintf(type, "%s", "   modified : ");
        if (temp->chk == 2) sprintf(type, "%s", "   deleted : ");
        printf("%s\"%s\" \n", type, temp->oripath);
    }
    printf("\n");
    
    
    /**
     * TODO: scandir cwd -> make list of files -> loop list -> find from loglist -> if not exists then 'untracked'
     *       exists but modified-> 'modified' exists in loop list but not exists in scandir -> 'removed' exists but same : X
     *      
     * 
    */
    

    // file * temp = version_cursor->head;
    // loglist * temp2 = head;
    // while(temp) {
    //     commitlog * latest = temp->top;
        
    // }
}