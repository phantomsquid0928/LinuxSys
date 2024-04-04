
#include "phantomutils.h"

queue q;
queue untracked;
queue changes;
queue newall;


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
    show_commit_log(NULL);
    // show_staging_log();
    show_fs(version_cursor->root, "");
    stagelog * temp = head;

    while(temp) { //o logn logn
        char * oripath = temp->log;
        char * name = substr(oripath, return_last_name(oripath) + 1, strlen(oripath));
        filedir * f = newfile();
        strcpy(f->oripath, oripath);
        strcpy(f->name, name);
        //as this is temp file, there is no path (commitpath)
        f->istrack = 1;
        filedir * found = addfiledir(f, 1);
        // if (found == NULL) { //staged new file, not 
        //     temp->isnew = 1;
        // }
        
        temp = temp->next;
    }
    show_fs(version_cursor->root, "");

    struct stat statbuf;
    struct dirent ** namelist;
    int cnt;


    filedir * root = version_cursor->root;
    
    // q = *initQueue();
    // changes = *initQueue();
    // untracked = *initQueue();
    // newall = *initQueue();
    // q.push(&q, root); //commit 한번 돌기-> commit할 파일 표시 -> 다시 돌기 -> 실제파일과 비교
    // while(!q.empty(&q)) { // two pointer 사용 필요
    //     filedir * f = q.front(&q);
    //     q.pop(&q);
    //     if ((cnt = scandir(f->oripath, &namelist, NULL, alphasort)) < 0) exit(1);
    //     //
    //     int i = 0, j = 0;
    //     while(1) {
    //         if (i > f->childscnt) { //removed
    //             if (j > cnt) break;
    //             if (f->childs[i]->childscnt != -1) {
    //                 q.push(&q, f->childs[i]);
    //             }
    //             j++;
    //         }
    //         if (j == cnt) { // new
    //             if (i > f->childscnt) break;
    //         }
    //         int res = strcmp(f->childs[i]->name, namelist[j]->d_name);
    //         if (res == 0) {
    //             if (f->childs[i]->childscnt != -1) { //dir
    //                 q.push(&q, f->childs[i]);
    //                 i++;
    //                 j++;
    //                 continue;
    //             }
    //             if (access(f->childs[i]->oripath, F_OK)) { //removed
    //                 f->childs[i]->chk = 2; //rem
    //                 changes.push(&changes, f->childs[i]);
    //                 i++;
    //                 j++;
    //                 continue;
    //             }
    //             if (lstat(f->childs[i]->oripath, &statbuf) < 0) exit(0);
    //             if (statbuf.st_mtime != f->childs[i]->top->statbuf.st_mtime) {//modified
    //                 f->childs[i]->chk = 1; //mod
    //                 changes.push(&changes, f->childs[i]);
    //                 i++;
    //                 j++;
    //                 continue;
    //             }
    //         }
    //         if (res < 0) { //removed
                
    //         }
    //         if (res > 0) { //new
                
    //         }
    //     }
    //     for (int i = 0; i <= f->childscnt; i++) {
    //         filedir * c = f->childs[i];
    //         if (c->childscnt == -1) //file
    //         {
    //          //   if 
                
    //         }
    //         else {
    //             q.push(&q, c);
    //         }
    //     }
    // }
    
    
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