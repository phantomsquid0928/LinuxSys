
#include "phantomutils.h"

int main(int argc, char * argv[]) {
    if (argc != 1) {
        printf("ERROR no args allowed\n");

        /**
         * TODO: usage
        */
        exit(1);
    }
    int loadres;

    //call init and init_version_controller to make path and cursor
    init();
    init_version_controller();

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
    // show_commit_asfile();
    show_commit_log();
    show_fs(version_cursor->root, "");
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