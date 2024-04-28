
#include "phantomutils.h"

int main(int argc, char * argv[]) {
    if (argc == 1) {
        printf("ERROR <PATH> is not include\n");
        printf("Usage: ");
        helpfuncs[0]();
        /**
         * TODO: usage
        */
        exit(1);
    }
    char * purepath = purifypath(argv[1]); //argv contains ""
    // printf("%s\n", purepath);
    if (purepath == NULL || strlen(purepath) > MAXPATH) {
        printf("ERROR: '%s' is wrong path\n", argv[1]);
        exit(2);
    }
    if (argc > 2) {
        printf("ERROR: TOO MANY ARGS\n");
        exit(4);
    }
    init();
    init_version_controller();

    
    /// routine of   initstatus -> makeUnionofMockReal -> load_staging_log -> managelogrecurs != 0 then -> save_staging_log : for add, remove.
    // this routine above will only chks real files\ (not commited files), calc istracked on fs.
    initstatus(); 
    //init q, tracked, untracked queue for makeUnionofMockReal() ->
    // none + real = real so not calling commit will only load real files

    int loadres;

    if ((loadres = load_commit_log()) < 0) {
        if (loadres == -1) {
            printf("ERROR: repo didn't initialized, you have to call ssu_repo to init repo first\n");
        }
        printf("FATAL: LOG FILE CORRUPTED OR NOT EXISTS");
        exit(3);
    }

    int errcode;
    if ((errcode = makeUnionofMockReal()) < 0) { //makefs = only from commit and real file input, descremenate mod, del, new, nonchange
        printf("%d error", errcode);
        exit(1);
    }

    if ((loadres = load_staging_log()) < 0) {
        if (loadres == -1) {
            printf("ERROR: repo didn't initialized, you have to call ssu_repo to init repo first\n");
        }
        // printf("FATAL: LOG FILE CORRUPTED OR NOT EXISTS");
        exit(3);
    }

    // show_fs(version_cursor->root, "");
    
    char * abpath = realpath(purepath, NULL);
    if (abpath == NULL) {
        printf("ERROR: '%s' is wrong path\n", purepath);
        exit(2);
    }
    char cwd[MAXPATH];
    char relpath[MAXPATH]= ".";
    struct stat statbuf;

    getcwd(cwd, MAXPATH);
    char * chk = strstr(abpath, cwd);
    // printf("%d", (int)(chk - abpath));
    if (chk == NULL || (int)(chk - abpath) != 0) {//CHK IF abpath starts with cwd
        printf("path should not be parent of cwd\n");
        exit(5);
    }
    char * chk2 = strstr(abpath, repopath);
    if (chk2 != NULL || (int)(chk2 - abpath) == 0) {
        printf("path should not be child of ./.repo\n");
        exit(5);
    }

    if (access(abpath, R_OK) != 0 || access(abpath, W_OK) != 0) {
        printf("have no needed permission to path\n");
        exit(7);
    }
    if (lstat(abpath, &statbuf) < 0) {
        printf("lstat err\n");
        exit(6);
    }
    if (!S_ISREG(statbuf.st_mode) && !S_ISDIR(statbuf.st_mode)) {
        printf("this path is not file nor dir\n");
        exit(8);
    }


    //all conditions ok
    char abpath_copy[MAXPATH];
    strcpy(abpath_copy, abpath);

    char * temp = substr(abpath_copy, strlen(cwd), strlen(abpath_copy));
    strcat(relpath, temp);// need mod
    
    if (managelogrecurs(abpath, 0) == 0) { //trying to stage but all files were same... //warning abpath becomes NULL, use abpath_copy
        printf("\"%s\" already exist in stage area\n", relpath);
        exit(0);
    }
    //success staging, add log
    printf("add \"%s\"\n", relpath);
    save_staging_log(abpath_copy, 1);

    // show_staging_log();

    // printf("add \"%s\"\n", argv[1]);
}