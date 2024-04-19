
#include "phantomutils.h"

void restore(char * path, char * path2);

int main(int argc, char * argv[]) {
    if (argc == 1) {
        printf("ERROR <NAME> is not include\n");
        printf("Usage: ");
        helpfuncs[4]();
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
    initstatus(); //must call this func to call 
    ///initstatus -> load_commit_log -> makeUnionofMockReal -> load_staging_log ->store2pockets routine
    
    
    int loadres;

    if ((loadres = load_commit_log()) < 0) { //load versioned files
        if (loadres == -1) {
            printf("ERROR: repo didn't initialized, you have to call ssu_repo to init repo first");
        }
        printf("FATAL: LOG FILE CORRUPTED OR NOT EXISTS");
        exit(3);
    }

    // printf("helo");
    char targetpath[MAXPATH];
    strcpy(targetpath, repopath);
    
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
    if (iscommitexists(purename) == 0) { //not existing commit.
        printf("\"%s\" is not exist in repo\n", purename);
        exit(3);
    }
    
    /**
     * TODO: change this after status all fixeds
    */
    // stagedtofs();
    // int err = 0;
    // if ((err = makeUnionofMockReal()) < 0) {
    //     printf("error %d", err);
    //     exit(100);
    // }

    // if ((loadres = load_staging_log()) < 0) {
    //     if (loadres == -1) {
    //         printf("ERROR: repo didn't initialized, you have to call ssu_repo to init repo first");
    //     }
    //     printf("FATAL: LOG FILE CORRUPTED OR NOT EXISTS");
    //     exit(3);
    // }

    // store2pockets();


    ///////////////before is same as status.c routine. /////////////


//  tracked, untracked queue now has only modification of backuped files... need changes queue to loop more clear?
/**
 * TODO: check if tracked, untracked has info of modification??
 *       ************* just load and ignore new files.!!
*/
////
    show_fs_all(version_cursor->root, "");

    char * cwd = getcwd(NULL, 0);
    int len = strlen(cwd);


    commitlog * temp = commitrear;

    int find = 0;
    while(temp) { //find that version and tries change.
        filedir * f = temp->flink;
        filever * v = temp->vlink; 
        if (!strcmp(v->version, purename)) break;
        v->status = -3;
        temp = temp->prev;
    }

    printf("starting...");

    show_fs_all(version_cursor->root, "");
    //v->status = -3 : newer version than target version 
    //v->status = 3 : target version.
    //v->status != 3 or -3 : older version.

    filedir * root = version_cursor->root;

    queue q = *initQueue();

    q.push(&q, root);
    int change = 0;

    while(!q.empty(&q)) { //root always ignored. : root is always dir.
        filedir * f = q.front(&q); 
        q.pop(&q);

        for (int i = 0; i <= f->childscnt; i++){ 
            filedir * child = f->childs[i];
            filever * nv = child->top;
            if (child->childscnt != -1) {
                q.push(&q, child);
                continue;
            }
            // if (child->chk == -2) continue;//new file on dir...
            while(nv->status == -3) {
                if (nv->next == NULL) break;
                nv = nv->next;
            }
            if (nv->status == -3 || nv->status == 2) {
                printf("%s not exists in that ver\n", child->name);
                continue;
            }
            
            char verpath[MAXPATH];
            strcpy(verpath, repopath);
            strcat(verpath, "/");
            strcat(verpath, nv->version);
            strcat(verpath, "/");
            char relpath[MAXPATH];
            sprintf(relpath, "%s", substr(child->oripath, len + 1, strlen(child->oripath)));
            strcat(verpath, relpath);
            // printf("verpath : %s\n", verpath);

            struct stat statbuf;
            if (access(child->oripath, F_OK) && nv->status != 2) { //deleted
                restore(verpath, child->oripath);
                // printf("%s restored deleted file on %s, from %s\n", child->name, child->oripath, verpath);
                printf("%s restored from deletion\n", child->name);
                continue;
            }
            
            if (lstat(child->oripath, &statbuf) < 0) { //deleted.
                fprintf(stderr, "error while reverting files... lstat \n");
                exit(1);
            }

            int res = compare_md5(child->oripath, verpath);

            if (statbuf.st_size == nv->statbuf.st_size && res == 0) {
                printf("%s got no change\n", child->name);
                continue;
            }
            else {
                if (res < 0) {
                    fprintf(stderr, "error while reverting files... md5\n");
                    exit(1);
                }
                if (res == 0) {
                    //same
                    printf("%s got no change\n", child->name);
                    continue;
                }
                restore(verpath, child->oripath);
                printf("%s restored from change\n", child->name);// file on %s, from %s\n", child->name, child->oripath, verpath);
            }
        }
    }

}

void restore(char * path, char * path2) {
    int originfd, commitfd;
    if ((originfd = open(path2, O_WRONLY | O_CREAT | O_TRUNC, 0777)) < 0) {
        printf("failed to revert\n");
        printf("path : %s", path2);
        exit(1);
    }

    if ((commitfd = open(path, O_RDONLY)) < 0) {
        printf("error while open file\n");
        printf("commit : %s\n", path);
        exit(1);
    }

    char buf[1024];
    int len;

    while((len = read(commitfd, buf, sizeof(buf))) > 0) {
        write(originfd, buf, len);
    }

    struct utimbuf temptime;
    struct stat statbuf;

    if (lstat(path, &statbuf) < 0) {
        printf("failed to do lstat");
        exit(1);
    }
    temptime.modtime = statbuf.st_mtime;
    temptime.actime = statbuf.st_atime;
    utime(path2, &temptime);

    close(originfd);
    close(commitfd);
}
    //bottom code is okay but cannot check curver easily...

    
    //revert changes that can chase




//     char * cwd = getcwd(NULL, 0);
//     int len = strlen(cwd);

//     int ischanged = 0;
//     while(!tracked.empty(&tracked)) {
//         filedir * f = tracked.front(&tracked);
//         filever * v = f->top;
//         tracked.pop(&tracked);
//         //mkdirs, restore.
//         if (f->chk == -2) continue;//new

//         char verpath[MAXPATH];
//         strcpy(verpath, repopath);
//         strcat(verpath, "/");
//         strcat(verpath, v->version);
//         strcat(verpath, "/");
//         char relpath[MAXPATH];
//         sprintf(relpath, "%s", substr(f->oripath, len + 1, strlen(f->oripath)));
//         strcat(verpath, relpath);
//         printf("verpath : %s\n", verpath);
//         int verstatus = v->status;

//         if (f->chk == 2 || f->chk == 1) { //del or mod, then revert to latest.
//             //restore(verpath, f->oripath); -> restore need mkdirs.
//             printf("restore tracked commited change on %s to ver : %s, verpath : %s\n", f->name, v->version, verpath);
//             ischanged = 1;
//         } 
//     }
//     while(!untracked.empty(&untracked)) {
//         filedir * f = untracked.front(&untracked);
//         untracked.pop(&untracked); 
//         filever * v = f->top;
//         //mkdirs, restore.
//         if (f->chk == -2) continue;//new

//         char verpath[MAXPATH];
//         strcpy(verpath, repopath);
//         strcat(verpath, "/");
//         strcat(verpath, v->version);
//         strcat(verpath, "/");
//         char relpath[MAXPATH];
//         sprintf(relpath, "%s", substr(f->oripath, len + 1, strlen(f->oripath)));
//         strcat(verpath, relpath);
//         printf("verpath : %s\n", verpath);
//         int verstatus = v->status;

//         if (f->chk == 2 || f->chk == 1) { //del or mod, then revert to latest.
//             printf("restore untracked but commited change on %s to ver : %s, verpath : %s\n", f->name, v->version, verpath);
//             //restore(verpath, f->oripath);
//             ischanged = 1;
//         } 
//     }

//     commitlog * temp = commitrear; //it will have some redundancy(latest one) but i dun care

//     queue deleted = *initQueue();

//     while(temp) { //temp = second commit -> user: trying to go bak to first.
//         if (!strcmp(temp->vlink->version, purename)) break;

//         filedir * f = temp->flink;
//         filever * cv = temp->vlink;
//         filever * v = temp->vlink->next;
//         int verstatus = cv->status;

//         if (verstatus == -2) { //new file on that commit, v is NULL
//             printf("removing new file on that ver\n");
//             //removing
//             temp = temp->prev;
//             continue;
//         }

//         //for delete recover

//         char verpath[MAXPATH];
//         strcpy(verpath, repopath);
//         strcat(verpath, "/");
//         strcat(verpath, v->version);
//         strcat(verpath, "/");
//         char relpath[MAXPATH];
//         sprintf(relpath, "%s", substr(f->oripath, len + 1, strlen(f->oripath)));
//         strcat(verpath, relpath);
//         printf("previous verpath : %s\n", verpath);
       

//         if (verstatus == 1 || verstatus == 2) { //del or mod, then revert to latest.
//             //if diff curdir's file , verpath
//             //then restore
//             //if it is 
//             //
//             //restore(verpath, f->oripath); -> restore need mkdirs.
            
//             printf("restore saved change on %s to ver : %s, verpath : %s\n", f->name, cv->version, verpath);
//             ischanged = 1;
//         }
//         // x
        
//         //mkdirs, restore.
//         temp = temp->prev; //going reverse of commit
        
//     }
//     //complete.
//     if (ischanged == 0) {
//         printf("no chanage");
//     }
//     else {
//         printf("ccc");
//     }   

// }