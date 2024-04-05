
#include "phantomutils.h"

int main(int argc, char * argv[]) {
    if (argc > 2) {
        printf("ERROR: TOO MANY ARGS\n");
        /**
         * TODO: usage
        */
        exit(4);
    }

    if (argc == 2 && strlen(argv[1]) > MAXDIR) {
        printf("ERROR: '%s' is wrong name\n", argv[1]);
        exit(2);
    }

    init();
    init_version_controller();


    char version_path[MAXPATH];
    char version[MAXPATH];
    if (argc == 2) {
        char name[MAXDIR];
        if (strstr(argv[1], "\"") != NULL || strstr(argv[1], "'")) {
            strcpy(name, substr(argv[1], 1, strlen(argv[1]) - 1));
        }
        else {
            strcpy(name, argv[1]);
        }
        strcpy(version_path, repopath);
        strcat(version_path, "/");
        strcat(version_path, name);
        // printf("%s\n", version_path);
        char * real_versionpath = realpath(version_path, NULL);
        if (real_versionpath == NULL) {
            printf("no version directory\n");
            exit(3);
        }
        strcpy(version, name);
        // if (access(version_path, F_OK)) { //deprecate if in debug
        //     printf("no version directory");
        //     exit(3);
        // }
        
    }

    int loadres;

    //call init and init_version_controller to make path and cursor

    if ((loadres = load_commit_log()) < 0) {
        if (loadres == -1) {
            printf("ERROR: repo didn't initialized, you have to call ssu_repo to init repo first");
        }
        // printf("FATAL: LOG FILE CORRUPTED OR NOT EXISTS");
        exit(3);
    }
    

    if (argc == 1) {
        show_commit_log(NULL);
    }
    else {
        int res = show_commit_log(version);
    }
    
}