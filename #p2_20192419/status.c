
#include "phantomutils.h"


int main(int argc, char * argv[]) {
    if (argc != 1) {
        printf("ERROR no args allowed\n");
        printf("Usage: ");
        helpfuncs[2]();
        exit(1);
    }
    int loadres;

    //call init and init_version_controller to make path and cursor
    init();
    init_version_controller();
    initstatus(); //must call this func to call initstatus -> load_commit_log -> makeUnionofMockReal -> load_staging_log ->store2pockets routine


    //get already existing commits from .repo
    if ((loadres = load_commit_log()) < 0) {
        if (loadres == -1) {
            printf("ERROR: repo didn't initialized, you have to call ssu_repo to init repo first\n");
        }
        // printf("FATAL: LOG FILE CORRUPTED OR NOT EXISTS");
        exit(3);
    }
    // show_commit_log(NULL);
    // show_staging_log();
    // show_fs(version_cursor->root, "");

    //compare cur dir and commit (top)
    int errcode = 0;
    if ((errcode = makeUnionofMockReal()) < 0) { //makefs = only from commit and real file input, descremenate mod, del, new, nonchange
        printf("%d error", errcode);
        exit(1);
    }
    // show_staging_log();/
    // show_fs(version_cursor->root, "");

    //now load log (log now traverse mockfs that contains all files that changed)
    //i can make this func also can manage istrack...?
    if ((loadres = load_staging_log(0)) < 0) {
        if (loadres == -1) {
            printf("ERROR: repo didn't initialized, you have to call ssu_repo to init repo first\n");
        }
        // printf("FATAL: LOG FILE CORRUPTED OR NOT EXISTS");
        exit(3);
    }

    // show_fs(version_cursor->root, "");

    store2pockets(0);
    // // printf("HELLO");

    if (tracked.empty(&tracked) && untracked.empty(&untracked)) {
        printf("\nNothing to commit\n");
        exit(0);
    }
    if (!tracked.empty(&tracked)) {
        printf("Changes to be commited:\n");
    }
    while(!tracked.empty(&tracked)) {
        filedir * temp = tracked.front(&tracked);
        tracked.pop(&tracked);
        char type[MAXDIR];

        if (temp->chk == -2) sprintf(type, "%s", "   new file : ");
        else if (temp->chk == 1) sprintf(type, "%s", "   modified : ");
        else if (temp->chk == 2) sprintf(type, "%s", "   removed : ");
        else {
            printf("OMG");
            exit(1);
        }
        printf("%s\"%s\" \n", type, temp->oripath);
    }
    printf("\n");

    if (!untracked.empty(&untracked)) {
        printf("Untracked files: \n");
    }
    
    while(!untracked.empty(&untracked)) {
        filedir * temp = untracked.front(&untracked);
        untracked.pop(&untracked);
        char type[MAXDIR];

        if (temp->chk == -2) sprintf(type, "%s", "   new file : ");
        else if (temp->chk == 1) sprintf(type, "%s", "   modified : ");
        else if (temp->chk == 2) sprintf(type, "%s", "   removed : ");
        else {
            printf("OMG");
            exit(1);
        }
        printf("%s\"%s\" \n", type, temp->oripath);
    }
    printf("\n");
}