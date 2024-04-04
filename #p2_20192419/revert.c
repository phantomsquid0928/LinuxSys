
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
    sprintf(targetpath, "/%s", purename);
    if (access(targetpath   , F_OK) == 0) {
        printf("%s is already exist in repo\n", purename);
        exit(3);
    }
    if (head == NULL) //empty //test
    {
        printf("there is no staged file\n");
        exit(4);
    }

    

    // file * temp = version_cursor->head;
    // while(temp) {
        
    // }

}